/*
 * OLD HEADER:
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

#include "mm.h"
#include "memlib.h"

/*********************************************************
 * NOTE TO STUDENTS: Before you do anything else, please
 * provide your team information in the following struct.
 ********************************************************/
team_t team = {
    /* Team name */
    "Hugh Mungus",
    /* First member's full name */
    "Taylan Gocmen",
    /* First member's email address */
    "taylan.gocmen@mail.utoronto.ca",
    /* Second member's full name */
    "Gligor Djogo",
    /* Second member's email address */
    "g.djogo@mail.utoronto.ca"
};

/*************************************************************************
 * Basic Constants and Macros
 * You are not required to use these macros but may find them helpful.
*************************************************************************/
#define WSIZE       sizeof(void *)            /* word size (bytes) */
#define DSIZE       (2 * WSIZE)            /* doubleword size (bytes) */
#define CHUNKSIZE   (1<<7)      /* initial heap size (bytes) */

#define MAX(x,y) ((x) > (y)?(x) :(y))

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

// new macros to get the next/prev link list pointers of the free list
#define NEXT_FREE(bp) ((char *)(bp) - 2*WSIZE)
#define PREV_FREE(bp) ((char *)(bp) - 3*WSIZE)

void* heap_listp = NULL;
void* free_shortcut = NULL;

/**********************************************************
 * mm_init
 * Initialize the heap, including "allocation" of the
 * prologue and epilogue
 **********************************************************/
 int mm_init(void)
 {
   if ((heap_listp = mem_sbrk(4*WSIZE)) == (void *)-1)
         return -1;
     PUT(heap_listp, 0);                         // alignment padding
     PUT(heap_listp + (1 * WSIZE), PACK(DSIZE, 1));   // prologue header
     PUT(heap_listp + (2 * WSIZE), PACK(DSIZE, 1));   // prologue footer
     PUT(heap_listp + (3 * WSIZE), PACK(0, 1));    // epilogue header
     heap_listp += DSIZE;
     
     free_shortcut = heap_listp;

     return 0;
 }

/**********************************************************
 * coalesce
 * Covers the 4 cases discussed in the text:
 * - both neighbours are allocated
 * - the next block is available for coalescing
 * - the previous block is available for coalescing
 * - both neighbours are available for coalescing
 **********************************************************/
void *coalesce(void *bp)
{
    size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    size_t size = GET_SIZE(HDRP(bp));
    
    // check whether the free_shortcut needs to be updated if this newly
    // freed block comes earlier than what was there before
    // if this newly freed block is near the beginning of heap, not a good speedup!
    if (bp < free_shortcut) {
        free_shortcut = bp;
    }

    if (prev_alloc && next_alloc) {       /* Case 1 */
        return bp;
    }

    else if (prev_alloc && !next_alloc) { /* Case 2 */
        size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));
        return (bp);
    }

    else if (!prev_alloc && next_alloc) { /* Case 3 */
        size += GET_SIZE(HDRP(PREV_BLKP(bp)));
        PUT(FTRP(bp), PACK(size, 0));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        return (PREV_BLKP(bp));
    }

    else {            /* Case 4 */
        size += GET_SIZE(HDRP(PREV_BLKP(bp)))  +
            GET_SIZE(FTRP(NEXT_BLKP(bp)))  ;
        PUT(HDRP(PREV_BLKP(bp)), PACK(size,0));
        PUT(FTRP(NEXT_BLKP(bp)), PACK(size,0));
        return (PREV_BLKP(bp));
    }
}

/**********************************************************
 * extend_heap
 * Extend the heap by "words" words, maintaining alignment
 * requirements of course. Free the former epilogue block
 * and reallocate its new header
 **********************************************************/
void *extend_heap(size_t words)
{
    char *bp;
    size_t size;

    /* Allocate an even number of words to maintain alignments */
    size = (words % 2) ? (words+1) * WSIZE : words * WSIZE;
    if ( (bp = mem_sbrk(size)) == (void *)-1 )
        return NULL;

    /* Initialize free block header/footer and the epilogue header */
    PUT(HDRP(bp), PACK(size, 0));                // free block header
    PUT(FTRP(bp), PACK(size, 0));                // free block footer
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1));        // new epilogue header
    
    // initialize

    /* Coalesce if the previous block was free */
    return coalesce(bp);
}


/**********************************************************
 * find_fit
 * Traverse the heap searching for a block to fit asize
 * Return NULL if no free blocks can handle that size
 * Assumed that asize is aligned
 **********************************************************/
