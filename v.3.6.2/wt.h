/*
  An implementation of the "wavelet tree" data structure presented at "High-order
  entropy-compressed text indexes" by Roberto Grossi, Ankur Gupta and
  Jeffrey Scott Vitter, ACM-SIAM Symposium on Discrete Algorithms(SODA)
  2003: 841-850. 
  
  Program by: Bojian Xu, bojianxu@tamu.edu
*/



#ifndef _wt_h_
#define _wt_h_

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include <assert.h>
#include <string.h>

#include "uti.h"
#include "wt_node.h"
#include "huffcode.h"

#define NORMAL 0
#define WBWT 1
#define HUFFMAN 2

#define TXT 0
#define NUM 1

#define DISK 0
#define MEM 1

typedef struct HuTucker_node{
  uint64_t freq; 
  struct HuTucker_node * previous; 
  struct HuTucker_node * next; 
  struct HuTucker_node * parent;
  struct HuTucker_node * left;
  struct HuTucker_node * right;
} HuTucker_node_t;


template<class Text_t>
class CWaveletTree
{
 public: 
  Text_t *alphabet;  //global alphabet, must be in ascending order, not necessary terminated with '\0'
  uint32_t alphabet_size; //global alphabet size
  class CHuffCode<Text_t> *huffcodes; //only used for H~uffman tree

  CWaveletTreeNode<Text_t> *root; //root of the tree
  CWaveletTreeNode<Text_t> *leaves; //pointing to left most leaf in the double linked list of leaves. 
  
  uint64_t total_bits; //total number of bits in the bit arrays. 

  /* used for the wbwt and huffman wt topology construction */
  uint64_t * freqs; //frequency of each symbol in the text. It is used for w.b.w.t. construction. 

  /* used for wbwt topology construction */
  uint32_t * levels; //levels of symbols in the wbwt. 'level' of a symbol is the number archs from the root to the leaf representing the symbol. 

  /* tree type */
  uint8_t tree_type;

 public: 
  CWaveletTree(void *src, uint32_t src_type, uint32_t p_tree_type, uint32_t text_type, uint64_t text_size);
  ~CWaveletTree(); 

  Text_t text_member(uint64_t pos);
  uint64_t rank(uint64_t pos, Text_t c);
  int64_t select(uint64_t rank, Text_t c); 
  void range_2d_4side(uint64_t left, uint64_t right, Text_t min, Text_t max, void (*user)(uint64_t));
  uint64_t count(Text_t c, uint64_t left, uint64_t right); 

  void extract_alphabet(void *src, uint32_t src_type, uint32_t text_type, uint64_t text_size);

  CWaveletTreeNode<Text_t> * build_wt(Text_t *abt, uint32_t abt_size, uint64_t * frequencies);
  CWaveletTreeNode<Text_t> * build_hwt(HuffCode_t code);
  CWaveletTreeNode<Text_t> * build_wbwt();  //build the wbwt. 
  void process_text(void *src, uint32_t src_type, uint32_t text_type, uint64_t text_size);
  void process_char(Text_t c);
  void create_blocks(CWaveletTreeNode<Text_t> *subroot);
  uint64_t subtree_bits(CWaveletTreeNode<Text_t> *subroot); 
  void delete_tree(CWaveletTreeNode<Text_t> * tree_root);

  //Connect leaves together. Only for huffman tree. 
  void  connect_leaves();

  /* member functions for weight balanced wavelet tree */
  void compute_levels(); //get the level of each symbol in the wbwt      
  void HuTucker_tree_destroy(HuTucker_node_t * subroot);
};



/*********************************************************************

           Implementation of the 'CWaveletTree' class 

*********************************************************************/

