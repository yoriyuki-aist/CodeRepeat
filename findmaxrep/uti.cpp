/*
  An implementation of the "wavelet tree" data structure presented at "High-order
  entropy-compressed text indexes" by Roberto Grossi, Ankur Gupta and
  Jeffrey Scott Vitter, ACM-SIAM Symposium on Discrete Algorithms(SODA)
  2003: 841-850. 
  
  Program by: Bojian Xu, bojianxu@tamu.edu
*/



#include <stdio.h>
#include <assert.h>
#include <math.h>
#include "uti.h"

/* table for popcount operation. 
   popcount_table[i] stores the number of 1-bits in the binary representation of the integer i.
*/
uint8_t popcount_table_8[256] = {
0,1,1,2,1,2,2,3,1,2,2,3,2,3,3,4,
1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,5,
1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,5,
2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,
1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,5,
2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,
2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,
3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,
1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,5,
2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,
2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,
3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,
2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,
3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,
3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,
4,5,5,6,5,6,6,7,5,6,6,7,6,7,7,8
};

//uint8_t popcount_table_16[65536];


/*
  This function assumes the size of 'bit_vector' is not smaller than 'pos'+1. 
  The caller is responsible for securing this condition.

  Warning: 
  (1) the postion of the first bit in the bit vector is 'zero', NOT 'one'. 
  (2) parameter 'pos' is from the domain {0,1,2,...,|bit_vector|-1}.
*/
uint32_t set_bit(uint64_t *bit_vector, uint64_t pos, uint32_t bit)
{
  //the bit at 'bit_offset' position of the byte bit_vector[byte_offset] is set to be 'bit'. 
  //  uint64_t byte_offset = pos / VECTOR_UNIT_SIZE;
  //  uint64_t bit_offset = pos % VECTOR_UNIT_SIZE; 
  uint64_t byte_offset = pos >> 6;
  uint64_t bit_offset = pos & (VECTOR_UNIT_SIZE-1); 

  uint64_t temp = 1;   
  temp <<= VECTOR_UNIT_SIZE - bit_offset - 1;  //cannot directly use "1<<....". This implicitely assume 32-bit variable. 
 
  if(bit == 0)
    bit_vector[byte_offset] &= (~temp); 
  else if (bit ==1)
    bit_vector[byte_offset] |= temp; 
  else
    {fprintf(stderr, "set bit error.\n"); exit(1);}

  return bit; 
}


/*
Task: return the pos^th bit in the bet vector

Warning: 
1) The function assumes that pos is not beyond the bit vector boundary. 
2) It is the caller the responsibility to secure this condition.
3) The domain of 'pos' starts from 0. 
*/
uint32_t get_bit(uint64_t *bit_vector, uint64_t pos)
{
  uint64_t byte_offset = pos >> 6;
  uint64_t bit_offset = pos & (VECTOR_UNIT_SIZE-1); 

  uint64_t temp = 1;
  temp <<= VECTOR_UNIT_SIZE - bit_offset - 1;  //cannot directly use "1<<....". This implicitely assume 32-bit variable. 

  return (bit_vector[byte_offset] & temp) > 0 ? 1:0;
}

/*
  Output the number of 1-bits in the binary represenation of the 64-bit integer x. 
*/
uint8_t _soft_popcount_u64_8(uint64_t x)
{

  //  cout << "Using software popcount." << endl; 
  uint8_t ret = 0; 
  int temp = sizeof(uint64_t); 

  for(int i = 0; i < temp; i++){
    ret += popcount_table_8[x & 0xff];
    x >>= BYTE_SIZE;
  }
  return ret;
}


/*
  Output the number of 1-bits in the binary represenation of the 64-bit integer x. 
*/
/*
uint8_t _soft_popcount_u64_16(uint64_t x)
{
  //  cout << "Using software popcount." << endl; 
  uint8_t ret = 0; 

  int temp1 = sizeof(uint64_t)/2; 
  int temp2 = BYTE_SIZE * 2; 

  for(int i = 0; i < temp1; i++){
    ret += popcount_table_16[x & 0xffff];
    x >>= temp2; 
  }
  return ret;
}
*/

/*

Task:
  Sequentially scan the bit vector and return
  1) the position of 'bit' where its rank is equal to 'rank', or
  2) -1, if no such position exsits. 
  
  Warning: 
  1) vector_size is in terms of bit. 
  2) 'p_rank' starts from 1, not 0.
*/

