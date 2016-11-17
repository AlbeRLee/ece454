/********************************************************
 * Kernels to be optimized for the CS:APP Performance Lab
 ********************************************************/

#include <stdio.h>
#include <stdlib.h>
#include "defs.h"
#include "assert.h"
#define MIN(x, y) ((x < y) ? x : y)
#define MAX(x, y) ((x > y) ? x : y)

/* 
 * ECE454 Students: 
 * Please fill in the following team struct 
 */
team_t team = {
  "Hugh Mungus", /* Team name */

  "Taylan Gocmen", /* First member full name */
  "taylan.gocmen@mail.utoronto.ca", /* First member email address */

  "Gligor Djogo", /* Second member full name */
  "g.djogo@mail.utoronto.ca" /* Second member email addr */
};

/***************
 * ROTATE KERNEL
 ***************/

/******************************************************
 * Your different versions of the rotate kernel go here
 ******************************************************/

/* 
 * naive_rotate - The naive baseline version of rotate 
 */
char naive_rotate_descr[] = "naive_rotate: Naive baseline implementation";

void naive_rotate(int dim, pixel *src, pixel *dst) {
  int i, j;

  for (i = 0; i < dim; i++)
    for (j = 0; j < dim; j++)
      dst[RIDX(dim - 1 - j, i, dim)] = src[RIDX(i, j, dim)];
}

/*
 * ECE 454 Students: Write your rotate functions here:
 * comments for this branch
 */

/*
 * Function definitions for the helper functions used in the final rotate function
 */
void attempt_seventeen(int dim, pixel *src, pixel *dst);
void attempt_nineteen(int dim, pixel *src, pixel *dst);
/* 
 * rotate - Your current working version of rotate
 * IMPORTANT: This is the version you will be graded on
 */
char rotate_descr[] = "rotate: Current working version";

void rotate(int dim, pixel *src, pixel *dst) {

  //  #define RIDX(i,j,n) ((i)*(n)+(j))
  //  dst[RIDX(dim - 1 - j, i, dim)] = src[RIDX(i, j, dim)];
  if(dim < 256) {
    int i, j, dstIndex, srcIndex, dstBase, srcBase;
    const int BASE = (dim * dim) - dim;

    for (i = 0; i < dim - 1; i+= 8) {

      dstBase = BASE + i;
      srcBase = dim * i;

      for (j = 0; j < dim; j++) {

        dstIndex = dstBase - j*dim;
        srcIndex = srcBase + j;

        dst[dstIndex++] = src[srcIndex]; srcIndex += dim;
        dst[dstIndex++] = src[srcIndex]; srcIndex += dim;
        dst[dstIndex++] = src[srcIndex]; srcIndex += dim;
        dst[dstIndex++] = src[srcIndex]; srcIndex += dim;

        dst[dstIndex++] = src[srcIndex]; srcIndex += dim;
        dst[dstIndex++] = src[srcIndex]; srcIndex += dim;
        dst[dstIndex++] = src[srcIndex]; srcIndex += dim;
        dst[dstIndex++] = src[srcIndex];
      }
    }
  }
  else if (dim <= 2048){
    return attempt_nineteen(dim, src, dst);
  }
  else {
    return attempt_seventeen(dim, src, dst);
  }
}


/* 
 * second attempt = Loop Invariant Code Motion
 * Code motion on source and destination indeces.
 */
char rotate_two_descr[] = "second attempt, as much LICM as possible";

void attempt_two(int dim, pixel *src, pixel *dst) {

  int i, j, dstIndex, srcIndex;
  const int temp = (dim - 1) * dim;

  for (i = 0; i < dim; i++) {
    srcIndex = i * dim;
    dstIndex = i + temp;

    for (j = 0; j < dim; j++) {
      dst[dstIndex] = src[srcIndex + j];
      dstIndex -= dim;
    }
  }
}


/* 
 * third attempt
 * Loop unrolling
 */
char rotate_three_descr[] = "third attempt, loop unrolling by 2";

