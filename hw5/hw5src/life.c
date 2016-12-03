/*****************************************************************************
 * life.c
 * Parallelized and optimized implementation of the game of life resides here
 ****************************************************************************/
#include "life.h"
#include "util.h"

/* 
 * Swapping the two boards only involves swapping pointers, not copying values.
 */
#define SWAP_BOARDS( b1, b2 )  do { \
  char* temp = b1; \
  b1 = b2; \
  b2 = temp; \
} while(0)

/* Simple mapping of board grid indeces (i,j) to array index */
#define BOARD( __board, __i, __j )  (__board[(__i) + LDA*(__j)])


//#define RUN_ORIGINAL

/*****************************************************************************
 * Helper function definitions
 ****************************************************************************/

/*****************************************************************************
 * Game of life implementation
 * 
 * header comment summarizing report (parallelization strategy & optimizations)
 ****************************************************************************/
char*
game_of_life(char* outboard,
  char* inboard,
  const int nrows,
  const int ncols,
  const int gens_max) {
  
    #ifdef RUN_ORIGINAL
        return sequential_game_of_life(outboard, inboard, nrows, ncols, gens_max);
    #endif
    
    /* RULES
     * - Given input file and some # iterations, produce identical output 
     *   as the original gol (sequential_game_of_life)
     * - Must be parallelized using pthreads or openMP, with at least
     *   two threads that improve overall performance.
     * - Own code only, must run on UG machines.
     * - Marking speedup only by running GoL 10,000 on 1k.pbm provided
     * 
     * ASSUMPTIONS
     * - input is NxN, N is power of 2
     * - max size is 10000x10000, exit gracefully if above
     * - do not have to parallelize for less than 32x32
     */
    
    if ( (nrows < 32) || (nrows != ncols) || (nrows%4) || (nrows > 10000) )
        return sequential_game_of_life(outboard, inboard, nrows, ncols, gens_max);
    
    // HINT: in the parallel decomposition, LDA may not be equal to nrows!!!
    const int LDA = nrows;
    int curgen, i, j;
    //int curcell;
    char destiny;
    //char Cnw, Cn, Cne, Cw, Ce, Csw, Cs, Cse;
    char tb, tbe, tbw, w, curr, e;
    int inorth, isouth, Nncols; // jwest; //jeast;
    
    for (curgen = 0; curgen < gens_max; curgen++)
    {
        /* HINT: you'll be parallelizing these loop(s) by doing a
           geometric decomposition of the output */
        for (i = 0; i < nrows; i++)
        {
            // why were these const int?
            inorth = mod (i-1, nrows);
            isouth = mod (i+1, nrows);
            Nncols = ncols-1;
            
            // assign the values one step ahead
            //Cnw = BOARD (inboard, inorth, Nncols-1);
            //Cn = BOARD (inboard, inorth, Nncols);
            //Cne = BOARD (inboard, inorth, 0);
            
            w = BOARD (inboard, i, Nncols-1);
            curr = BOARD (inboard, i, Nncols);
            e = BOARD (inboard, i, 0);
            
            //Csw = BOARD (inboard, isouth, Nncols-1);
            //Cs = BOARD (inboard, isouth, Nncols);
            //Cse = BOARD (inboard, isouth, 0);
            
            tb = BOARD (inboard, inorth, Nncols) + BOARD (inboard, isouth, Nncols);
            tbw = BOARD (inboard, inorth, Nncols-1) + BOARD (inboard, isouth, Nncols-1);
            tbe = BOARD (inboard, inorth, 0) + BOARD (inboard, isouth, 0);
            
            const char neighborCnt = w + e + tb + tbw + tbe;
            destiny = alivep (neighborCnt, curr);
            BOARD(outboard, i, Nncols) = destiny;
            
            for (j = 1; j < ncols; j++) //up to ncols-1
            {
                //jeast = mod (j+1, ncols);
                
                // sliding window tactic, cut down the #of mem reads,
                // only read on the leading edge of 3x3 game box front
                tbw = tb;
                tb = tbe;
                w = curr;
                curr = e;
                tbe = BOARD (inboard, inorth, j) + BOARD (inboard, isouth, j);
                e = BOARD (inboard, i, j);
                
                const char neighbor_count = w + e + tb + tbw + tbe;
                destiny = alivep (neighbor_count, curr);
                BOARD(outboard, i, j-1) = destiny;
            }
            
        }
        SWAP_BOARDS( outboard, inboard );

    }
    
    
    /* 
     * We return the output board, so that we know which one contains
     * the final result (because we've been swapping boards around).
     * Just be careful when you free() the two boards, so that you don't
     * free the same one twice!!! 
     */
    return inboard;
}
