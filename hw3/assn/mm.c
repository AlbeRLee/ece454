/*
 * This implementation replicates the implicit list implementation
 * provided in the textbook
 * "Computer Systems - A Programmer's Perspective"
 * Blocks are never coalesced or reused.
 * Realloc is implemented directly using mm_malloc and mm_free.
 *
 * NOTE TO STUDENTS: Replace this header comment with your own header
 * comment that gives a high level description of your solution.
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>
#include "assert.h"
#include <stdbool.h>

#include "mm.h"
#include "memlib.h"

/******************************************************************************
 * NOTE TO STUDENTS: Before you do anything else, please
 * provide your team information in the following struct.
 ******************************************************************************/
team_t team = {
  /* Team name */
  "Hugh Mungus",
  /* First member's full name */
  "Taylan Gocmen",
  /* First member's email address */
  "taylan.gocmen@mail.utoronto.ca",
  /* Second member's full name (leave blank if none) */
  "Gligor Djogo",
  /* Second member's email address (leave blank if none) */
  "g.djogo@mail.utoronto.ca"
};

/******************************************************************************
 * Function Declarations
 * Provided Functions
 ******************************************************************************/
int mm_init(void);
void *coalesce(void *bp);
void *extend_heap(size_t words);
void * find_fit(size_t asize);
void place(void* bp, size_t asize);
void mm_free(void *bp);
void *mm_malloc(size_t size);
void *mm_realloc(void *ptr, size_t size);
int mm_check(void);

/******************************************************************************
 * Function Declarations
 * Developed Functions
 ******************************************************************************/
void init_freelistp();
static inline int list_index(int size);
void insert_block(void *bp);
void insert_block_sized(void *bp, size_t size);
void remove_block(void *bp);
void remove_block_sized(void *bp, size_t size);
//size_t aligned_size(size_t size);

/******************************************************************************
 * Basic Constants and Macros
 * You are not required to use these macros but may find them helpful.
 ******************************************************************************/
#define WSIZE       sizeof(void *)            /* word size (bytes) */
#define DSIZE       (2 * WSIZE)            /* doubleword size (bytes) */
#define CHUNKSIZE   (1<<10)      /* initial heap size (bytes) */
#define BLOCKSIZE   32
#define ALIGN_SIZE(s) ((s > (2*DSIZE)) ?(DSIZE * ((s +(2 *DSIZE) -1) /DSIZE)) :(2 *DSIZE))


#define MAX(x,y) ((x) > (y)?(x) :(y))
#define MIN(x,y) ((x) > (y)?(y) :(x))

/* Pack a size and allocated bit into a word */
#define PACK(size, alloc) ((size) | (alloc))

/* Read and write a word at address p */
#define GET(p)          (*(uintptr_t *)(p))
#define PUT(p,val)      (*(uintptr_t *)(p) = (val))
#define PUT_NEXT(p, val)  PUT((p + DSIZE), val);
#define PUT_PREV(p, val)  PUT((p + WSIZE), val);

/* Read the size and allocated fields from address p */
#define GET_SIZE(p)     (GET(p) & ~(DSIZE - 1))
#define GET_ALLOC(p)    (GET(p) & 0x1)

/* Given block ptr bp, compute address of its header and footer */
#define HDRP(bp)        ((char *)(bp) - WSIZE)
#define FTRP(bp)        ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)

#define NEXT_FREE(bp)   ((void *)GET(bp))
#define PREV_FREE(bp)   ((void *)GET(bp + WSIZE))

/* Given block ptr bp, compute address of next and previous blocks */
#define NEXT_BLKP(bp)   ((char *)(bp) + GET_SIZE(((char *)(bp) - WSIZE)))
#define PREV_BLKP(bp)   ((char *)(bp) - GET_SIZE(((char *)(bp) - DSIZE)))

// Zeroth list is 0 - 32
// First list is 33 - 64
// Second list is 65 - 128
// ...
// Thirteenth list is 131073 - 262144
// Fourteenth list is 262145 +

// Fourteenth list is 26213 - 524288
// Fifteenth list is 524289 +
#define NUM_FREE_LIST 15

#define LEAST_SIGNIFICANT(x) (x & 0x1)

#define FIRST_LIST_MIN_SIZE_POW 5
#define FIRST_LIST_MAX_SIZE_POW 6 // first list is from 32 to 64

#define LIST_SIZE_MIN(i) (1 << (FIRST_LIST_MIN_SIZE_POW + i))
#define LIST_SIZE_MAX(i) ((1 << (FIRST_LIST_MAX_SIZE_POW + i)) -1)

