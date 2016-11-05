/*
 * Dynamic Memory Allocation Library
 * ECE 454 ~ Lab 3
 * Taylan Gocmen and Gligor Djogo, from the starter code supplied by Prof.
 * 
 * This implementation of malloc, realloc, and free uses a segregated free list
 * structure with 15 size classes. It also implements the first fit search
 * algorithm, constant time coalescing, block splitting, and it tries to check
 * for efficient shortcuts in realloc.
 * 
 * Segregated free list:
 *  is comprised of 15 size classes corresponding to 15 explicitly linked lists
 * of free blocks. The first class is size 0 to 32, followed by 33 to 64,
 * 65 to 128, etc. up to the last class which is everything above 2^18.
 * Inserting is always at the front of the list, removing is done like a doubly
 * linked list, and there is an index function that will map a given size to 
 * its corresponding class. Pointers to the beginning of each list are stored
 * in the global array 'free_listp' to facilitate access. These are initialized
 * to NULL during mm_init.
 * 
 * Allocated block structure: same as implicit list, with sizes and an alloc bit
 * 
 * [header(1)][- - - - - - - - - data payload - - - - - - - - -][footer(1)]
 * 
 * Free block structure: the first two words of unused space store next and the
 *  previous pointers forming the explicit free lists. There is still a header
 *  and footer storing the size and alloc bit = 0
 * 
 * [header(0)][prev ptr][next ptr][x x x padding/old data x x x][footer(0)]
 * 
 * Malloc
 *  will find the corresponding index into the segregated list, and search its
 * classes in ascending order until it finds the first fitting block. This block
 * is marked as allocated and removed from the free list. It is passed to the
 * 'place' function which will split it into a used and an unused part. It
 * returns the unused part to the free list and returns the pointer to the used
 * part to the caller. There is no added padding due to internal fragmentation.
 * If the first fit search fails, then the heap needs to be extended through a
 * call to 'mm_sbrk' and the new heap chunk is split into a desired size.
 * 
 * Free
 *  will mark the block as unallocated, join it with its neighbors if possible
 * (using the constant time coalescing cases) and insert it into the free list
 * into the appropriate class depending on its size.
 *
 * Realloc
 *  will try to avoid copying memory by exploiting possible coalescing with 
 * adjacent blocks. However, if no shortcuts can be found, it is implemented
 * by calls to malloc, memcopy, and free.
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
static inline int list_index(size_t size);
void insert_block(void *bp);
void remove_block(void *bp);
void remove_block_sized(void *bp, size_t size);

/******************************************************************************
 * Basic Constants and Macros
 * We implemented our own macros to facilitate manipulation of the free list
 ******************************************************************************/
#define WSIZE       sizeof(void *)            /* word size (bytes) */
#define DSIZE       (2 * WSIZE)            /* doubleword size (bytes) */
#define CHUNKSIZE   (1<<8)      /* initial heap size (bytes) */

/* minimum block size to store useful information */
#define BLOCKSIZE   32
/* align given size to a double word boundary */
#define ALIGN_SIZE(s) ((s > (DSIZE)) ?(DSIZE * ((s +(2 *DSIZE) -1) /DSIZE)) :(2 *DSIZE))


#define MAX(x,y) ((x) > (y)?(x) :(y))
#define MIN(x,y) ((x) > (y)?(y) :(x))

/* Pack a size and allocated bit into a word */
#define PACK(size, alloc) ((size) | (alloc))

/* Read and write a word at address p */
#define GET(p)          (*(uintptr_t *)(p))
#define PUT(p,val)      (*(uintptr_t *)(p) = (val))

/* Read and write a pointer at address p; avoid compiler conversion warnings */
#define GET_P(p)          (*(uintptr_t **)(p))
#define PUT_P(p,val)      (*(uintptr_t **)(p) = (uintptr_t *)val)

/* Write to next and previous list pointers of block p */
#define PUT_NEXT(p, val)  (PUT_P((p + DSIZE), val))
#define PUT_PREV(p, val)  (PUT_P((p + WSIZE), val))

