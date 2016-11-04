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
int list_index(size_t size);
void insert_block(void *bp);
void insert_block_sized(void *bp, size_t size);
void remove_block(void *bp);
void split(void *bp, size_t target_size, size_t orig_size);

/******************************************************************************
 * Basic Constants and Macros
 * You are not required to use these macros but may find them helpful.
 ******************************************************************************/
#define WSIZE       sizeof(void *)            /* word size (bytes) */
#define DSIZE       (2 * WSIZE)            /* doubleword size (bytes) */
#define BLOCK_SIZE  (2 * DSIZE)
#define CHUNKSIZE   (1<<7)      /* initial heap size (bytes) */
#define ASIZE(s) ((s > 0) ?(DSIZE * ((s +(2 *DSIZE) -1) /DSIZE)) :(2 *DSIZE))


#define MAX(x,y) ((x) > (y)?(x) :(y))
#define MIN(x,y) ((x) > (y)?(y) :(x))

/* Pack a size and allocated bit into a word */
#define PACK(size, alloc) ((size) | (alloc))

/* Read and write a word at address p */
#define GET(p)          (*(uintptr_t *)(p))
#define PUT(p,val)      (*(uintptr_t *)(p) = (val))

/* Read and write a pointer at address p */
#define GETP(p)         (*(uintptr_t **)(p))
#define PUTP(p,val)     (*(uintptr_t **)(p) = (uintptr_t *)val)

/* Read the size and allocated fields from address p */
#define GET_SIZE(p)     (GET(p) & ~(DSIZE - 1))
#define GET_ALLOC(p)    (GET(p) & 0x1)

/* Given block ptr bp, compute address of its header and footer */
#define HDRP(bp)        ((char *)(bp) - WSIZE)
#define FTRP(bp)        ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)

/* Given block ptr bp, compute address of next and previous free blocks*/
#define NFBP(bp)        ((char *)(bp))
#define PFBP(bp)        ((char *)(bp) + WSIZE)

/* Given block ptr bp, compute address of next and previous blocks */
#define NEXT_BLKP(bp) ((char *)(bp) + GET_SIZE(((char *)(bp) - WSIZE)))
#define PREV_BLKP(bp) ((char *)(bp) - GET_SIZE(((char *)(bp) - DSIZE)))

// Zeroth list is 0 - 32
// First list is 33 - 64
// Second list is 65 - 128
// ...
// Thirteenth list is 131072 - 262144
// Fourteenth list is 262145+
#define NUM_FREE_LIST 15

#define LEAST_SIGNIFICANT(x) (x & 0x1)

#define FIRST_LIST_MIN_SIZE_POW 5
#define FIRST_LIST_MAX_SIZE_POW 6 // first list is [32,64)

#define IS_LIST(x) (LEAST_SIGNIFICANT(x))
#define FIRST_LIST(x) (x >> FIRST_LIST_MIN_SIZE_POW)
#define NEXT_LIST(x) (x >> 1)

void* heap_listp = NULL;
void* free_listp[NUM_FREE_LIST];

/******************************************************************************
 * mm_init
 * Initialize the heap, including "allocation" of the
 * prologue and epilogue
 ******************************************************************************/
int mm_init(void) {

  // INITIAL HEAP is NUM FREE LIST + 3 (freelists + prologue(2) + epilogue)
  if ((heap_listp = mem_sbrk((NUM_FREE_LIST + 3) * WSIZE)) == (void *) - 1)
    return -1;

  init_freelistp();

//  PUT(heap_listp, 0); // alignment padding
  PUT(heap_listp + ((NUM_FREE_LIST + 0) * WSIZE), PACK(DSIZE, 1)); // prologue header //DSIZE overhead
  PUT(heap_listp + ((NUM_FREE_LIST + 1) * WSIZE), PACK(DSIZE, 1)); // prologue footer //DSIZE overhead
  PUT(heap_listp + ((NUM_FREE_LIST + 2) * WSIZE), PACK(0, 1)); // epilogue header

  //BEGIN HEAP is NUM FREE LIST + 1 (prologue footer)
  heap_listp += (WSIZE * (NUM_FREE_LIST + 1));

  
  return 0;
}

/******************************************************************************
 * init_freelistp
 * initialize free_listp 's to NULL
 ******************************************************************************/
