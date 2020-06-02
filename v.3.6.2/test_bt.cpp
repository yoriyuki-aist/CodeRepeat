#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include <assert.h>
#include <string.h>
#include "bt.h"
#include "uti.h"
#include "bv.h"
 
#define MAX_LEAVES 10000000

using namespace std; 

int main()
{
  while(1){
    uint64_t leaves = (uint64_t) ((double long)(MAX_LEAVES-1) * drand48()) + 10; 
    cout << "leaves = " << leaves << endl; 

    CBitTree *bt = new CBitTree(leaves); 
 
    CBitVector *bv = new CBitVector(leaves); 
    set_bit(bv->bit_vector, 0, 1);
    set_bit(bv->bit_vector, leaves-1, 1);

    uint64_t leaf_pos; 

    //set up a few bits to be 1.
    for(uint64_t i = 0; i < leaves/8; i++){
      leaf_pos = (uint64_t) ((double long)(leaves-1) * drand48()); 
      bt->set_leaf(leaf_pos); 
      set_bit(bv->bit_vector, leaf_pos, 1); 
      assert(get_bit(bv->bit_vector, leaf_pos) == bt->get_leaf(leaf_pos) );
    }

    // test prev and next operations on a few positions that are assigned 0-bits
    for(int i = 0; i < 1000; i ++){
      leaf_pos = (uint64_t) ((double long)(leaves-1) * drand48()); 
      assert(get_bit(bv->bit_vector, leaf_pos) == bt->get_leaf(leaf_pos) );

      if(get_bit(bv->bit_vector, leaf_pos) == 0){
	//	cout << "compare prev and next operations.... ";
	uint64_t left1;
	for(uint64_t j = leaf_pos; j>=0; j--)
	  if(get_bit(bv->bit_vector, j)==1){
	    left1 = j;
	    break; 
	  }
	uint64_t left2 = bt->prev(leaf_pos); 
	
	uint64_t right1;
	for(uint64_t j = leaf_pos; j<=leaves-1; j++)
	  if(get_bit(bv->bit_vector, j)==1){
	    right1 = j;
	    break; 
	  }
	uint64_t right2 = bt->next(leaf_pos); 
	
	assert(left1 == left2);
	assert(right1 == right2); 
	
	//	cout << " good. " << endl; 
      }//if
      
    }//for
    
    delete bt;
    delete bv; 
  }//while
}


