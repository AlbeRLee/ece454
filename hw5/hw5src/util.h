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
}

/**
 * Given neighbor count and current state, return zero if cell will be
 * dead, or nonzero if cell will be alive, in the next round.
 */
static inline char
alivep(char count, char state) {
  return (!state && (count == (char) 3)) ||
          (state && (count >= 2) && (count <= 3));
}
#define IS_ODD(x) (x & 0x1)
#define MAX(x,y) ((x) > (y) ? (x) : (y))
#define MIN(x,y) ((x) > (y) ? (y) : (x))

#define INCR(__board, __i, __j)  (__board[(__i) + nrows*(__j)]++)
#define DECR(__board, __i, __j)  (__board[(__i) + nrows*(__j)]--)


#define MOD(x, m) ((x + m) % m)
#define LOWBOUND(x, n) ((x == -1) ? (n - 1) : (x))
#define HIGHBOUND(x, n) ((x == n) ? (0) : (x))


/* Living and dying conditions */
#define DEAD 0
#define ALIVE 1
#define IS_ALIVE(x) ((x >> 4) & 0x1)
#define LIVE(x) (x |= (1 << 4))
#define DIE(x) (x &= ~(1 << 4))
#define ALIVE_SHOULD_DIE(x) ((x < (char)0x12) || (x > (char)0x13))
#define DEAD_SHOULD_LIVE(x) (x == (char)0x3)

#endif /* _util_h */