void init_freelistp() {
  // FREE_LIST = HEAP_LIST
  int i;
  for (i = 0; i < NUM_FREE_LIST; i++) {
    PUT(heap_listp + (i * WSIZE), 0);
    free_listp[i] = heap_listp + (i * WSIZE);
  }
  
}

/******************************************************************************
 * list_index
 * 
 ******************************************************************************/
int list_index(size_t size) {
//  int index = 0;
//  int size = MIN_LIST(size);
//
//  while (LEAST_SIGNIFICANT(size)) {
//    index += 1;
//    size = NEXT_LIST(size);
//  }
//
//  index = MIN(index, (NUM_FREE_LIST - 1));
//
//  // each list is 1 WSIZE, use this index as index * WSIZE
//  // use with LIST_BY_INDEX
//  return index;
  
  int index;

	if (size <= 32)
		index = 0;
	else if (size <= 64)
		index = 1;
	else if (size <= 128)
		index = 2;
	else if (size <= 256)
		index = 3;
	else if (size <= 512)
		index = 4;
	else if (size <= 1024)
		index = 5;
	else if (size <= 2048)
		index = 6;
	else if (size <= 4096)
		index = 7;
	else if (size <= 8192)
		index = 8;
	else if (size <= 16384)
		index = 9;
	else if (size <= 32768)
		index = 10;
	else if (size <= 65536)
		index = 11;
	else if (size <= 131072)
		index = 12;
	else if (size <= 262144)
		index = 13;
	else
		index = 14;
	return index;
}

/******************************************************************************
 * insert_block
 * 
 ******************************************************************************/
void insert_block(void *bp) {

  size_t size = GET_SIZE(HDRP(bp));
  insert_block_sized(bp, size);
}

void insert_block_sized(void *bp, size_t size) {

  int index = list_index(size);
  void* list = free_listp[index];

  PUT(bp, GET(list));
  PUTP(PFBP(bp), list);
  
	if (GET(list) != 0) {
		PUTP(PREVP(*list), bp);
	}
  
	PUTP(listp, bp);
}

/******************************************************************************
 * remove_block
 * 
 ******************************************************************************/
void remove_block(void *bp) {

  if(GET_ALLOC(HDRP(bp))) return;

//  size_t size = GET_SIZE(HDRP(bp));
//  unsigned index = list_index(size);

  if(*NFBP(bp) == 0){
    PUT(PFBP(bp), 0);
  } else {
    PUT(PFBP(bp), NFBP(bp));
    PUT(PFBP(NFBP(bp)), PFBP(bp));
  }
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
  size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
  size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
  size_t size = GET_SIZE(HDRP(bp));
  
  if (prev_alloc && next_alloc) {
    /* 
     * Case 1 
     */
//    insert_block_sized(bp, size);
    return bp;
  } else if (prev_alloc && !next_alloc) {
    /* 
     * Case 2 
     */
    remove_block(NEXT_BLKP(bp));
    size += GET_SIZE(HDRP(NEXT_BLKP(bp)));

    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));

//    insert_block_sized(bp, size);
    return (bp);
  } else if (!prev_alloc && next_alloc) {
    /* 
     * Case 3 
     */
    remove_block(PREV_BLKP(bp));

    size += GET_SIZE(HDRP(PREV_BLKP(bp)));
    PUT(FTRP(bp), PACK(size, 0));
    PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));

//    insert_block_sized(PREV_BLKP(bp), size);
    return (PREV_BLKP(bp));
  } else {
    /* 
     * Case 4 
     */
    remove_block(PREV_BLKP(bp));
    remove_block(NEXT_BLKP(bp));

    size += GET_SIZE(HDRP(PREV_BLKP(bp))) +
      GET_SIZE(FTRP(NEXT_BLKP(bp)));

    PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
    PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));

//    insert_block_sized(PREV_BLKP(bp), size);
    return (PREV_BLKP(bp));
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

//  void* lastFooter = mem_heap_hi();
//  lastFooter -= sizeof (char)*15;

//  if (GET_ALLOC(lastFooter) == 0) {
//    size -= GET_SIZE(lastFooter);
//  }

  if ((bp = mem_sbrk(size)) == (void *) - 1)
    return NULL;

