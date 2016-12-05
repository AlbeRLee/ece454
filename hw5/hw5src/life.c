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

#define NUM_THREADS 4

typedef struct ThreadArgs {
  char * outboard;
  char * inboard;
  int size;
  int first_row;
  int last_row;
  int r_per_th;
  int gens_max;
  pthread_barrier_t *barrier;
  pthread_mutex_t *top_lock;
  pthread_mutex_t *bot_lock;
} ThreadArgs;

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

  if ((nrows > 10000) || (ncols > 10000))
    return NULL;
  else if ((nrows < 32) || (nrows != ncols) || (nrows % 32) || (ncols % 32))
    return sequential_game_of_life(outboard, inboard, nrows, ncols, gens_max);
  else
    return parallel_game_of_life(outboard, inboard, nrows, gens_max);
}

char* parallel_game_of_life(char* outboard, char* inboard,
  const int size, const int gens_max) {

  int i, j;
  const int r_per_th = size / NUM_THREADS;
  const int arraysize = size * size;

  pthread_t threads[NUM_THREADS];
  pthread_mutex_t row_locks[NUM_THREADS];
  pthread_barrier_t barrier;

  pthread_barrier_init(&barrier, NULL, NUM_THREADS);
  for (i = 0; i < NUM_THREADS; i++)
    pthread_mutex_init(&row_locks[i], NULL);

  ThreadArgs targs[NUM_THREADS];
  //  ThreadArgs targs_firsts[NUM_THREADS];
  //  ThreadArgs targs_lasts[NUM_THREADS];

  for (i = 0; i < arraysize; i++)
    if (inboard[i] == 1)
      LIVE(inboard[i]);
  int inorth, isouth, jwest, jeast;
  for (j = 0; j < size; j++) {
    jwest = LOWBOUND(j - 1, size);
    jeast = HIGHBOUND(j + 1, size);
    for (i = 0; i < size; i++) {
      if (IS_ALIVE(BOARD(inboard, i, j))) {
        inorth = LOWBOUND(i - 1, size);
        isouth = HIGHBOUND(i + 1, size);
        INCR_ALL_NEIGHBORS(inboard, i, j, inorth, isouth, jeast, jwest);
      }
    }
  }
  memmove(outboard, inboard, size * size * sizeof (char));

  for (i = 0; i < NUM_THREADS; i++) {
    targs[i].outboard = outboard;
    targs[i].inboard = inboard;
    targs[i].size = size;
    targs[i].first_row = i * r_per_th;
    targs[i].last_row = (i + 1) * r_per_th;
    targs[i].r_per_th = r_per_th;
    targs[i].gens_max = gens_max;
    targs[i].barrier = &barrier;
    targs[i].top_lock = &row_locks[i];
    targs[i].bot_lock = &row_locks[HIGHBOUND(i + 1, NUM_THREADS)];
    pthread_create(&threads[i], NULL, thread_stub, (void*) &targs[i]);
  }
  for (i = 0; i < NUM_THREADS; i++) pthread_join(threads[i], NULL);

  for (i = 0; i < arraysize; i++)
    outboard[i] = IS_ALIVE(outboard[i]);

  return outboard;
}

void* thread_stub(void* arg) {

  ThreadArgs* targ = (ThreadArgs*) arg;
  char * outboard = targ -> outboard;
  char * inboard = targ -> inboard;
  const int size = targ -> size;
  const int first_row = targ -> first_row;
  const int last_row = targ -> last_row;
  const int r_per_th = targ -> r_per_th;
  const int gens_max = targ -> gens_max;
  pthread_barrier_t *barrier = targ -> barrier;
  pthread_mutex_t *top_lock = targ -> top_lock;
  pthread_mutex_t *bot_lock = targ -> bot_lock;

  int curgen, i, j, inorth, isouth, jwest, jeast;

  for (curgen = 0; curgen < gens_max; curgen++) {
    for (j = 0; j < size; j++) {
      jwest = LOWBOUND(j - 1, size);
      jeast = HIGHBOUND(j + 1, size);

      for (i = first_row; i < last_row; i++) {

        const char c = BOARD(inboard, i, j);
        if (IS_ALIVE(c)) {
          if (ALIVE_SHOULD_DIE(c)) {
            inorth = LOWBOUND(i - 1, size);
            isouth = HIGHBOUND(i + 1, size);

            if ((i == first_row) || (i == first_row + 1)) {
              pthread_mutex_lock(top_lock);
              DIE(BOARD(outboard, i, j));
              DECR_ALL_NEIGHBORS(outboard, i, j, inorth, isouth, jeast, jwest);
              pthread_mutex_unlock(top_lock);
            } else if ((i == last_row - 2) || (i == last_row - 1)) {
              pthread_mutex_lock(bot_lock);
              DIE(BOARD(outboard, i, j));
              DECR_ALL_NEIGHBORS(outboard, i, j, inorth, isouth, jeast, jwest);
              pthread_mutex_unlock(bot_lock);
            } else {
              DIE(BOARD(outboard, i, j));
              DECR_ALL_NEIGHBORS(outboard, i, j, inorth, isouth, jeast, jwest);
            }
          }
        } else {
          if (DEAD_SHOULD_LIVE(c)) {
            inorth = LOWBOUND(i - 1, size);
            isouth = HIGHBOUND(i + 1, size);

            if ((i == first_row) || (i == first_row + 1)) {
              pthread_mutex_lock(top_lock);
              LIVE(BOARD(outboard, i, j));
              INCR_ALL_NEIGHBORS(outboard, i, j, inorth, isouth, jeast, jwest);
              pthread_mutex_unlock(top_lock);
            } else if ((i == last_row - 2) || (i == last_row - 1)) {
              pthread_mutex_lock(bot_lock);
              LIVE(BOARD(outboard, i, j));
              INCR_ALL_NEIGHBORS(outboard, i, j, inorth, isouth, jeast, jwest);
              pthread_mutex_unlock(bot_lock);
            } else {
              LIVE(BOARD(outboard, i, j));
              INCR_ALL_NEIGHBORS(outboard, i, j, inorth, isouth, jeast, jwest);
            }
          }
        }
      }
    }

    pthread_barrier_wait(barrier);

    memcpy(inboard + first_row * size, outboard + first_row * size, r_per_th * size * sizeof (char));

    pthread_barrier_wait(barrier);
  }

  pthread_exit(NULL);
}
