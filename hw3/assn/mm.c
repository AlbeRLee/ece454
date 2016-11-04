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
int list_index(size_t size_);
void insert_block(void *block_);
void insert_block_sized(void *block_, size_t size_);
void remove_block(void *block_);
//size_t aligned_size(size_t size_);

/******************************************************************************
 * Basic Constants and Macros
 * You are not required to use these macros but may find them helpful.
 ******************************************************************************/
#define WSIZE       sizeof(void *)            /* word size (bytes) */
#define DSIZE       (2 * WSIZE)            /* doubleword size (bytes) */
#define CHUNKSIZE   (1<<7)      /* initial heap size (bytes) */
#define ALIGN_SIZE(s) ((s > 0) ?(DSIZE * ((s +(2 *DSIZE) -1) /DSIZE)) :(2 *DSIZE))


#define MAX(x,y) ((x) > (y)?(x) :(y))
#define MIN(x,y) ((x) > (y)?(y) :(x))

/* Pack a size and allocated bit into a word */
#define PACK(size, alloc) ((size) | (alloc))

/* Read and write a word at address p */
#define GET(p)          (*(uintptr_t *)(p))
#define PUT(p,val)      (*(uintptr_t *)(p) = (val))

/* Read the size and allocated fields from address p */
#define GET_SIZE(p)     (GET(p) & ~(DSIZE - 1))
#define GET_ALLOC(p)    (GET(p) & 0x1)

/* Given block ptr bp, compute address of its header and footer */
#define HDRP(bp)        ((char *)(bp) - WSIZE)
#define FTRP(bp)        ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)

/* Given block ptr bp, compute address of next and previous blocks */
#define NEXT_BLKP(bp) ((char *)(bp) + GET_SIZE(((char *)(bp) - WSIZE)))
#define PREV_BLKP(bp) ((char *)(bp) - GET_SIZE(((char *)(bp) - DSIZE)))

// First list is 32 - 63
// Second list is 64 - 127
// ...
// Fourteenth list is 262144 - 524287
// Fifteenth list is 524288+
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
int list_index(size_t size_) {
  int index = 0;
  int size = MIN_LIST(size_);

  while (LEAST_SIGNIFICANT(size)) {
    index += 1;
    size = NEXT_LIST(size);
  }

  index = MIN(index, (NUM_FREE_LIST - 1));

  // each list is 1 WSIZE, use this index as index * WSIZE
  // use with LIST_BY_INDEX
  return index;
}

/******************************************************************************
 * insert_block
 * 
 ******************************************************************************/
void insert_block(void *block_) {

  if (block_ == NULL) return;

  size_t size = GET_SIZE(HDRP(block_));

  insert_block_sized(block_, size);

}

void insert_block_sized(void *block_, size_t size_) {

  if (block_ == NULL) return;

  unsigned index = list_index(size_);

  PUT(block_, free_listp[index]);

  free_listp[index] = block_;
}

/******************************************************************************
 * remove_block
 * 
 ******************************************************************************/
void remove_block(void *block_) {

  if (block_ == NULL) return;

  size_t size = GET_SIZE(HDRP(block_));
  unsigned index = list_index(size);

  void* prev = NULL;
  void* curr = NULL;

  for (curr = free_listp[index]; (curr && curr != block_); prev = curr, curr = (void*) GET(curr)) {

    assert(curr != NULL);
    assert(curr != block_);

    continue;
  }

  assert(curr == block_);

  if (prev == NULL) {
    (free_listp[index] = curr);
  } else {
    (PUT(prev, GET(curr)));
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
     * insert block to into one of the free lists
     */
    insert_block(bp);
    return bp;
  } else if (prev_alloc && !next_alloc) {
    /* 
     * Case 2 
     */
    size += GET_SIZE(HDRP(NEXT_BLKP(bp)));

    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));

    insert_block_sized(bp, size);
    return (bp);
  } else if (!prev_alloc && next_alloc) {
    /* 
     * Case 3 
     */
    remove_block(PREV_BLKP(bp));

    size += GET_SIZE(HDRP(PREV_BLKP(bp)));

    PUT(FTRP(bp), PACK(size, 0));
    PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));

    insert_block_sized(PREV_BLKP(bp), size);
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

    insert_block_sized(PREV_BLKP(bp), size);
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

  void* lastFooter = mem_heap_hi();
  lastFooter -= sizeof (char)*15;

  if (GET_ALLOC(lastFooter) == 0) {
    size -= GET_SIZE(lastFooter);
  }

  if ((bp = mem_sbrk(size)) == (void *) - 1)
    return NULL;

  if (GET_ALLOC(lastFooter) == 0) {
    bp = lastFooter - GET_SIZE(lastFooter) + DSIZE;
    remove_block(bp);
    size = (words % 2) ? (words + 1) * WSIZE : words * WSIZE;
  }

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
    if (free_listp[index]) {
      void* prev = NULL;
      void* curr = NULL;

      for (curr = free_listp[index]; (curr && GET_SIZE(HDRP(curr)) < asize); prev = curr, curr = (void*) GET(curr)) {
        assert(curr != NULL);
        continue;
      }

      if (curr == free_listp[index]) {
        (free_listp[index] = (void*) GET(curr));
      } else {
        (PUT(prev, GET(curr)));
      }

      return curr;
    }
    continue;
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

  if (diff < 64) {
    PUT(HDRP(bp), PACK(bsize, 1));
    PUT(FTRP(bp), PACK(bsize, 1));
  } else {
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
  coalesce(bp);
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
  //  extendsize = asize;
  //  if (size == 112) extendsize = 144; //binary2
  //  if (size == 448) extendsize = 528; //binary1


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
  size_t copySize;

  size_t prevSize = GET_SIZE(HDRP(PREV_BLKP(ptr)));
  size_t currSize = GET_SIZE(HDRP(ptr));
  size_t nextSize = GET_SIZE(HDRP(NEXT_BLKP(ptr)));

  assert(size >= 0);

  size_t alignedSize = ALIGN_SIZE(size);

  if (alignedSize < currSize) {
    return oldptr;
  } else if (GET_ALLOC(HDRP(PREV_BLKP(ptr))) == 0 && prevSize + currSize >= alignedSize) {
    alignedSize = currSize + prevSize;

    newptr = PREV_BLKP(ptr);

    remove_block(newptr);

    memmove(newptr, oldptr, currSize);

    PUT(HDRP(newptr), PACK(alignedSize, 1));
    PUT(FTRP(newptr), PACK(alignedSize, 1));

    return newptr;
  } else if (GET_ALLOC(HDRP(NEXT_BLKP(ptr))) == 0 && currSize + nextSize >= alignedSize) {
    remove_block(NEXT_BLKP(ptr));

    alignedSize = currSize + nextSize;

    PUT(HDRP(oldptr), PACK(alignedSize, 1));
    PUT(FTRP(oldptr), PACK(alignedSize, 1));

    return oldptr;
  } else if (GET_ALLOC(HDRP(PREV_BLKP(ptr))) == 0 &&
    GET_ALLOC(HDRP(NEXT_BLKP(ptr))) == 0 &&
    prevSize + currSize + nextSize >= alignedSize) {

    alignedSize = currSize + nextSize + prevSize;


    remove_block(NEXT_BLKP(ptr));
    remove_block(PREV_BLKP(ptr));

    newptr = PREV_BLKP(ptr);

    memmove(newptr, oldptr, alignedSize);

    PUT(HDRP(newptr), PACK(alignedSize, 1));
    PUT(FTRP(newptr), PACK(alignedSize, 1));

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