/*
  Input: disk file handler or text array. 
  Task: create a wavelet tree over the data in the file or in the text string.

  Notes:
   - if src==FILE* and src_type==TXT, then: 
     -- text_size is useless
     -- characters are stored in one line consecutively. 

   - if src==FILE* and src_type==NUM, then:
     -- text_size is useless
     -- each integer occupies one line in the file
  
   - if src==Text_t*, then data are stored in the array src, and text_size is the number of elements in src. 
*/
template<class Text_t>
CWaveletTree<Text_t>::CWaveletTree(void *src, uint32_t src_type, uint32_t p_tree_type, uint32_t text_type, uint64_t text_size)
{
  /* Initialization */ 
  this->alphabet = NULL;  
  this->alphabet_size = 0;
  this->root = NULL; 
  this->leaves = NULL; 
  this->freqs = NULL; 
  this->levels = NULL; 
  this->total_bits = 0; 
  this->huffcodes = NULL;
  this->tree_type = (uint8_t)p_tree_type;

  /* Extract the alphabet from text. The resulting alphabet is in ascending order */
  extract_alphabet(src, src_type, text_type, text_size); //after this step, freqs[], alphabet[], alphabet_size are assigned. 

  //create the topology and the raw bit arrays of the wavelet tree
  if(tree_type==NORMAL)
    root = build_wt(this->alphabet, this->alphabet_size, this->freqs);
  else if(tree_type==WBWT){
    compute_levels();   
    root = build_wbwt();
  }
  else if(tree_type==HUFFMAN){
    //create the huffman code for each symbol in the alphabet
    huffcodes = new CHuffCode<Text_t>(alphabet, freqs, alphabet_size);
    HuffCode_t code;  code.code = 0; code.length = 0; 
    root = build_hwt(code);
    //connect the leaves together here in lexicographical order !!!
    connect_leaves();
  }
  else{
    cout << "unknown wavelet tree type. exit. " << endl;
    exit(1);
  }
  process_text(src, src_type, text_type, text_size); //process the text and assign the bit arrays 
  create_blocks(this->root);  //create blocks at the tree nodes; 
  
  root->parent = NULL; 
  total_bits = subtree_bits(root); 

  //  delete[] freqs;  //it will be deleted in the deconstructor as it will be used in the pattern matching. 
  //  freqs = NULL; 

  delete[] levels; // it is useless once the tree is constructed.  
  levels = NULL;  
}






/*
Input: 
(1) The data file over which to create the wavelet tree. 
(2) source: FILE or MEM
(3) if src_type = FILE, then source is of FILE* type, text_type = TXT or NUM, text_size is undefined
(4) if src_type = MEM,  then source is of Text_t * type, text_type is undefined, text_size is the string size 


The data type : TXT|NUM; if TXT, all symbols are consecutive; if NUM, each integer occupies one line

Output: 
(1) Extract the alphabet from the input file. 
(2) Save the alphabet in the array 'alphabet'. 
(3) The resulting 'alphabet' is in ascending order
(4) The resulting 'alphabet' is NOT terminated by '\0'.

Notes: 
(1) This function uses STL 'map' container to extract the alphabet and each symbol's frequencies. 

(2) this funciton does not need 'map' if the alphabet size upper bound is known, and thus
    the time performance can be very much speeded up !!!!! USTOMIZE FOR EXPERIMENTS. 
*/

template<class Text_t>
void CWaveletTree<Text_t>::extract_alphabet(void *source, uint32_t src_type, uint32_t text_type, uint64_t text_size)
{
  if(text_type == TXT){
    uint64_t *counters = new uint64_t[256]; 
    for(int i = 0; i < 256; i++)
      counters[i] = 0; 
    
    if(src_type == DISK){
      int d;
      FILE* fp = (FILE*) source; 
      rewind(fp);
      while((d = fgetc(fp))!=EOF)
	  counters[d]++;
    }
    else{ //src == MEM
      Text_t *text = (Text_t *)source;
      for(uint64_t i=0; i<text_size; i++)
	counters[text[i]] ++;
    }
    //compute the alphabet, alphabet_size and frequencies
    alphabet_size = 0; 
    for(int i = 0; i < 256; i++)
      if(counters[i]>0)
	alphabet_size ++; 
    alphabet = new Text_t[alphabet_size];
    freqs = new uint64_t[alphabet_size]; 
    int j = 0; 
    for(int i = 0; i < 256; i++){
      if(counters[i] > 0){
	alphabet[j] = (Text_t)i; 
	freqs[j] = counters[i];
	j++;
      }
    }
    delete[] counters; 
  } //if(text_type == TXT)

  else{  //text_type == NUM
    class map<Text_t, uint64_t> abt;
    class map<Text_t, uint64_t>::iterator iter_abt;
    int d; 
    
    if(src_type == DISK){
      FILE *fp = (FILE *)source; 
      rewind(fp); 
      
      while(fscanf(fp,"%d\n", &d)!=EOF){
	iter_abt = abt.find((Text_t)d);
	if(iter_abt == abt.end()){
	  abt.insert(pair<Text_t, uint64_t>((Text_t)d, 1));
	  assert(abt.size() < abt.max_size());
	}
	else
	  iter_abt->second ++;
      }
    }
    else /*if(src_type == MEM)*/ {
      Text_t *text = (Text_t *)source; 
      for(uint64_t i=0; i<text_size; i++){
	iter_abt = abt.find(text[i]);
	if(iter_abt == abt.end()){
	  abt.insert(pair<Text_t, uint64_t>(text[i], 1));
	  assert(abt.size() < abt.max_size());
	}
	else
	  iter_abt->second ++;
      }
    }
    alphabet_size = abt.size(); 
    alphabet = new Text_t[alphabet_size]; 
    freqs = new uint64_t[alphabet_size]; 
    
    uint32_t i = 0; 
    for(iter_abt = abt.begin(); iter_abt != abt.end(); iter_abt ++){
      alphabet[i] = iter_abt->first;
      freqs[i] = iter_abt->second;
      i++;
    }
  }//else text_type == NUM
}


