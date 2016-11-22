#ifndef HASH_LOCK_H
#define HASH_LOCK_H

#include <stdio.h>
#include <pthread.h>
#include "list.h"

#define HASH_INDEX(_addr,_size_mask) (((_addr) >> 2) & (_size_mask))

template<class Ele, class Keytype> class hash_lock;

template<class Ele, class Keytype> class hash_lock {
private:
  unsigned my_size_log;
  unsigned my_size;
  unsigned my_size_mask;
  list<Ele, Keytype> *entries;
  list<Ele, Keytype> *get_list(unsigned the_idx);
  pthread_mutex_t *mutexs;

public:
  void setup(unsigned the_size_log = 5);
  void insert(Ele *e);
  Ele *lookup(Keytype the_key);
  void print(FILE *f = stdout);
  void reset();
  void cleanup();
  void lock_list(Keytype the_key);
  void unlock_list(Keytype the_key);
};

template<class Ele, class Keytype> void hash_lock<Ele, Keytype>::setup(unsigned the_size_log) {
  my_size_log = the_size_log;
  my_size = 1 << my_size_log;
  my_size_mask = (1 << my_size_log) - 1;
  entries = new list<Ele, Keytype>[my_size];
  mutexs = new pthread_mutex_t[my_size];
  for (int i = 0; i < my_size; i++) {
    pthread_mutex_init(&mutexs[i], NULL);
  }
}

template<class Ele, class Keytype> list<Ele, Keytype> *hash_lock<Ele, Keytype>::get_list(unsigned the_idx) {
  if (the_idx >= my_size) {
    fprintf(stderr, "hash_lock<Ele,Keytype>::list() public idx out of range!\n");
    exit(1);
  }
  return &entries[the_idx];
}

template<class Ele, class Keytype> Ele *hash_lock<Ele, Keytype>::lookup(Keytype the_key) {
  list<Ele, Keytype> *l;

  l = &entries[HASH_INDEX(the_key, my_size_mask)];
  return l->lookup(the_key);
}

template<class Ele, class Keytype> void hash_lock<Ele, Keytype>::print(FILE *f) {
  unsigned i;

  for (i = 0; i < my_size; i++) {
    entries[i].print(f);
  }
}

template<class Ele, class Keytype> void hash_lock<Ele, Keytype>::reset() {
  unsigned i;
  for (i = 0; i < my_size; i++) {
    entries[i].cleanup();
  }
}

template<class Ele, class Keytype> void hash_lock<Ele, Keytype>::cleanup() {
  reset();
  delete [] entries;
  delete [] mutexs;
}

template<class Ele, class Keytype> void hash_lock<Ele, Keytype>::insert(Ele *e) {
  entries[HASH_INDEX(e->key(), my_size_mask)].push(e);
}

template<class Ele, class Keytype> void hash_lock<Ele, Keytype>::lock_list(Keytype the_key) {
  int mutex_index = HASH_INDEX(the_key, my_size_mask);
  pthread_mutex_lock(&mutexs[mutex_index]);
}

template<class Ele, class Keytype> void hash_lock<Ele, Keytype>::unlock_list(Keytype the_key) {
  int mutex_index = HASH_INDEX(the_key, my_size_mask);
  pthread_mutex_unlock(&mutexs[mutex_index]);
}

#endif /* HASH_LOCK_H */