//  if (GET_ALLOC(lastFooter) == 0) {
//    bp = lastFooter - GET_SIZE(lastFooter) + DSIZE;
//    remove_block(bp);
//    size = (words % 2) ? (words + 1) * WSIZE : words * WSIZE;
//  }

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

  int index;

  for (index = list_index(asize); index < NUM_FREE_LIST; index++) {
    void* list = free_listp[index];
    if(list != NULL){
      if(GET_SIZE(HDRP(list)) >= asize){
        return list;
      }
    }
  }
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

  if (diff < BLOCK_SIZE) {
    PUT(HDRP(bp), PACK(bsize, 1));
    PUT(FTRP(bp), PACK(bsize, 1));
    
  } else {
    PUT(HDRP(bp), PACK(asize, 1));
    PUT(FTRP(bp), PACK(asize, 1));

    PUT(HDRP(NEXT_BLKP(bp)), PACK(diff, 0));
    PUT(FTRP(NEXT_BLKP(bp)), PACK(diff, 0));

    insert_block(NEXT_BLKP(bp));
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
  
  bp = coalesce(bp);
  size_t size = GET_SIZE(HDRP(bp));
  PUT(HDRP(bp), PACK(size, 0));
  PUT(FTRP(bp), PACK(size, 0));
  insert_block(bp);
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
    remove_block(bp);
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

  size_t prevSize = GET_SIZE(HDRP(PREV_BLKP(ptr)));
  size_t currSize = GET_SIZE(HDRP(ptr));
  size_t nextSize = GET_SIZE(HDRP(NEXT_BLKP(ptr)));

  size_t aSize = ASIZE(size);
  size_t diff = aSize - currSize;

  if (diff < 0) {
    return ptr;
    
  } else if (GET_ALLOC(HDRP(PREV_BLKP(ptr))) == 0 && prevSize + currSize >= aSize) {
    aSize = currSize + prevSize;

    void* new = PREV_BLKP(ptr);

    remove_block(new);

    memmove(new, ptr, currSize);

    PUT(HDRP(new), PACK(aSize, 1));
    PUT(FTRP(new), PACK(aSize, 1));

    return new;
  } else if (GET_ALLOC(HDRP(NEXT_BLKP(ptr))) == 0 && currSize + nextSize >= aSize) {
    remove_block(NEXT_BLKP(ptr));

    aSize = currSize + nextSize;

    PUT(HDRP(oldptr), PACK(aSize, 1));
    PUT(FTRP(oldptr), PACK(aSize, 1));

    return oldptr;
  } else if (GET_ALLOC(HDRP(PREV_BLKP(ptr))) == 0 &&
    GET_ALLOC(HDRP(NEXT_BLKP(ptr))) == 0 &&
    prevSize + currSize + nextSize >= aSize) {

    aSize = currSize + nextSize + prevSize;


    remove_block(NEXT_BLKP(ptr));
    remove_block(PREV_BLKP(ptr));

    newptr = PREV_BLKP(ptr);

    memmove(newptr, oldptr, aSize);

    PUT(HDRP(newptr), PACK(aSize, 1));
    PUT(FTRP(newptr), PACK(aSize, 1));

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

void split(void *bp, size_t target_size, size_t orig_size) {
	void * split_bp;

	// Mark header and footer
	PUT(HDRP(bp), PACK(target_size, 1));
	PUT(FTRP(bp), PACK(target_size, 1));

	// Find the next block ptr as split ptr
	split_bp = NEXT_BLKP(bp);

	// Mark split ptr's header and footer
	PUT(HDRP(split_bp), PACK((orig_size-target_size), 0));
	PUT(FTRP(split_bp), PACK((orig_size-target_size), 0));

	// Insert this free block to the list
	insert_block(split_bp);

}
//void *split_block(void *bp, size_t asize) {
//
//  void *end_of_block, *new_bp;
//  size_t old_size = GET_SIZE(HDRP(bp));
//  end_of_block = (char *) bp + old_size - WSIZE;
//  new_bp = (char *) end_of_block - asize + WSIZE;
//  PUT(HDRP(new_bp), PACK(asize, 0));
//  PUT(FTRP(new_bp), PACK(asize, 0));
//
//  PUT(HDRP(bp), PACK(old_size - asize, 0));
//  PUT(FTRP(bp), PACK(old_size - asize, 0));
//
//  return new_bp;
//}

/******************************************************************************
 * mm_check
 * Check the consistency of the memory heap
 * Return nonzero if the heap is consistant.
 ******************************************************************************/
int mm_check(void) {

  return 1;
}