/*
  create the topology of the huffman shaped wavelet tree and the bit arrays, but the bit arrays are not 
  assigned with meaningful bits. 
*/
template<class Text_t>
CWaveletTreeNode<Text_t> * CWaveletTree<Text_t>::build_hwt(HuffCode_t p_code)
{
  int non_leaf = 0;   //notify whether the node is a leaf node; 
  uint64_t temp;

  //compute the size of the bit vector at the current node
  uint64_t text_size = 0;
  
  if(p_code.length == 0){ // this code assumes the global alphabet size > 1. 
    for(uint32_t i = 0; i < alphabet_size; i++)
      text_size += freqs[i];
    non_leaf = 1; 
  }
  else
    for(uint32_t i = 0; i < alphabet_size; i++){
      class map<Text_t, HuffCode_t *>::iterator it = huffcodes->code_table.find(alphabet[i]);
      if(it != huffcodes->code_table.end()){
	if(it->second->length >= p_code.length  &&  (it->second->code >> (64-p_code.length)) == (p_code.code >> (64-p_code.length)) ){
	  text_size += freqs[i]; 
	  temp = i;
	  if(it->second->length > p_code.length)
	    non_leaf = 1;
	}
      }
    }
  assert(text_size > 0);

  CWaveletTreeNode<Text_t> * node = new CWaveletTreeNode<Text_t>(); 
  node->vector_size = text_size; 
  node->alphabet = alphabet + temp; 

  if(non_leaf){
    /*create the raw bit array without bit assignment. */
    uint64_t vector_array_size = (uint64_t)ceill((long double)text_size/(long double)VECTOR_UNIT_SIZE);
    node->bit_vector = new uint64_t[vector_array_size];
    memset(node->bit_vector, 0, VECTOR_UNIT_SIZE/BYTE_SIZE * vector_array_size);

    //create the left child node
    HuffCode_t code_left; 
    code_left.code = p_code.code;
    code_left.length = p_code.length + 1; 
    node->left = build_hwt(code_left);

    //create the right child node
    HuffCode_t code_right; 
    uint64_t temp = 1; 
    temp <<= (63 - p_code.length); 
    code_right.code = p_code.code | temp;
    code_right.length = p_code.length + 1; 
    node->right = build_hwt(code_right); 

    //let the two child nodes point to the parent node
    node->left->parent = node; 
    node->right->parent = node; 
  }
  return node; 
}


template<class Text_t>
void  CWaveletTree<Text_t>::connect_leaves()
{
  for(uint32_t i = 0; i < alphabet_size; i++){
    CWaveletTreeNode<Text_t> *node = root; 
    class map<Text_t, HuffCode_t *>::iterator it = huffcodes->code_table.find(alphabet[alphabet_size-i-1]);

#ifdef _DEBUG_
    assert(it != huffcodes->code_table.end());
#endif

    //to reach the leaf node
    uint64_t temp = 1;
    temp <<= 63; 
    for(uint8_t j = 0; j < it->second->length; j++){
      if((temp & it->second->code) == 0)
	node = node->left; 
      else
	node = node->right; 
      temp >>= 1; 
    }//for

#ifdef _DEBUG_
    assert(node->left == NULL);
    assert(node->right == NULL);
#endif

    //connect it into the leaves link list. 
    node->previous = NULL;
    node->next = leaves; 
    if(leaves != NULL)
      leaves->previous = node; 
    leaves = node; 
  }//for
}



