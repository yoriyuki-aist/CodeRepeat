#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include <assert.h>
#include <string.h>
#include "bv.h"
#include "uti.h"



using namespace std; 

#define MAX_SIZE 1000000
#define TEST_SIZE 100

int main()
{
  drand48(); 

  while(1){

    /* Randomly set up the bit vector */
    uint64_t size = (uint64_t) ((double long)MAX_SIZE * drand48()); ; 
    CBitVector bv(size); 

    for(int i = 0; i < size/2; i++){
      uint64_t pos = (uint64_t) ((double long)size * drand48()); ; 
      set_bit(bv.bit_vector, pos, 1); 
    }
    bv.create_blocks();

    cout << "vector size = " << bv.vector_size << endl; 
    cout << "size = " << size << endl; 

    /*vector member query experiments*/
    cout << "Do " << TEST_SIZE << " times of vector member query experiments ... " ; 
    for(int i = 0; i < TEST_SIZE; i++){
      uint64_t pos = (uint64_t) ((double long)size * drand48()); 
      assert(get_bit(bv.bit_vector, pos)==bv.vector_member(pos));
    }
    cout << "all good." << endl << endl << endl; 

    /*rank query experiments*/
    cout << "Do " << TEST_SIZE << " times of rank query experiments ... ";
    for(int i = 0; i < TEST_SIZE; i++){
      uint64_t pos = (uint64_t) ((double long)size * drand48()); 
      uint64_t rank_0 = 0; 
      uint64_t rank_1 = 0; 
      for(int j=0; j <= pos; j++){
	if(get_bit(bv.bit_vector, j)==0)
	  rank_0 ++;
	else 
	  rank_1 ++; 
      }
      assert(rank_0 == bv.rank(pos, 0)); 
      assert(rank_1 == bv.rank(pos, 1)); 
    }
    cout << "all good." << endl << endl << endl; 

    /*select member query experiments*/
    cout << "Do " << TEST_SIZE << " times of select query experiments ... "; 
    for(int i = 0; i < TEST_SIZE; i++){
      uint64_t rank = 1 + (uint64_t) ((double long)size/4 * drand48()); 
      int64_t pos_0 = -1; 
      int64_t pos_1 = -1; 
      uint64_t rank_0 = 0;
      uint64_t rank_1 = 0; 
      for(uint j=0; j<size; j++){
	if(get_bit(bv.bit_vector, j) == 0){
	  rank_0 ++;
	  if(rank_0 == rank)
	    pos_0 = j; 
	}
	else{
	  rank_1 ++; 
	  if(rank_1 == rank)
	    pos_1 = j; 
	}
      }
      assert(pos_0 == bv.select(rank, 0)); 
      if(pos_1 != bv.select(rank,1)){
	cout << "rank = " << rank << endl; 
	cout << "pos_1 = " << pos_1 << "   select(rank,1) = " << bv.select(rank,1) << endl; 
	cout << "bit vector = "; 
	for(int k = 0; k < size; k++)
	  cout << get_bit(bv.bit_vector,k) << " " ; 
	cout << endl; 
      }
      assert(pos_1 == bv.select(rank, 1)); 
    }
    cout << "all good." << endl << endl << endl; 

  }//while

  return 0;

}