/* Read the size and allocated fields from address p */
#define GET_SIZE(p)     (GET(p) & ~(DSIZE - 1))
#define GET_ALLOC(p)    (GET(p) & 0x1)

/* Given block ptr bp, compute address of its header and footer */
#define HDRP(bp)        ((char *)(bp) - WSIZE)
#define FTRP(bp)        ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)
/* Given block ptr bp, compute the address of the next block in its free list
   and the address of the previous block in its free list */
#define PREV_FREE(bp)   ((void *)GET(bp))
#define NEXT_FREE(bp)   ((void *)GET(bp + WSIZE))

/* Given block ptr bp, compute address of next and previous blocks */
#define NEXT_BLKP(bp)   ((char *)(bp) + GET_SIZE(((char *)(bp) - WSIZE)))
#define PREV_BLKP(bp)   ((char *)(bp) - GET_SIZE(((char *)(bp) - DSIZE)))

// Segmented Free List Boundaries
// Zeroth list is 0 - 32
// First list is 33 - 64
// Second list is 65 - 128
// ...
// Thirteenth list is 131073 - 262144
// Fourteenth list is 262145 +
#define NUM_FREE_LIST 15

void* heap_listp = NULL;
void* free_listp[NUM_FREE_LIST];


/******************************************************************************
 * mm_init
 * Initialize the heap, including "allocation" of the prologue and epilogue, 
 * moving the heap pointer to the empty heap, and padding it to a DSIZE boundary
 * Initialize the segregated free list via function call.
 ******************************************************************************/
int mm_init(void) {

  // first "system call" to get an initial heap to work with
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
 * Initialize every free_listp element to NULL, meaning that every size class
 * in the segregated free list points to an empty list.
 ******************************************************************************/
void init_freelistp() {

  int i;
  for (i = 0; i < NUM_FREE_LIST; i++) {
    free_listp[i] = NULL; // empty list
  }
}

/******************************************************************************
 * list_index
 * Resolve a given size into its corresponding index in the free list.
 * Output is in range 0 to 14 and the size classes follow powers of 2.
 * This value represents the size class that 'size' fits into and can be used
 * to index into the global array of list head pointers.
 * Negative value means a fail occured.
 ******************************************************************************/
static inline int list_index(size_t size) {
  
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

	return index; // return a negative if fail occurs
}

/******************************************************************************
 * insert_block
 * Insert the given block, pointed to by bp, into the beginning of free list.
 * Find the corresponding list_index using its block size, then find the head
 * pointer to the start of its size class list, and insert the block.
 ******************************************************************************/
void insert_block(void *bp) {

  /* find the index into the global free list array, depending on size of bp */
  void *bpStart = HDRP(bp);
  size_t size = GET_SIZE(bpStart);
  int index = list_index(size); // get size class index from block size
  
  /* setting bp's prev to NULL means its at the beginning of list
   * connect bp's next pointer to the rest of the free list
   * connect the head to point to bp */
  if(free_listp[index] != NULL)
    PUT_NEXT(free_listp[index], bpStart);
  PUT_PREV(bp, NULL);
  PUT_P(bp, free_listp[index]);
  free_listp[index] = bpStart;
}

/******************************************************************************
 * remove_block
 * Remove the block pointed to by bp from its current position in the free list.
 * There are four cases depending on which neighbors bp has:
 * - either it is alone in the list
 * - first
 * - last
 * - or between two other blocks.
 ******************************************************************************/

void remove_block(void *bp){

  size_t size = GET_SIZE(HDRP(bp));
  int index = list_index(size); // get size class index
  void* prev = PREV_FREE(bp);
  void* next = NEXT_FREE(bp);
  bool prevAlloced = (prev != NULL); // check if bp is linked back to prev
  bool nextAlloced = (next != NULL); // check if bp is linked fwrd to next
  
  if(!prevAlloced && !nextAlloced){
    /* Case 1 bp is alone in the list, no next or prev pointers */
    free_listp[index] = NULL;
    
  } else if (!prevAlloced && nextAlloced){
    /* Case 2 bp is last element
              disconnect it from the list by writing in NULL*/
    PUT_PREV(next, NULL);
    
  } else if (prevAlloced && !nextAlloced){
    /* Case 3  bp is the first element
               change the head value in free_listp to point to the next block */
    PUT_NEXT(prev, NULL);
    free_listp[index] = prev;
    
  } else {
    /* Case 4 bp is between elements, swap pointers in prev and next
       so that the two neighbors do not point to bp any more, they skip over it*/
    PUT_PREV(next, prev);
    PUT_NEXT(prev, next);
  }
  
  /* remove all links coming from bp now that it has been removed */
  PUT_P(bp, NULL);
  PUT_PREV(bp, NULL);
}

/******************************************************************************
 * coalesce
 * Covers the 4 cases:
 * - both neighbours are allocated
 * - the next block is available for coalescing
 * - the previous block is available for coalescing
 * - both neighbours are available for coalescing
 * Removes any free blocks that were coalesced from the free list.
 * The newly coalesced superblock needs to be added to free list after return.
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
     * there is no free neighbor to join together with bp, no change to bp
     */
    return bp;
  } else if (prev_alloc && !next_alloc) {
    /* 
     * Case 2 
     * only the next neighbor is free, join bp with the next block
     * remove the next block from the free list as it will be inserted later
     * along with the newly coalesced superblock
     */
    remove_block(next);
    size += GET_SIZE(HDRP(next));
    // overwrite the current block to include the total size of both blocks
    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));

    return bp;
  } else if (!prev_alloc && next_alloc) {
    /* 
     * Case 3 
     * only the previous neighbor is free, join bp with the prev block
     * remove the prev block from the free list
     */
    remove_block(prev);
    size += GET_SIZE(HDRP(prev));
    // overwrite prev block's header to include the total size of both blocks
    PUT(FTRP(bp), PACK(size, 0));
    PUT(HDRP(prev), PACK(size, 0));

    return prev;
  } else {
    /* 
     * Case 4 
     * both neighbors are free, combine three blocks into one by overwriting
     * the previous block's header with their total size
     */
    remove_block(prev);
    remove_block(next);
    size += GET_SIZE(HDRP(prev)) + GET_SIZE(FTRP(next));
    PUT(HDRP(prev), PACK(size, 0));
    PUT(FTRP(next), PACK(size, 0));
    
    return prev;
  }
}