void attempt_three(int dim, pixel *src, pixel *dst) {

  int i, j, dstIndex, srcIndex;

  for (i = 0; i < dim - 1; i += 2)
    for (j = 0; j < dim; j++) {
      dstIndex = RIDX(dim - 1 - j, i, dim);
      srcIndex = RIDX(i, j, dim);
      dst[dstIndex] = src[srcIndex];
      dst[dstIndex + 1] = src[srcIndex + dim];
    }
}


/* 
 * fourth attempt
 * Loop unrolling swapped the order of the loops
 */
char rotate_four_descr[] = "fourth attempt, loop unrolling by 2 swap loop order";

void attempt_four(int dim, pixel *src, pixel *dst) {

  int i, j, dstIndex, srcIndex;

  for (j = 0; j < dim; j++)
    for (i = 0; i < dim - 1; i += 2) {
      dstIndex = RIDX(dim - 1 - j, i, dim);
      srcIndex = RIDX(i, j, dim);
      dst[dstIndex] = src[srcIndex];
      dst[dstIndex + 1] = src[srcIndex + dim];
    }
}


/* 
 * fifth attempt
 * Loop unrolling increase the unroll amount
 */
char rotate_five_descr[] = "fifth attempt, loop unrolling by 16";

void attempt_five(int dim, pixel *src, pixel *dst) {

  int i, j, dstIndex, srcIndex;

  for (i = 0; i < dim - 15; i += 16)
    for (j = 0; j < dim; j++) {
      dstIndex = RIDX(dim - 1 - j, i, dim);
      srcIndex = RIDX(i, j, dim);
      dst[dstIndex] = src[srcIndex];
      dstIndex += 1;
      srcIndex += dim;
      dst[dstIndex] = src[srcIndex];
      dstIndex += 1;
      srcIndex += dim;
      dst[dstIndex] = src[srcIndex];
      dstIndex += 1;
      srcIndex += dim;
      dst[dstIndex] = src[srcIndex];
      dstIndex += 1;
      srcIndex += dim;
      dst[dstIndex] = src[srcIndex];
      dstIndex += 1;
      srcIndex += dim;
      dst[dstIndex] = src[srcIndex];
      dstIndex += 1;
      srcIndex += dim;
      dst[dstIndex] = src[srcIndex];
      dstIndex += 1;
      srcIndex += dim;
      dst[dstIndex] = src[srcIndex];
      dstIndex += 1;
      srcIndex += dim;
      dst[dstIndex] = src[srcIndex];
      dstIndex += 1;
      srcIndex += dim;
      dst[dstIndex] = src[srcIndex];
      dstIndex += 1;
      srcIndex += dim;
      dst[dstIndex] = src[srcIndex];
      dstIndex += 1;
      srcIndex += dim;
      dst[dstIndex] = src[srcIndex];
      dstIndex += 1;
      srcIndex += dim;
      dst[dstIndex] = src[srcIndex];
      dstIndex += 1;
      srcIndex += dim;
      dst[dstIndex] = src[srcIndex];
      dstIndex += 1;
      srcIndex += dim;
      dst[dstIndex] = src[srcIndex];
      dstIndex += 1;
      srcIndex += dim;
      dst[dstIndex] = src[srcIndex];
    }
}


/* 
 * sixth attempt
 */
char rotate_six_descr[] = "sixth attempt, loop unrolling by 16 swap loop order";

