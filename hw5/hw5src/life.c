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

#define BOARD( __board, __i, __j )  (__board[(__i) + size*(__j)])

#define RUN_SEQUENTIAL 0

/*****************************************************************************
 * Game of life implementation
 ****************************************************************************/
char* game_of_life(char* outboard, char* inboard,
  const int nrows, const int ncols, const int gens_max) {

#if RUN_SEQUENTIAL
  return sequential_game_of_life(outboard, inboard, nrows, ncols, gens_max);
#endif

  if ((nrows < 32) || (nrows != ncols) || (nrows % 4) || (nrows > 10000))
    return sequential_game_of_life(outboard, inboard, nrows, ncols, gens_max);
  else
    return parallel_game_of_life(outboard, inboard, nrows, gens_max);
}

char* parallel_game_of_life(char* outboard, char* inboard,
  const int size, const int gens_max) {

  int curgen, i, j, i_rows_per_thread;

  for (j = 0; j < size; j++) {
    const int jwest = LOWBOUND(j - 1, size);
    const int jeast = HIGHBOUND(j + 1, size);
    for (i = 0; i < size; i++) {
      if (IS_ALIVE(BOARD(inboard, i, j))) {

        const int inorth = LOWBOUND(i - 1, size);
        const int isouth = HIGHBOUND(i + 1, size);

        INCR_ALL_NEIGHBORS(inboard, i, j, inorth, isouth, jeast, jwest);
      }
    }
  }

  pthread_t threads[NUM_THREADS];
  ThreadArgs targs[NUM_THREADS];
  ThreadArgs targs_firsts[NUM_THREADS];
  ThreadArgs targs_lasts[NUM_THREADS];


  const int rows_per_thread = size / NUM_THREADS;
  const int array_size = size * size;

  for (i = 0, i_rows_per_thread = 0; i < NUM_THREADS; i++) {

    targs[i].first_row = i_rows_per_thread + 1;
    targs_firsts[i].first_row = i_rows_per_thread;
    targs_firsts[i].last_row = i_rows_per_thread + 1;

    i_rows_per_thread += rows_per_thread;

    targs[i].last_row = i_rows_per_thread - 1;
    targs_lasts[i].first_row = i_rows_per_thread - 1;
    targs_lasts[i].last_row = i_rows_per_thread;

    targs[i].size = size;
    targs_firsts[i].size = size;
    targs_lasts[i].size = size;
  }

  for (curgen = 0; curgen < gens_max; curgen++) {

    memmove(outboard, inboard, array_size * sizeof (char));

    for (i = 0; i < NUM_THREADS; i++) {
      targs_firsts[i].inboard = inboard;
      targs_firsts[i].outboard = outboard;
      pthread_create(&threads[i], NULL, thread_stub_single_row, (void*) &targs_firsts[i]);
    }
    for (i = 0; i < NUM_THREADS; i++)
      pthread_join(threads[i], NULL);

    for (i = 0; i < NUM_THREADS; i++) {
      targs_lasts[i].inboard = inboard;
      targs_lasts[i].outboard = outboard;
      pthread_create(&threads[i], NULL, thread_stub_single_row, (void*) &targs_lasts[i]);
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

  for (j = 0; j < size; j++) {
    for (i = 0; i < size; i++) {
      BOARD(inboard, i, j) = IS_ALIVE(BOARD(inboard, i, j));
    }
  }

  return inboard;
}

void* thread_stub(void* arg) {

  ThreadArgs* targ = (ThreadArgs*) arg;
  char * outboard = targ -> outboard;
  char * inboard = targ -> inboard;
  const int size = targ -> size;
  const int first_row = targ -> first_row;
  const int last_row = targ -> last_row;

  int i, j;

  for (j = 0; j < size; j++) {
    const int jwest = LOWBOUND(j - 1, size);
    const int jeast = HIGHBOUND(j + 1, size);

    for (i = first_row; i < last_row; i++) {
      const char c = BOARD(inboard, i, j);
      if (IS_ALIVE(c)) {
        if (ALIVE_SHOULD_DIE(c)) {
          DIE(BOARD(outboard, i, j));
          const int inorth = LOWBOUND(i - 1, size);
          const int isouth = HIGHBOUND(i + 1, size);
          DECR_ALL_NEIGHBORS(outboard, i, j, inorth, isouth, jeast, jwest);
        }
      } else {
        if (DEAD_SHOULD_LIVE(c)) {
          LIVE(BOARD(outboard, i, j));
          const int inorth = LOWBOUND(i - 1, size);
          const int isouth = HIGHBOUND(i + 1, size);
          INCR_ALL_NEIGHBORS(outboard, i, j, inorth, isouth, jeast, jwest);
        }
      }
    }
  }

  pthread_exit(NULL);
}

void* thread_stub_single_row(void* arg) {

  ThreadArgs* targ = (ThreadArgs*) arg;
  char * outboard = targ -> outboard;
  char * inboard = targ -> inboard;
  const int size = targ -> size;
  const int first_row = targ -> first_row;
  int j;

  const int i = first_row;
  const int inorth = LOWBOUND(i - 1, size);
  const int isouth = HIGHBOUND(i + 1, size);

  for (j = 0; j < size; j++) {
    const int jwest = LOWBOUND(j - 1, size);
    const int jeast = HIGHBOUND(j + 1, size);
    const char c = BOARD(inboard, i, j);
    if (IS_ALIVE(c)) {
      if (ALIVE_SHOULD_DIE(c)) {
        DIE(BOARD(outboard, i, j));
        DECR_ALL_NEIGHBORS(outboard, i, j, inorth, isouth, jeast, jwest);
      }
    } else {
      if (DEAD_SHOULD_LIVE(c)) {
        LIVE(BOARD(outboard, i, j));
        INCR_ALL_NEIGHBORS(outboard, i, j, inorth, isouth, jeast, jwest);
      }
    }
  }

  pthread_exit(NULL);
}