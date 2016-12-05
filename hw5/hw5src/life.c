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

#define BOARD( __board, __i, __j )  (__board[(__i) + nrows*(__j)])

#define RUN_SEQUENTIAL 0

const static signed char NEED_2_OR_3 = 0b00110000;
const static signed char NEED_3 = 0b00010000;


//#load the masks
//for i = 1 to ncells:   
//   if even[i] < 0:
//      odd[i] = 00110000 # a live cell needs 2 or 3 left shifts
//   else:
//      odd[i] = 00010000 # a dead cell needs 3 left shifts
//
//# Shift the masks
//for i = 1 to ncells:
//   if even[i] < 0:
//      for j in neighborsOf(i):
//         odd[j] <<= 1


/*****************************************************************************
 * Game of life implementation
 ****************************************************************************/
char*
game_of_life(char* outboard, char* inboard,
  const int nrows, const int ncols, const int gens_max) {

#if RUN_SEQUENTIAL
  return sequential_game_of_life(outboard, inboard, nrows, ncols, gens_max);
#endif

  if ((nrows < 32) || (nrows != ncols) || (nrows % 4) || (nrows > 10000))
    return sequential_game_of_life(outboard, inboard, nrows, ncols, gens_max);
  else
    return parallel_game_of_life(outboard, inboard, nrows, ncols, gens_max);
}

static inline void init_board(char* inboard,
  const int nrows, const int ncols) {

  int i, j;
  for (i = 0; i < nrows; i++) {
    for (j = 0; j < ncols; j++) {
      if (IS_ALIVE(BOARD(inboard, i, j))) {

        const int inorth = LOWBOUND(i - 1, nrows);
        const int isouth = HIGHBOUND(i + 1, nrows);
        const int jwest = LOWBOUND(j - 1, ncols);
        const int jeast = HIGHBOUND(j + 1, ncols);
        INCR(inboard, inorth, jwest);
        INCR(inboard, inorth, j);
        INCR(inboard, inorth, jeast);
        INCR(inboard, i, jwest);
        INCR(inboard, i, jeast);
        INCR(inboard, isouth, jwest);
        INCR(inboard, isouth, j);
        INCR(inboard, isouth, jeast);
      }
    }
  }
}

char* parallel_game_of_life(char* outboard, char* inboard,
  const int nrows, const int ncols, const int gens_max) {

  init_board(inboard, nrows, ncols);

  int curgen, i, j, i_rows_per_thread;
  
//  const int narray = nrows * ncols;
//  signed char* odd_gen;
//  signed char* even_gen;
//  
//  odd_gen = (signed char*)malloc(narray * sizeof (signed char));
//  even_gen = (signed char*)malloc(narray * sizeof (signed char));

  pthread_t* threads = (pthread_t*)malloc(NUM_THREADS * sizeof (pthread_t));
  ThreadArgs* targs = (ThreadArgs*)malloc(NUM_THREADS * sizeof (ThreadArgs));
  ThreadArgs* targs_firsts = (ThreadArgs*)malloc(NUM_THREADS * sizeof (ThreadArgs));
  ThreadArgs* targs_lasts = (ThreadArgs*)malloc(NUM_THREADS * sizeof (ThreadArgs));


  const int rows_per_thread = nrows / NUM_THREADS;

  for (i = 0, i_rows_per_thread = 0; i < NUM_THREADS; i++) {

    targs[i].first_row = i_rows_per_thread + 1;
    targs_firsts[i].first_row = i_rows_per_thread;
    targs_firsts[i].last_row = i_rows_per_thread + 1;

    i_rows_per_thread += rows_per_thread;

    targs[i].last_row = i_rows_per_thread - 1;
    targs_lasts[i].first_row = i_rows_per_thread - 1;
    targs_lasts[i].last_row = i_rows_per_thread;

    targs[i].nrows = nrows;
    targs_firsts[i].nrows = nrows;
    targs_lasts[i].nrows = nrows;

    targs[i].ncols = ncols;
    targs_firsts[i].ncols = ncols;
    targs_lasts[i].ncols = ncols;
  }

  for (curgen = 0; curgen < gens_max; curgen++) {

    memmove(outboard, inboard, nrows * ncols * sizeof (char));

    for (i = 0; i < NUM_THREADS; i++) {
      targs_firsts[i].inboard = inboard;
      targs_firsts[i].outboard = outboard;
      pthread_create(&threads[i], NULL, thread_stub, (void*) &targs_firsts[i]);
    }
    for (i = 0; i < NUM_THREADS; i++)
      pthread_join(threads[i], NULL);




    for (i = 0; i < NUM_THREADS; i++) {
      targs_lasts[i].inboard = inboard;
      targs_lasts[i].outboard = outboard;
      pthread_create(&threads[i], NULL, thread_stub, (void*) &targs_lasts[i]);
    }
    for (i = 0; i < NUM_THREADS; i++)
      pthread_join(threads[i], NULL);




    for (i = 0; i < NUM_THREADS; i++) {
      targs[i].inboard = inboard;
      targs[i].outboard = outboard;
      pthread_create(&threads[i], NULL, thread_stub, (void*) &targs[i]);
    }

    for (i = 0; i < NUM_THREADS; i++)
      pthread_join(threads[i], NULL);

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
      const char c = BOARD(inboard, i, j);
      if (IS_ALIVE(c)) {
        if (ALIVE_SHOULD_DIE(c)) {
          DIE(BOARD(outboard, i, j));

          const int inorth = LOWBOUND(i - 1, nrows);
          const int isouth = HIGHBOUND(i + 1, nrows);
          const int jwest = LOWBOUND(j - 1, ncols);
          const int jeast = HIGHBOUND(j + 1, ncols);

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

          const int inorth = LOWBOUND(i - 1, nrows);
          const int isouth = HIGHBOUND(i + 1, nrows);
          const int jwest = LOWBOUND(j - 1, ncols);
          const int jeast = HIGHBOUND(j + 1, ncols);

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
  }

  pthread_exit(NULL);
}


