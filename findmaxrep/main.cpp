#include <stdio.h>
#include <math.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>

#include "bt.h"
#include "bv.h"
#include "maxrep.h"
#include "report.h"
#include "wt_node.h"
#include "wt.h"
#include "uti.h"
#include "bwt_2_lcp.h"
#include "arg.h"
#include "main.h"
#include "huffcode.h"

using namespace std; 

/*
  cout << wt_type << endl; 
  cout << tradeoff << endl; 
  cout << output_txt << endl;
  cout << output_sa << endl; 
  cout << output_pos << endl;
  cout << min_size << endl; 
  cout << max_size << endl; 
  cout << min_occ << endl; 
  cout << max_occ << endl; 
  cout <<  bwt_file << endl; 
  cout <<  bwt_pos_file << endl; 
  cout <<   output_file << endl;
*/

clock_t find_text_start, find_text_end; 
unsigned long long int find_text_time = 0; 

clock_t find_pos_start, find_pos_end; 
unsigned long long int find_pos_time = 0; 

int main(int argc, char **argv)
{
  //  clock_t start, finish;
  //  start = clock();
  time_t start, end; 
  time_t wt_start, wt_end; 
  time_t lcp_start, lcp_end;
  time_t wt_lcp_start, wt_lcp_end; 
  time_t lcp_bit_tree_start, lcp_bit_tree_end; 
  time_t bit_array_bwt_start, bit_array_bwt_end; 
  time_t sample_bit_array_start, sample_bit_array_end; 
  time_t sample_pos_start, sample_pos_end; 
  time_t find_start, find_end; 



  struct timeval start2, end2; 
  struct timeval wt_start2, wt_end2; 
  struct timeval lcp_start2, lcp_end2;
  struct timeval wt_lcp_start2, wt_lcp_end2; 
  struct timeval lcp_bit_tree_start2, lcp_bit_tree_end2; 
  struct timeval bit_array_bwt_start2, bit_array_bwt_end2; 
  struct timeval sample_bit_array_start2, sample_bit_array_end2; 
  struct timeval sample_pos_start2, sample_pos_end2; 
  struct timeval find_start2, find_end2; 

  int junk;

  start = time(NULL);

  gettimeofday(&start2, NULL);

  if(get_args(argc,argv)) {
    usage(stderr, argv[0]);
    exit(1);
  }
  
  FILE *fp_bwt, *fp_bwt_pos, *fp_lcp, *fp_output; 

  /* Init 16-bit lookup table for popcount */
  /*
  for(uint64_t i = 0; i < 65536; i ++){
    uint64_t temp1 = i & 0xff;
    uint64_t temp2 = i & 0xff00;
    popcount_table_16[i] = _soft_popcount_u64_8(temp1) + _soft_popcount_u64_8(temp2);    
  }
  */

  /* Open all relevant files */
  // binary because EOF gets interpreted on windows in text mode
  if((fp_bwt=fopen(bwt_file, "rb"))==NULL){ cout << "bwt file open fails. exit." << endl; exit(1); }
  if((fp_bwt_pos=fopen(bwt_pos_file, "r"))==NULL){ cout << "bwt pos file open fails. exit." << endl; exit(1); }
  if((fp_output=fopen(output_file, "w"))==NULL){ cout << "output file open fails. exit." << endl; exit(1); }
 
  /* Get the bwt size */
  unsigned long long int bwt_size;
  bwt_size = file_size(fp_bwt); 

  /* Set the max repeat size and max repeat occurrences if they are not given by the user */
  if(flag_max_size == 0)
    max_size = bwt_size; 
  if(flag_max_occ == 0)
    max_occ = bwt_size; 

  /* Get the text position in the bwt */
  unsigned long long int bwt_pos; 
  if(fscanf(fp_bwt_pos, "%llu\n", &bwt_pos)==EOF) {cout << "bwt_pos file read error. exit" << endl; exit(1); }
  fclose(fp_bwt_pos); 
  
  /* Create a wavelet tree for the bwt of the text. */
  wt_start = time(NULL);
  gettimeofday(&wt_start2, NULL);

  cout << "creating BWT wavelet tree ... " << endl;
  CWaveletTree<uchar_t> *wt_bwt = NULL; 
  if(output_pos==1 || output_txt==1 || flag_lcp_file == 0)
    wt_bwt = new CWaveletTree<uchar_t>((void*)fp_bwt, DISK, wt_type, TXT, bwt_size);  
  wt_end = time(NULL); 
  gettimeofday(&wt_end2, NULL);

  /* Create the LCP */
  lcp_start = time(NULL);
  gettimeofday(&lcp_start2, NULL);

  cout << "creating LCP ... " << endl;
  if(flag_lcp_file == 0){
    // sprintf(lcp_file, "%s.lcp", bwt_file);
    if((fp_lcp=fopen(lcp_file, "w"))==NULL){ cout << "lcp file open fails. exit." << endl; exit(1); }
    bwt_2_lcp_fast(wt_bwt, bwt_pos, fp_lcp);
    fclose(fp_lcp);
  }
  lcp_end = time(NULL);
  gettimeofday(&lcp_end2, NULL);


  /* delete wt_bwt if the repeated subtexts and their text positions do not need to be reported */
  if(output_pos==0 && output_txt==0){
    delete wt_bwt;
    wt_bwt = NULL;
  }

  /* Create a wavelet tree for the LCP */
  wt_lcp_start = time(NULL);
  gettimeofday(&wt_lcp_start2, NULL);

  cout << "creating LCP wavelet tree ... " << endl;
  if((fp_lcp=fopen(lcp_file, "r"))==NULL){ cout << "lcp file open fails. exit." << endl; exit(1); }
  CWaveletTree<uint32_t> *wt_lcp = new CWaveletTree<uint32_t>((void *)fp_lcp, DISK, wt_type, NUM, bwt_size); 
  //  fclose(fp_lcp);
  wt_lcp_end = time(NULL); 
  gettimeofday(&wt_lcp_end2, NULL);

  /* create a binary bit tree for the lcp array. Size is 1 larger than the bwt size. 
     Mark those where corresponding lcp value is larger than 'min_size' */
  
  lcp_bit_tree_start = time(NULL); 
  gettimeofday(&lcp_bit_tree_start2, NULL);

  cout << "creating binary bit tree for LCP ... " << endl;
  CBitTree *bt_lcp = new CBitTree(bwt_size + 1); 
  rewind(fp_lcp);
  uint32_t d;
  uint32_t i = 0; 
  while(fscanf(fp_lcp, "%u\n", &d)!=EOF){
    if(d < min_size)
      bt_lcp->set_leaf((uint64_t)i);
    i++;
  }
  fclose(fp_lcp);
  lcp_bit_tree_end = time(NULL); 
  gettimeofday(&lcp_bit_tree_end2, NULL);

  /* create a bit vector for the bwt and set up its bits, for verifying max repeat to the left */
  bit_array_bwt_start = time(NULL);
  gettimeofday(&bit_array_bwt_start2, NULL);

  cout << "creating bit vector for the BWT ... " << endl;
  CBitVector *bv_bwt = new CBitVector(bwt_size); 
  set_bv(bv_bwt, fp_bwt); 
  fclose(fp_bwt); 
  bit_array_bwt_end = time(NULL); 
  gettimeofday(&bit_array_bwt_end2, NULL);


  /* create the samapled text positions and its mark bit vector */
  CBitVector *sam_bv = NULL;
  uint32_t *sample = NULL;
  if(output_pos == 1){
    // First pass of the text: create the bit vector indicating which suffixes are sampled 
    sample_bit_array_start = time(NULL); 
    gettimeofday(&sample_bit_array_start2, NULL);

    cout << "creating the bit vector to mark the sampled positions ... " << endl;
    sam_bv = new CBitVector(bwt_size); 
    //sample_bv_fwd(wt_bwt, bwt_pos, sam_bv);
    sample_bv_bwd(wt_bwt, bwt_pos, sam_bv);
    sample_bit_array_end = time(NULL); 
    gettimeofday(&sample_bit_array_end2, NULL);


    // Second pass of the text: record the sampled text positions 
    sample_pos_start = time(NULL); 
    gettimeofday(&sample_pos_start2, NULL);

    cout << "creating the sampled text positions ... " << endl;
    uint64_t sample_size = (uint64_t)ceill((long double)bwt_size/(long double)SAMPLE_INTERVAL_SIZE); 
    sample = new uint32_t[sample_size]; 
    //sample_text_fwd(wt_bwt, bwt_pos, sample, sam_bv); 
    sample_text_bwd(wt_bwt, bwt_pos, sample, sample_size, sam_bv); 
    sample_pos_end = time(NULL); 
    gettimeofday(&sample_pos_end2, NULL);

  }

  /* Find all the repeats and store them in the file pointed by 'fp_output' */
  cout << "Finding maximum repeats ..." << endl;

  find_start = time(NULL); 
  gettimeofday(&find_start2, NULL);

  master(wt_bwt, wt_lcp, bwt_pos, bt_lcp, bv_bwt, sam_bv, sample, fp_output); 
  find_end = time(NULL); 
  gettimeofday(&find_end2, NULL);

  
  delete wt_bwt; 
  delete sam_bv; 
  delete[] sample; 
  delete wt_lcp;
  delete bt_lcp;
  delete bv_bwt;

  fclose(fp_output);

  //  finish = clock();
  //  fprintf(stdout, "\n%.0f sec\n\n", (double)(finish - start) / (double)CLOCKS_PER_SEC);

  end = time(NULL);
  gettimeofday(&end2, NULL);
  
  cout << endl; 
  cout << "BWT wavelet tree construction: " << wt_end - wt_start <<" seconds" << endl; 
  cout << "lcp array construction: " << lcp_end - lcp_start << " seconds" << endl;
  cout << "lcp array wavelet tree construction: " << wt_lcp_end - wt_lcp_start << " seconds" << endl; 
  cout << "lcp binary bit tree construction: " << lcp_bit_tree_end - lcp_bit_tree_start << " seconds" << endl; 
  cout << "BWT bit array construction: " << bit_array_bwt_end - bit_array_bwt_start << " seconds" << endl; 
  cout << "sampled bit array: " << sample_bit_array_end - sample_bit_array_start << " seconds" << endl; 
  cout << "sampled positions array construction: " << sample_pos_end - sample_pos_start << " seconds" << endl; 
  cout << "find repeats time cost: " << find_end - find_start << " seconds" << endl; 
  cout << "--find repeat subtext: " << find_text_time/CLOCKS_PER_SEC << " seconds" << endl; 
  cout << "--find repeat text locations: " << find_pos_time/CLOCKS_PER_SEC << " seconds" << endl; 
  cout << "total time cost: " << end-start << endl;


  long double total_wt = (long double)((wt_end2.tv_sec*1000000 + wt_end2.tv_usec) - (wt_start2.tv_sec*1000000 + wt_start2.tv_usec))/1000000.0;  
  long double total_lcp = (long double)((lcp_end2.tv_sec*1000000 + lcp_end2.tv_usec) - (lcp_start2.tv_sec*1000000 + lcp_start2.tv_usec))/1000000.0; 
  long double total_wt_lcp = (long double)((wt_lcp_end2.tv_sec*1000000 + wt_lcp_end2.tv_usec) - (wt_lcp_start2.tv_sec*1000000 + wt_lcp_start2.tv_usec))/1000000.0; 
  long double total_lcp_bit_tree = (long double)((lcp_bit_tree_end2.tv_sec*1000000 + lcp_bit_tree_end2.tv_usec) - (lcp_bit_tree_start2.tv_sec*1000000 + lcp_bit_tree_start2.tv_usec))/1000000.0;  
  long double total_bit_array_bwt = (long double)((bit_array_bwt_end2.tv_sec*1000000 + bit_array_bwt_end2.tv_usec) - (bit_array_bwt_start2.tv_sec*1000000 + bit_array_bwt_start2.tv_usec))/1000000.0; 
  long double total_sample_bit_array = (long double)((sample_bit_array_end2.tv_sec*1000000 + sample_bit_array_end2.tv_usec) - (sample_bit_array_start2.tv_sec*1000000 + sample_bit_array_start2.tv_usec))/1000000.0; 
  long double total_sample_pos = (long double)((sample_pos_end2.tv_sec*1000000 + sample_pos_end2.tv_usec) - (sample_pos_start2.tv_sec*1000000 + sample_pos_start2.tv_usec))/1000000.0; 
  long double total_find = (long double)((find_end2.tv_sec*1000000 + find_end2.tv_usec) - (find_start2.tv_sec*1000000 + find_start2.tv_usec))/1000000.0; 
  long double total = (long double)((end2.tv_sec*1000000 + end2.tv_usec) - (start2.tv_sec*1000000 + start2.tv_usec))/1000000.0; 


  cout << endl; 
  cout << endl; 
  cout << "******better resolution in time measurement********" << endl <<endl; 
  cout << "BWT wavelet tree construction: " << total_wt << " seconds" << endl; 
  cout << "lcp array construction: " << total_lcp << " seconds" << endl;
  cout << "lcp array wavelet tree construction: " << total_wt_lcp << " seconds" << endl; 
  cout << "lcp binary bit tree construction: " << total_lcp_bit_tree << " seconds" << endl; 
  cout << "BWT bit array construction: " << total_bit_array_bwt << " seconds" << endl; 
  cout << "sampled bit array: " << total_sample_bit_array << " seconds" << endl; 
  cout << "sampled positions array construction: " << total_sample_pos  << " seconds" << endl; 
  cout << "find repeats time cost: " << total_find << " seconds" << endl; 
  cout << "--find repeat subtext: " << (long double)find_text_time/(long double)CLOCKS_PER_SEC << " seconds" << endl; 
  cout << "--find repeat text locations: " << (long double)find_pos_time/(long double)CLOCKS_PER_SEC << " seconds" << endl; 
  cout << "total time cost: " << total << endl;


  return 0; 
}