void * first_fit(size_t asize);
void * best_fit(size_t asize);
void * good_fit(size_t asize);
void * shortcut_fit(size_t asize);

void * find_fit(size_t asize)
{
    return first_fit(asize);
    //return best_fit(asize); //doesnt improve score, performance too slow
    //return good_fit(asize); //somehow utilization is worse by 1 point
    //return shortcut_fit(asize); //basically no effect on score
}
// find first block that has over the required capacity
void * first_fit(size_t asize)
{
    void *bp;
    for (bp = heap_listp; GET_SIZE(HDRP(bp)) > 0; bp = NEXT_BLKP(bp))
    {
        if (!GET_ALLOC(HDRP(bp)) && (asize <= GET_SIZE(HDRP(bp))))
        {
            return bp;
        }
    }
    return NULL;
}
// find smallest block that is still over the required capacity
void * best_fit(size_t asize)
{
    void *bp;
    void *best_bp = NULL;
    float best_fit;
    for (bp = heap_listp; GET_SIZE(HDRP(bp)) > 0; bp = NEXT_BLKP(bp))
    {
        if (!GET_ALLOC(HDRP(bp)) && (asize <= GET_SIZE(HDRP(bp))))
        {
            if (best_bp && (GET_SIZE(HDRP(bp))/asize) < best_fit) {
                best_bp = bp;
                best_fit = GET_SIZE(HDRP(bp))/asize;
            }
            else {
                best_bp = bp;
                best_fit = GET_SIZE(HDRP(bp))/asize;
            }
        }
    }
    return best_bp;
}
// find a good enough fit for the block, without spending too much time searching
void * good_fit(size_t asize)
{
    void *bp;
    void *best_bp = NULL;
    float best_fit;
    float good_enough_ratio = 2.0;
    
    for (bp = heap_listp; GET_SIZE(HDRP(bp)) > 0; bp = NEXT_BLKP(bp))
    {
        if (!GET_ALLOC(HDRP(bp)) && (asize <= GET_SIZE(HDRP(bp))))
        {
            // get the best_fit ratio and check if this block is better than previous best
            if (best_bp && (GET_SIZE(HDRP(bp))/asize) < best_fit) {
                best_bp = bp;
                best_fit = GET_SIZE(HDRP(bp))/asize;
            }
            else {
                best_bp = bp;
                best_fit = GET_SIZE(HDRP(bp))/asize;
            }
            // exit right away if this block is good enough
            if (best_fit <= good_enough_ratio) {
                break;
            }
        }
    }
    return best_bp;
}
// find a free block quickly, use shortcut pointer and pick best block from first five options
void * shortcut_fit(size_t asize)
{
    void *bp;
    void *best_bp = NULL;
    //float best_fit;
    //float good_enough_ratio = 2.0;
    //int fit_counter = 0;
    
    for (bp = free_shortcut; GET_SIZE(HDRP(bp)) > 0; bp = NEXT_BLKP(bp))
    {
        if (!GET_ALLOC(HDRP(bp)) && (asize <= GET_SIZE(HDRP(bp))))
        {
            best_bp = bp;
            break;
            /*
            // get the best_fit ratio and check if this block is better than previous best
            if (best_bp && (GET_SIZE(HDRP(bp))/asize) < best_fit) {
                best_bp = bp;
                best_fit = GET_SIZE(HDRP(bp))/asize;
                fit_counter++;
            }
            else {
                best_bp = bp;
                best_fit = GET_SIZE(HDRP(bp))/asize;
                fit_counter++;
            }
            // exit right away if this block is good enough
            if (best_fit <= good_enough_ratio || fit_counter > 0) {
                break;
            }
            */
        }
    }
    return best_bp;
}

/**********************************************************
 * place
 * Mark the block as allocated
 **********************************************************/
void * find_next_free_block(void* start_bp);

