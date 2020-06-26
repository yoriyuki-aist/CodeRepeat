#ifndef _huffcode_h_
#define _huffcode_h_

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include <assert.h>
#include <string.h>
#include <map>
#include <iostream>

using namespace std; 

int huffnode_comp(const void *p1, const void *p2);



/* Data stuctures used in CHufCode class */
typedef struct huffcode{
  uint64_t code;     //huffman code. bits are used from left to right
  uint8_t length;   //number of useful bits in the code
} HuffCode_t;


typedef struct huffnode{
  uint64_t freq; 
  uint32_t height; 
  uint32_t id;
  struct huffnode *left; 
  struct huffnode *right; 
}HuffNode_t;


/*****************************************
   Declaration of the CHuffCode class 
*****************************************/

template<class Text_t>
class CHuffCode
{
 public:
  class map<Text_t, HuffCode_t *> code_table;

  CHuffCode(Text_t *abt, uint64_t *freqs, uint32_t abt_size); 
  ~CHuffCode();

  void generate_huffcode(HuffNode_t *root, HuffCode_t code, Text_t *abt);
  void delete_hufftree(HuffNode_t *root);
  void print_code(HuffCode_t code);
  void print_codetable();
};


/* ==================================================
        Implementation of the CHuffCode class 
==================================================== */

/* 
   Input: alphabet, frequecy of each character in the alphabet, and the alphabet size
   Output: create the huffman code for each character in the alphabet. 

   Warning: 
   - the code assumes that the frequency of every character is larger than 0. 
   - the code assumes that all the characters in 'abt' are distinct. 
*/
template<class Text_t>
CHuffCode<Text_t>::CHuffCode(Text_t *abt, uint64_t *freqs, uint32_t abt_size)
{
#ifdef _DEBUG_
  for(int i = 0 ; i < abt_size; i++)
    assert(freqs[i] > 0); 
#endif

  assert(abt_size > 1); 

  HuffNode_t *leaves = new HuffNode_t[abt_size];
  for(uint32_t i = 0; i < abt_size; i++){
    leaves[i].freq = freqs[i];
    leaves[i].height = 0; 
    leaves[i].id = i; 
    leaves[i].left = NULL;
    leaves[i].right = NULL;
  }

  /* construct the huffman tree by combining the nodes in the buffer */
  for(uint32_t i = 0; i < abt_size - 1; i++){
    qsort((void *)(leaves + i), (size_t)(abt_size-i), sizeof(HuffNode_t), huffnode_comp);

    //create two new nodes.
    HuffNode_t *left = new HuffNode_t;
    left->freq = leaves[i].freq;
    left->height = leaves[i].height;
    left->id = leaves[i].id; 
    left->left = leaves[i].left;
    left->right = leaves[i].right; 

    HuffNode_t *right = new HuffNode_t;
    right->freq = leaves[i+1].freq;
    right->height = leaves[i+1].height;
    right->id = leaves[i+1].id; 
    right->left = leaves[i+1].left; 
    right->right = leaves[i+1].right; 

    //combine the two nodes
    leaves[i+1].freq += leaves[i].freq;
    leaves[i+1].height = (leaves[i].height > leaves[i+1].height ? leaves[i].height : leaves[i+1].height) + 1;
           //the id's of the non leaf nodes has no use, so don't need to change 'leaves[i+1].id'. 
    leaves[i+1].left = left; 
    leaves[i+1].right = right; 

    assert(leaves[i+1].height < 64); 
  }

  //  copy the root node;
  HuffNode_t *root = new HuffNode_t; 
  root->freq = leaves[abt_size-1].freq;
  root->height = leaves[abt_size-1].height;
  root->id = leaves[abt_size-1].id; //this is needed in case the alpahbet size is 1
  root->left = leaves[abt_size-1].left; 
  root->right = leaves[abt_size-1].right; 

  delete[] leaves;

  //retrieve and save the code
  HuffCode_t code; 
  code.code = 0; 
  code.length = 0; 
  generate_huffcode(root, code, abt); 

  //delete the huffman tree
  delete_hufftree(root); 


}


template<class Text_t>
CHuffCode<Text_t>::~CHuffCode()
{
  class map<Text_t, HuffCode_t *>::iterator it;

  for(it = code_table.begin(); it != code_table.end(); it++)
    delete it->second;
}


template<class Text_t>
void CHuffCode<Text_t>::print_codetable()
{
  class map<Text_t, HuffCode_t *>::iterator it;
  for(it = code_table.begin(); it!=code_table.end(); it++){
    cout << "Huffman code of '" << it->first << "': ";
    print_code(*(it->second));
    cout << endl; 
  }
}


template<class Text_t>
void CHuffCode<Text_t>::print_code(HuffCode_t p_code)
{
  uint64_t temp = 1;
  temp <<= 63; 

  for(uint8_t i= 0; i < p_code.length; i++){
    int bit = ((temp & p_code.code) == 0 ? 0 : 1);
    cout << bit  << " "; 
    temp >>= 1; 
  }
}



template<class Text_t>
void CHuffCode<Text_t>::generate_huffcode(HuffNode_t *node, HuffCode_t p_code, Text_t *abt)
{
  //leaf node. save the code in the code table.
  if(node->left == NULL && node->right == NULL){
    HuffCode_t *code = new HuffCode_t;
    code->code = p_code.code;
    code->length = p_code.length; 
    code_table.insert(pair<Text_t, HuffCode_t *>(abt[node->id], code));
  
    return; 
  }
  
  //generate the new code for the left child. 
  //The code was initialized as 0, so it does not need to be changed. 
  HuffCode_t code_left; 
  code_left.code = p_code.code;
  code_left.length = p_code.length + 1; 
  generate_huffcode(node->left, code_left, abt);
  
  //generate the new code for the right child
  HuffCode_t code_right; 
  uint64_t temp = 1; 
  temp <<= (63 - p_code.length); 
  code_right.code = p_code.code | temp;
  code_right.length = p_code.length + 1; 
  generate_huffcode(node->right, code_right, abt);
}


template<class Text_t>
void CHuffCode<Text_t>::delete_hufftree(HuffNode_t *root)
{
  if(root->left != NULL)
    delete_hufftree(root->left);
  if(root->right != NULL)
    delete_hufftree(root->right);
  
  delete root; 
}




#endif