void master(CWaveletTree<uchar_t> *wt_bwt, CWaveletTree<uint32_t> *wt_lcp, uint64_t bwt_pos,  
            CBitTree *bt_lcp,  CBitVector *bv_bwt, CBitVector *sam_bv, uint32_t *sample, FILE *fp_output)
{
  assert(min_size <= max_size);
  assert(min_size >= 1);
  assert(min_occ <= max_occ);
  assert(min_occ >= 2); 


  /* Pre-compute fequently used data variables in reporting the text position. */
  uchar_t dollar;
  uint64_t rank_dollar;
  uint64_t bwt_size;
  uint32_t abt_size;
  uint64_t *c = NULL;

  if(output_txt==1 || output_pos == 1){
    dollar = wt_bwt->text_member(bwt_pos); 
    rank_dollar = wt_bwt->rank(bwt_pos, dollar); 
    bwt_size = wt_bwt->root->vector_size;
    abt_size = wt_bwt->alphabet_size;
    c = new uint64_t[abt_size];
    create_c_array(wt_bwt, c, dollar); 
  }

  rewind(fp_output); 

  /* set all the bits that correspond to lcp values less than min_size in the binary bit tree to be 1 */
  CWaveletTreeNode<uint32_t> * leaf;

  //turn on all the bits in bt_lcp that represents the lcp values less than min_size. 
  for(leaf = wt_lcp->leaves; leaf != NULL && leaf->alphabet[0] < min_size; leaf = leaf->next)
    ;
    /*
  {
    cout << "Processing LCP value: " << leaf->alphabet[0] << "..." << endl;
    for(uint64_t i = 0; i < leaf->vector_size; i++){
      uint64_t cur_pos = i; 
      for(CWaveletTreeNode<uint32_t> *node = leaf; node->parent != NULL; node = node->parent){
	uint32_t bit = ( (node == node->parent->left)? 0:1 );  
	cur_pos = node->parent->select(cur_pos+1, bit); 
      }
      bt_lcp->set_leaf(cur_pos);  //turn on the bit at the position of 'cur_pos'
    }
  }
    */

  //find all the max repeats with length belonging to [min_size, max_size], and occurrence in [min_occ, max_occ].
  for(; leaf != NULL && (leaf->alphabet[0]) <= max_size; leaf = leaf->next){
    cout << "Processing LCP value: " << leaf->alphabet[0] << "..." << endl;
    for(uint64_t i = 0; i < leaf->vector_size; i++){
      uint64_t cur_pos = i; 
      for(CWaveletTreeNode<uint32_t> *node = leaf; node->parent != NULL; node = node->parent){
	uint32_t bit = ( (node == node->parent->left)? 0:1 );  
	cur_pos = node->parent->select(cur_pos+1, bit); 
      }
      unsigned long long int prev = bt_lcp->prev(cur_pos);
      unsigned long long int next = bt_lcp->next(cur_pos); 
      if(wt_lcp->text_member(prev) != leaf->alphabet[0]){ //it is a max repeat to the right. 
	if( (prev<=bwt_pos && next-1>=bwt_pos)  && (next-prev >= min_occ && next-prev <= max_occ))  //it is a max repeat to the left
	  output(wt_bwt, sam_bv, sample, &fp_output, leaf->alphabet[0], prev, next-1, dollar, rank_dollar, bwt_size, abt_size, c, bwt_pos);
	else{ 
	  uint64_t rank1 = bv_bwt->rank(prev-1,1);  uint64_t rank2 = bv_bwt->rank(next-1,1); 
	  if ( ( (rank2-rank1 > 1)  ||  (rank2-rank1 == 1 && bv_bwt->vector_member(prev) == 0) ) && (next-prev >= min_occ && next-prev <= max_occ) ) //it is a max repeat to the left
	    output(wt_bwt, sam_bv, sample, &fp_output, leaf->alphabet[0], prev, next-1, dollar, rank_dollar, bwt_size, abt_size, c, bwt_pos);
	}
      }
      bt_lcp->set_leaf(cur_pos);  //turn on the bit at the position of 'cur_pos'
    }//for
  }//for

  delete[] c;
}