/******************************************************************************
 * extend_heap
 * Extend the heap by 'words' # of words, maintaining the alignment requirement.
 * The former epilogue block is freed by overwriting it with the size of new 
 * heap. A new epilogue is written back in the last word of new heap memory.
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

  return bp;
}

/******************************************************************************
 * find_fit
 * Traverse the segmented free list searching for a block to fit asize, using
 * the first fit algorithm along with segregated list search.
 * First find the index into the class size corresponding to the range in which
 * asize should fall. Then check whether there are any blocks in that list, and
 * if there are not, move up to the next higher size class.
 * When the first free block larger than asize is found, return its pointer.
 * Return NULL if no free blocks currently exist that can handle that size.
 * Assumed that asize has previously been DSIZE aligned.
 ******************************************************************************/
void * find_fit(size_t asize) {

  int index = list_index(asize); // get size class index
  int i;
  void *list = free_listp[index];
  size_t size;

  /* Check if there is a block larger than asize
     Start saerching at the free list at the index and move on to the next
     higher i value if end of list (NULL) is reached */
  for (i = index; i < NUM_FREE_LIST; i++, list = free_listp[i]) {
    size = (list != NULL) ? GET_SIZE(list): 0;
    if (size >= asize){
      break;
    }
  }
  
  /* Remove the found block from the traversed free list */
  for (i = index; i < NUM_FREE_LIST; i++, list = free_listp[i]) {
      if (list != NULL) {
          remove_block(list + WSIZE);
          return (list + WSIZE);
      }
  }
  
  return NULL; // nothing >asize found
}

/******************************************************************************
 * place
 * Mark the block as allocated and decide on block splitting.
 * Block splitting is only done if there is enough unused space left over,
 * which is determined by the overhead requirements and empirical tests.
 ******************************************************************************/
