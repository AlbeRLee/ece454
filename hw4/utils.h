/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   utils.h
 * Author: gocmenta
 *
 * Created on November 15, 2016, 10:21 PM
 */

#ifndef UTILS_H
#define UTILS_H


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

class ThreadArgs{
public:
  int index;
  int itrns;
  int endex;

  ThreadArgs(int index_, int itrns_){
    index = index_;
    itrns = itrns_;
    endex = index_ + itrns_;
  }
};

#endif /* UTILS_H */

