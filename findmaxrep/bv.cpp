#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include <assert.h>
#include <string.h>
#include "bv.h"
#include "uti.h"


CBitVector::CBitVector()
{
  bit_vector = NULL; 
  vector_size = 0;

  super_block_size = 0;   //number of bits in each super block
  super_block_array = NULL; //Each array item stores the #1-bits in the current super block and its preceeding super blocks. 
  super_block_array_size = 0; //number of super blocks in the bit vector
}


CBitVector::CBitVector(uint64_t size)
{
  assert(size > 0); 
  
  vector_size = size; 
  
  uint64_t vector_array_size = (uint64_t)ceill((long double)(vector_size)/(long double)VECTOR_UNIT_SIZE);
  bit_vector = new uint64_t[vector_array_size];
  memset(bit_vector, 0, VECTOR_UNIT_SIZE/BYTE_SIZE * vector_array_size);

  super_block_size = 0;   //number of bits in each super block
  super_block_array = NULL; //Each array item stores the #1-bits in the current super block and its preceeding super blocks. 
  super_block_array_size = 0; //number of super blocks in the bit vector
}



CBitVector::~CBitVector()
{
  delete[] bit_vector; 
  delete[] super_block_array;
}


/*
  Create blocks for faster rank/select query. 
*/

void CBitVector::create_blocks(void)
{
  if(bit_vector == NULL){  //there is no vector at leaf node. 
    super_block_size = 0; //useless value
    super_block_array = NULL; //no super blocks
    super_block_array_size = 0; //no super blocks

    return; 
  }

  /***** Vector size is too small. There is no need to create the super blocks *****/
  if(vector_size < MIN_BLOCK_SIZE){
    super_block_size = 0; //useless value
    super_block_array = NULL; //no super blocks
    super_block_array_size = 0; //no super blocks

    return; 
  }
    
  /***** Need to create super blocks *****/

  //compute the super block size
  long double temp = logl((long double)vector_size)/logl(2.0); 
  uint16_t s = (uint16_t)(temp*temp/2.0);  //s=(log n)^2/2, where n is the vector size 
  if(s < MIN_BLOCK_SIZE)
    super_block_size = MIN_BLOCK_SIZE; 
  else {
    /* Set the super block size to be the next larger power of 2 starting from s. 
       Note that this super block size must be a multiple of POP_COUNT_SIZE.
       Note that this super block size  must be smaller than the vector size, so no risk of illegal memory access.
    */
    uint16_t i; 
    for(i = 0; s > 0; i++)
      s >>= 1; 
    super_block_size = (1 << i); 
  }

  //compute the number of super blocks
  super_block_array_size = (Super_block_t)floorl((long double)vector_size/(long double)super_block_size);
  assert(super_block_array_size > 0);

  //allocate space for the super block array
  super_block_array = new Super_block_t[super_block_array_size]; 
  uint16_t num_pop = super_block_size / POP_COUNT_SIZE;  /* number of 64-bit smaller blocks in each super block. */

  //assign the value for the super block array
  for(Super_block_t i = 0; i < super_block_array_size; i++){
    Super_block_t count = 0; //#1-bit in the current super block. 

    for(uint16_t j = 0; j < num_pop; j++)
      count += my_popcount_u64(bit_vector[super_block_size/VECTOR_UNIT_SIZE * i + j]);

    if(i==0) 
      super_block_array[i] = count; 
    else
      super_block_array[i] = super_block_array[i-1] + count; 
  } // for

  return; 
}

/*
  Return the bit at the position 'pos'. 
  Note: position starts from 0. 
*/
uint32_t CBitVector::vector_member(uint64_t pos)
{
  return get_bit(bit_vector, pos);
}


