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
#define NEXT_FREE(bp) ((char *)(bp)) //compute addr of next ptr
#define PREV_FREE(bp) ((char *)(bp) + WSIZE) //compute addr of prev ptr
//compute the addr of head ptr, i = 0 to 14
#define HEAD_FREE(i)  ((char *)(heap_listp) - DSIZE - (i*WSIZE))
// due to type conversion warnings/errors make special pointer put/get for lists
#define PUT_P(p,val)  (*(uintptr_t **)(p) = (uintptr_t *)val)
#define GET_P(p)  (*(uintptr_t **)(p))

void* heap_listp = NULL;

/**********************************************************
 * mm_init
 * Initialize the heap, including "allocation" of the
 * prologue and epilogue
 **********************************************************/
 int mm_init(void)
 {
     if ((heap_listp = mem_sbrk(18*WSIZE)) == (void *)-1)
         return -1;
     
     //initialize the segregated lists to nulls
     int i;
     for (i = 0; i < 15; i++) {
         PUT_P(heap_listp + (i*WSIZE), NULL); // head ptr of free list = NULL
     }
     PUT(heap_listp + (15 * WSIZE), PACK(DSIZE, 1)); // prologue header
     //zero data
     PUT(heap_listp + (16 * WSIZE), PACK(DSIZE, 1)); // prologue footer
     //future heap data
     PUT(heap_listp + (17 * WSIZE), PACK(0, 1)); // epilogue header
     
     //heap ptr points to the prologue footer
     heap_listp += 16*WSIZE;

     return 0;
 }


// Explicit list manipulation functions

int free_list_index(size_t asize) {
    int index = 14; //default go to largest list
    // return index value 0 to 14 based on the corresponding list size
    
    // first list is at most DSIZE
    // next lists are capped at powers of 2
    // last list will hold everythin 2^14 and larger
    int listSize = 64;
    int i;
    // find the smallest existing listSize that will fit the entire asize
    for (i = 0; i < 15; i++) {
        if (asize < listSize) {
            index = i;
            break;
        }
        listSize = listSize << 1;
    }
    return index;
} 

void free_list_insert(void* bp) {
    if (bp == NULL)
        return;
    
    //get appropriate seg list
    int li = free_list_index(GET_SIZE(HDRP(bp)));
    
    // check if list empty
    if (GET_P(HEAD_FREE(li)) == NULL) {
        PUT_P(NEXT_FREE(bp), NULL);
        PUT_P(PREV_FREE(bp), NULL);
        PUT_P(HEAD_FREE(li), bp); // point head to this block
    }
    // insert new block at front of existing free list
    else {
        PUT_P(NEXT_FREE(bp), GET_P(HEAD_FREE(li))); // connect to start of list
        PUT_P(PREV_FREE(bp), NULL); // no prev blocks
        PUT_P(PREV_FREE(GET_P(HEAD_FREE(li))), bp); // set second block to point back
        PUT_P(HEAD_FREE(li), bp); // point head to this block
    }
}

void free_list_remove(void* bp){
    if (bp == NULL)
        return;
    
    int li = free_list_index(GET_SIZE(HDRP(bp)));
    if (GET_P(HEAD_FREE(li)) == NULL)
        return;
    
    // remove first item
    if (GET_P(PREV_FREE(bp)) == NULL) {
        PUT_P(HEAD_FREE(li), GET_P(NEXT_FREE(bp))); // connect head to skip this block
        if (GET_P(HEAD_FREE(li)) != NULL) {
            PUT_P(PREV_FREE(GET_P(HEAD_FREE(li))), NULL); // make next block the first block
            PUT_P(NEXT_FREE(bp), NULL); // disconnect first block's fwd ptr
        }
    }
    // remove last item
    else if (GET_P(NEXT_FREE(bp)) == NULL) {
        PUT_P(GET_P(PREV_FREE(bp)), NULL); // disconnect prev block's ptr
        PUT_P(PREV_FREE(bp), NULL); // disconnect last block's back ptr
    }
    // remove middle item
    else {
        // connect prev to next via fwd ptr (bypass bp)
        PUT_P(NEXT_FREE(GET_P(PREV_FREE(bp))), GET_P(NEXT_FREE(bp)));
        // connect next to prev via back ptr
        PUT_P(PREV_FREE(GET_P(NEXT_FREE(bp))), GET_P(PREV_FREE(bp)));
        // disconnect this block from list
        PUT_P(NEXT_FREE(bp), NULL);
        PUT_P(PREV_FREE(bp), NULL);
    }
}

