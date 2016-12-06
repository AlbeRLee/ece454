/*****************************************************************************
 * life.c
 * Parallelized and optimized implementation of the game of life resides here
 ****************************************************************************/
#include "life.h"
#include "util.h"

#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>

/* Swapping the two boards only involves swapping pointers, not copying values
 * inside the entire arrays. */
#define SWAP_BOARDS( b1, b2 )  do { \
  char* temp = b1; \
  b1 = b2; \
  b2 = temp; \
} while(0)

/* Simple mapping of board grid indeces (i,j) to array index Row Major Order */
#define BOARD( __board, __i, __j )  (__board[(__i) + LDA*(__j)])

/* Macro facilitating the running of the original sequential code */
//#define RUN_SEQUENTIAL


/*****************************************************************************
 * Thread Parallelization Data & Macros
 ****************************************************************************/

/* Number of threads used, except when the game board size becomes too small */
#define NUM_THREADS 8

/* Code block macro to facilitate readability inside the unrolled nested for
 * loop. Used in 'thread_worker' function */
#define THREAD_COMPUTE_SLIDING_WINDOW( __readidx, __writeidx) \
  tbn = tbc; tbc = tbs; \
  n = c; c = s; \
  tbs = BOARD(inboard, __readidx, jwest) + BOARD(inboard, __readidx, jeast); \
  s = BOARD(inboard, __readidx, j); \
  destiny = alivep((n + s + tbc + tbn + tbs), c); \
  BOARD(outboard, __writeidx, j) = destiny;

/* Arguments that need to be passed into each thread */
typedef struct thread_arguments {
  char* outboard;
  char* inboard;
  int nrows;
  int ncols;
  int LDA;
  int start_col;
  int end_col;
  int gens_max;
  pthread_barrier_t* gens_barrier;
} thread_arg;


/*****************************************************************************
 * Game of life implementation
 * 
 * Our strategy in this GoL implementation was to utilize simple, yet effective
 * optimization methods. The approach can be grouped into two areas, parallel
 * multithreading and loop optimization. For the parallelization, we chose the 
 * simplest strategy of block assignments, since the boards were set up randomly
 * and there should not be any excessive work imbalance. We used barriers to
 * cut down on overhead. As for loop optimization, we used the known techniques
 * which are listed in the summary and REPORT.txt. Essentially, we noticed
 * redundancy in the array accesses and opportunities to restructure the loops
 * so that local variables and cache was better utilized.
 * 
 * Optimizations summary: (more details in the report)
 * - eliminate redundant reads from array, store values locally for reuse
 *   we call this 'sliding window'
 * - loop peeling, where we calculated the last column element first along
 *   with the first column element, which allows us to only cross the 'toroidal'
 *   boundary conditions once, thus not replacing cache unnecessarily
 * - reorder the loops to traverse entire columns (Column Major Order)
 * - loop invariant code motion, common subexpression elimination
 *   constant boundary values are pre-calculated outside of loops
 * - multithreading, giving each thread a block of columns from the inner loop
 * - using barrier instead of fork-join to lower thread overhead
 * - loop unrolling innermost loop
 * - reduction in strength, eliminating modulo or 'mod' function by using
 *   ternary operator conditions to catch edge cases of board
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

  // check that the game board size is valid to be parallelized
  if ((nrows < 32) || (nrows != ncols) || (nrows > 10000)) // || (nrows & !0x4))
    return sequential_game_of_life(outboard, inboard, nrows, ncols, gens_max);
  else
    return parallel_game_of_life(outboard, inboard, nrows, ncols, gens_max);
}


/*****************************************************************************
 * Parallelization function definitions & descriptions
 ****************************************************************************/

/*
 * Thread Execution Function
 * 
 * This function is passed to each thread and each will run independently.
 * Threads receive different boundaries for the outer loop (through columns)
 * but the execution pattern is the same. They all run 'gens_max' number of 
 * generation cycles, synchronizing at a barrier placed at the end of each cycle
 * Inside a single cycle, it loops through the entire game board array, and
 * calculates if each cell is alive/dead based on its eight neighbors.
 * 
 * Optimizations include the 'sliding window', loop reordering, loop unrolling,
 * and loop invariant code motion, amongst others. Details are in the REPORT.txt
 */