void attempt_six(int dim, pixel *src, pixel *dst) {

  int i, j, dstIndex, srcIndex;

  for (j = 0; j < dim; j++)
    for (i = 0; i < dim - 15; i += 16) {
      dstIndex = RIDX(dim - 1 - j, i, dim);
      srcIndex = RIDX(i, j, dim);
      dst[dstIndex] = src[srcIndex];
      dstIndex += 1;
      srcIndex += dim;
      dst[dstIndex] = src[srcIndex];
      dstIndex += 1;
      srcIndex += dim;
      dst[dstIndex] = src[srcIndex];
      dstIndex += 1;
      srcIndex += dim;
      dst[dstIndex] = src[srcIndex];
      dstIndex += 1;
      srcIndex += dim;
      dst[dstIndex] = src[srcIndex];
      dstIndex += 1;
      srcIndex += dim;
      dst[dstIndex] = src[srcIndex];
      dstIndex += 1;
      srcIndex += dim;
      dst[dstIndex] = src[srcIndex];
      dstIndex += 1;
      srcIndex += dim;
      dst[dstIndex] = src[srcIndex];
      dstIndex += 1;
      srcIndex += dim;
      dst[dstIndex] = src[srcIndex];
      dstIndex += 1;
      srcIndex += dim;
      dst[dstIndex] = src[srcIndex];
      dstIndex += 1;
      srcIndex += dim;
      dst[dstIndex] = src[srcIndex];
      dstIndex += 1;
      srcIndex += dim;
      dst[dstIndex] = src[srcIndex];
      dstIndex += 1;
      srcIndex += dim;
      dst[dstIndex] = src[srcIndex];
      dstIndex += 1;
      srcIndex += dim;
      dst[dstIndex] = src[srcIndex];
      dstIndex += 1;
      srcIndex += dim;
      dst[dstIndex] = src[srcIndex];
      dstIndex += 1;
      srcIndex += dim;
      dst[dstIndex] = src[srcIndex];
    }
}


/* 
 * seventh attempt
 */
char rotate_seven_descr[] = "seventh attempt, loop unrolling by 16, increment inside index";

void attempt_seven(int dim, pixel *src, pixel *dst) {

  int i, j, dstIndex, srcIndex;

  for (i = 0; i < dim - 15; i += 16)
    for (j = 0; j < dim; j++) {
      dstIndex = RIDX(dim - 1 - j, i, dim);
      srcIndex = RIDX(i, j, dim);

      dst[dstIndex++] = src[srcIndex];
      srcIndex += dim;
      dst[dstIndex++] = src[srcIndex];
      srcIndex += dim;
      dst[dstIndex++] = src[srcIndex];
      srcIndex += dim;
      dst[dstIndex++] = src[srcIndex];
      srcIndex += dim;

      dst[dstIndex++] = src[srcIndex];
      srcIndex += dim;
      dst[dstIndex++] = src[srcIndex];
      srcIndex += dim;
      dst[dstIndex++] = src[srcIndex];
      srcIndex += dim;
      dst[dstIndex++] = src[srcIndex];
      srcIndex += dim;

      dst[dstIndex++] = src[srcIndex];
      srcIndex += dim;
      dst[dstIndex++] = src[srcIndex];
      srcIndex += dim;
      dst[dstIndex++] = src[srcIndex];
      srcIndex += dim;
      dst[dstIndex++] = src[srcIndex];
      srcIndex += dim;

      dst[dstIndex++] = src[srcIndex];
      srcIndex += dim;
      dst[dstIndex++] = src[srcIndex];
      srcIndex += dim;
      dst[dstIndex++] = src[srcIndex];
      srcIndex += dim;
      dst[dstIndex] = src[srcIndex];
    }
}


/* 
 * eight attempt
 */
char rotate_eight_descr[] = "eight attempt, tiling by 32";

void attempt_eight(int dim, pixel *src, pixel *dst) {

  int i1, j1, i2, j2;
  const int TILE_FACTOR = 32;

  for (i1 = 0; i1 < dim; i1 += TILE_FACTOR)
    for (j1 = 0; j1 < dim; j1 += TILE_FACTOR)
      for (i2 = i1; (i2 - i1) < TILE_FACTOR; i2++)
        for (j2 = j1; (j2 - j1) < TILE_FACTOR; j2++)
          dst[RIDX(dim - 1 - j2, i2, dim)] = src[RIDX(i2, j2, dim)];
}


/* 
 * ninth attempt
 */
char rotate_nine_descr[] = "ninth attempt, tiling by 32 and column traversal";

void attempt_nine(int dim, pixel *src, pixel *dst) {

  int i1, j1, i2, j2;
  const int TILE_FACTOR = 32;

  for (i1 = 0; i1 < dim; i1 += TILE_FACTOR)
    for (j1 = 0; j1 < dim; j1 += TILE_FACTOR)
      for (i2 = j1; (i2 - j1) < TILE_FACTOR; i2++)
        for (j2 = i1; (j2 - i1) < TILE_FACTOR; j2++)
          dst[RIDX(dim - 1 - j2, i2, dim)] = src[RIDX(i2, j2, dim)];
}