// End of explicit list fcns

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

    // Case 1 = neighbors allocated, cannot coalesce
    if (prev_alloc && next_alloc) {
        return bp;
    }
    // Case 2 = next block free, coalesce forward, return combined block (not in free list)
    else if (prev_alloc && !next_alloc) {
        size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
        free_list_remove(NEXT_BLKP(bp)); //remove NEXT
        
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));
        return (bp);
    }
    // Case 3 = prev block free, coalesce backward, return combined block (not in free list)
    else if (!prev_alloc && next_alloc) { 
        size += GET_SIZE(HDRP(PREV_BLKP(bp)));
        free_list_remove(PREV_BLKP(bp)); //remove PREV
        
        PUT(FTRP(bp), PACK(size, 0));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        return (PREV_BLKP(bp));
    }
    // Case 4 = both neighbors free, coalesce all three into single block
    else {
        size += GET_SIZE(HDRP(PREV_BLKP(bp)))  +
            GET_SIZE(FTRP(NEXT_BLKP(bp)))  ;
        free_list_remove(NEXT_BLKP(bp)); //remove NEXT
        free_list_remove(PREV_BLKP(bp)); //  and  PREV
        
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

    /* Coalesce if the previous block was free */
    return coalesce(bp);
}


/**********************************************************
 * find_fit
 * Traverse the heap searching for a block to fit asize
 * Return NULL if no free blocks can handle that size
 * Assumed that asize is aligned
 **********************************************************/
void * find_fit(size_t asize)
{
    void* bp;
    int li = free_list_index(asize);
    
    // search only free list
    // find the first block large enough to fit asize
    // if no blocks are found in this list, move on to next higher index
    for (; li < 15; li++) {
        // search the current list size starting from head
        for (bp = GET_P(HEAD_FREE(li)); bp != NULL; bp = GET_P(NEXT_FREE(bp))) {
            // return the first good result
            if ( asize <= GET_SIZE(HDRP(bp)) ) {
                free_list_remove(bp);
                return bp;
            }
        }
    }
    return NULL;
    
    /*for (bp = heap_listp; GET_SIZE(HDRP(bp)) > 0; bp = NEXT_BLKP(bp))
    {
        if (!GET_ALLOC(HDRP(bp)) && (asize <= GET_SIZE(HDRP(bp))))
        {
            free_list_remove(bp);
            return bp;
        }
    }
    return NULL;*/
}

/**********************************************************
 * place
 * Mark the block as allocated
 * Split the block if there is sufficient useful space left over
 * and add the new split onto the free list
 **********************************************************/
void * place(void* bp, size_t asize, size_t reallocing)
{
    /* Get the current block size */
    size_t bsize = GET_SIZE(HDRP(bp));
    /*size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    size_t prev_size = GET_SIZE(HDRP(PREV_BLKP(bp)));
    size_t next_size = GET_SIZE(HDRP(NEXT_BLKP(bp)));
    */
    // asize is the adjusted block size, includes hdrp ftrp

    // Block Splitting
    // if there are more than 4 unused words it is considered useful space
    // (4 = hd + next + prev + ft)
    if (bsize - asize > 0*WSIZE + DSIZE) {
        //check if it one side has a free block, which will be good for coalescing
        /*if (!reallocing && !prev_alloc) {
            //reverse the order of split & alloc so that free split block is to the LEFT
            if (next_alloc || (!next_alloc && (prev_size > next_size))) {
		        // mark the first part as free
		        PUT(HDRP(bp), PACK(bsize-asize, 0));
		        PUT(FTRP(bp), PACK(bsize-asize, 0));
		        // make the remaining words into a block, mark as alloc
		        PUT(HDRP(NEXT_BLKP(bp)), PACK(asize, 1));
		        PUT(FTRP(NEXT_BLKP(bp)), PACK(asize, 1));
		        // coalesce the new split block and then add it to the free list
		        void* free_bp = coalesce(bp);
		        free_list_insert(free_bp);
		        return NEXT_BLKP(free_bp);
            }
        }*/
        //default is to put split block on the RIGHT
        // mark the required words of this block as allocated
        PUT(HDRP(bp), PACK(asize, 1));
        PUT(FTRP(bp), PACK(asize, 1));
        // make the remaining words into an unused block, mark as free
        PUT(HDRP(NEXT_BLKP(bp)), PACK(bsize-asize, 0));
        PUT(FTRP(NEXT_BLKP(bp)), PACK(bsize-asize, 0));
        // coalesce the new split block and then add it to the free list
        void* free_bp = coalesce(NEXT_BLKP(bp));
        free_list_insert(free_bp);
        return bp;
    }
    // otherwise there will be internal fragmentation, hopefully negligibly small
    else {
        PUT(HDRP(bp), PACK(bsize, 1));
        PUT(FTRP(bp), PACK(bsize, 1));
        return bp;
    }
}