/*
  Return the number of 'bit' in the bit vector from the beginning to the position 'pos'.
  Note: positions start from 0. 
*/
uint64_t CBitVector::rank(uint64_t pos, uint32_t bit)
{
  assert(bit==0 || bit==1);
  assert(bit_vector != NULL);

  /***** counting #0-bits from bit_vector[0...pos] *****/
  if(bit==0)
    return pos + 1 - rank(pos, 1); 

  /***** couting #1-bits from bit_vector[0...pos] *****/

  uint64_t ret = 0; 
  uint64_t *new_vector_start = bit_vector;
  uint64_t new_pos = pos;

  
  if(super_block_array_size > 0){  /*** the case of having super blocks. ***/
    uint64_t block_offset =(uint64_t)floorl((long double)pos/(long double)super_block_size);

    if(block_offset > 0){
      ret = super_block_array[block_offset-1]; 
      new_vector_start = bit_vector + block_offset*(super_block_size/VECTOR_UNIT_SIZE); 
      new_pos = pos - block_offset * super_block_size;  //the query postion relative to the new starting position of the bit vector. 
    }
  }

  uint64_t byte_offset = new_pos >> 6;   // new_pos/64
  uint64_t bit_offset = new_pos & (VECTOR_UNIT_SIZE-1);  //new_pos % 64

  for(uint64_t i = 0; i < byte_offset; i++)
    ret += my_popcount_u64(new_vector_start[i]); 

  //temp1 with value 0xffffffffffffffff. gcc does not allow to assign such a large value directly. 
  uint64_t temp1 = 0xffffffff, temp2 = 0xffffffff; 
  temp1 <<= 32; 
  temp1 |= temp2; 

  temp1 <<= VECTOR_UNIT_SIZE - bit_offset - 1;
  
  ret += my_popcount_u64(new_vector_start[byte_offset]&temp1);
  
  return ret; 
}


/*
  Return the smallest position where the rank of 'bit' is 'p_rank'. 
  Note: the position starts from 0. 
*/
int64_t CBitVector::select(uint64_t p_rank, uint32_t bit)
{
  /* total number of 'bit' in the vector is smaller than 'p_rank' */

  assert(p_rank > 0); 

#ifdef _DEBUG_
  if(rank(vector_size-1, bit) < p_rank) 
    return -1; 
#endif

  /* The case of no super blocks */

  if(super_block_array_size == 0)
    return  select_bit_array_seq(bit_vector, vector_size, p_rank, bit); //select operation over a bit array without blocks 


  /* The case of having super blocks */

  //unit is sizeof(bit vector array data type). The new position to start search after the binary search over blocks. 
  uint64_t * new_vector_start;
  
  //the length in bits from new_vector_start to search after the binary search over blocks. 
  uint64_t new_vector_length;   
  
  //the new rank used in the search starting from  new_vector_start
  uint64_t new_rank;  

  uint64_t ones = super_block_array[super_block_array_size-1];
  uint64_t zeros = super_block_size*super_block_array_size - ones; 
  uint64_t rank_block = (bit == 1 ? ones:zeros);  //number of 'bit' in the super blocks 

  if(rank_block < p_rank){ //the position locates after the super blocks
    new_vector_start = bit_vector + super_block_array_size * (super_block_size / VECTOR_UNIT_SIZE);
    new_vector_length = vector_size - super_block_array_size * super_block_size; 
    new_rank = p_rank - rank_block; 

    uint64_t ret = select_bit_array_seq(new_vector_start, new_vector_length, new_rank, bit);
    
    return ret < 0 ? ret : ret + super_block_array_size * super_block_size; 
  }
  else { //The position must be in a super block. Do a binary search over the super block array. 
    uint64_t low = 0; 
    uint64_t high = super_block_array_size - 1; 
    uint64_t mid; 
    uint64_t rank_mid; 

    while(low <= high){
      mid = low + (high-low)/2; 

      rank_mid =  ( bit==1 ? super_block_array[mid] : super_block_size*(mid+1) - super_block_array[mid] ); 
      if(rank_mid < p_rank)
	low = mid + 1; 
      else{  //rank_mid >= p_rank 
	if(mid == 0){ //position in the first super block 
	  new_vector_start = bit_vector; 
	  new_vector_length = super_block_size;  
	  new_rank = p_rank; 
	  
	  return select_bit_array_seq(new_vector_start, new_vector_length, new_rank, bit); 
	}
	else if( ( bit==1 ? super_block_array[mid-1] : super_block_size*(mid) - super_block_array[mid-1] ) >= p_rank )
	  high = mid - 1; 
	else{ //position must be in this block pointed by 'mid'.
	  new_vector_start = bit_vector + mid * (super_block_size/VECTOR_UNIT_SIZE); 
	  new_vector_length = super_block_size; 
	  new_rank = p_rank - ( bit==1 ? super_block_array[mid-1] : super_block_size*(mid) - super_block_array[mid-1] );

	  return select_bit_array_seq(new_vector_start, new_vector_length, new_rank, bit) + mid * super_block_size; 
	}
      }//else
    }//while
  }//else
  
  return 0;
}


