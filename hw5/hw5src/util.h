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

// TODO: see if these are better with ++ or +=1
#define INCR(__board, __i, __j)  (__board[(__i) + size*(__j)]+=1)
#define DECR(__board, __i, __j)  (__board[(__i) + size*(__j)]-=1)


#define INCR_ALL_NEIGHBORS(board, i, j, inorth, isouth, jeast, jwest) \
do { \
  INCR(board, inorth, jwest); \
  INCR(board, inorth, j); \
  INCR(board, inorth, jeast); \
  INCR(board, i, jwest); \
  INCR(board, i, jeast); \
  INCR(board, isouth, jwest); \
  INCR(board, isouth, j); \
  INCR(board, isouth, jeast); \
} while (0)

#define DECR_ALL_NEIGHBORS(board, i, j, inorth, isouth, jeast, jwest) \
do { \
  DECR(board, inorth, jwest); \
  DECR(board, inorth, j); \
  DECR(board, inorth, jeast); \
  DECR(board, i, jwest); \
  DECR(board, i, jeast); \
  DECR(board, isouth, jwest); \
  DECR(board, isouth, j); \
  DECR(board, isouth, jeast); \
} while (0)


#define MOD(x, m) ((x + m) % m)
#define LOWBOUND(x, n) ((x == -1) ? (n - 1) : (x))
#define HIGHBOUND(x, n) ((x == n) ? (0) : (x))


  /* Living and dying conditions */
#define DEAD 0
#define ALIVE 1
#define IS_ALIVE(x) ((x >> 4) & 1)
#define LIVE(x) (x |= (1 << 4))
#define DIE(x) (x &= ~(1 << 4))
#define ALIVE_SHOULD_DIE(x) ((x < (char)0x12) || (x > (char)0x13))
#define DEAD_SHOULD_LIVE(x) (x == (char)0x3)

#endif /* _util_h */