template<class Text_t>
void  CWaveletTree<Text_t>::HuTucker_tree_destroy(HuTucker_node_t * subroot)
{
  if(subroot->left)
    HuTucker_tree_destroy(subroot->left);
  if(subroot->right)
    HuTucker_tree_destroy(subroot->right); 
  delete subroot; 
}



/*
  create the topology of the tree and the bit arrays, but the bit arrays are not 
  assigned with meaningful bits. 
*/
template<class Text_t>
CWaveletTreeNode<Text_t> * CWaveletTree<Text_t>::build_wt(Text_t *abt, uint32_t abt_size, uint64_t * frequencies)
{
  CWaveletTreeNode<Text_t> * node = new CWaveletTreeNode<Text_t>(); 
  node->alphabet = abt;
  node->alphabet_size = abt_size; 
  node->gate = abt[(uint32_t)floorl((long double)abt_size/2.0)]; 

  uint64_t text_size = 0;
  for(uint32_t i=0; i<abt_size; i++)
    text_size += frequencies[i];
  node->vector_size = text_size;

  if(abt_size > 1){
    /*create the raw bit array without bit assignment. */
    uint64_t vector_array_size = (uint64_t)ceill((long double)text_size/(long double)VECTOR_UNIT_SIZE);
    node->bit_vector = new uint64_t[vector_array_size];
    memset(node->bit_vector, 0, VECTOR_UNIT_SIZE/BYTE_SIZE * vector_array_size);

    /*must create the right subchild first in order to connect all the leaves into a double linked list*/
    node->right = build_wt(abt+(uint32_t)floorl((long double)abt_size/2.0), 
                           abt_size-(uint32_t)floorl((long double)abt_size/2.0),
                           frequencies+(uint32_t)floorl((long double)abt_size/2.0));

    node->left = build_wt(abt, (uint32_t)floorl((long double)abt_size/2.0), frequencies); 

    node->right->parent = node; 
    node->left->parent = node; 
  }

  /*connect leaf node into a double linked list*/
  if(abt_size == 1){
    node->next = leaves;
    node->previous = NULL;
    if(leaves != NULL)
      leaves->previous = node; 
    leaves = node;
  }
  return node; 
}