#define MIN_LIST(x) (x >> FIRST_LIST_MAX_SIZE_POW)
#define NEXT_LIST(x) (x >> 1)

void* heap_listp = NULL;
void* free_listp[NUM_FREE_LIST];

/******************************************************************************
 * mm_init
 * Initialize the heap, including "allocation" of the
 * prologue and epilogue
 ******************************************************************************/
int mm_init(void) {

  if ((heap_listp = mem_sbrk(4 * WSIZE)) == (void *) - 1)
    return -1;

  PUT(heap_listp, 0); // alignment padding
  PUT(heap_listp + (1 * WSIZE), PACK(DSIZE, 1)); // prologue header
  PUT(heap_listp + (2 * WSIZE), PACK(DSIZE, 1)); // prologue footer
  PUT(heap_listp + (3 * WSIZE), PACK(0, 1)); // epilogue header

  heap_listp += DSIZE;

  init_freelistp();

  return 0;
}

/******************************************************************************
 * init_freelistp
 * initialize free_listp 's to NULL
 ******************************************************************************/
void init_freelistp() {

  int i;
  for (i = 0; i < NUM_FREE_LIST; i++) {
    free_listp[i] = NULL;
  }
}

/******************************************************************************
 * list_index
 * 
 ******************************************************************************/
static inline int list_index(int size) {
	int index = -1;

	if (size <= 32) {
		index = 0;
	} else if (size <= 64) {
		index = 1;
	} else if (size <= 128) {
		index = 2;
	} else if (size <= 256) {
		index = 3;
	} else if (size <= 512) {
		index = 4;
	} else if (size <= 1024) {
		index = 5;
	} else if (size <= 2048) {
		index = 6;
	} else if (size <= 4096) {
		index = 7;
	} else if (size <= 8192) {
		index = 8;
	} else if (size <= 16384) {
		index = 9;
	} else if (size <= 32768) {
		index = 10;
	} else if (size <= 65536) {
		index = 11;
	} else if (size <= 131072) {
		index = 12;
	} else if (size <= 262144) {
		index = 13;
	} else {
		index = 14;
	}

	return index;
}

/******************************************************************************
 * insert_block
 * 
 ******************************************************************************/
void insert_block(void *bp) {

  void *bpStart = HDRP(bp);
  size_t size = GET_SIZE(bpStart);
  insert_block_sized(bp, size);
}

void insert_block_sized(void *bp, size_t size) {

  int index = list_index(size);
  void *bpStart = HDRP(bp);
  
  if(free_listp[index] != NULL)
    PUT_NEXT(free_listp[index], bpStart);

  PUT_PREV(bp, NULL);
	PUT(bp, free_listp[index]);
	
	free_listp[index] = bpStart;
	
}

/******************************************************************************
 * remove_block
 * 
 ******************************************************************************/
void remove_block(void *bp) {

  size_t size = GET_SIZE(HDRP(bp));
  insert_block_sized(bp, size);
}

void remove_block_sized(void *bp, size_t size){

  int index = list_index(size);
  void* prev = PREV_FREE(bp);
  void* next = NEXT_FREE(bp);
  bool prevAlloced = (prev != NULL);
  bool nextAlloced = (next != NULL);
  
  if(!prevAlloced && !nextAlloced){
    free_listp[index] = NULL;
    
  } else if (!prevAlloced && nextAlloced){
    PUT_NEXT(next, NULL);
    free_listp[index] = next;
    
  } else if (prevAlloced && !nextAlloced){
    PUT_PREV(prev, NULL);
    
  } else {
    PUT_PREV(prev, next);
    PUT_NEXT(next, prev);
  }
  
  PUT(bp, NULL);
  PUT_PREV(bp, NULL);
}

/******************************************************************************
 * coalesce
 * Covers the 4 cases discussed in the text:
 * - both neighbours are allocated
 * - the next block is available for coalescing
 * - the previous block is available for coalescing
 * - both neighbours are available for coalescing
 ******************************************************************************/
void *coalesce(void *bp) {
  
  void *prev = PREV_BLKP(bp);
  void *next = NEXT_BLKP(bp);
  size_t prev_alloc = GET_ALLOC(FTRP(prev));
  size_t next_alloc = GET_ALLOC(HDRP(next));
  size_t size = GET_SIZE(HDRP(bp));

  if (prev_alloc && next_alloc) {
    /* 
     * Case 1 
     * insert block to into one of the free lists
     */
//    insert_block(bp);
    return bp;
  } else if (prev_alloc && !next_alloc) {
    /* 
     * Case 2 
     */
    remove_block(next);
    size += GET_SIZE(HDRP(next));
    
    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));