/* 
 * tenth attempt
 */
char rotate_ten_descr[] = "tenth attempt, tiling by 32 and block optimization";

void attempt_ten(int dim, pixel *src, pixel *dst) {

  int i1, j1, i2, j2;
  const int TILE_FACTOR = 32;

  for (i1 = 0; i1 < dim; i1 += TILE_FACTOR)
    for (j1 = 0; j1 < dim; j1 += TILE_FACTOR)
      for (i2 = i1; (i2 - i1) < MIN(dim - i1, TILE_FACTOR); i2++)
        for (j2 = j1; (j2 - j1) < MIN(dim - j1, TILE_FACTOR); j2++)
          dst[RIDX(dim - 1 - j2, i2, dim)] = src[RIDX(i2, j2, dim)];
}


/* 
 * eleventh attempt
 */
char rotate_eleven_descr[] = "eleventh attempt, tiling by 16";

void attempt_eleven(int dim, pixel *src, pixel *dst) {

  int i1, j1, i2, j2;
  const int TILE_FACTOR = 16;

  for (i1 = 0; i1 < dim; i1 += TILE_FACTOR)
    for (j1 = 0; j1 < dim; j1 += TILE_FACTOR)
      for (i2 = i1; (i2 - i1) < TILE_FACTOR; i2++)
        for (j2 = j1; (j2 - j1) < TILE_FACTOR; j2++)
          dst[RIDX(dim - 1 - j2, i2, dim)] = src[RIDX(i2, j2, dim)];
}
char rotate_five_descr[] = "rotate5";
void attempt_five(int dim, pixel *src, pixel *dst) 
{
    rotate_tile(dim, src, dst, 32, 16);
}
char rotate_six_descr[] = "rotate6";
void attempt_six(int dim, pixel *src, pixel *dst) 
{
    rotate_tile(dim, src, dst, 32, 32);
}
char rotate_seven_descr[] = "rotate7";
void attempt_seven(int dim, pixel *src, pixel *dst) 
{
    rotate_tile(dim, src, dst, 32, 64);
}
char rotate_eight_descr[] = "rotate8";
void attempt_eight(int dim, pixel *src, pixel *dst) 
{
    rotate_tile(dim, src, dst, 32, 128);
}*/


/* 
 * twelfth attempt
 */
char rotate_twelve_descr[] = "twelfth attempt, tiling by 64";

void attempt_twelve(int dim, pixel *src, pixel *dst) {

  int i1, j1, i2, j2;
  const int TILE_FACTOR = 64;

  for (i1 = 0; i1 < dim; i1 += TILE_FACTOR)
    for (j1 = 0; j1 < dim; j1 += TILE_FACTOR)
      for (i2 = i1; (i2 - i1) < TILE_FACTOR; i2++)
        for (j2 = j1; (j2 - j1) < TILE_FACTOR; j2++)
          dst[RIDX(dim - 1 - j2, i2, dim)] = src[RIDX(i2, j2, dim)];
}


/* 
 * thirteenth attempt
 */
char rotate_thirteen_descr[] = "thirteenth attempt, tiling by 32 * 8";

void attempt_thirteen(int dim, pixel *src, pixel *dst) {

  int i1, j1, i2, j2;
  const int TILE_FACTOR_i = 32;
  const int TILE_FACTOR_j = 8;

  for (i1 = 0; i1 < dim; i1 += TILE_FACTOR_i)
    for (j1 = 0; j1 < dim; j1 += TILE_FACTOR_j)
      for (i2 = i1; (i2 - i1) < TILE_FACTOR_i; i2++)
        for (j2 = j1; (j2 - j1) < TILE_FACTOR_j; j2++)
          dst[RIDX(dim - 1 - j2, i2, dim)] = src[RIDX(i2, j2, dim)];
}


/* 
 * fourteenth attempt
 */
char rotate_fourteen_descr[] = "fourteenth attempt, tiling by 64 * 16";