/*
  Build the topology of the wbwt and the raw bit arrays, 
  using the 'level' of each symbol.
*/
template<class Text_t>
CWaveletTreeNode<Text_t> * CWaveletTree<Text_t>::build_wbwt()
{
  if(alphabet_size == 1){
    CWaveletTreeNode<Text_t> * node = new CWaveletTreeNode<Text_t>();
    node->alphabet = alphabet; 
    node->alphabet_size = alphabet_size;  
    node->gate = alphabet[0]; 
    node->vector_size = freqs[0]; 

    this->leaves = node; 
    return node; 
  }

  CWaveletTreeNode<Text_t> ** leaves = new CWaveletTreeNode<Text_t> *[alphabet_size+2];
  //create the leaves
  for(uint32_t i = 0; i < alphabet_size+2; i++){
    leaves[i] = new CWaveletTreeNode<Text_t>();
    if(i > 0 && i < alphabet_size+1){
      leaves[i]->alphabet = alphabet+i-1; 
      leaves[i]->alphabet_size = 1; 
      leaves[i]->gate = alphabet[i-1];		  
      leaves[i]->level = levels[i-1]; 
      leaves[i]->vector_size = freqs[i-1];
    }
    else 
      leaves[i]->level = 1000000;  //infinite level number. 
  }

  //link the leaves in a double linked list. The first and last leaf are not useful, only to help simplify the operations. 
  for(uint32_t i = 0; i < alphabet_size+2; i++){
    if(i==0){
      leaves[i]->previous = NULL;
      leaves[i]->next = leaves[i+1]; 
    }
    else if(i==alphabet_size+1){
      leaves[i]->previous = leaves[i-1]; 
      leaves[i]->next = NULL;
    }
    else{
      leaves[i]->previous = leaves[i-1]; 
      leaves[i]->next = leaves[i+1]; 
    }
  }

  //create the topology of the wbwt. 
  CWaveletTreeNode<Text_t> *node;
  CWaveletTreeNode<Text_t> *next;
  CWaveletTreeNode<Text_t> *sub_tree; 
  for(uint32_t i = 0; i < alphabet_size-1; i++){
    for(node = leaves[0]; node->level != node->next->level; node = node->next)
      ;
    next = node->next; 
    sub_tree = new CWaveletTreeNode<Text_t>(); 
    
    sub_tree->alphabet = node->alphabet;
    sub_tree->alphabet_size = node->alphabet_size+next->alphabet_size;
    sub_tree->gate = next->alphabet[0];
    sub_tree->left = node; 
    sub_tree->right = next; 
    sub_tree->previous = node->previous; 
    sub_tree->next = next->next; 
    sub_tree->level = node->level-1;
    sub_tree->vector_size = node->vector_size + next->vector_size; 

    uint64_t vector_array_size = (uint64_t)ceill((long double)(sub_tree->vector_size)/(long double)VECTOR_UNIT_SIZE);
    sub_tree->bit_vector = new uint64_t[vector_array_size];
    memset(sub_tree->bit_vector, 0, VECTOR_UNIT_SIZE/BYTE_SIZE * vector_array_size);


    node->previous->next = sub_tree; 
    next->next->previous = sub_tree; 

    node->parent = sub_tree; 
    node->previous = NULL;
    node->next = NULL;
    
    next->parent = sub_tree; 
    next->previous = NULL; 
    next->next = NULL; 
  }
  
  //link the leaves in a double linked list. The first and last leaf are not useful, only to help simplify the operations. 
  for(uint32_t i = 1; i < alphabet_size+1; i++){
    if(i==1){
      leaves[i]->previous = NULL;
      leaves[i]->next = leaves[i+1]; 
    }
    else if(i==alphabet_size){
      leaves[i]->previous = leaves[i-1]; 
      leaves[i]->next = NULL;
    }
    else{
      leaves[i]->previous = leaves[i-1]; 
      leaves[i]->next = leaves[i+1]; 
    }
  }
  this->leaves = leaves[1]; 

  delete leaves[0];
  delete leaves[alphabet_size+1]; 
  delete[] leaves; 
  
  return sub_tree; 
}



/*
  Compute the level of each symbol in the wbwt using the frequency of each symbol in the text. 
*/
template<class Text_t>
void CWaveletTree<Text_t>::compute_levels()
{
  levels = new uint32_t[alphabet_size];  

  if(alphabet_size == 1){
    levels[0] = 0;
    return; 
  }
  if(alphabet_size == 2){
    levels[0] = levels[1] = 1; 
    return; 
  }

  uint64_t head_freq = 1, tail_freq; 
  for(unsigned int i = 0; i < alphabet_size; i++)
    head_freq += freqs[i]; 
  tail_freq = head_freq + 1; 

  HuTucker_node_t ** leaves = new HuTucker_node_t *[alphabet_size + 2]; 
  
  //allocate the space for leaves and init them. 
  for(uint32_t i = 0; i < alphabet_size+2; i ++){
    leaves[i] = new HuTucker_node_t; 
    if(i == 0)
      leaves[i]->freq = head_freq;       
    else if(i == alphabet_size+1)
      leaves[i]->freq = tail_freq; 
    else
      leaves[i]->freq = freqs[i-1]; 

    leaves[i]->parent = NULL;
    leaves[i]->left = NULL;
    leaves[i]->right = NULL; 
    leaves[i]->next = NULL;
    leaves[i]->previous = NULL;
  }
  
  // link the leaves together using a double linked list
  for(uint32_t i = 0; i < alphabet_size+2; i++){
    if(i==0){ 
      leaves[i]->next = leaves[i+1];     
    }
    else if(i==alphabet_size+1){
      leaves[i]->previous = leaves[i-1]; 
    }
    else{
      leaves[i]->previous = leaves[i-1]; 
      leaves[i]->next = leaves[i+1];
    }
  }

  /* second phase of Hu-Tucker algorithm:  Find the level of each symbol  */

  HuTucker_node_t * node; 
  HuTucker_node_t * prev;
  HuTucker_node_t * sub_root;
  for(uint32_t i = 0; i < alphabet_size - 1; i ++){   //iterate "alphabet_size-1" loops. 
    for(node = leaves[alphabet_size+1]; node->freq > node->previous->previous->freq; node = node->previous) //find the two nodes to combine 
      ;
    //combine the two nodes: 'node' and 'prev'.  
    prev = node ->previous;

    sub_root = new HuTucker_node_t; 
    sub_root->freq = node->freq + prev->freq; 
    sub_root->previous = prev->previous; 
    sub_root->next = node->next; 
    sub_root->left = prev; 
    sub_root->right = node; 
    sub_root->parent = NULL;

    prev->previous->next = sub_root; 
    node->next->previous = sub_root; 

    node->parent = sub_root; 

    node->next = NULL; 
    prev->parent = sub_root; 
    prev->previous = NULL;
    prev->next = NULL; 

    //shift the new node to the right place
    while(sub_root->freq > sub_root->next->freq){
      HuTucker_node_t * next = sub_root->next; 
      next->previous = sub_root->previous;
      sub_root->next = next->next; 
      next->next = sub_root; 
      sub_root->previous = next; 
      sub_root->next->previous = sub_root; 
      next->previous->next = next; 
    }
  }//for

  //compute the level of each symbol 
  for(uint32_t i = 0 ; i < alphabet_size; i++){
    levels[i] = 0; 
    for(node = leaves[i+1]; node->parent != NULL; node=node->parent)
      levels[i]++; 
  }

  HuTucker_tree_destroy(sub_root);
  delete leaves[0];
  delete leaves[alphabet_size+1]; 
  delete[] leaves; 
}


