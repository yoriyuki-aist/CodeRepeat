/*
  An implementation of the "wavelet tree" data structure presented at "High-order
  entropy-compressed text indexes" by Roberto Grossi, Ankur Gupta and
  Jeffrey Scott Vitter, ACM-SIAM Symposium on Discrete Algorithms(SODA)
  2003: 841-850. 
  
  Program by: Bojian Xu, bojianxu@tamu.edu
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <assert.h>
#include <set>
#include <sys/time.h>
#include <sys/resource.h>
#include <unistd.h>
#include "wt.h"
#include "huffcode.h"

#define MAX_TEXT_SIZE 52428800
#define TEST_CASE_SIZE  50
#define _TEST_

typedef uchar_t T; 
//typedef uint32_t T; 

int src_type=DISK;
//int src_type=MEM;
//int data_type=NUM;   //remember to change typedef T
int data_type=TXT;    //remember to change typedef T


//only for range query query test. Store the returned text positions. 
set<uint64_t> *positions = NULL;

int main(int argc, char **argv)
{
  if(argc != 3){
    cout << "Illegal usage." << endl << "Usage: " << argv[0] << " datafile NORMAL|WBWT|HUFFMAN" << endl; 
    exit(1); 
  }

  int tree_type; //NORMAL or WBWT
  if(strcmp(argv[2], "NORMAL") ==0)
    tree_type = NORMAL; 
  else if(strcmp(argv[2], "WBWT") ==0)
    tree_type = WBWT; 
  else if(strcmp(argv[2], "HUFFMAN") ==0)
    tree_type = HUFFMAN; 
  else{
    cout << "tree type undefined. exit" << endl; 
    return 1; 
  }

  while(1){

  FILE * fp = fopen(argv[1], "r");
  if(!fp){
    cout << "file open fails. exit" << endl;
    return 1; 
  }

  T * text = NULL;  
  uint64_t text_size = 0; 
  text = new T[MAX_TEXT_SIZE+1];  
  
  //read text
  int c;
  switch(data_type){
  case TXT:
    while((c = fgetc(fp)) != EOF){
      text[text_size] = (T)c;
      text_size ++;
    }
    break;
  case NUM:  
    while(fscanf(fp, "%d\n", &c) != EOF){
      text[text_size] = (T)c;
      text_size ++;
    }
    break; 
  defaul:
    cout << "data type undefined. exit" << endl; 
    return 1; 
  }

  cout << "text size = " << text_size << endl; 
  cout << "Text is read." << endl;;
  cout << "Start to creat the wavelet tree for the text ..."  << endl; 
  
  void *data_ptr; 
  
  /* assign the data source */
  switch(src_type){
  case DISK:
    data_ptr = (void *)fp; 
    break;
  case MEM:
    data_ptr = (void *)text; 
    break;
  default:
    cout << "data source undefined. exit. " << endl; 
    return 1; 
  }

  //create the wavelet tree
  CWaveletTree<T> *test_tree =  new CWaveletTree<T>(data_ptr, src_type, tree_type, data_type, text_size); 
  fclose(fp); 

  cout << "Wavelet tree of the text is successfully created. " << endl << endl; 
  cout << "total #bits: " << test_tree->total_bits << endl; 
  cout << "alphabet size: " << test_tree->alphabet_size << endl; 
  cout << "alphabet: "; 

  for(int i=0; i < test_tree->alphabet_size; i++)
    cout << test_tree->alphabet[i]; 
  cout << endl; 


  #ifdef _TEST_

  time_t time_now;             
  unsigned rand_seed = (unsigned)time(&time_now);
  srand48((long int)rand_seed);

  /** experiments with leaf link list**/
  cout << "experiments on leaf link list ..." << endl; 
  int i = 0;
  for(CWaveletTreeNode<T> *leaves = test_tree->leaves; leaves!=NULL; leaves=leaves->next){
    cout << "char: " << leaves->alphabet[0] << "  Local alphabet size: " << leaves->alphabet_size << endl; 
    i ++;
  }
  cout << "abt size : " << i << endl; 
  cout << endl; 

  cout << "Huffman code table: " <<endl;
  //  test_tree->huffcodes->print_codetable();

  /*  
  cout << endl << "frequencies:" << endl; 
  for(uint32_t i = 0; i < test_tree->alphabet_size; i++)
    cout << "freq of " << test_tree->alphabet[i] << ": " << test_tree->freqs[i] << endl; 
  cout << endl; 
  */

  /** experiments with text_member() query **/
  cout << "Do " << TEST_CASE_SIZE << " times of experiments on text member query and compare with naive solutions ..." << endl; 
  for(uint64_t j = 0; j < TEST_CASE_SIZE; j++){
    uint64_t pos = (uint64_t) ((double long)(text_size-1) * drand48()); 
    //cout << text[pos] << " ";
    assert(test_tree->text_member(pos) == text[pos]);
  }
  cout << endl; 
  
  
  /** experiments with text rank() query **/
  cout << "Do " << TEST_CASE_SIZE << " times of experiments on text rank query and compare with naive solutions ..." << endl; 
  for(uint64_t j = 0; j < TEST_CASE_SIZE; j++){
    //randomly pick a character
    T c = text[(uint64_t) ((double long)(text_size-1) * drand48())];
    //randomly pick a position
    uint64_t pos = (uint64_t) ((double long)(text_size-1) * drand48()); 
    //count the number of "c" in text[0...pos]
    uint64_t count = 0;
    for(uint64_t k = 0; k <= pos; k++)
      if(text[k] == c)
	count ++; 
    //cout << "Accurate rank of '" << c << "' is : " << count; 
    assert(test_tree->rank(pos,c) == count); 
    //      cout << "  --------> Test is passed. " << endl; 
  }
  cout << endl; 
  
  
  
  /** experiments with text select() query **/
  cout << "Do " << TEST_CASE_SIZE << " times of experiments on text select query and compare with naive solutions ..." << endl ;
  for(uint64_t j = 0; j < TEST_CASE_SIZE; j++){
    //randomly pick a character
    T c = text[(uint64_t) ((double long)(text_size-1) * drand48())];
    //randomly pick a rank
    uint64_t rank = (uint64_t) ((double long)(text_size-1) / 20.0 * drand48());
    if(rank == 0) 
      rank = 1; 
    
    //count the number of "c" in text[0...pos]
    uint64_t count = 0;
    for(uint64_t k = 0; k < text_size; k++){
      if(text[k] == c)
	count ++; 
      if(count == rank){
	//cout << "Position is: " << k << endl; 
	assert(k == test_tree->select(rank,c));
	break;
      }
    }
    if(count < rank){
      //	cout << "Not enough character '"<< c <<"' in the text " << endl; 
      assert(test_tree->select(rank,c) == -1);
    }
  }
  
  cout << endl; 


  /** experiements with count query **/

  cout << "Do " << TEST_CASE_SIZE << " times of experiments on count query and compare with naive solutions ..." << endl; 
  for(uint64_t j = 0; j < TEST_CASE_SIZE; j++){
    //randomly pick a character
    T c = text[(uint64_t) ((double long)(text_size-1) * drand48())];
    //randomly select the LEFT and RIGHT position boundary. 
    uint64_t L = (uint64_t) ((double long)(text_size-1) * drand48()); 
    uint64_t R = L + (uint64_t) ((double long)(text_size-L) * drand48()); 
    //count the number of "c" in text[L, R]
    uint64_t count = 0;
    for(uint64_t k = L; k <= R; k++)
      if(text[k] == c)
	count ++; 
    //cout << "Accurate rank of '" << c << "' is : " << count; 
    assert(test_tree->count(c, L, R) == count); 
    //      cout << "  --------> Test is passed. " << endl; 
  }
  cout << endl; 

  
  /** experiments with 2-d 4-sided range query  **/
  /*  
  cout << "Do " << TEST_CASE_SIZE << " times of experiments on text 2-d 4-sided range query and compare with naive solutions ..." << endl; 
  for(uint64_t j = 0; j < TEST_CASE_SIZE; j++){
    cout << "experiment " << j << endl; 
    //randomly select the LEFT and RIGHT position boundary. 
    uint64_t L = (uint64_t) ((double long)(text_size-1) * drand48()); 
    uint64_t R = L + (uint64_t) ((double long)(text_size-L) * drand48()); 
    //      cout << "L=" << L << ", R=" << R << endl;
    
    //randomly select the value range [min, max]
    uint32_t temp = (uint32_t)((double long)(test_tree->alphabet_size-1) * drand48());
    T min = (test_tree->alphabet)[temp];
    temp = temp + (uint32_t)((double long)(test_tree->alphabet_size-temp) * drand48());
    T max = (test_tree->alphabet)[temp];
    //      cout << "min=" << min << ", max=" << max << endl; 
    
    //    cout << endl << "Results from naive search:" << endl; 


       
    set<uint64_t> *naive = new set<uint64_t>;
    for(uint64_t i=L; i<=R; i++)
      if(text[i]>=min && text[i]<=max)
	naive->insert(i);
    //	  cout << "text[" << i << "] = " << text[i] << endl; 
    
    //      cout << endl << "Results from range query on wavelet tree:" << endl; 
    positions = new set<uint64_t>;

    cout << "start range searching ..." << endl; 
    
    test_tree->range_2d_4side(L, R, min, max, user_print); 
    
    cout << "range search finishes. " << endl; 

    assert(naive->size() == positions->size());
    cout << "naive size: " << naive->size() << "  range query size: " << positions->size() <<endl;  

    set<uint64_t>::iterator iter_naive = naive->begin();
    set<uint64_t>::iterator iter_pos = positions->begin();

    while(iter_naive != naive->end()){
      assert(*iter_naive == *iter_pos);
      iter_naive ++; 
      iter_pos ++;
    }

    cout << "range search comparison finishes. " << endl; 
    delete naive;
    delete positions; 
    
  }
  */

  cout << endl << endl; 
  
  cout << "Experiments are all good !! " << endl << endl; 
  
#endif
  
  delete[] text; 
  
  cout << "Start to delete the wavelet tree ... " << endl; 
  delete test_tree; 
  cout << "Wavelet tree is successfully deleted. " << endl << endl;
  
  
  
  cout << "Summary: text size = " << text_size << endl << endl; 
  cout << "======================================================================" << endl << endl;
  

}//while

  return 0;
  
}





/** test function **/
void user_print(uint64_t pos)
{
  //return; 
    // extern set<uint64_t> *positions;
  positions->insert(pos); 
}


