#ifndef _wt_node_h_
#define _wt_node_h_


#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include "uti.h"



template<class Text_t>
class CWaveletTreeNode
{
 public: 
  //'alphabet, alphabet_size,gate, leve' are all not used in huffman wavelet tree
  Text_t *alphabet;   //This local alphabet is only a reference to a portion of the global alphabet. So no need to delete. 
  uint32_t alphabet_size;  
  Text_t gate;    //the median to divide the local alphabet;
  uint32_t level; 

  uint64_t *bit_vector; 
  uint64_t vector_size; 
  uint64_t cur; //the position of the next bit assignment

  uint16_t super_block_size;   //number of bits in each super block
  Super_block_t *super_block_array; //Each array item stores the #1-bits in the current super block and its preceeding super blocks. 
  Super_block_t super_block_array_size; //number of super blocks in the bit vector

 public:
  CWaveletTreeNode *parent;   //necessary for 'select' query
  CWaveletTreeNode *left;
  CWaveletTreeNode *right;
  CWaveletTreeNode * previous;  // only used for wbwt topology construction
  CWaveletTreeNode * next;  // only used for wbwt topology construction

  CWaveletTreeNode();
  ~CWaveletTreeNode();
  
  void create_blocks(void); 

  uint32_t vector_member(uint64_t pos);
  uint64_t rank(uint64_t pos, uint32_t bit);
  int64_t select(uint64_t p_rank, uint32_t bit);
  void range_2d_4side(uint64_t left, uint64_t right, Text_t min, Text_t max, void (*user)(uint64_t));
};



/*********************************************************************
         
         Implementation of the 'CWaveletTreeNode' class

*********************************************************************/

/* constructor */
template <class Text_t> 
CWaveletTreeNode<Text_t>::CWaveletTreeNode()
{
  this->alphabet = NULL;    //not defined for huffman tree except leaf node
  this->alphabet_size = 0; //not defined for huffman tree
  this->gate = 0;  //not defined for huffman tree
  
  this->bit_vector = NULL; 
  this->vector_size = 0; 
  
  this->super_block_size = 0;   //number of bits in each super block
  this->super_block_array = NULL; //Each array item stores the #1-bits in the current super block and its preceeding super blocks. 
  this->super_block_array_size = 0; //number of super blocks in the bit vector
  
  this->level = 0;  //only defined for wbwt. 
  this->cur = 0; //the position of the next bit assignment
  
  this->parent = NULL;
  this->left = NULL; 
  this->right = NULL; 
  this->previous = NULL;
  this->next = NULL;  
}

/* deconstructor */
template <class Text_t>
CWaveletTreeNode<Text_t>::~CWaveletTreeNode()
{
  if(alphabet_size > 1)  // only non-leaf node has the bit vector 
    delete[] bit_vector;
  if(super_block_array_size > 0)
    delete[] super_block_array; 
}


/*
  1) Create blocks for the bit vector for efficient rank/select query. 
  2) Currently implementation uses one level and popcount instruction. 
  3) the reason of not using the second level block is that the popcount instruction has already involved 64 bits, 
     which is already larger than the second level block size (log n/2) of any data set of reasonable size. 
  4) the super block size is at least 64*4 bits (MIN_BLOCK_SIZE). 
*/
template <class Text_t>
void CWaveletTreeNode<Text_t>::create_blocks()
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


/* return bit_vector[pos] */
template <class Text_t>
uint32_t CWaveletTreeNode<Text_t>::vector_member(uint64_t pos)
{
  return get_bit(bit_vector, pos);
}

/* 
   Task: return the number of 'bit' in bit_vector[0...pos]

   Warning: 
   1) 'pos' starts from 0, NOT 1.
   2) the function does not check whether 'pos' is a legal postion in the bit vector. 
   3) it is the caller the repsponsibility to secure 'pos' is legal. 
*/
template <class Text_t>
uint64_t CWaveletTreeNode<Text_t>::rank(uint64_t pos, uint32_t bit)
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
   Output: 
   1) the position of the p_rank'th bit in bit_vector. 
   2) if total number of 'bit' in the vector is smaller than 'p_rank', -1 is returned. 
   
   Warnings:
   1) the parameter 'p_rank' starts from 1, not 0. 
   2) the returned position starts from 0, not 1. 
*/
template <class Text_t>
int64_t CWaveletTreeNode<Text_t>::select(uint64_t p_rank, uint32_t bit)
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



/*
Task: 
  1) In the text represented by this tree node, find all the positions in [L,R] where the character is in the range [m,M].
  2) Each found text position will be transmitted as a parameter to the function 'user', during the range query.

Warning:
  1) The text positions that are transmitted to 'user' are not necessarility in the ascending order
  2) Text text position starts from 0.

Notes:
  1) This function call inside is recursive. All the found position will be back tracked to the root node to find the actual text positions. 
  2) The function does not return the list of positions, because the output size can be huge which can potentially consumes lots of memory. 
  3) Instead, the function transmits every found position in the original text to the function 'user'. It leaves the use to decide what to do 
     with the found positions. 
*/
template <class Text_t>
void CWaveletTreeNode<Text_t>::range_2d_4side(uint64_t L, uint64_t R, Text_t m, Text_t M, void (*user)(uint64_t))
{
  if(m > alphabet[alphabet_size-1] || M < alphabet[0])
    return; 
  
  if( m <= alphabet[0] && M >= alphabet[alphabet_size-1]){
    for(uint64_t i = L; i <= R; i++){  //back track to the root for each bit in the position [L,R] in the current tree node.
      uint64_t cur_pos = i; 
      for(CWaveletTreeNode *node = this; node->parent != NULL; node = node->parent){
	uint32_t bit = ( (node == node->parent->left)? 0:1 );  
	cur_pos = node->parent->select(cur_pos+1, bit); 
      }//for
      user(cur_pos);  /******* send the text position to the user. *******/
    }//for
    return; 
  }//if

  uint64_t new_L, new_R;
  if(m < gate && (new_R=rank(R,0)) > 0){
    new_R --; 
    new_L = rank(L,0); 
    if( get_bit(bit_vector,L) == 0 )
      new_L --;

    if(new_L <= new_R)
      left->range_2d_4side(new_L, new_R, m, M, user);
  }

  if(M >= gate && (new_R=rank(R,1)) > 0){
    new_R --; 
    new_L = rank(L,1); 
    if( get_bit(bit_vector,L) == 1 )
      new_L --;

    if(new_L <= new_R)
      right->range_2d_4side(new_L, new_R, m, M, user);
  }
  return;
}


#endif