template<class Text_t>
void CWaveletTree<Text_t>::process_text(void *src, uint32_t src_type, uint32_t text_type, uint64_t text_size)
{
  int d; 

  if(src_type == DISK){
    FILE * fp = (FILE *)src; 
    rewind(fp); 
    if(text_type == TXT)
      while((d = fgetc(fp)) != EOF)
	process_char((Text_t)d); 
    else //text_type == NUM
      while(fscanf(fp,"%d\n", &d)!=EOF)
	process_char((Text_t)d); 
  }
  else{ //src_type == MEM
    Text_t *text = (Text_t *)src; 
    for(uint64_t i = 0; i < text_size; i ++)
      process_char(text[i]);
  }
}


template<class Text_t>
void CWaveletTree<Text_t>::process_char(Text_t c)
{
  CWaveletTreeNode<Text_t> * node = root; 

  if(tree_type == HUFFMAN){
    class map<Text_t, HuffCode_t *>::iterator it = huffcodes->code_table.find(c);
    uint64_t temp = 1;
    temp <<= 63; 
    for(uint8_t i = 0; i < it->second->length; i++){
      if((temp & it->second->code) == 0){
	set_bit(node->bit_vector, node->cur, 0);
	node->cur ++;
	node = node->left; 
      }
      else{
	set_bit(node->bit_vector, node->cur, 1);
	node->cur ++;
	node = node->right; 
      }
      temp >>= 1;
    }
#ifdef _DEBUG_
    assert(node->left == NULL);
    assert(node->right == NULL);
#endif

  }//if
  else{  //tree_type == NORMAL  or  tree_type == WBWT
    //    while(node->alphabet_size > 1){
    while(node->left != NULL && node->right != NULL){
      if(c < node->gate){
	set_bit(node->bit_vector, node->cur, 0);
	node->cur ++;
	node = node->left;
      }
      else{
	set_bit(node->bit_vector, node->cur, 1);
	node->cur ++;
	node = node->right; 
      }
    }//while
  }//else
}

template<class Text_t>
void CWaveletTree<Text_t>::create_blocks(CWaveletTreeNode<Text_t> *subroot)
{
  //  if(subroot->alphabet_size > 1) {
  if(subroot->left != NULL && subroot->right != NULL){
    subroot->create_blocks(); 
    create_blocks(subroot->left);
    create_blocks(subroot->right); 
  }
  return; 
}




/*
  return the number of bits in the bit arrays at the nodes of the subtree rooted at 'subroot'
*/
template<class Text_t>
uint64_t CWaveletTree<Text_t>::subtree_bits(CWaveletTreeNode<Text_t> *subroot)
{
  if(subroot->left == NULL && subroot->right == NULL)
    return 0;  //not bit array at the leaf nodes. 
  
  else
    return subroot->vector_size + subtree_bits(subroot->left) + subtree_bits(subroot->right);
}