void attempt_fourteen(int dim, pixel *src, pixel *dst) {

  int i1, j1, i2, j2;
  const int TILE_FACTOR_i = 64;
  const int TILE_FACTOR_j = 16;

  for (i1 = 0; i1 < dim; i1 += TILE_FACTOR_i)
    for (j1 = 0; j1 < dim; j1 += TILE_FACTOR_j)
      for (i2 = i1; (i2 - i1) < TILE_FACTOR_i; i2++)
        for (j2 = j1; (j2 - j1) < TILE_FACTOR_j; j2++)
          dst[RIDX(dim - 1 - j2, i2, dim)] = src[RIDX(i2, j2, dim)];
}


/* 
 * fifteenth attempt
 */
char rotate_fifteen_descr[] = "fifteenth attempt, tiling with vectors";

void attempt_fifteen(int dim, pixel *src, pixel *dst) {

  int register i1, j1, i2, j2, dstIndex;
  const int TILE_FACTOR = 32;

  for (i1 = 0; i1 < dim; i1 += TILE_FACTOR)
    for (j1 = 0; j1 < dim; j1++) {
      i2 = i1 + TILE_FACTOR;
      dstIndex = (dim - 1 - j1) * dim;

      for (j2 = i1; j2 < i2; j2++)
        dst[dstIndex + j2] = src[RIDX(j2, j1, dim)];

    }
}


/* 
 * sixteenth attempt
 */
char rotate_sixteen_descr[] = "sixteenth attempt, another promising tiling attempt, with statistics";

void attempt_sixteen(int dim, pixel *src, pixel *dst) {

  
    int i, j, ti, tj, tile_i, tile_j;
    tile_i = 32;
    tile_j = 256;
    while (dim % tile_j != 0) {
        tile_j >>= 1;
    }
    for (j = 0; j < dim/tile_j; j++)
        for (i = 0; i < dim/tile_i; i++)
            for (tj = j*tile_j; tj < (j+1)*tile_j; tj++)
                for (ti = i*tile_i; ti < (i+1)*tile_i; ti++)
                    dst[(dim-1-tj)*dim + ti] = src[ti*dim + tj];
}
 /*
  * // STATISTICS tile_i tile_j speedup
  * //                2   2  1.2x
  * //                4   4 1.6x
  * //                8   8  2.1x
  * //               16  16  2.6x 
  * //               32  32  2.8x
  * //               64  64  x (fails on dim=96=3*32) 
  * 
  * //               16  32  2.7x
  * //                8  32  2.4x
  * //                4  32  1.8x
  * //               32  16  2.7x
  * //               32   8  2.3x
  * //               32   4  2.0x
  * 
  * //               64  64  2.7x (with if condition reverting to 32 32)
  * //               32  64  2.9x
  * //               64  32  2.7x
  * //               32 128  2.9x
  * //               32 128  3.0x (with while condition halving down tj)
  * //               32 256  3.0x
  * //               32 512  2.9x
  * //               32 1024 3.0x
  * //               32 2048 3.0x
  * //               32  64  2.9x
 */


/* 
 * seventeenth attempt
 */
char rotate_seventeen_descr[] = "seventeenth attempt, tiling by 16 and improved MATH with LICM, diagonal traverse for loop unrolling";