void place(void* bp, size_t asize) {
  /* Get the current block size
   * and find the difference between the actual size and the desired size */
  size_t bsize = GET_SIZE(HDRP(bp));
  size_t diff = bsize - asize;

  /* Block splitting
   * if the unused space in the current block is larger than the minimum
   * useful block size, then we can perform a meaningful split; otherwise we 
   * accept some internal fragmentation which is hopefully negligible */
  if (diff < BLOCKSIZE) {
    PUT(HDRP(bp), PACK(bsize, 1));
    PUT(FTRP(bp), PACK(bsize, 1));
    
  } else {
    PUT(HDRP(bp), PACK(asize, 1)); // to split, do not write in full size
    PUT(FTRP(bp), PACK(asize, 1));

    PUT(HDRP(NEXT_BLKP(bp)), PACK(diff, 0)); // un-alloc the rest of block
    PUT(FTRP(NEXT_BLKP(bp)), PACK(diff, 0));
    
    mm_free(NEXT_BLKP(bp)); // free the unused block memory back to free list
  }
}

/******************************************************************************
 * mm_free
 * Free the block by marking its alloc bits = 0 inside the header.
 * Coalesce it with neighboring blocks and insert the newly coalesced superblock
 * into the free list.
 ******************************************************************************/
void mm_free(void *bp) {
  if (bp == NULL) { // freeing NULL is invalid
    return;
  }
  size_t size = GET_SIZE(HDRP(bp));
  PUT(HDRP(bp), PACK(size, 0));
  PUT(FTRP(bp), PACK(size, 0));
  
  insert_block(coalesce(bp)); // insert into segregated free list via fcn call
}

/******************************************************************************
 * mm_malloc
 * Allocate a block of size bytes and return pointer to the beginning.
 * The type of search is a first fit algorithm through the segregated list.
 * If no block satisfies the request, the heap is extended.
 * Once free block is found, the call to place will make the decision of
 * splitting the block, or not and the block pointer is returned thereafter.
 ******************************************************************************/
void *mm_malloc(size_t size) {
  size_t asize; /* adjusted block size */
  size_t extendsize; /* amount to extend heap if no fit */
  char * bp;

  /* Ignore spurious requests */
  if (size == 0)
    return NULL;

  /* Adjust block size to include overhead and DSIZE alignment requirement */
  asize = ALIGN_SIZE(size);
  
  /* Search the free list for a block that fits */
  if ((bp = find_fit(asize)) != NULL) { // first fit search through seg list
    place(bp, asize); // block splitting
    return bp;
  }

  /* No fit found. Get more memory and place the block in the new */
  extendsize = MAX(asize, CHUNKSIZE);

  if ((bp = extend_heap(extendsize / WSIZE)) == NULL)
    return NULL;
  place(bp, asize); // split new heap memory into usable block
  return bp;

}

