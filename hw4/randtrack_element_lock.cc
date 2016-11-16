
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <assert.h>

#include "defs.h"
#include "hash.h"
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
  "taylan.gocmen@mail.utoronto.ca, /* First member email address */

  "Gligor Djogo", /* Second member full name */
  "1000884206", /* Second member student number */
  "g.djogo@mail.utoronto.ca" /* Second member email address */
};

unsigned num_threads;
unsigned samples_to_skip;

// This instantiates an empty hash table
// it is a C++ template, which means we define the types for
// the element and key value here: element is "class sample" and
// key value is "unsigned".  
hash<sample, unsigned> h;
pthread_mutex_t mutexs[RAND_NUM_UPPER_BOUND];

void *count_samples(void* args_);

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
  for (int j = 0; j < RAND_NUM_UPPER_BOUND; j++) {
    int ret = pthread_mutex_init(&mutexs[j], NULL);
    assert(ret == 0);
  }

  //initialize pthread structure with size num_threads 
  pthread_t* tid = (pthread_t*) malloc(num_threads * sizeof (pthread_t));
  ThreadArgs** targs = (ThreadArgs**) malloc(num_threads * sizeof (ThreadArgs*));
  int num_iterations = NUM_SEED_STREAMS / num_threads;
  int i;

  // process streams starting with different initial numbers
  for (i = 0; i < num_threads; i++) {
    targs[i] = new ThreadArgs(i, num_iterations);
    pthread_create(&tid[i], NULL, count_samples, (void*) targs[i]);
        }

  for (i = 0; i < num_threads; i++) {
    pthread_join(tid[i], NULL);
    free(targs[i]);
  }

  // print a list of the frequency of all samples
  h.print();
  free(tid);
  free(targs);
}

void *count_samples(void* args_) {
  int i, j, k;
  int rnum;
  unsigned key;
  sample *s;

  ThreadArgs* args = (ThreadArgs*) args_;

  // process streams starting with different initial numbers
  for (i = args.index; i < args.endex; i++) {
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
      pthread_mutex_lock(&mutexs[key]);

      // if this sample has not been counted before
      if (!(s = h.lookup(key))) {

        // insert a new element for it into the hash table
        s = new sample(key);
        h.insert(s);
      }

      // increment the count for the sample
      s->count++;

      pthread_mutex_unlock(&mutexs[key]);
/************************ End of the critical section ************************/
    }
  }
}