/*Need to delete the whole wavelet tree in the destructor*/
template<class Text_t>
CWaveletTree<Text_t>::~CWaveletTree()
{
  delete[] alphabet; 
  delete[] freqs;  
  delete_tree(root);
  delete huffcodes; 

  //delete huffman code table here if the wavelet tree is huffman shapted. 
}

/*delete the substree rooted at 'tree_root' in the wavelet tree*/
template<class Text_t>
void CWaveletTree<Text_t>::delete_tree(CWaveletTreeNode<Text_t> * subtree_root)
{
  if(subtree_root->left != NULL)
    delete_tree(subtree_root->left);

  if(subtree_root->right != NULL)
    delete_tree(subtree_root->right);

  delete subtree_root; 
}

/* 
   Task: return text[pos]
   
   Warning: 
   1) 'pos' starts from 0, not 1.
   2) The function does not check whether 'pos' is legal. 
   3) The caller should secure 'pos' is legal. 
*/
template<class Text_t>
Text_t CWaveletTree<Text_t>::text_member(uint64_t pos)
{
  CWaveletTreeNode<Text_t> *node = root;
  uint64_t cur_pos = pos; 

  //  while(node->alphabet_size > 1){
  while(node->left != NULL && node->right != NULL){
    if(node->vector_member(cur_pos) == 0){
      cur_pos = node->rank(cur_pos, 0) - 1;   //need to minus one, because parameter 'pos' starts from 0 in the function rank(); 
      node = node->left; 
    }
    else{
      cur_pos = node->rank(cur_pos, 1) - 1;   //need to minus one, because parameter 'pos' starts from 0 in the function rank(); 
      node = node->right;
    }
  }
  return (node->alphabet)[0]; 
}


/*
  Task: return the number character c in text[0...pos]

  Warning: 
  1) 'pos' starts from 0, not 1.
  2) The function does not check whether 'pos' is legal. 
  3) The caller should secure 'pos' is legal.
*/
template<class Text_t>
uint64_t CWaveletTree<Text_t>::rank(uint64_t pos, Text_t c)
{
  CWaveletTreeNode<Text_t> * node = root; 
  uint64_t cur_pos = pos; 

  if(tree_type == HUFFMAN){
    class map<Text_t, HuffCode_t *>::iterator it = huffcodes->code_table.find(c);
    if(it == huffcodes->code_table.end())
      return 0;
    uint64_t code = it->second->code;
    uint8_t length = it->second->length; 
    uint64_t temp = 1; 
    temp <<= 63; 
    for(uint8_t i = 0; i < length; i++){
      if((temp & code) == 0){
	if ( (cur_pos = node->rank(cur_pos, 0)) == 0 ) //character c does not occur in text[0...pos]
	  return 0; 
	cur_pos --; //need to minus one, because parameter 'pos' starts from 0 in the function rank(); 
	node = node -> left; 
      }
      else{
	if( (cur_pos = node->rank(cur_pos, 1)) == 0 )  //character c does not occur in text[0...pos]
	  return 0;
	cur_pos --; //need to minus one, because parameter 'pos' starts from 0 in the function rank();  
	node = node -> right; 
      }

      temp >>= 1;
    }
#ifdef _DEBUG_
    assert(node->left == NULL);
    assert(node->right == NULL);
 #endif

  }
  
  else{ //tree_type == NORMAL or WBWT
    //  while(node->alphabet_size > 1){
    while(node->left != NULL && node->right != NULL){
      if(c < node->gate){
	if ( (cur_pos = node->rank(cur_pos, 0)) == 0 ) //character c does not occur in text[0...pos]
	  return 0; 
	cur_pos --; //need to minus one, because parameter 'pos' starts from 0 in the function rank(); 
	node = node -> left; 
      }
      else{
	if( (cur_pos = node->rank(cur_pos, 1)) == 0 )  //character c does not occur in text[0...pos]
	  return 0;
	cur_pos --; //need to minus one, because parameter 'pos' starts from 0 in the function rank();  
	node = node -> right; 
      }
    }//while
  }
  if((node->alphabet)[0] == c)
    return cur_pos + 1; 
  else   //'c' does not exist in the text. 
    return 0; 
}

  
/*
  Task: (1) return the position of the rank'th c in the text, if it exists.
        (2) or, return -1; 

*/
template<class Text_t>
int64_t CWaveletTree<Text_t>::select(uint64_t rank, Text_t c)
{
  CWaveletTreeNode<Text_t> * node; 

  assert(rank > 0); 

  /* First to reach the leaf representing 'c' */
  if(tree_type == HUFFMAN){
    class map<Text_t, HuffCode_t *>::iterator it = huffcodes->code_table.find(c);
    if(it == huffcodes->code_table.end())  return -1;
    uint64_t code = it->second->code;
    uint8_t length = it->second->length; 
    uint64_t temp = 1; 
    temp <<= 63; 

    node = root; 
    for(uint8_t i = 0; i < length; i++){
      if((temp & code) == 0)
	node = node -> left; 
      else
	node = node -> right; 

      temp >>= 1;
    }
#ifdef _DEBUG_
    assert(node->left == NULL);
    assert(node->right == NULL);
#endif
  }
  else{
    //  for(node = root; node->alphabet_size > 1;){
    for(node = root; node->left != NULL && node->right != NULL;){
      if(c < node->gate)
	node = node -> left; 
      else 
	node = node -> right; 
    }
  }
  
  if((node->alphabet)[0] != c)  //'c' does not exist in the text. 
    return -1;
  
  if(node->vector_size < rank)   //the number 'c' in the text is smaller than 'rank'. 
    return -1; 
  

  /* Search from the leaf back to the root */
  uint64_t rank_cur; 
  for(rank_cur = rank; node->parent != NULL; node = node->parent){
    //if(c < node->parent->gate)
    if(node == node->parent->left)
      rank_cur = node->parent->select(rank_cur, 0) + 1;
    else
      rank_cur = node->parent->select(rank_cur, 1) + 1; 
    
#ifdef _DEBUG_
    assert(rank_cur != -1);
#endif
  }
  return rank_cur - 1; 
}



