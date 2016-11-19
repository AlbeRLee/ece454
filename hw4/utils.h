#ifndef UTILS_H
#define UTILS_H

#include <pthread.h>

#include "hash.h"

class sample;

class sample {
  unsigned my_key;
public:
  sample *next;
  unsigned count;

  sample(unsigned the_key) {
    my_key = the_key;
    count = 0;
  };

  unsigned key() {
    return my_key;
  }

  void print(FILE *f) {
    printf("%d %d\n", my_key, count);
  }
};

class sample_lock;

class sample_lock {
  unsigned my_key;

public:
  sample_lock *next;
  unsigned count;
  pthread_mutex_t mutex;

  sample_lock(unsigned the_key) {
    my_key = the_key;
    count = 0;
    pthread_mutex_init(&mutex, NULL);
  };

  unsigned key() {
    return my_key;
  }

  void print(FILE *f) {
    printf("%d %d\n", my_key, count);
  }

  void lock_sample() {
    pthread_mutex_lock(&mutex);
  }

  void unlock_sample() {
    pthread_mutex_unlock(&mutex);
  }
};

class ThreadArgs;

class ThreadArgs {
public:
  int index;
  int endex;

  hash<sample, unsigned>* h;

  ThreadArgs(int index_, int tstreams_) {
    index = index_ * tstreams_;
    endex = (index + 1) * tstreams_;
    h = NULL;
  }

  ThreadArgs(int index_, int tstreams_, hash<sample, unsigned>* h_) {
    index = index_ * tstreams_;
    endex = (index + 1) * tstreams_;
    h = h_;
  }
};

#endif /* UTILS_H */

