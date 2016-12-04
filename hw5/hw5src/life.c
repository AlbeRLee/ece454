/*****************************************************************************
 * life.c
 * Parallelized and optimized implementation of the game of life resides here
 ****************************************************************************/
#include "life.h"
#include "util.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <semaphore.h>


#define SWAP_BOARDS( b1, b2 )  do { \
  char* temp = b1; \
  b1 = b2; \
  b2 = temp; \
} while(0)

#define BOARD( __board, __i, __j )  (__board[(__i) + LDA*(__j)])

/*****************************************************************************
 * Game of life implementation
 ****************************************************************************/
char*
game_of_life(char* outboard, char* inboard,
  const int nrows, const int ncols, const int gens_max) {

  if ((nrows < 32) || (nrows != ncols) || (nrows % 4) || (nrows > 10000))
    return sequential_game_of_life(outboard, inboard, nrows, ncols, gens_max);
  else
    return parallel_game_of_life(outboard, inboard, nrows, ncols, gens_max);
}

static inline void cell_destiny(char* outboard, char* inboard,
  const int nrows, const int ncols, const int i, const int j) {

  const char c = BOARD(inboard, i, j);

  if (IS_ALIVE(c)) {
    if (ALIVE_SHOULD_DIE(c)) {
      DIE(BOARD(outboard, i, j));

      const int inorth = mod(i - 1, nrows);
      const int isouth = mod(i + 1, nrows);
      const int jwest = mod(j - 1, ncols);
      const int jeast = mod(j + 1, ncols);

      DECR(outboard, inorth, jwest);
      DECR(outboard, inorth, j);
      DECR(outboard, inorth, jeast);
      DECR(outboard, i, jwest);
      DECR(outboard, i, jeast);
      DECR(outboard, isouth, jwest);
      DECR(outboard, isouth, j);
      DECR(outboard, isouth, jeast);
    }
  } else {
    if (DEAD_SHOULD_LIVE(c)) {
      LIVE(BOARD(outboard, i, j));

      const int inorth = mod(i - 1, nrows);
      const int isouth = mod(i + 1, nrows);
      const int jwest = mod(j - 1, ncols);
      const int jeast = mod(j + 1, ncols);

      INCR(outboard, inorth, jwest);
      INCR(outboard, inorth, j);
      INCR(outboard, inorth, jeast);
      INCR(outboard, i, jwest);
      INCR(outboard, i, jeast);
      INCR(outboard, isouth, jwest);
      INCR(outboard, isouth, j);
      INCR(outboard, isouth, jeast);
    }
  }

}

char* parallel_game_of_life(char* outboard, char* inboard,
  const int nrows, const int ncols, const int gens_max) {
  
  int curgen, i, j, rows_per_thread, i_rows_per_thread;

  pthread_t* threads = malloc(NUM_THREADS * sizeof (pthread_t));
  ThreadArgs *targs = malloc(NUM_THREADS * sizeof (ThreadArgs));

  rows_per_thread = nrows / NUM_THREADS;
  
  for (i = 0, i_rows_per_thread = 0; i < NUM_THREADS; i++) {
    
    targs[i].first_row = i_rows_per_thread + 1;
    
    i_rows_per_thread += rows_per_thread;
    
    targs[i].last_row = i_rows_per_thread - 1;
    
    targs[i].nrows = nrows;
    targs[i].ncols = ncols;
  }

  for (curgen = 0; curgen < gens_max; curgen++) {
    for (i = 0, i_rows_per_thread = 0; i < NUM_THREADS; i++) {
      for (j = 0; j < ncols; j++)
        cell_destiny(outboard, inboard, nrows, ncols, i_rows_per_thread, j);
      i_rows_per_thread += rows_per_thread;
      for (j = 0; j < ncols; j++)
        cell_destiny(outboard, inboard, nrows, ncols, i_rows_per_thread - 1, j);
    }

    for (i = 0; i < NUM_THREADS; i++) {
      targs[i].inboard = inboard;
      targs[i].outboard = outboard;
      pthread_create(&threads[i], NULL, thread_stub, (void*) &targs[i]);
    }

    for (i = 0; i < NUM_THREADS; i++) {
      pthread_join(threads[i], NULL);
    }

    SWAP_BOARDS(outboard, inboard);
  }

  for (i = 0; i < nrows; i++) {
    for (j = 0; j < ncols; j++) {
      BOARD(inboard, i, j) = IS_ALIVE(BOARD(inboard, i, j));
    }
  }

  free(threads);
  free(targs);
  return inboard;
}



void* thread_stub(void* arg) {
  
  ThreadArgs* targ = (ThreadArgs*) arg;
  char * outboard = targ -> outboard;
  char * inboard = targ -> inboard;
  const int nrows = targ -> nrows;
  const int ncols = targ -> ncols;
  const int first_row = targ -> first_row;
  const int last_row = targ -> last_row;
  
  
  int i, j;
  
  for (i = first_row; i < last_row; i++) {
    for (j = 0; j < ncols; j++) {
      cell_destiny(outboard, inboard, nrows, ncols, i, j);
    }
  }

  pthread_exit(NULL);
}


