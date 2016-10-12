/********************************************************
 * Kernels to be optimized for the CS:APP Performance Lab
 ********************************************************/

#include <stdio.h>
#include <stdlib.h>
#include "defs.h"

/* 
 * ECE454 Students: 
 * Please fill in the following team struct 
 */
team_t team = {
    "Hugh Mungus", /* Team name */

    "Taylan Gocmen", /* First member full name */
    "taylan.gocmen@mail.utoronto.ca", /* First member email address */

    "Gligor Djogo", /* Second member full name (leave blank if none) */
    "g.djogo@mail.utoronto.ca" /* Second member email addr (leave blank if none) */
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

void rotate_tile(int dim, pixel *src, pixel *dst, int tile_i, int tile_j){
    int i, j, ti, tj;
    while (dim%tile_j != 0) {
        tile_j >>= 1;
    }

    for (i = 0; i < dim/tile_i; i++)
        for (j = 0; j < dim/tile_j; j++)
            for (tj = j*tile_j; tj < (j + 1)*tile_j; tj++)
                for (ti = i*tile_i; ti < (i + 1)*tile_i; ti++)
                    dst[(dim - 1 - tj)*dim + ti] = src[ti*dim + tj];
}

/* 
 * rotate - Your current working version of rotate
 * IMPORTANT: This is the version you will be graded on
 */
char rotate_descr[] = "rotate: Current working version";
void rotate(int dim, pixel *src, pixel *dst) {

    rotate_tile(dim, src, dst, 32, 64);
}

/*
char rotate_two_descr[] = "rotate2";
void attempt_two(int dim, pixel *src, pixel *dst) 
{
    rotate_tile(dim, src, dst, 32, 2);
}
char rotate_three_descr[] = "rotate3";
void attempt_three(int dim, pixel *src, pixel *dst) 
{
    rotate_tile(dim, src, dst, 32, 4);
}
char rotate_four_descr[] = "rotate4";
void attempt_four(int dim, pixel *src, pixel *dst) 
{
    rotate_tile(dim, src, dst, 32, 8);
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
 * second attempt (commented out for now)
char rotate_two_descr[] = "rotate: replaced Macros [1.0X speedup]";
void attempt_two(int dim, pixel *src, pixel *dst) 
{
    int i, j;

    for (i = 0; i < dim; i++)
        for (j = 0; j < dim; j++)
            dst[(dim-1-j)*dim + i] = src[i*dim + j];
}
 */

/* 
 * third attempt (commented out for now)
char rotate_three_descr[] = "rotate: loop reordering [1.3X speedup]";
void attempt_three(int dim, pixel *src, pixel *dst) 
{
    int i, j;

    for (j = 0; j < dim; j++)
        for (i = 0; i < dim; i++)
            dst[(dim-1-j)*dim + i] = src[i*dim + j];
}
 */

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
    add_rotate_function(&naive_rotate, naive_rotate_descr);
    add_rotate_function(&rotate, rotate_descr);

    //add_rotate_function(&attempt_two, rotate_two_descr);   
    //add_rotate_function(&attempt_three, rotate_three_descr);   
    //add_rotate_function(&attempt_four, rotate_four_descr);   
    //add_rotate_function(&attempt_five, rotate_five_descr);   
    //add_rotate_function(&attempt_six, rotate_six_descr);   
    //add_rotate_function(&attempt_seven, rotate_seven_descr);   
    //add_rotate_function(&attempt_eight, rotate_eight_descr);   
    //add_rotate_function(&attempt_nine, rotate_nine_descr);   
    //add_rotate_function(&attempt_ten, rotate_ten_descr);   
    //add_rotate_function(&attempt_eleven, rotate_eleven_descr);   

    /* ... Register additional rotate functions here */
}