/******************************************************************************
 * mm_realloc
 * If size = 0 it is just a free, and if ptr == NULL it is just a malloc.
 * Test for the following shortcut cases to save time and resources:
 * - realloc size smaller than original block
 * - prev block is free and will make realloc successful
 * - next block is free and is large enough to coalesce and successful realloc
 * - both surrounding blocks are free and only their combined size is enough
 *   to successfully realloc, while moving over the memory
 * Default is to implemented simply in terms of mm_malloc, memcopy, and mm_free
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
  
  /* check the neighbors in the heap using implicit listing */
  bool prevAlloced = GET_ALLOC(HDRP(PREV_BLKP(ptr)));
  bool nextAlloced = GET_ALLOC(HDRP(NEXT_BLKP(ptr)));
  
  /* size variables used to determine if realloc shortcuts can be taken */
  size_t prevSize = GET_SIZE(HDRP(PREV_BLKP(ptr)));
  size_t copySize = GET_SIZE(HDRP(oldptr));
  size_t nextSize = GET_SIZE(HDRP(NEXT_BLKP(ptr)));
  size_t algnSize = ALIGN_SIZE(size);
  
  /* extra memory which needs to be found */
  int diffSize = (int)algnSize - (int)copySize;
  
  if (diffSize < 0) {
    /* Case 1
     * size is smaller than the original block size, no realloc needed */
    return oldptr;
    
  } else if (!prevAlloced && prevSize >= diffSize) {
    /* Case 2 
     * The prev block is free to be coalesced and its size is large enough
     * to meet our realloc size requirement. */
    algnSize = copySize + prevSize;

    newptr = PREV_BLKP(ptr);

    remove_block(newptr);

    memmove(newptr, oldptr, copySize);

    PUT(HDRP(newptr), PACK(algnSize, 1));
    PUT(FTRP(newptr), PACK(algnSize, 1));

    return newptr;
    
  } else if (!nextAlloced && nextSize >= diffSize) {
    /* Case 3 
     * The next block is free to be coalesced and its size is large enough
     * to meet our realloc size requirement. */
    
    remove_block(NEXT_BLKP(ptr));
    algnSize = copySize + nextSize;
    PUT(HDRP(oldptr), PACK(algnSize, 1));
    PUT(FTRP(oldptr), PACK(algnSize, 1));

    return oldptr;
    
  } else if (!prevAlloced && !nextAlloced && prevSize + nextSize >= diffSize) {
    /* Case 4 
     * Both surrounding blocks are free and the addition of the prev block size
     * to the current + next sizes meets our realloc size requirement. */
    
    algnSize = copySize + nextSize + prevSize;
    remove_block(NEXT_BLKP(ptr));
    remove_block(PREV_BLKP(ptr));
    newptr = PREV_BLKP(ptr);
    
    memmove(newptr, oldptr, algnSize); // only slightly move the mem to the left
    PUT(HDRP(newptr), PACK(algnSize, 1));
    PUT(FTRP(newptr), PACK(algnSize, 1));

    return newptr;
    
  } else {
    /* Default:
     * If we cannot find any shortcut allocation we call malloc and free 
     * and just copy over the requirement memory.
     * This brings down our throughput, which is why we prefer shortcuts! */
    newptr = mm_malloc(size);
    if (newptr == NULL)
      return NULL;
    
    copySize = GET_SIZE(HDRP(oldptr));
    if (size < copySize)
      copySize = size;
    memcpy(newptr, oldptr, copySize); // copy over old memory to new location
    mm_free(oldptr);
    
    return newptr;
  }
}

/******************************************************************************
 * memory checking
 ******************************************************************************/
//unsigned NUM_BLOCKS_IN_HEAP; (deprecated)
//unsigned NUM_BLOCKS_IN_LIST; (deprecated)

/******************************************************************************
 * Some examples of what a heap checker might check are:
 * 
 * (1) Is every block in the free list marked as free?
 *    checked in mm_check-(c)
 * 
 * (2) Are there any contiguous free blocks that somehow escaped coalescing?
 *    checked in check_heap-(a)
 * 
 * (3) Is every free block actually in the free list?
 *    checked in check_heap-(b)
 * 
 * (4) Do the pointers in the free list point to valid free blocks?
 *    checked in check_all_lists-(a)
 * 
 * (5) Do any allocated blocks overlap?
 *    checked in check_heap-(c & d)
 * 
 * (6) Do the pointers in a heap block point to valid heap addresses?
 *    checked in check_heap-(e)
 * 
 ******************************************************************************/

/******************************************************************************
 * search_list
 *  search the given list for a block
 *  return true if found false elsewise
 ******************************************************************************/
bool search_list(void* searchB, void* searchL) {

  void* block;
  for (block = searchL; block != NULL; block = NEXT_FREE(block)) {
    if (block == searchB)
      return true;
  }
  return false;
}

/******************************************************************************
 * search_list
 *  search all lists for a given block
 *  return true if found in any one of the lists false elsewise
 ******************************************************************************/
bool search_all_lists(void* searchB) {

  int i;
  void *list;

  // go through every list
  for (i = 0; i < NUM_FREE_LIST; i++) {
    list = free_listp[i];
    // search list for block
    if (search_list(searchB, list) == true)
      return true;
  }
  return false;

}