void place(void* bp, size_t asize)
{
    /* Get the current block size */
    size_t bsize = GET_SIZE(HDRP(bp));
    
    // asize is the adjusted block size, includes hdrp ftrp

    // BLOCK SPLITTING
    // if there are more than 5 unused words (6 = char + hd + ft) it is useful space
    if (bsize > asize + 1*WSIZE + DSIZE) {
        // mark this block allocated
        PUT(HDRP(bp), PACK(asize, 1));
        PUT(FTRP(bp), PACK(asize, 1));
        // make remaining words into unused block, mark free
        PUT(HDRP(NEXT_BLKP(bp)), PACK(bsize-asize, 0));
        PUT(FTRP(NEXT_BLKP(bp)), PACK(bsize-asize, 0));
        coalesce(NEXT_BLKP(bp));
    }
    // otherwise there will be internal fragmentation, hopefully negligibly small
    else {
        PUT(HDRP(bp), PACK(bsize, 1));
        PUT(FTRP(bp), PACK(bsize, 1));
    }
    // update the free_shortcut global variable if needed
    if (GET_ALLOC(HDRP(free_shortcut))) {
        free_shortcut = find_next_free_block(free_shortcut);
    }
}
// linear search to find the next free block, else return heap_listp if DNE
void * find_next_free_block(void* start_bp)
{
    void *bp;
    for (bp = start_bp; GET_SIZE(HDRP(bp)) > 0; bp = NEXT_BLKP(bp))
    {
        if (!GET_ALLOC(HDRP(bp))) {
            return bp;
        }
    }
    return heap_listp;
}

/**********************************************************
 * mm_free
 * Free the block and coalesce with neighbouring blocks
 **********************************************************/
void mm_free(void *bp)
{
    if(bp == NULL){
      return;
    }
    size_t size = GET_SIZE(HDRP(bp));
    PUT(HDRP(bp), PACK(size,0));
    PUT(FTRP(bp), PACK(size,0));
    coalesce(bp);
}


/**********************************************************
 * mm_malloc
 * Allocate a block of size bytes.
 * The type of search is determined by find_fit
 * The decision of splitting the block, or not is determined
 *   in place(..)
 * If no block satisfies the request, the heap is extended
 **********************************************************/
void *mm_malloc(size_t size)
{
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
        asize = DSIZE * ((size + (DSIZE) + (DSIZE-1))/ DSIZE);
    
    /* Search the free list for a fit */
    if ((bp = find_fit(asize)) != NULL) {
        place(bp, asize);
        return bp;
    }

    /* No fit found. Get more memory and place the block */
    // Increase the chunk size to 12, avoid extending heap too often
    extendsize = MAX(asize, (1<<12));
    if ((bp = extend_heap(extendsize/WSIZE)) == NULL)
        return NULL;
    place(bp, asize);
    return bp;

}

/**********************************************************
 * mm_realloc
 * Implemented simply in terms of mm_malloc and mm_free
 *********************************************************/
void *mm_realloc(void *ptr, size_t size)
{
    /* If size == 0 then this is just free, and we return NULL. */
    if(size == 0){
      mm_free(ptr);
      return NULL;
    }
    /* If oldptr is NULL, then this is just malloc. */
    if (ptr == NULL)
      return (mm_malloc(size));
    
    // check if this pointer is from a valid block
    /*if (!GET_ALLOC(HDRP(ptr))) {
        return NULL;
    }*/
    
    void *oldptr = ptr;
    void *newptr;
    size_t copySize = GET_SIZE(HDRP(oldptr));
    
    //size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(oldptr)));
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(ptr)));
    size_t curr_size = GET_SIZE(HDRP(ptr));
    size_t next_size = GET_SIZE(HDRP(NEXT_BLKP(ptr)));
    
    // Adjust block size to include overhead and alignment reqs
    size_t asize;
    if (size <= DSIZE)
        asize = DSIZE + DSIZE;
    else
        asize = DSIZE * ((size + (DSIZE) + (DSIZE-1))/ DSIZE);
    
    
    // check if there is a free block after the current one
    // if there is, then check if it can be coalesced to give the new size
    if (asize <= curr_size) {
        PUT(HDRP(ptr), PACK(curr_size, 0));
        PUT(FTRP(ptr), PACK(curr_size, 0));
        place(ptr, asize);
        return ptr;
    }
    else if (!next_alloc && (asize <= (curr_size + next_size))) {
        PUT(HDRP(ptr), PACK(curr_size + next_size, 0));
        PUT(FTRP(ptr), PACK(curr_size + next_size, 0));
        place(ptr, asize);
        return ptr;
    }
    
    // otherwise, new realloc size cannot fit, need to malloc new block for it
    // maybe assume that block being realloc once will be so again
    // give it some extra padding
    //size_t extraSize = size + (size - curr_size);
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

/**********************************************************
 * mm_check
 * Check the consistency of the memory heap
 * Return nonzero if the heap is consistant.
 *********************************************************/
int mm_check(void){
    /* TODO:
    * is every block in the free list marked as free?
    * are there any contiguous free blocks that somehow escaped coalescing?
    * is every free block actually in the free list?
    * do the pointers in the free list point to valid free blocks?
    * do any allocated blocks overlap?
    * do the pointers in a heap block point to valid head addresses?
    */
    return 1;
}
