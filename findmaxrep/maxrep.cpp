#include "maxrep.h"
#include "arg.h"

using namespace std; 


void maxrep(char *bwt_file, char *bwt_pos_file, char *lcp_file, uint32_t ml)
{
  assert(ml > 0); 

  char filename[128];
  FILE *fp_bwt, *fp_bwt_pos, *fp_lcp, *fp_reps; 

  sprintf(filename, "%s.reps", bwt_file);  //filename of the file contatining the SA intervals of the repeats. 


  // binary because EOF gets interpreted on windows in text mode
  if((fp_bwt = fopen(bwt_file, "rb")) == NULL){ cout << "bwt file open fail. exit(1)" << endl; exit(1); }
  if((fp_bwt_pos = fopen(bwt_pos_file, "r")) == NULL){ cout << "bwt pos file open fail. exit(1)" << endl; exit(1); }
  if((fp_lcp = fopen(lcp_file, "r")) == NULL){ cout << "lcp file open fail. exit(1)" << endl; exit(1); }
  if((fp_reps = fopen(filename, "w")) == NULL){ cout << "repeat SA intervals file open fail. exit(1)" << endl; exit(1); }



  /* Get the size of bwt and the bwt_pos */
  unsigned long long int bwt_size, pos;  
  bwt_size = file_size(fp_bwt); 
  if(fscanf(fp_bwt_pos, "%llu\n", &pos)==EOF) {cout << "bwt_pos file read error. exit" << endl; exit(1); }

  /* create a bit vector for the bwt and set up its bits */
  CBitVector *bv_bwt = new CBitVector(bwt_size); 
  set_bv(bv_bwt, fp_bwt); 

  /* create a wavelet tree for the lcp */
  CWaveletTree<uint32_t> *wt_lcp = new CWaveletTree<uint32_t>((void *)fp_lcp, DISK, WBWT, NUM, bwt_size); 

  /* create the binary bit tree for the lcp array. Size is 1 larger than the bwt size */
  CBitTree *bt_lcp = new CBitTree(bwt_size + 1); 

  /* find the max repeats */
  find_maxrep(wt_lcp, bt_lcp, bv_bwt, pos, ml, fp_reps); 
   

  delete bv_bwt; 
  delete wt_lcp;
  delete bt_lcp; 

  fclose(fp_bwt);
  fclose(fp_bwt_pos);
  fclose(fp_lcp); 
  fclose(fp_reps); 

  return; 
}
 

/*
  Find max repeats of size at least ml. Output the SA intervals where the repeats occur into file handler 'fp_reps'. 
*/

void  find_maxrep(CWaveletTree<uint32_t> *wt_lcp, CBitTree * bt_lcp, CBitVector *bv_bwt, uint64_t bwt_pos, uint32_t ml, FILE *fp_reps)
{
  fprintf(fp_reps, "repeats size (>=%u), suffix interval\n", ml); 
  fprintf(fp_reps, "====================================\n"); 

  /* set all the bits that correspond to lcp values less than ml in the binary bit tree to be 1 */
  CWaveletTreeNode<uint32_t> * leaf;

  //turn on all the bits in bt_lcp that represents the lcp values less than ml. 
  for(leaf = wt_lcp->leaves; leaf != NULL && leaf->alphabet[0] < ml; leaf = leaf->next){
    for(uint64_t i = 0; i < leaf->vector_size; i++){
      uint64_t cur_pos = i; 
      for(CWaveletTreeNode<uint32_t> *node = leaf; node->parent != NULL; node = node->parent){
	uint32_t bit = ( (node == node->parent->left)? 0:1 );  
	cur_pos = node->parent->select(cur_pos+1, bit); 
      }
      bt_lcp->set_leaf(cur_pos);  //turn on the bit at the position of 'cur_pos'
    }
  }

  //find all the max repeats with length at least ml. 
  for(; leaf != NULL; leaf = leaf->next){
    for(uint64_t i = 0; i < leaf->vector_size; i++){
      uint64_t cur_pos = i; 
      for(CWaveletTreeNode<uint32_t> *node = leaf; node->parent != NULL; node = node->parent){
	uint32_t bit = ( (node == node->parent->left)? 0:1 );  
	cur_pos = node->parent->select(cur_pos+1, bit); 
      }
      unsigned long long int prev = bt_lcp->prev(cur_pos);
      unsigned long long int  next = bt_lcp->next(cur_pos); 
      if(wt_lcp->text_member(prev) != leaf->alphabet[0]) //it is a max repeat to the right. 
	if(prev<=bwt_pos && next-1>=bwt_pos) //it is a max repeat to the left
	  fprintf(fp_reps, "%u, [%llu, %llu]\n", leaf->alphabet[0], prev, next-1);
		  //	  cout << "Max repeat size: " << leaf->alphabet[0]  << "   Suffix interval: <" << prev << ", " << next-1 << ">" << endl; 
	else{ 
	  uint64_t rank1 = bv_bwt->rank(prev-1,1);
	  uint64_t rank2 = bv_bwt->rank(next-1,1); 
	  if( (rank2-rank1 > 1)  ||  (rank2-rank1 == 1 && bv_bwt->vector_member(prev) == 0) ) //it is a max repeat to the left
	    fprintf(fp_reps, "%u, [%llu, %llu]\n", leaf->alphabet[0], prev, next-1);
		    //	    cout << "Max repeat size: " << leaf->alphabet[0]  << "   Suffix interval: <" << prev << ", " << next-1 << ">" << endl; 
	}
      bt_lcp->set_leaf(cur_pos);  //turn on the bit at the position of 'cur_pos'
    }//for
  }//for

  return;   
}


void set_bv(CBitVector *bv, FILE *fp_bwt)
{
  FILE *fp_bv;
  uint64_t vector_array_size = (uint64_t)ceill((long double)(bv->vector_size)/(long double)VECTOR_UNIT_SIZE);
  uint64_t j;

  if(flag_bwt_bv_file == 1){
    fp_bv = fopen(bwt_bv_file, "r");
    if(fp_bv == NULL){fprintf(stderr, "sample mark bit vector file open for read fails. exit.\n"); exit(1);}
    for(j = 0; fscanf(fp_bv, "%llu\n", (unsigned long long int *)(bv->bit_vector)+j) != EOF; j++) 
      ;
    assert(j == vector_array_size);
  }

  else{
    fp_bv = fopen(bwt_bv_file, "w");
    if(fp_bv == NULL){fprintf(stderr, "bwt bit vector file open for write fails. exit.\n"); exit(1);}

    rewind(fp_bwt); 
    int c1 = EOF;  
    int c2;
    uint64_t i = 0; 
    while((c2=fgetc(fp_bwt))!=EOF) { 
      if(c2 != c1){
	set_bit(bv->bit_vector, i, 1);  //the bits are initialized to be 0's 
	c1 = c2; 
      }
      i++;
    }

    for(j=0; j < vector_array_size; j++)
      fprintf(fp_bv, "%llu\n", ((unsigned long long int *)(bv->bit_vector))[j]); 
  }

  fclose(fp_bv); 
  bv->create_blocks(); 
}