int64_t select_bit_array_seq(uint64_t *vector, uint64_t vector_size, uint64_t p_rank, uint32_t bit)
{
#ifdef _DEBUG_
  assert(bit == 0 || bit == 1); 
  assert(p_rank > 0); 
#endif

  uint64_t ret;

  //get the size of the array 'vector'.
  uint64_t vector_array_size = (uint64_t)ceill((long double)vector_size/(long double)(VECTOR_UNIT_SIZE));

  /* Get the last array element in 'vector'. Some of the bits may not be useful, so need to mask them off */
  uint64_t last = vector[vector_array_size -1]; 
  uint64_t temp1 = 0xffffffff;
  uint64_t temp2 = 0xffffffff;
  temp1 <<= 32; 
  temp1 |= temp2; 

  if(bit == 1){
    temp1 <<= (vector_array_size * VECTOR_UNIT_SIZE - vector_size); 
    last &= temp1; 
  }
  else{
    temp1 >>= (vector_size - (vector_array_size-1)*VECTOR_UNIT_SIZE - 1);
    temp1 >>= 1; //note: >>64 is equal to >>0 in gcc, so you have to seperate >>64 into two steps.
    last |= temp1; 
  }
  /* End of getting the last element for the array 'vector */


  // count the 'bit' until having seens enough or reaching the last second element of array 'vector'
  uint64_t bits_count = 0; 
  uint64_t i;
  uint32_t ones = 0, zeros = 0; 
  for(i = 0; i < vector_array_size - 1; i ++){
    ones =  my_popcount_u64(vector[i]);
    zeros = VECTOR_UNIT_SIZE - ones; 
    bits_count += ( bit==1? ones:zeros ); 
    if(bits_count >= p_rank)
      break; 
  }

  //set back to the last second element of the array 'vector'
  if(bits_count >= p_rank)
    bits_count -= ( bit==1? ones:zeros ); 

  //the number of positions in 'vector' that have been checked
  ret = i * VECTOR_UNIT_SIZE;


  /* start to count the number of 'bit' in the last relevant element*/
  if(i == vector_array_size-1)
    temp1 = last; 
  else 
    temp1 = vector[i]; 

  //slower method: scan bit by bit in the 64-bit integer
/*  
  temp2 = 1; 
  temp2 <<= 63; 
  
  for(uint32_t j = 0; j < VECTOR_UNIT_SIZE; j++){
    if((temp2 & temp1) > 0 )
      bits_count += (bit==1?1:0);
    else
      bits_count += (bit==0?1:0); 

    if(bits_count == p_rank)
      return (ret + j) ; 

    temp2 >>= 1;
  }
  */
  
  //faster method, scan byte by byte until getting to a byte where bit by bit scanning is needed. 
  uint64_t filter = 0xff; 
  filter <<= VECTOR_UNIT_SIZE - BYTE_SIZE;
  for(uint32_t j = 0; j < VECTOR_UNIT_SIZE/BYTE_SIZE; j++){  //search over bytes by using lookup table
    uint64_t filted = temp1 & filter;
    filted >>= VECTOR_UNIT_SIZE - (j+1)*BYTE_SIZE; 
    bits_count += (bit == 1 ? popcount_table_8[filted] : BYTE_SIZE - popcount_table_8[filted]); 

    if(bits_count >= p_rank){  //search over bits
      bits_count -= (bit == 1 ? popcount_table_8[filted] : BYTE_SIZE - popcount_table_8[filted]); 
      uint64_t smaller_filter = 1;
      smaller_filter <<= BYTE_SIZE - 1;
      for(uint32_t k = 0; k < BYTE_SIZE; k++){
	if((filted & smaller_filter) > 0 )
	  bits_count += (bit==1?1:0);
	else
	  bits_count += (bit==0?1:0); 
	
	if(bits_count == p_rank)
	  return (ret + j*BYTE_SIZE + k);

	smaller_filter >>= 1;
      }
    }
    filter >>= BYTE_SIZE; 
  }

  return -1; 
}



uint64_t file_size(FILE *fp)
{
  uint64_t n; 
  if(fseek(fp, 0, SEEK_END) == 0) {
    n = ftell(fp);
    rewind(fp);
    if(n < 0) {
      fprintf(stderr, "ftell error\n");
      perror(NULL);
      exit(1);
    }
  } else {
    fprintf(stderr, "fseek error\n");
    perror(NULL);
    exit(1);
  }
  return n; 
}



/*
void insert_map(map<Text_t, uint64_t> & abt,Text_t c)
{
  map<Text_t, uint64_t> :: iterator iter_abt;

  iter_abt = abt.find(c);
  if(iter_abt == abt.end()){
    abt.insert(pair<Text_t, uint64_t>(c, 1));
    assert(abt.size() < abt.max_size());
  }
  else
    iter_abt->second ++;
}
*/

