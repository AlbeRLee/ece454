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
    int curcell;
    char destiny;
    char Cnw, Cn, Cne, Cw, Ce, Csw, Cs, Cse;
    int inorth, isouth, jwest, jeast;
    
    for (curgen = 0; curgen < gens_max; curgen++)
    {
        /* HINT: you'll be parallelizing these loop(s) by doing a
           geometric decomposition of the output */
        for (i = 0; i < nrows; i++)
        {
            j = 0;
            // why were these const int?
            inorth = mod (i-1, nrows);
            isouth = mod (i+1, nrows);
            jwest = mod (j-1, ncols);
            jeast = mod (j+1, ncols);
            
            // assign the values one step ahead
            Cnw = 0;
            Cn = BOARD (inboard, inorth, jwest);
            Cne = BOARD (inboard, inorth, j);
            //Cne = BOARD (inboard, inorth, jeast);
            Cw = 0;
            curcell = BOARD (inboard, i, jwest);
            Ce = BOARD (inboard, i, j);
            //Ce = BOARD (inboard, i, jeast);
            Csw = 0;
            Cs = BOARD (inboard, isouth, jwest);
            Cse = BOARD (inboard, isouth, j);
            //Cse = BOARD (inboard, isouth, jeast);
            
            for (j = 0; j < ncols; j++)
            {
                jeast = mod (j+1, ncols);
                
                // sliding window tactic, cut down the #of mem reads,
                // only read on the leading edge of 3x3 game box front
                Cnw = Cn;
                Cn = Cne;
                Cne = BOARD (inboard, inorth, jeast);
                Cw = curcell;
                curcell = Ce;
                Ce = BOARD (inboard, i, jeast);
                Csw = Cs;
                Cs = Cse;
                Cse = BOARD (inboard, isouth, jeast);
                
                const char neighbor_count = ((Cnw + Cn) + (Cne + Cw)) + ((Ce + Csw) + (Cs + Cse));
                
                destiny = alivep (neighbor_count, curcell);
                
                BOARD(outboard, i, j) = destiny;

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
