/*
 * Pthread implementation with very fine grained locks
 * both hash level and sample level locks for every key and every sample
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <assert.h>

#include "defs.h"
#include "hash_lock.h"
#include "utils.h"

using namespace std;

#define SAMPLES_TO_COLLECT   10000000
#define RAND_NUM_UPPER_BOUND   100000
#define NUM_SEED_STREAMS            4

/* 
 * ECE454 Students: 
 * Please fill in the following team struct 
 */
team_t team = {
  "Hugh Mungus", /* Team name */

  "Taylan Gocmen", /* First member full name */
  "1000379949", /* First member student number */
  "taylan.gocmen@mail.utoronto.ca", /* First member email address */

  "Gligor Djogo", /* Second member full name */
  "1000884206", /* Second member student number */
  "g.djogo@mail.utoronto.ca" /* Second member email address */
};

unsigned num_threads;
unsigned samples_to_skip;

// This instantiates an empty hash table
// it is a C++ template, which means we define the types for
// the element and key value here: element is "class sample_lock" and
// key value is "unsigned".
hash_lock<sample_lock, unsigned> h;

void *tfunction(void* args_);

int main(int argc, char* argv[]) {

  // Print out team information
  printf("Team Name: %s\n", team.team);
  printf("\n");
  printf("Student 1 Name: %s\n", team.name1);
  printf("Student 1 Student Number: %s\n", team.number1);
  printf("Student 1 Email: %s\n", team.email1);
  printf("\n");
  printf("Student 2 Name: %s\n", team.name2);
  printf("Student 2 Student Number: %s\n", team.number2);
  printf("Student 2 Email: %s\n", team.email2);
  printf("\n");

  // Parse program arguments
  if (argc != 3) {
    printf("Usage: %s <num_threads> <samples_to_skip>\n", argv[0]);
    exit(1);
  }
  sscanf(argv[1], " %d", &num_threads); // not used in this single-threaded version
  sscanf(argv[2], " %d", &samples_to_skip);

  // initialize a 16K-entry (2**14) hash of empty lists
  h.setup(14);

  // create thread and argument pointer arrays with size num_threads
  pthread_t* tid = new pthread_t[num_threads];
  ThreadArgs** targs = new ThreadArgs*[num_threads];

  // 1 threads -> 4 streams each
  // 2 threads -> 2 streams each
  // 4 threads -> 1 streams each
  int tstreams = NUM_SEED_STREAMS / num_threads;
  int i;

  // process streams starting with different initial numbers
  // process streams starting with different initial numbers
  // 1 threads -> tid[0] = streams[0, 4)
  // 2 threads -> tid[0] = streams[0, 2)
  //           -> tid[1] = streams[2, 4)
  // 4 threads -> tid[0] = streams[0, 1)
  //           -> tid[1] = streams[1, 2)
  //           -> tid[2] = streams[2, 3)
  //           -> tid[3] = streams[3, 4)
  for (i = 0; i < num_threads; i++) {
    targs[i] = new ThreadArgs(i, tstreams);
    pthread_create(&tid[i], NULL, tfunction, (void*) targs[i]);
  }

  // join the threads and free the arguments
  for (i = 0; i < num_threads; i++) {
    pthread_join(tid[i], NULL);
    delete targs[i];
  }

  // print a list of the frequency of all samples
  h.print();

  // remove the remaining data structures
  delete [] tid;
  delete [] targs;
}

void *tfunction(void* args_) {
  int i, j, k;
  int rnum;
  unsigned key;
  sample_lock *s;

  // cast the argument to thread arguments
  ThreadArgs* args = (ThreadArgs*) args_;

  // process streams starting with different initial numbers
  for (i = args->index; i < args->endex; i++) {
    rnum = i;

    // collect a number of samples
    for (j = 0; j < SAMPLES_TO_COLLECT; j++) {

      // skip a number of samples
      for (k = 0; k < samples_to_skip; k++) {
        rnum = rand_r((unsigned int*) &rnum);
      }

      // force the sample to be within the range of 0..RAND_NUM_UPPER_BOUND-1
      key = rnum % RAND_NUM_UPPER_BOUND;

      /********************* Beginning of the critical section *********************/
      h.lock_list(key);
      // if this sample has not been counted before
      if (!(s = h.lookup(key))) {

        // insert a new element for it into the hash table
        s = new sample_lock(key);
        h.insert(s);
      }
      h.unlock_list(key);
      /************************ End of the critical section ************************/

      /********************* Beginning of the critical section *********************/
      s->lock_sample();
      // increment the count for the sample
      s->count++;

      s->unlock_sample();
      /************************ End of the critical section ************************/
    }
  }
}