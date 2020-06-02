#ifndef _bt_h_
#define _bt_h_

#include "uti.h"

#define IS_ROOT(n) ((n)==0)
#define IS_LEFT_CHILD(n) ((n)-1 == ((n)-1) / 2 * 2)
#define IS_RIGHT_CHILD(n) ((n)-1 == ((n)-1) / 2 * 2 + 1)
#define IS_LEAF(n) (2*(n)+1 >= tree_size)
#define PARENT(n) (((n)-1)/2)
#define LEFT_CHILD(n) ((n)*2+1) 
#define RIGHT_CHILD(n) (((n)+1)*2)



/* 
   A complete binary tree of bits 
   
   Warnings: This binary tree assumes that the first and last leaf node must be set to be 1 when
             the tree is constructed. 
*/


class CBitTree
{
 private:
  uint64_t upper_leaves; //number of leaves at the second lowest level
  uint64_t lower_leaves;  //number of leaves at the lowest level
  uint32_t levels; //the number of nodes in the longest path of the tree 
   
  uint64_t leaf_2_tree(uint64_t leaf_pos);  
  uint64_t tree_2_leaf(uint64_t tree_pos); 

 public:

  uint64_t *tree;  //the complete binary bit tree is implemented as a bit array which is implementated as an array of 64-bit integers. 
  uint64_t tree_size; //number of nodes in the tree. Each node is a bit. 
  uint64_t leaf_size; //number of leaves in the tree

  CBitTree(uint64_t leafsize); 
  ~CBitTree(); 

  uint32_t get_leaf(uint64_t pos); 
  void set_leaf(uint64_t pos);
  
  uint64_t prev(uint64_t pos);
  uint64_t next(uint64_t pos); 

};

#endif