void attempt_seventeen(int dim, pixel *src, pixel *dst) {

  //  #define RIDX(i,j,n) ((i)*(n)+(j))
  //  dst[RIDX(dim - 1 - j, i, dim)] = src[RIDX(i, j, dim)]
  //  dst[(dim -1 -j)* dim + i] = src[i * dim + j]

  int i, j, 
    dstIndex,  // = dim * dim - dim - j * dim + i 
    srcIndex,  // = dim * i + j
    dstBase, 
    srcBase;
  const int BASE = (dim * dim) - dim;

  for (i = 0; i < dim - 1; i+= 16) {
    
    dstBase = BASE + i;
    srcBase = dim * i;
    
    for (j = 0; j < dim; j++) {

      dstIndex = dstBase - j*dim;
      srcIndex = srcBase + j;

      dst[dstIndex++] = src[srcIndex]; srcIndex += dim;
      dst[dstIndex++] = src[srcIndex]; srcIndex += dim;
      dst[dstIndex++] = src[srcIndex]; srcIndex += dim;
      dst[dstIndex++] = src[srcIndex]; srcIndex += dim;
      
      dst[dstIndex++] = src[srcIndex]; srcIndex += dim;
      dst[dstIndex++] = src[srcIndex]; srcIndex += dim;
      dst[dstIndex++] = src[srcIndex]; srcIndex += dim;
      dst[dstIndex++] = src[srcIndex]; srcIndex += dim;
      
      dst[dstIndex++] = src[srcIndex]; srcIndex += dim;
      dst[dstIndex++] = src[srcIndex]; srcIndex += dim;
      dst[dstIndex++] = src[srcIndex]; srcIndex += dim;
      dst[dstIndex++] = src[srcIndex]; srcIndex += dim;
      
      dst[dstIndex++] = src[srcIndex]; srcIndex += dim;
      dst[dstIndex++] = src[srcIndex]; srcIndex += dim;
      dst[dstIndex++] = src[srcIndex]; srcIndex += dim;
      dst[dstIndex++] = src[srcIndex];
    }
  }
}


/* 
 * nineteenth attempt
 */
char rotate_nineteen_descr[] = "nineteenth attempt, tiling by 32 and improved MATH, , diagonal traverse for loop unrolling";

void attempt_nineteen(int dim, pixel *src, pixel *dst) {

  //  #define RIDX(i,j,n) ((i)*(n)+(j))
  //  dst[RIDX(dim - 1 - j, i, dim)] = src[RIDX(i, j, dim)];
  
  int i, j, dstIndex, srcIndex, dstBase, srcBase;
  const int BASE = (dim * dim) - dim;

  for (i = 0; i < dim - 1; i+= 32) {
    
    dstBase = BASE + i;
    srcBase = dim * i;
    
    for (j = 0; j < dim; j++) {

      dstIndex = dstBase - j*dim;
      srcIndex = srcBase + j;

      dst[dstIndex++] = src[srcIndex]; srcIndex += dim;
      dst[dstIndex++] = src[srcIndex]; srcIndex += dim;
      dst[dstIndex++] = src[srcIndex]; srcIndex += dim;
      dst[dstIndex++] = src[srcIndex]; srcIndex += dim;
      
      dst[dstIndex++] = src[srcIndex]; srcIndex += dim;
      dst[dstIndex++] = src[srcIndex]; srcIndex += dim;
      dst[dstIndex++] = src[srcIndex]; srcIndex += dim;
      dst[dstIndex++] = src[srcIndex]; srcIndex += dim;
      
      dst[dstIndex++] = src[srcIndex]; srcIndex += dim;
      dst[dstIndex++] = src[srcIndex]; srcIndex += dim;
      dst[dstIndex++] = src[srcIndex]; srcIndex += dim;
      dst[dstIndex++] = src[srcIndex]; srcIndex += dim;
      
      dst[dstIndex++] = src[srcIndex]; srcIndex += dim;
      dst[dstIndex++] = src[srcIndex]; srcIndex += dim;
      dst[dstIndex++] = src[srcIndex]; srcIndex += dim;
      dst[dstIndex++] = src[srcIndex]; srcIndex += dim;
      
      dst[dstIndex++] = src[srcIndex]; srcIndex += dim;
      dst[dstIndex++] = src[srcIndex]; srcIndex += dim;
      dst[dstIndex++] = src[srcIndex]; srcIndex += dim;
      dst[dstIndex++] = src[srcIndex]; srcIndex += dim;
      
      dst[dstIndex++] = src[srcIndex]; srcIndex += dim;
      dst[dstIndex++] = src[srcIndex]; srcIndex += dim;
      dst[dstIndex++] = src[srcIndex]; srcIndex += dim;
      dst[dstIndex++] = src[srcIndex]; srcIndex += dim;
      
      dst[dstIndex++] = src[srcIndex]; srcIndex += dim;
      dst[dstIndex++] = src[srcIndex]; srcIndex += dim;
      dst[dstIndex++] = src[srcIndex]; srcIndex += dim;
      dst[dstIndex++] = src[srcIndex]; srcIndex += dim;
      
      dst[dstIndex++] = src[srcIndex]; srcIndex += dim;
      dst[dstIndex++] = src[srcIndex]; srcIndex += dim;
      dst[dstIndex++] = src[srcIndex]; srcIndex += dim;
      dst[dstIndex++] = src[srcIndex];
      
    }
  }
}