//    insert_block_sized(bp, size);
    return bp;
  } else if (!prev_alloc && next_alloc) {
    /* 
     * Case 3 
     */
    remove_block(prev);
    size += GET_SIZE(HDRP(prev));

    PUT(FTRP(bp), PACK(size, 0));
    PUT(HDRP(prev), PACK(size, 0));

//    insert_block_sized(prev, size);
    return prev;
  } else {
    /* 
     * Case 4 
     */
    remove_block(prev);
    remove_block(next);
    size += GET_SIZE(HDRP(prev)) + GET_SIZE(FTRP(next));

    PUT(HDRP(prev), PACK(size, 0));
    PUT(FTRP(next), PACK(size, 0));

//    insert_block_sized(prev, size);
    return prev;
  }
}

/******************************************************************************
 * extend_heap
 * Extend the heap by "words" words, maintaining alignment
 * requirements of course. Free the former epilogue block
 * and reallocate its new header
 ******************************************************************************/
void *extend_heap(size_t words) {

  char *bp;
  size_t size;

  /* Allocate an even number of words to maintain alignments */
  size = (words % 2) ? (words + 1) * WSIZE : words * WSIZE;

  if ((bp = mem_sbrk(size)) == (void *) - 1)
    return NULL;

  /* Initialize free block header/footer and the epilogue header */
  PUT(HDRP(bp), PACK(size, 0)); // free block header
  PUT(FTRP(bp), PACK(size, 0)); // free block footer
  PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1)); // new epilogue header

  /* Coalesce if the previous block was free */
  return bp;
}

/******************************************************************************
 * find_fit
 * Traverse the heap searching for a block to fit asize
 * Return NULL if no free blocks can handle that size
 * Assumed that asize is aligned
 ******************************************************************************/
void * find_fit(size_t asize) {

  int index = list_index(asize);
  int i;
  void *list = free_listp[index];
  size_t size = list ? GET_SIZE(list): 0;

//  while(index < 15){
//    if(list != NULL){
//
//      if(curr_size < asize){
//        curr_size = GET_SIZE(list);
//      }
//    }
//    
//    index++;
//		list = free_list[index];
//  }

  
  for (i = index; i < 15; i++, list = free_listp[i]) {

    if (size >= asize) break;
		if (list != NULL) size = GET_SIZE(list);
    
  }
	
	for (i = index; i < 15; i++, list = free_listp[i]) {
		
    if (list != NULL) {
      remove_block(list + WSIZE);
      return (list + WSIZE);
    };
    
	}
  
  assert(list == NULL);
  return NULL;
}

/******************************************************************************
 * place
 * Mark the block as allocated
 ******************************************************************************/
void place(void* bp, size_t asize) {
  /* Get the current block size */
  size_t bsize = GET_SIZE(HDRP(bp));
  size_t diff = bsize - asize;

  if (diff < BLOCKSIZE) {
    PUT(HDRP(bp), PACK(bsize, 1));
    PUT(FTRP(bp), PACK(bsize, 1));
  } else { // split the block
    
    PUT(HDRP(bp), PACK(asize, 1));
    PUT(FTRP(bp), PACK(asize, 1));

    PUT(HDRP(NEXT_BLKP(bp)), PACK(diff, 0));
    PUT(FTRP(NEXT_BLKP(bp)), PACK(diff, 0));
    
    mm_free(NEXT_BLKP(bp));
  }
}

/******************************************************************************
 * mm_free
 * Free the block and coalesce with neighbouring blocks
 ******************************************************************************/
void mm_free(void *bp) {
  if (bp == NULL) {
    return;
  }
  size_t size = GET_SIZE(HDRP(bp));
  PUT(HDRP(bp), PACK(size, 0));
  PUT(FTRP(bp), PACK(size, 0));
  
  insert_block(coalesce(bp));
}

/******************************************************************************
 * mm_malloc
 * Allocate a block of size bytes.
 * The type of search is determined by find_fit
 * The decision of splitting the block, or not is determined
 *   in place(..)
 * If no block satisfies the request, the heap is extended
 ******************************************************************************/
void *mm_malloc(size_t size) {
  size_t asize; /* adjusted block size */
  size_t extendsize; /* amount to extend heap if no fit */
  char * bp;

  /* Ignore spurious requests */
  if (size == 0)
    return NULL;

  /* Adjust block size to include overhead and alignment reqs. */
  if (size <= DSIZE)
    asize = 2 * DSIZE;
  else
    asize = DSIZE * ((size + (DSIZE) + (DSIZE - 1)) / DSIZE);

  /* Search the free list for a fit */
  if ((bp = find_fit(asize)) != NULL) {
    place(bp, asize);
    return bp;
  }

  /* No fit found. Get more memory and place the block */
  extendsize = MAX(asize, CHUNKSIZE);

  if ((bp = extend_heap(extendsize / WSIZE)) == NULL)
    return NULL;
  place(bp, asize);
  return bp;

}