void*
thread_worker(void* args_) {
  
  // get thread arguments from the passed in pointer
  thread_arg* arg = (thread_arg*) args_;

  // values assigned from thread structure to local variables, easier access
  char* outboard = arg->outboard;
  char* inboard = arg->inboard;
  const int nrows = arg->nrows;
  const int ncols = arg->ncols;
  const int LDA = arg->LDA;
  const int start_col = arg->start_col;
  const int end_col = arg->end_col;
  const int gens_max = arg->gens_max;
  pthread_barrier_t* next_gen = arg->gens_barrier;

  // local accumulator variables used inside loop
  int curgen, i, j;
  char destiny;
  char tbc, tbs, tbn, n, c, s;
  int jwest, jeast;
  // local constant subexpressions are pulled out of loops here
  const int nrows_1 = nrows - 1;
  const int nrows_2 = nrows - 2;
  const int nrows_loopMax = nrows - 8; // -8 to compensate loop unrolling
  
  for (curgen = 0; curgen < gens_max; curgen++) {

    // swap the loop order to traverse by column major order
    for (j = start_col; j < end_col; j++) {

      // loop back when bottom edge of grid is reached
      jwest = (j == 0) ? (ncols-1) : (j-1);
      // loop back when right edge of grid is reached
      jeast = (j == ncols-1) ? (0) : (j+1);

      // assign the initial values one step ahead of the start of the inner loop
      n = BOARD(inboard, nrows_2, j);
      c = BOARD(inboard, nrows_1, j);
      s = BOARD(inboard, 0, j);

      tbn = BOARD(inboard, nrows_2, jwest) + BOARD(inboard, nrows_2, jeast);
      tbc = BOARD(inboard, nrows_1, jwest) + BOARD(inboard, nrows_1, jeast);
      tbs = BOARD(inboard, 0, jwest) + BOARD(inboard, 0, jeast);
      
      // compute the value of the last cell in the column, to avoid having to
      // wrap around the indeces twice (once for [0] and once for [nrows-1])
      destiny = alivep((n + s + tbc + tbn + tbs), c);
      BOARD(outboard, nrows_1, j) = destiny;

      for (i = 1; i < nrows_loopMax; i += 8) {
        
        // Sliding window tactic = cut down the number of mem reads
        // Only read on the leading edge of the 3x3 game 'window' and compute
        // the interior cell value, while remembering past values.
        // Plus, then we applied loop unrolling...
        
        // Example of first block of code represented by the first macro:
        // *First slide old values into the new vars*
        //tbn = tbc; tbc = tbs;
        //n = c; c = s;
        // *Then read from the array into the vacated vars*
        //tbs = BOARD(inboard, i+1, jwest) + BOARD(inboard, i+1, jeast);
        //s = BOARD(inboard, i+1, j);
        // *Finally compute alive/dead and store it in second board's cell*
        //destiny = alivep((n + s + tbc + tbn + tbs), c);
        //BOARD(outboard, i, j) = destiny;
        
        THREAD_COMPUTE_SLIDING_WINDOW(i, i-1);
        THREAD_COMPUTE_SLIDING_WINDOW(i+1, i);
        THREAD_COMPUTE_SLIDING_WINDOW(i+2, i+1);
        THREAD_COMPUTE_SLIDING_WINDOW(i+3, i+2);
        
        THREAD_COMPUTE_SLIDING_WINDOW(i+4, i+3);
        THREAD_COMPUTE_SLIDING_WINDOW(i+5, i+4);
        THREAD_COMPUTE_SLIDING_WINDOW(i+6, i+5);
        THREAD_COMPUTE_SLIDING_WINDOW(i+7, i+6);
        
      }
      
      // finish computing the remaining cells in column, loop unroll epilogue
      
      THREAD_COMPUTE_SLIDING_WINDOW(i, i-1);
      THREAD_COMPUTE_SLIDING_WINDOW(i+1, i);
      THREAD_COMPUTE_SLIDING_WINDOW(i+2, i+1);
      THREAD_COMPUTE_SLIDING_WINDOW(i+3, i+2);
      
      THREAD_COMPUTE_SLIDING_WINDOW(i+4, i+3);
      THREAD_COMPUTE_SLIDING_WINDOW(i+5, i+4);
      THREAD_COMPUTE_SLIDING_WINDOW(i+6, i+5);

    }

    SWAP_BOARDS(outboard, inboard); // swap pointers
    
    // synch - wait for other threads
    pthread_barrier_wait(next_gen);
  }
  
  pthread_exit(NULL);
}

/*
 * Parallel Implementation (multithreaded)
 * 
 * This function handles the creation of threads and their required structures.
 * Each thread is passed in arguments through its own thread_arg structure,
 * which also specifies its game-board boundaries as [start_row, end_row).
 * Threads are forked, they execute, and then joined and destroyed.
 * They are passed a common barrier so that they can synchronize themselves.
 */
char*
parallel_game_of_life(char* outboard,
    char* inboard,
    const int nrows,
    const int ncols,
    const int gens_max) {

  int i, i_rows;
  const int Num_Threads = (ncols < 128) ? (4) : (NUM_THREADS);
  const int rows_per_thread = nrows/Num_Threads; // assuming powers of two

  // allocate threads and their supporting structures
  pthread_t* threads = malloc(sizeof(pthread_t) * Num_Threads);
  thread_arg* thread_args = malloc(sizeof(thread_arg) * Num_Threads);
  pthread_barrier_t* gens_barrier = malloc(sizeof(pthread_barrier_t));
  pthread_barrier_init(gens_barrier, 0, Num_Threads);
  
  // fill each thread's struct of args
  i_rows = 0;
  for (i = 0; i < Num_Threads; i++) {
    thread_args[i].nrows = nrows;
    thread_args[i].ncols = ncols;
    thread_args[i].LDA = nrows;
    thread_args[i].start_col = i_rows;
    i_rows += rows_per_thread;
    thread_args[i].end_col = i_rows;
    thread_args[i].gens_max = gens_max;
    thread_args[i].gens_barrier = gens_barrier;
  }
  
  // fork, execute, join
  for (i = 0; i < Num_Threads; i++) {
    thread_args[i].inboard = inboard;
    thread_args[i].outboard = outboard;
    pthread_create(&threads[i], NULL, thread_worker, (void*) &thread_args[i]);
  }
  /*
   * Compute all generations of the game-of-life grid inside here *
   */
  for (i = 0; i < Num_Threads; i++) {
    pthread_join(threads[i], NULL);
  }
  
  // clean up dynamic mem structures
  free(threads);
  free(thread_args);
  free(gens_barrier);

  /* 
   * We return the output board, so that we know which one contains
   * the final result (because we've been swapping boards around).
   * Just be careful when you free() the two boards, so that you don't
   * free the same one twice!!! 
   */
  return inboard;
}
