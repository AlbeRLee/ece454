/*****************************************************************************
 * life.c
 * Parallelized and optimized implementation of the game of life resides here
 ****************************************************************************/
#include "life.h"
#include "util.h"

#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>

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


//#define RUN_SEQUENTIAL


/*****************************************************************************
 * Parallelization
 ****************************************************************************/

#define NUM_THREADS 8
#define NUM_THREADS_1 7 // NUM_TRHEADS - 1

typedef struct thread_arguments {
  char* outboard;
  char* inboard;
  int nrows;
  int ncols;
  int LDA;
  int start_row;
  int end_row;
} thread_arg;

/*****************************************************************************
 * Parallelized function definitions
 ****************************************************************************/

void*
thread_worker(void* args_){
  // get thread arguments with the row boundaries
  thread_arg* arg = (thread_arg*)args_;
  
  char* outboard = arg->outboard;
  char* inboard = arg->inboard;
  const int nrows = arg->nrows;
  const int ncols = arg->ncols;
  // HINT: in the parallel decomposition, LDA may not be equal to nrows!!!
  const int LDA = arg->LDA;
  const int start_row = arg->start_row;
  const int end_row = arg->end_row;
  
  int i, j;
  char destiny;
  char tbc, tbe, tbw, w, c, e;
  int inorth, isouth, ncols_1, ncols_2; // jwest; //jeast;
  ncols_1 = ncols - 1;
  ncols_2 = ncols - 2;
  
  
  for (i = start_row; i < end_row; i++) {
    
    // why were these const int?
    inorth = mod(i - 1, nrows);
//    inorth = (i-1 < 0) ? (nrows-1) : (i-1);
    isouth = mod(i + 1, nrows);
//    isouth = (i+1 == nrows) ? (0) : (i+1);
    
    // assign the values one step ahead
    w = BOARD(inboard, i, ncols_2);
    c = BOARD(inboard, i, ncols_1);
    e = BOARD(inboard, i, 0);

    tbw = BOARD(inboard, inorth, ncols_2) + BOARD(inboard, isouth, ncols_2);
    tbc = BOARD(inboard, inorth, ncols_1) + BOARD(inboard, isouth, ncols_1);
    tbe = BOARD(inboard, inorth, 0) + BOARD(inboard, isouth, 0);

    const char neighborCnt = w + e + tbc + tbw + tbe;
    destiny = alivep(neighborCnt, c);
    BOARD(outboard, i, ncols_1) = destiny;

    for (j = 1; j < ncols; j++) // will write up to ncols-1
    {
      // sliding window tactic, cut down the #of mem reads,
      // only read on the leading edge of 3x3 game box front
      tbw = tbc;
      tbc = tbe;
      w = c;
      c = e;
      tbe = BOARD(inboard, inorth, j) + BOARD(inboard, isouth, j);
      e = BOARD(inboard, i, j);

      const char neighbor_count = w + e + tbc + tbw + tbe;
      destiny = alivep(neighbor_count, c);
      BOARD(outboard, i, j - 1) = destiny;
    }

  }
  
  pthread_exit(NULL);
}

char*
parallel_game_of_life(char* outboard,
        char* inboard,
        const int nrows,
        const int ncols,
        const int gens_max) {
  
  // HINT: in the parallel decomposition, LDA may not be equal to nrows!!!
  //const int LDA = nrows;
  int curgen, i, i_rows;
  const int rows_per_thread = nrows/NUM_THREADS; //except last thread gets extra
  
  pthread_t* threads = malloc(sizeof(pthread_t) * NUM_THREADS);
  thread_arg* thread_args = malloc(sizeof(thread_arg) * NUM_THREADS);
//  pthread_barrier_t next_gen;
//  pthread_barrier_init(next_gen, 0, NUM_THREADS);
  
  i_rows = 0;
  for (i = 0; i < NUM_THREADS_1; i++) {
      thread_args[i].nrows = nrows;
      thread_args[i].ncols = ncols;
      thread_args[i].LDA = nrows;
      thread_args[i].start_row = i_rows;
      i_rows += rows_per_thread;
      thread_args[i].end_row = i_rows;
  }
  thread_args[i].nrows = nrows;
  thread_args[i].ncols = ncols;
  thread_args[i].LDA = nrows;
  thread_args[i].start_row = i_rows;
  thread_args[i].end_row = nrows;

  for (curgen = 0; curgen < gens_max; curgen++) {
    
    for (i = 0; i < NUM_THREADS; i++) {
      thread_args[i].inboard = inboard;
      thread_args[i].outboard = outboard;
      pthread_create(&threads[i], NULL, thread_worker, (void*) &thread_args[i]);
    }
    /* Compute one generation of the game-of-life grid */
    for (i = 0; i < NUM_THREADS; i++) {
      pthread_join(threads[i], NULL);
    }
    
    SWAP_BOARDS(outboard, inboard);
  }
  
  // clean up dynamic mem structures
  free(threads);
  free(thread_args);
  
  /* 
   * We return the output board, so that we know which one contains
   * the final result (because we've been swapping boards around).
   * Just be careful when you free() the two boards, so that you don't
   * free the same one twice!!! 
   */
  return inboard;
}

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

#ifdef RUN_SEQUENTIAL
  return sequential_game_of_life(outboard, inboard, nrows, ncols, gens_max);
#endif

  /* OPTIMIZATION RULES:
   * - Given input file and some # iterations, produce identical output 
   *   as the original gol (sequential_game_of_life)
   * - Must be parallelized using pthreads or openMP, with at least
   *   two threads that improve overall performance.
   * - Own code only, must run on UG machines.
   * - Marking speedup only by running GoL 10,000 on 1k.pbm provided
   * 
   * ASSUMPTIONS:
   * - input is NxN, N is power of 2
   * - max size is 10000x10000, exit gracefully if above
   * - do not have to parallelize for less than 32x32
   */

  if ((nrows < 32) || (nrows != ncols) ||(nrows > 10000)) // || (nrows % 4)) (nrows & !0x4)
    return sequential_game_of_life(outboard, inboard, nrows, ncols, gens_max);
  else
    return parallel_game_of_life(outboard, inboard, nrows, ncols, gens_max);
}