/**********************************************************
 * mm_free
 * Free the block and coalesce with neighbouring blocks
 **********************************************************/
void mm_free(void *bp)
{
    if(bp == NULL) {
      return;
    }
    // mark free bit inside header as 0
    size_t size = GET_SIZE(HDRP(bp));
    PUT(HDRP(bp), PACK(size,0));
    PUT(FTRP(bp), PACK(size,0));
    
    // coalesce the newly freed block, avoid external fragmentation
    void* free_bp = coalesce(bp);
    
    // add the newly coalesced free block tot the free list
    free_list_insert(free_bp);
}

/**********************************************************
 * mm_malloc
 * Allocate a block of size bytes.
 * The type of search is determined by find_fit
 *   also block removed from free list
 * The decision of splitting the block, or not is determined
 *   in place(..) and new split is put in free list
 * If no block satisfies the request, the heap is extended
 *   it's coalesced, and new block is placed into it
 **********************************************************/
void *mm_malloc(size_t size)
{
    size_t asize; /* adjusted block size */
    size_t extendsize; /* amount to extend heap if no fit */
    char * bp;
    char * newbp;

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
        newbp = place(bp, asize, 0);
        return newbp;
    }

    /* No fit found. Get more memory and place the block */
    // Increase the chunk size to 12, avoid extending heap too often
    //printf("extend");
    extendsize = MAX(asize, (1<<12));
    if ((bp = extend_heap(extendsize/WSIZE)) == NULL)
        return NULL;
    newbp = place(bp, asize, 0);
    return newbp;

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
    /* If ptr is NULL, then this is just malloc. */
    if (ptr == NULL)
      return (mm_malloc(size));
    
    // check if this pointer is from a valid block
    /*if (!GET_ALLOC(HDRP(ptr))) {
        return NULL;
    }*/
    
    void *oldptr = ptr;
    void *newptr;
    
    //size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(ptr)));
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(ptr)));
    size_t curr_size = GET_SIZE(HDRP(ptr));
    size_t next_size = GET_SIZE(HDRP(NEXT_BLKP(ptr)));
    
    // Adjust block size to include overhead and alignment reqs
    size_t asize;
    if (size <= DSIZE)
        asize = DSIZE + DSIZE;
    else
        asize = DSIZE * ((size + (DSIZE) + (DSIZE-1))/ DSIZE);
    
    // check if we are shrinking the memory used, and there is a chance of splitting
    if (asize <= curr_size) {
        PUT(HDRP(ptr), PACK(curr_size, 0));
        PUT(FTRP(ptr), PACK(curr_size, 0));
        newptr = place(ptr, asize, 1);
        return newptr;
    }
    // otherwise we need to look for additional memory
    
    // check if there is a free block after the current one
    // if there is, then check if it can be coalesced to give the new size
    if ( !next_alloc && (asize <= (curr_size + next_size)) ) {
        free_list_remove(NEXT_BLKP(ptr)); //remove NEXT
        // mark as free
        PUT(HDRP(ptr), PACK(curr_size + next_size, 0));
        PUT(FTRP(ptr), PACK(curr_size + next_size, 0));
        // place extended block in new memory, not overwriting old
        newptr = place(ptr, asize, 1);
        return newptr;
    }
    
    // If above fails, need to malloc new block of the appropriate size
    // maybe assume that block being realloc once will be so again
    // give it some extra padding?
    //size_t extraSize = size + (size - curr_size);
    newptr = mm_malloc(size);
    if (newptr == NULL)
        return NULL;
    // Copy the old data
    size_t copySize = GET_SIZE(HDRP(oldptr));
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

