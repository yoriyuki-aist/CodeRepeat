/*
  An implementation of the "wavelet tree" data structure presented at "High-order
  entropy-compressed text indexes" by Roberto Grossi, Ankur Gupta and
  Jeffrey Scott Vitter, ACM-SIAM Symposium on Discrete Algorithms(SODA)
  2003: 841-850. 
  
  Program by: Bojian Xu, bojianxu@tamu.edu
*/


#ifndef _utilities_h_
#define _utilities_h_

#include <stdlib.h>
#include <stdint.h>
#include <iostream>
#include <map>
#include <queue>
#include <stdint.h>

using namespace std; 

#define BYTE_SIZE 8
#define VECTOR_UNIT_SIZE (sizeof(uint64_t)*BYTE_SIZE)  //size of the data type in byte that represents the bit vector

/* Lower bound of super block size. No super blocks will be created if bit vector size is smaller than MIN_BLOCK_SIZE */
#define MIN_BLOCK_SIZE  (POP_COUNT_SIZE*5)       
#define POP_COUNT_SIZE VECTOR_UNIT_SIZE   //number of bits in one popcount operation  

/*SELECT THE APPROPRIATE DATA TYPE FOR THE SUPER BLOCK ARRAY*/
typedef uint32_t Super_block_t; 
//typedef uint64_t Super_block_t; 



#define max(x,y)  (x)>(y)? (x):(y)



/* define the pop count instrcution */
#ifdef __SSE4_2__
#include <smmintrin.h>
//#include <nmmintrin.h>
#define my_popcount_u64(x) _mm_popcnt_u64(x)
#else
#define my_popcount_u64(x) _soft_popcount_u64_8(x)
//#define my_popcount_u64(x) _soft_popcount_u64_16(x)
#endif

/*filter of screen output*/
//#define _DEBUG_   //show running trace
//#define _PRINT_TEXT_  //show text
//#define _PRINT_VECTOR_  //show bit vector

extern uint8_t popcount_table_8[];
//extern uint8_t popcount_table_16[];


#ifndef uchar_t
typedef unsigned char uchar_t;
#endif

uint32_t set_bit(uint64_t *bit_vector, uint64_t pos, uint32_t bit);
uint32_t get_bit(uint64_t *bit_vector, uint64_t pos); 
uint8_t _soft_popcount_u64_8(uint64_t x);
uint8_t _soft_popcount_u64_16(uint64_t x);
int64_t select_bit_array_seq(uint64_t *bit_vector, uint64_t vector_size, uint64_t rank, uint32_t bit);
uint64_t file_size(FILE *fp); 


//void insert_map(map<Text_t, uint64_t> & abt, Text_t c);

//test function
void user_print(uint64_t pos);

#endif
