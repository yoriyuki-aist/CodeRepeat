#ifndef _bv_h_
#define _bv_h_


#include "uti.h"

/*bit vector*/

class CBitVector
{
 public:
  uint64_t *bit_vector;  //bit vector is implemented as an array 64-bit integers. 
  uint64_t vector_size;  //number of bits in the bit vector

  uint16_t super_block_size;   //number of bits in each super block
  Super_block_t *super_block_array; //Each array item stores the #1-bits in the current super block and its preceeding super blocks. 
  Super_block_t super_block_array_size; //number of super blocks in the bit vector

  CBitVector(); 
  CBitVector(uint64_t size); 
  ~CBitVector(); 

  void create_blocks(void);
 
  uint32_t vector_member(uint64_t pos);
  uint64_t rank(uint64_t pos, uint32_t bit);
  int64_t select(uint64_t p_rank, uint32_t bit);
};

#endif