/* 
 * twentieth attempt
 */
char rotate_twenty_descr[] = "twentieth attempt, tiling by 8 or combination of 17th and 19th attempts";

void attempt_twenty(int dim, pixel *src, pixel *dst) {

  //  #define RIDX(i,j,n) ((i)*(n)+(j))
  //  dst[RIDX(dim - 1 - j, i, dim)] = src[RIDX(i, j, dim)];
  if(dim < 256) {
    int i, j, dstIndex, srcIndex, dstBase, srcBase;
    const int BASE = (dim * dim) - dim;

    for (i = 0; i < dim - 1; i+= 8) {

      dstBase = BASE + i;
      srcBase = dim * i;

      for (j = 0; j < dim; j++) {

        dstIndex = dstBase - j*dim;
        srcIndex = srcBase + j;

        dst[dstIndex++] = src[srcIndex]; srcIndex += dim;
        dst[dstIndex++] = src[srcIndex]; srcIndex += dim;
        dst[dstIndex++] = src[srcIndex]; srcIndex += dim;
        dst[dstIndex++] = src[srcIndex]; srcIndex += dim;

        dst[dstIndex++] = src[srcIndex]; srcIndex += dim;
        dst[dstIndex++] = src[srcIndex]; srcIndex += dim;
        dst[dstIndex++] = src[srcIndex]; srcIndex += dim;
        dst[dstIndex++] = src[srcIndex];
      }
    }
  }
  else if (dim <= 2048){
    return attempt_nineteen(dim, src, dst);
  }
  else {
    return attempt_seventeen(dim, src, dst);
  }
}

/* 
 * fourth attempt (commented out for now)
char rotate_four_descr[] = "rotate: loop-invariant code motion [1.0X]";
void attempt_four(int dim, pixel *src, pixel *dst) 
{
    int i, j, dst_hold, dst_hold2, dim2, src_hold;

    dim2 = dim * dim;
    dst_hold = dim2 - dim;
    src_hold = 0;

    for (i = dst_hold; i < dim2; i++) {
        dst_hold2 = i;
        for (j = 0; j < dim; j++) {
            dst[dst_hold2] = src[src_hold];
            dst_hold2 -= dim;
            src_hold++;
        }
    }
}
 */

/* 
 * fifth attempt (commented out for now)
char rotate_five_descr[] = "rotate: loop reorder+invariant [1.2X]";
void attempt_five(int dim, pixel *src, pixel *dst) 
{
    int i, j, dst_h, src_h;
    // 1.1x, 1.2x, 1.1x, 1.2x
    dst_h = dim*dim - 1;
    src_h = dst_h + 1 - dim;

    for (j = 0; j < dim; j++){
        for (i = src_h; i >= 0; i -= dim){
            dst[dst_h] = src[i];
            dst_h--;
        }
        src_h++;
    }
}
 */

/* 
 * sixth attempt (commented out for now)
char rotate_six_descr[] = "super simple tiling size 4ti x 4tj [1.6X]";
void attempt_six(int dim, pixel *src, pixel *dst) 
{
    int i, j, ti, tj, tile_i, tile_j;
    tile_i = 32;
    tile_j = 64;
    while (dim % tile_j != 0) {
        tile_j >>= 1;
    }

    for (i = 0; i < dim/tile_i; i++)
        for (j = 0; j < dim/tile_j; j++)
            for (tj = j*tile_j; tj < (j+1)*tile_j; tj++)
                for (ti = i*tile_i; ti < (i+1)*tile_i; ti++)
                    dst[(dim-1-tj)*dim + ti] = src[ti*dim + tj];
}
 * // STATISTICS tile_i tile_j speedup
 * //                2   2  1.2x
 * //                4   4 1.6x
 * //                8   8  2.1x
 * //               16  16  2.6x 
 * //               32  32  2.8x
 * //               64  64  x (fails on dim=96=3*32) 
 * 
 * //               16  32  2.7x
 * //                8  32  2.4x
 * //                4  32  1.8x
 * //               32  16  2.7x
 * //               32   8  2.3x
 * //               32   4  2.0x
 * 
 * //               64  64  2.7x (with if condition reverting to 32 32)
 * //               32  64  2.9x
 * //               64  32  2.7x
 * //               32 128  2.9x
 * //               32 128  3.0x (with while condition halving down tj, and i then j)
 * //               32 256  3.0x
 * //               32 512  2.9x
 * //               32 1024 3.0x
 * //               32 2048 3.0x
 * //               32  64  2.9x
 */

