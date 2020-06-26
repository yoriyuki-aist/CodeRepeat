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
#include "wavelet_tree.h"
#include "utilities.h"

#define MAX_TEXT_SIZE 1250000
#define TEST_CASE_SIZE  100


//only for range query query test. Store the returned text positions. 
set<uint64_t> *positions = NULL;

int main()
{
  //generate the seed for random numbers
  time_t time_now;             
  unsigned rand_seed = (unsigned)time(&time_now);
  srand48((long int)rand_seed);

  while(1){
    //Text will be deleted inside of the tree construction algorithm. So don't delete it here !!!
    Text_t * text = new Text_t[MAX_TEXT_SIZE+1];  
 
    //copy of the text just for experiments
    Text_t *text_cp = new Text_t[MAX_TEXT_SIZE+1]; 

    //uint64_t text_size = MAX_TEXT_SIZE - 1; 
    uint64_t text_size = (uint64_t) ((double long)MAX_TEXT_SIZE * drand48()); 
    //uint64_t text_size = 1; 
    
    cout << "Generating a random text of size " << text_size << "......" << endl;
    
    uint64_t i; 
    for(i = 0; i < text_size; i++)
      text[i] = (Text_t)(32+ (126-32) * drand48());  //ascii text
      //text[i] = (Text_t)(97+ (123-97)*drand48());   //english text
      //text[i] = (Text_t)(97+ (110-97)*drand48());   //english text with smaller alphabet
      //text[i] = (Text_t)(1+ (MAX_TEXT_SIZE*1000)*drand48()); //integer text
    text[i] = 0;

    memcpy(text_cp, text, text_size * sizeof(Text_t));
    
    cout << "Random text is sucessfully generated."  << endl << endl;;
    cout << "Start to creat the wavelet tree for the text ..."  << endl; 

    //    CWaveletTree *test_tree = new CWaveletTree(text, text_size, NORMAL);  //text will be delete inside of this function call !!!
    CWaveletTree *test_tree = new CWaveletTree(text, text_size, WBWT);  //text will be delete inside of this function call !!!

    cout << "Wavelet tree of the text is successfully created. " << endl << endl; 
    cout << "total #bits: " << test_tree->get_total_bits() << endl; 


#ifdef _TEST_

    /** experiments with text_member() query **/
    cout << "Do " << TEST_CASE_SIZE << " times of experiments on text member query and compare with naive solutions ..." << endl; 
    for(uint64_t j = 0; j < TEST_CASE_SIZE; j++){
      uint64_t pos = (uint64_t) ((double long)(text_size-1) * drand48()); 
      //cout << text_cp[pos] << " ";
      assert(test_tree->text_member(pos) == text_cp[pos]);
    }
    cout << endl; 
    

    /** experiments with text rank() query **/
    cout << "Do " << TEST_CASE_SIZE << " times of experiments on text rank query and compare with naive solutions ..." << endl; 
    for(uint64_t j = 0; j < TEST_CASE_SIZE; j++){
      //randomly pick a character
      Text_t c = text_cp[(uint64_t) ((double long)(text_size-1) * drand48())];
      //randomly pick a position
      uint64_t pos = (uint64_t) ((double long)(text_size-1) * drand48()); 
      //count the number of "c" in text[0...pos]
      uint64_t count = 0;
      for(uint64_t k = 0; k <= pos; k++)
	if(text_cp[k] == c)
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
      Text_t c = text_cp[(uint64_t) ((double long)(text_size-1) * drand48())];
      //randomly pick a rank
      uint64_t rank = (uint64_t) ((double long)(text_size-1) / 20.0 * drand48());
      if(rank == 0) 
	rank = 1; 

      //count the number of "c" in text[0...pos]
      uint64_t count = 0;
      for(uint64_t k = 0; k < text_size; k++){
	if(text_cp[k] == c)
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

    /** experiments with 2-d 4-sided range query  **/

    cout << "Do " << TEST_CASE_SIZE << " times of experiments on text 2-d 4-sided range query and compare with naive solutions ..." << endl; 
    for(uint64_t j = 0; j < TEST_CASE_SIZE; j++){
      //randomly select the LEFT and RIGHT position boundary. 
      uint64_t L = (uint64_t) ((double long)(text_size-1) * drand48()); 
      uint64_t R = L + (uint64_t) ((double long)(text_size-L) * drand48()); 
      //      cout << "L=" << L << ", R=" << R << endl;

      //randomly select the value range [min, max]
      uint32_t temp = (uint32_t)((double long)(test_tree->get_alphabet_size()-1) * drand48());
      Text_t min = test_tree->get_alphabet()[temp];
      temp = temp + (uint32_t)((double long)(test_tree->get_alphabet_size()-temp) * drand48());
      Text_t max = test_tree->get_alphabet()[temp];
      //      cout << "min=" << min << ", max=" << max << endl; 

      //    cout << endl << "Results from naive search:" << endl; 
      set<uint64_t> *naive = new set<uint64_t>;
      for(uint64_t i=L; i<=R; i++)
	if(text_cp[i]>=min && text_cp[i]<=max)
	  naive->insert(i);
      //	  cout << "text[" << i << "] = " << text_cp[i] << endl; 
     
      //      cout << endl << "Results from range query on wavelet tree:" << endl; 
      positions = new set<uint64_t>;
      test_tree->range_2d_4side(L, R, min, max, user_print); 

      assert(naive->size() == positions->size());
      set<uint64_t>::iterator iter_naive = naive->begin();
      set<uint64_t>::iterator iter_pos = positions->begin();
      while(iter_naive != naive->end()){
	assert(*iter_naive == *iter_pos);
	iter_naive ++; 
	iter_pos ++;
      }
      delete naive;
      delete positions; 
    }

    cout << endl << endl; 

    cout << "Experiments are all good !! " << endl << endl; 

#endif


    delete[] text_cp; 

    cout << "Start to delete the wavelet tree ... " << endl; 
    delete test_tree; 
    cout << "Wavelet tree is successfully deleted. " << endl << endl;
    


    cout << "Summary: text size = " << text_size << endl << endl; 
    cout << "======================================================================" << endl << endl;

    //cout << "Press Enter to general a new random text or exit by Ctrl+c" << endl << endl; 
    //getchar();
    return 0;
  }//while

  return 0;
}





/** test function **/
void user_print(uint64_t pos)
{
  //  extern set<uint64_t> *positions;
  positions->insert(pos); 
}