/******************************************************************************
 * check_list
 * check every block in the given list for the following:
 *  (a) wheter or not the block is allocated
 *    satisfies (4) checks if pointers in the free list point to valid free blocks
 ******************************************************************************/
bool check_list(void* checkL) {

  void* block;
  for (block = checkL; block != NULL; block = NEXT_FREE(block)) {

    // (a) block is not allocated not a free block
    if (GET_ALLOC(block))
      return false;

//    NUM_BLOCKS_IN_LIST++;
  }
  return true;
}

/******************************************************************************
 * check_all_lists
 *  go through every list and check list for consistency
 ******************************************************************************/
bool check_all_lists() {
  int i;
  void *list;

  // go through every list
  for (i = 0; i < NUM_FREE_LIST; i++) {
    list = free_listp[i];

    // check list for consistency
    if (check_list(list) == false)
      return false;
  }
  return true;
}

/******************************************************************************
 * check_heap
 * check the heap list for consistency
 * check every block in the heap_listp for the following:
 *  (a) wheter or not two free blocks next to each other exist ie. coalescing
 *    satisfies (2) checks if any contiguous free blocks that escaped coalescing
 *  (b) wheter a non allocated block is in the segregated lists (deprecated)
 *    satisfies (3) checks if every free block actually in the free list
 *  (c) wheter or not the header and the footer of the block are allocated
 *  (d) wheter or not the header and the footer of the block are of same size
 *    c & d satisfies (5) checks if any allocated blocks overlap
 *  (e) wheter or not the block has a valid adress ie. within memory heap
 *    satisfies (6) checks if pointers in a heap block point to valid heap addresses
 ******************************************************************************/
bool check_heap() {
  void* block = heap_listp;
  void* next;
  bool blockAlloced = GET_ALLOC(HDRP(block));
  bool nextAlloced;
  size_t blockSize = GET_SIZE(HDRP(block));

  while (blockSize != 0) {

    // set up next to check coalesceing
    next = NEXT_BLKP(block);
    nextAlloced = GET_ALLOC(HDRP(next));

    // (a) couldn't coalesce two free blocks next to each other
    if (!blockAlloced && !nextAlloced)
      return false;

    //    // (b) a non allocated block is in the segregated lists
    //    // depracated this because it takes too long to check
    //    if(!blockAlloced && search_all_lists(block))
    //      return false;

    // (c) header and footer are not allocated
    if (blockAlloced != GET_ALLOC(FTRP(block)))
      return false;

    // (d) header and footer are not the same size
    if (blockSize != GET_SIZE(FTRP(block)))
      return false;

    // (e) the block does not have a valid address 
    if (block > mem_heap_hi() || block < mem_heap_lo())
      return false;

//    if (blockAlloced)
//      (NUM_BLOCKS_IN_HEAP)++;

    // move the block to next one
    block = next;
    blockSize = GET_SIZE(HDRP(block));
    blockAlloced = GET_ALLOC(HDRP(block));

  }

  return true;
}

/******************************************************************************
 * mm_check
 * Check the consistency of the following:
 *  (a) check_heap: memory heap list, counts the number of blocks
 *  (b) check_all_lists: segregated lists, counts the number of blocks
 *  (c) checks if there are same number of blocks in the heap that are free 
 *      as all the ones in the lists (depracated)
 *    satisfies (1) check if every block in the free list is marked as free
 * Return nonzero if the heap is consistant.
 ******************************************************************************/
int mm_check(void) {

//  NUM_BLOCKS_IN_HEAP = 0;
//  NUM_BLOCKS_IN_LIST = 0;
  
  // (a) Check the heap
  if (check_heap() == false)
    return 0;

  // (b) Now check the free lists
  if (check_all_lists() == false)
    return 0;

//  // (c) Check if there are same number of block (depracated: will not work)
//  if (NUM_BLOCKS_IN_HEAP != NUM_BLOCKS_IN_LIST)
//    return 0;
  
  return 1;
}
