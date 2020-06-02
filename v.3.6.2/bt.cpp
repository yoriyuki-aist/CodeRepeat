#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include <assert.h>
#include <string.h>
#include "bt.h"
#include "uti.h"



/*
  Input: the number of leaves of the tree

  Task: 
  1. create the a complete binary bit tree of size 2*leaves-1
  2. the first and last leaf node and their ancestors are set to be 1, all other nodes are set to be 0. 
*/
CBitTree::CBitTree(uint64_t leaves)
{
  assert(leaves > 0); 

  leaf_size = leaves; 
  tree_size = 2 * leaves - 1; 

  uint64_t temp = (uint64_t)ceill((long double)(tree_size)/(long double)VECTOR_UNIT_SIZE);
  tree = new uint64_t[temp];
  memset(tree, 0, VECTOR_UNIT_SIZE/BYTE_SIZE * temp);

  /* computer the number of leaves at higher level and lower level */

  //compute the number of levels of the tree
  levels = 0;
  temp = tree_size; 
  while(temp > 0){  
    levels ++; 
    temp >>= 1; 
  }

  //compute the number of leaves at the lowest and the second lowest level
  uint64_t size = 1;
  size <<= levels;
  size --; 
  if(size == tree_size){
    upper_leaves = leaf_size; 
    lower_leaves = 0;
  }
  else{ //size > tree_size
    size = 1;
    size <<= (levels-1);
    size --; 
    
    lower_leaves = tree_size - size;
    upper_leaves = leaf_size - lower_leaves; 
  }

  set_leaf(0); 
  set_leaf(leaves-1); 

}

CBitTree::~CBitTree()
{
  delete[] tree; 
}

/* 
Input: The leaf position(from 0 to leaf_size-1, from left to right)
Output: The leaf node position in the tree (from 0 to tree_size-1, from the root to the leaves) 
*/
uint64_t CBitTree::leaf_2_tree(uint64_t leaf_pos)
{
  assert(leaf_pos < leaf_size); 
  
  if(lower_leaves == 0)
    return leaf_size - 1 + leaf_pos; 
  
  if(leaf_pos+1 > lower_leaves)
    return leaf_size - 1 + leaf_pos - lower_leaves; 
  
  //leaf_pos+1 <= lower_leaves
  return leaf_size - 1 + upper_leaves + leaf_pos; 
}

/*
Input: The leaf node position in the tree (from 0 to tree_size-1, from the root to the leaves) 
Output: The leaf position(from 0 to leaf_size-1, from left to right)
*/
uint64_t CBitTree::tree_2_leaf(uint64_t tree_pos)
{
  assert(tree_pos >= leaf_size - 1); 
  assert(tree_pos < tree_size); 

  if(lower_leaves == 0)
    return tree_pos - (leaf_size - 1); 

  uint64_t size = 1;
  size <<= (levels-1);
  size --; 

  if(tree_pos >= size) //tree_pos occurs at the lower leaves
    return tree_pos - (leaf_size - 1) - upper_leaves; 

  //tree_pos occurs at the upper leaves
  return tree_pos - (leaf_size - 1) + lower_leaves; 
}


/* 
   Input: The leaf position(from 0 to leaf_size-1, from left to right)
   Output: The bit of the leaf node at position 'pos'. 
*/
uint32_t CBitTree::get_leaf(uint64_t leaf_pos)
{
  return get_bit(tree, leaf_2_tree(leaf_pos)); 
}

/* 
   Input: The leaf position(from 0 to leaf_size-1, from left to right)
   Output: Set the leaf node at position 'pos' and its all ancestors to be 1. 
*/

void CBitTree::set_leaf(uint64_t leaf_pos)
{
  assert(leaf_pos >= 0); 
  assert(leaf_pos <= leaf_size - 1); 
#ifdef _DEBUG_  
  assert(get_leaf(leaf_pos) == 0);
#endif

  uint64_t tree_pos = leaf_2_tree(leaf_pos);  

  while(get_bit(tree, tree_pos) == 0){
    set_bit(tree, tree_pos, 1); 
    if(IS_ROOT(tree_pos))
      break; 
    tree_pos = PARENT(tree_pos); 
  }
}


/*
  Input: position, 'pos', of a leaf node  which must be 0
  Output: position of a leaf node that is in front of 'pos' and is 1. 

*/
uint64_t CBitTree::prev(uint64_t leaf_pos) 
{
#ifdef _DEBUG_
  assert(get_leaf(leaf_pos) == 0);
#endif
  assert(leaf_pos > 0); 
  assert(leaf_pos < leaf_size - 1); 

  uint64_t tree_pos = leaf_2_tree(leaf_pos);  //cur_pos is regard to the position in the whole bit tree

  //find the lowest ancestor that is 1   
  while(get_bit(tree, tree_pos) == 0){ //the root is initialed to be 1, so this loop will stop before or at the tree root
    if(IS_LEFT_CHILD(tree_pos))
      tree_pos = tree_pos - 1; 
    else
      tree_pos = PARENT(tree_pos); 
  }

  //go down to find the right most leaf that is 1
  while(!IS_LEAF(tree_pos)){
    if(get_bit(tree, RIGHT_CHILD(tree_pos)) == 1)
      tree_pos =  RIGHT_CHILD(tree_pos);
    else
      tree_pos =  LEFT_CHILD(tree_pos);
  }
  return tree_2_leaf(tree_pos); 
}

/*
  Input: position, 'pos', of a leaf node which must be 0
  Output: position of a leaf node that is after 'pos' and is 1. 
 */
uint64_t CBitTree::next(uint64_t leaf_pos)
{
#ifdef _DEBUG_
  assert(get_leaf(leaf_pos) == 0);
#endif
  assert(leaf_pos > 0); 
  assert(leaf_pos < leaf_size - 1); 

  uint64_t tree_pos = leaf_2_tree(leaf_pos);  //cur_pos is regard to the position in the whole bit tree

  //find the lowest ancestor that is 1   
  while(get_bit(tree, tree_pos) == 0){ //the root is initialed to be 1, so this loop will stop before or at the tree root
    if(IS_RIGHT_CHILD(tree_pos))
      tree_pos = tree_pos + 1; 
    else
      tree_pos = PARENT(tree_pos); 
  }

  //go down to find the left most leaf that is 1
  while(!IS_LEAF(tree_pos)){
    if(get_bit(tree, LEFT_CHILD(tree_pos)) == 1)
      tree_pos =  LEFT_CHILD(tree_pos);
    else
      tree_pos =  RIGHT_CHILD(tree_pos);
  }

  return tree_2_leaf(tree_pos); 
}



