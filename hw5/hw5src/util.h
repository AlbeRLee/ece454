#ifndef _util_h
#define _util_h

/**
 * C's mod ('%') operator is mathematically correct, but it may return
 * a negative remainder even when both inputs are nonnegative.  This
 * function always returns a nonnegative remainder (x mod m), as long
 * as x and m are both positive.  This is helpful for computing
 * toroidal boundary conditions.
 */
static inline int
mod(int x, int m) {
  return (x < 0) ? ((x % m) + m) : (x % m);
  //  	return (x >= 0) ? (x % m) : ((x % m) + m);
  //	//int res = x % m;
  //  	//return (res > 0) ? res : (res + m);
}

/**
 * Given neighbor count and current state, return zero if cell will be
 * dead, or nonzero if cell will be alive, in the next round.
 */
static inline char
alivep(char count, char state) {
  return (!state && (count == (char) 3)) ||
          (state && (count >= 2) && (count <= 3));
  //  // if state == 0 and count == 3, return 1; if state!=0 and (count==2 or count ==3), return 1
  //	return (state == 0) ? (count == 3) : (count == 2 || count == 3);
  //  	//return (state && count >= 2 && count <= 3) || (!state && (count == (char) 3));
}

#endif /* _util_h */