/******************************************************************************
 * mm_realloc
 * Implemented simply in terms of mm_malloc and mm_free
 ******************************************************************************/
void *mm_realloc(void *ptr, size_t size) {
  /* If size == 0 then this is just free, and we return NULL. */
  if (size == 0) {
    mm_free(ptr);
    return NULL;
  }
  /* If oldptr is NULL, then this is just malloc. */
  if (ptr == NULL)
    return (mm_malloc(size));

  void *oldptr = ptr;
  void *newptr;
  
  size_t prevSize = GET_SIZE(HDRP(PREV_BLKP(ptr)));
  size_t copySize = GET_SIZE(HDRP(oldptr));
  size_t nextSize = GET_SIZE(HDRP(NEXT_BLKP(ptr)));
  size_t algnSize = ALIGN_SIZE(size);
//  size_t diff = copySize - algnSize;

  if (algnSize < copySize) {
    return oldptr;
  } else if (GET_ALLOC(HDRP(PREV_BLKP(ptr))) == 0 && prevSize + copySize >= algnSize) {
    algnSize = copySize + prevSize;

    newptr = PREV_BLKP(ptr);

    remove_block(newptr);

    memmove(newptr, oldptr, copySize);

    PUT(HDRP(newptr), PACK(algnSize, 1));
    PUT(FTRP(newptr), PACK(algnSize, 1));

    return newptr;
  } else if (GET_ALLOC(HDRP(NEXT_BLKP(ptr))) == 0 && copySize + nextSize >= algnSize) {
    remove_block(NEXT_BLKP(ptr));

    algnSize = copySize + nextSize;

    PUT(HDRP(oldptr), PACK(algnSize, 1));
    PUT(FTRP(oldptr), PACK(algnSize, 1));

    return oldptr;
  } else if (GET_ALLOC(HDRP(PREV_BLKP(ptr))) == 0 &&
    GET_ALLOC(HDRP(NEXT_BLKP(ptr))) == 0 &&
    prevSize + copySize + nextSize >= algnSize) {

    algnSize = copySize + nextSize + prevSize;


    remove_block(NEXT_BLKP(ptr));
    remove_block(PREV_BLKP(ptr));

    newptr = PREV_BLKP(ptr);

    memmove(newptr, oldptr, algnSize);

    PUT(HDRP(newptr), PACK(algnSize, 1));
    PUT(FTRP(newptr), PACK(algnSize, 1));

    return newptr;
  }

  newptr = mm_malloc(size);
  if (newptr == NULL)
    return NULL;

  /* Copy the old data. */
  copySize = GET_SIZE(HDRP(oldptr));
  if (size < copySize)
    copySize = size;
  memcpy(newptr, oldptr, copySize);
  mm_free(oldptr);
  return newptr;
}

/******************************************************************************
 * mm_check
 * Check the consistency of the memory heap
 * Return nonzero if the heap is consistant.
 ******************************************************************************/
int check_exist(void* bp) {
  int i;
  for (i = 0; i < NUM_FREE_LIST; i++) {
    void* temp = free_listp[i];
    while (temp != NULL) {
      if (temp == bp) return 0;
      temp = (void*) GET(temp);
    }
  }
  return 1;
}

int mm_check(void) {
  //Check if every block in the free list is marked as free
  int i;
  for (i = 0; i < NUM_FREE_LIST; i++) {
    void* temp = free_listp[i];
    while (temp != NULL) {
      if (GET_ALLOC(HDRP(temp))) return 1;
      temp = (void*) GET(temp);
    }
  }

  //check if any contiguous blocks that escaped coalescing
  void *bp;
  for (bp = heap_listp; GET_SIZE(HDRP(bp)) != 0; bp = NEXT_BLKP(bp)) {
    int alloc = GET_ALLOC(HDRP(bp));
    int allocnext = GET_ALLOC(NEXT_BLKP(HDRP(bp)));

    //check for contiguous free blocks
    if (!alloc && !allocnext) return 2;

    //check if the free block exists in freelist
    if (!alloc && check_exist(bp)) return 3;

    //check that pointers in a heap block point to valid heap address
    if (bp > mem_heap_hi() || bp < mem_heap_lo()) return 4;
  }

  return 0;
}