/* 
 * seventh attempt (commented out for now)
char rotate_seven_descr[] = "attempted loop invariant [2.9X]";
void attempt_seven(int dim, pixel *src, pixel *dst) 
{
    int i, j, ti, tj, tile_i, tile_j;
    tile_i = 32;
    tile_j = 128;
    while (dim % tile_j != 0) {
        tile_j >>= 1;
    }
    
    int Base;
    int dstIndex1, dstIndex2;
    int srcIndex1, srcIndex2, srcIndex3;
    Base = dim*dim - dim;
    //srcIndex      //ti*dim + tj
    //dstIndex      //dim*dim - dim - tj*dim + ti
                    //(dim*dim - dim) - j*dim - loopj*dim + i + loopi
                    //(dim*dim - dim) + i - j*dim - loopj*dim + loopi

    for (i = 0; i < dim; i += tile_i) {
        dstIndex1 = Base + i;
        srcIndex1 = i*dim;
        for (j = 0; j < dim; j += tile_j) {
            srcIndex2 = srcIndex1 + j;
            
            for (tj = j; tj < (j + tile_j); tj++) {
                dstIndex2 = dstIndex1 - tj*dim;
                srcIndex3 = srcIndex2++;
                for (ti = 0; ti < tile_i; ti++) {
                    
                    dst[dstIndex2] = src[srcIndex3];
                    dstIndex2++;
                    srcIndex3 += dim;
                }
            }
        }
    }
}
 */

/*********************************************************************
 * register_rotate_functions - Register all of your different versions
 *     of the rotate kernel with the driver by calling the
 *     add_rotate_function() for each test function. When you run the
 *     driver program, it will test and report the performance of each
 *     registered test function.  
 *********************************************************************/

void register_rotate_functions() {
//  add_rotate_function(&naive_rotate, naive_rotate_descr);
  add_rotate_function(&rotate, rotate_descr);
  
/* ... Register additional rotate functions here */
  
//  add_rotate_function(&attempt_two, rotate_two_descr);
//  add_rotate_function(&attempt_three, rotate_three_descr);
//  add_rotate_function(&attempt_four, rotate_four_descr);
//  add_rotate_function(&attempt_five, rotate_five_descr);
//  add_rotate_function(&attempt_six, rotate_six_descr);
//  add_rotate_function(&attempt_seven, rotate_seven_descr);
//  add_rotate_function(&attempt_eight, rotate_eight_descr);
//  add_rotate_function(&attempt_nine, rotate_nine_descr);
//  add_rotate_function(&attempt_ten, rotate_ten_descr);
//  add_rotate_function(&attempt_eleven, rotate_eleven_descr);
//  add_rotate_function(&attempt_twelve, rotate_twelve_descr);
//  add_rotate_function(&attempt_thirteen, rotate_thirteen_descr);
//  add_rotate_function(&attempt_fourteen, rotate_fourteen_descr);
//  add_rotate_function(&attempt_fifteen, rotate_fifteen_descr);
//  add_rotate_function(&attempt_sixteen, rotate_sixteen_descr);
//  add_rotate_function(&attempt_seventeen, rotate_seventeen_descr);
//  add_rotate_function(&attempt_eighteen, rotate_eighteen_descr);
//  add_rotate_function(&attempt_nineteen, rotate_nineteen_descr);
//  add_rotate_function(&attempt_twenty, rotate_twenty_descr);
  


}

