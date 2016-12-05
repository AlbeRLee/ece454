#ifndef _life_h
#define _life_h

/*
 * TeamName: Hugh Mungus
 * Member1Name: Taylan Gocmen
 * Member1ID: 1000379949
 * member1Email: taylan.gocmen@mail.utoronto.ca
 * Member2Name: Gligor Djogo
 * Member2ID: 1000884206
 * member2Email: g.djogo@mail.utoronto.ca
 */

/**
 * Given the initial board state in inboard and the board dimensions
 * nrows by ncols, evolve the board state gens_max times by alternating
 * ("ping-ponging") between outboard and inboard.  Return a pointer to 
 * the final board; that pointer will be equal either to outboard or to
 * inboard (but you don't know which).
 */

char* game_of_life(char* outboard, char* inboard,
        const int nrows, const int ncols, const int gens_max);

/**
 * Same output as game_of_life() above, except this is not
 * parallelized.  Useful for checking output.
 */
char* sequential_game_of_life(char* outboard, char* inboard,
        const int nrows, const int ncols, const int gens_max);

/**
 *  
 */
char* parallel_game_of_life(char* outboard, char* inboard,
        const int size, const int gens_max);


void* thread_stub(void* arg);
//void* thread_stub_single_row(void* arg);

#endif /* _life_h */