void output(CWaveletTree<uchar_t> *wt_bwt, CBitVector *sam_bv, uint32_t *sample, 
            FILE **fp_output, uint32_t rep_size, unsigned long long int suf_start, unsigned long long int suf_end,
	    uchar_t dollar, uint64_t rank_dollar, uint64_t bwt_size, uint32_t abt_size, uint64_t *c, uint64_t bwt_pos)
{
  fprintf(*fp_output, "Repeat size: %u\nNumber of occurrences: %llu\n", rep_size, suf_end + 1 - suf_start);

  Fitem_t fitem;

  //print the repeat text
  find_text_start = clock();
  if(output_txt == 1){
    fprintf(*fp_output, "Repeat subtext: ");
    fitem.pos = suf_start; 
    for(uint64_t i = 0; i < rep_size; i++){
      F_search(c, abt_size, &fitem); 
      fputc(wt_bwt->alphabet[fitem.abt_pos], *fp_output); 
      fitem.pos=(uint64_t)FL(wt_bwt, fitem, dollar, rank_dollar); 
    }
    fprintf(*fp_output, "\n");
  }

  fprintf(*fp_output, "Suffix array interval of this repeat: [%llu, %llu]\n", suf_start, suf_end);
  find_text_end = clock();
  find_text_time += find_text_end - find_text_start; 


  /*
  //print text positions, using forward search. Forward search uses select query and is slower. 
    
  if(output_pos == 1){
    fprintf(*fp_output, "Text positions of this repeat: "); 
    for(uint64_t i = suf_start; i <= suf_end; i++){
      fitem.pos = i; 
      for(uint64_t j = 0; j < SAMPLE_INTERVAL_SIZE; j++){
	if(fitem.pos == 0){
	  fprintf(*fp_output, " %u", (uint32_t)(bwt_size - 1 - j));
	  break;
	}
	if(sam_bv->vector_member(fitem.pos) == 1){
	  fprintf(*fp_output, " %u", sample[sam_bv->rank(fitem.pos, 1)-1] - (uint32_t)j);
	  break; 
	}
	F_search(c, abt_size, &fitem); 
	fitem.pos=(uint64_t)FL(wt_bwt, fitem, dollar, rank_dollar); 
      }//for
    }//for
  }//if
  */

  
  //print text positions, using backward search. Backward search uses rank query and is faster. 
  find_pos_start = clock(); 
  if(output_pos == 1){
    fprintf(*fp_output, "Text positions of this repeat: "); 
    for(uint64_t i = suf_start; i <= suf_end; i++){
      fitem.pos = i; 
      for(uint64_t j = 0; j < SAMPLE_INTERVAL_SIZE; j++){
	if(fitem.pos == bwt_pos){
	  fprintf(*fp_output, " %u", (uint32_t)j);
	  break; 
	}
	if(sam_bv->vector_member(fitem.pos) == 1){
	  fprintf(*fp_output, " %u", sample[sam_bv->rank(fitem.pos, 1)-1] + (uint32_t)j);
	  break; 
	}
	uchar_t l = wt_bwt->text_member(fitem.pos);  //the item in the L list at the position of 'fitem.pos'. This is the previous text element. 
	uint64_t rank = wt_bwt->rank(fitem.pos, l);
	if(dollar == l){
	  assert(rank != rank_dollar);
	  rank = rank > rank_dollar ? rank-1 : rank;
	}
	fitem.pos = F_search_pos(wt_bwt, c, l, rank);   //given the character and its rank, return its position in the F list. 
      }//for
    }//for
  }//if
  find_pos_end = clock(); 
  find_pos_time += find_pos_end - find_pos_start; 

  fprintf(*fp_output, "\n\n");
}