/*
Task: 
  1) Find all the text positions in [left, right] where the character is in the range [min, max].
  2) Each found text position will be transmitted as a parameter to the function 'user', during the range query.

Warning:
  1) The text positions that are transmitted to 'user' are not necessarility in the ascending order
  2) Text position starts from 0.

Notes:
  1) This function call inside is recursive. All the found position will be back tracked to the root node to find the actual text positions. 
  2) The function does not return the list of positions, because the output size can be huge which can potentially consumes lots of memory. 
  3) Instead, the function transmits every found position in the original text to the function 'user'. It leaves the use to decide what to do 
     with the found positions. 
->4) We can create another version of function that return the list of positions. The function will not be recursive but will be a post-order 
     traversal of the wavelet tree. Merge each the two list of the two child to find a new list and return the new list to the parent. This will
     need a new version of rank/select functions that can support query for a list of positions in a more time efficient manner. 

  5) This is function is not defined for the huffman shaped wavelet tree. 
*/
template<class Text_t>
void CWaveletTree<Text_t>::range_2d_4side(uint64_t left, uint64_t right, Text_t min, Text_t max, void (*user)(uint64_t))
{
  assert(left <= right); 
  assert(min <= max); 
  assert(right < root->vector_size);

  if(tree_type == HUFFMAN){
    cout << "Huffman shaped wavelet tree does not support 2-d 4-sided range query. exit. " << endl; 
    exit(1); 
  }
    
  if(root == NULL)  //tree is empty 
    return; 

  if(min > alphabet[alphabet_size-1] || max < alphabet[0]) 
    return; 

/*  
  if(min < alphabet[0])
    min = alphabet[0];

  if(max > alphabet[alphabet_size-1])
    max = alphabet[alphabet_size-1]; 

  if(right > root->vector_size-1)
    right = root->vector_size-1;
*/

  root->range_2d_4side(left, right, min, max, user); 
}




/*
  Return the number of symbol c in the positions bwteen 'left' and 'right', inclusive. 
  Waning: text position starts from 0. 
*/
template<class Text_t>
uint64_t CWaveletTree<Text_t>::count(Text_t c, uint64_t left, uint64_t right)
{
  assert(left <= right); 
  assert(right < root->vector_size);

  if(root == NULL)  //tree is empty 
    return 0; 

  if(left == right)
    return text_member(left)==c ?  1:0 ; 

  /* case of left < right */
  return text_member(left)==c ? rank(right,c)-rank(left,c)+1 : rank(right,c)-rank(left,c); 
}


#endif

