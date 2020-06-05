#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include <assert.h>
#include <string.h>
#include "uti.h"
#include "report.h"
#include "bwt_2_lcp.h"
#include "arg.h"

/*
  Input: 
  - bwt file
  - bwt position file that stores the position of $ in bwt
  - file that contains the suffix intervals of the repeats

  Output: 
  - file that stores all the text positions of the repeats 
*/
void report(char *bwt_file, char *bwt_pos_file, char *reps_sa_file)
{
  FILE *fp_bwt, *fp_bwt_pos, *fp_sa, *fp_txt_pos;

  char filename[256]; 
  sprintf(filename, "%s.reptext", bwt_file);

  // binary because EOF gets interpreted on windows in text mode
  if((fp_bwt = fopen(bwt_file, "rb")) == NULL){ cout << "bwt file open fail. exit(1)" << endl; exit(1); }
  if((fp_bwt_pos = fopen(bwt_pos_file, "r")) == NULL){ cout << "bwt pos file open fail. exit(1)" << endl; exit(1); }
  if((fp_sa = fopen(reps_sa_file, "r")) == NULL){ cout << "SA interval file open fail. exit(1)" << endl; exit(1); }
  if((fp_txt_pos = fopen(filename, "w")) == NULL){ cout << "Repeats text position file open fail. exit(1)" << endl; exit(1); }

  /* First pass of the text: create the bit vector indicating which suffixes are sampled */
  unsigned long long int bwt_size = file_size(fp_bwt); 
  CBitVector *bv = new CBitVector(bwt_size); 
  unsigned long long int bwt_pos;
  if(fscanf(fp_bwt_pos, "%llu\n", &bwt_pos)==EOF) {cout << "bwt_pos file read error. exit" << endl; exit(1); }
  CWaveletTree<uchar_t> *wt = new CWaveletTree<uchar_t>(fp_bwt, DISK, WBWT, TXT, bwt_size);
  sample_bv_fwd(wt, bwt_pos, bv);

  /* Second pass of the text: record the sampled text positions */
  uint64_t sample_size = (uint64_t)ceill((long double)bwt_size/(long double)SAMPLE_INTERVAL_SIZE); 
  uint32_t * sample = new uint32_t[sample_size]; 
  sample_text_fwd(wt, bwt_pos, sample, sample_size, bv); 

  /* Report the text positions of the suffix array intervals */
  find_text(wt, bwt_pos, sample, bv, fp_sa, fp_txt_pos); 


  delete bv; 
  delete wt; 
  delete sample; 
  fclose(fp_bwt);
  fclose(fp_bwt_pos);
  fclose(fp_sa);
  fclose(fp_txt_pos);

}

/*
  The actual function that finds and stores the text positions of the repeats
*/
void find_text(CWaveletTree<uchar_t> *wt, uint64_t pos, uint32_t *sample, CBitVector *bv, FILE *fp_sa, FILE *fp_txt_pos)
{
  uchar_t dollar = wt->text_member(pos); 
  uint64_t rank_dollar = wt->rank(pos, dollar); 
  uint64_t bwt_size = wt->root->vector_size;
  uint32_t abt_size = wt->alphabet_size;

  uint64_t c[abt_size]; 
  create_c_array(wt, c, dollar); 

  char buf[256]; 

  //skip the first two lines of the sa interval file
  rewind(fp_sa);
  if(fgets(buf, 255, fp_sa) == NULL) {fprintf(stderr, "SA interval file read error.\n"); exit(1);}
  if(fgets(buf, 255, fp_sa) == NULL) {fprintf(stderr, "SA interval file read error.\n"); exit(1);}

  //convert suffix intervals to text positions
  uint32_t rep_size;
  unsigned long long int suf_start, suf_end; 
  rewind(fp_txt_pos); 

  while(fscanf(fp_sa, "%u, [%llu, %llu]\n", &rep_size, &suf_start, &suf_end)!=EOF){
    Fitem_t fitem;
    fprintf(fp_txt_pos, "Repeat size: %u\n", rep_size);
    fprintf(fp_txt_pos, "Repeat subtext: ", rep_size);
    //print the repeat text
    fitem.pos = suf_start; 
    for(uint64_t i = 0; i < rep_size; i++){
      F_search(c, abt_size, &fitem); 
      fputc(wt->alphabet[fitem.abt_pos], fp_txt_pos); 
      fitem.pos=(uint64_t)FL(wt, fitem, dollar, rank_dollar); 
    }
    fprintf(fp_txt_pos, "\n");
    fprintf(fp_txt_pos, "Suffix array interval of this repeat: [%llu, %llu]\n", suf_start, suf_end);
    fprintf(fp_txt_pos, "Text positions of this repeat: "); 
    //print text positions
    for(uint64_t i = suf_start; i <= suf_end; i++){
      fitem.pos = i; 
      for(uint64_t j = 0; j < SAMPLE_INTERVAL_SIZE; j++){
	if(fitem.pos == 0){
	  fprintf(fp_txt_pos, " %u", (uint32_t)(bwt_size - 1 - j));
	  break;
	}
	if(bv->vector_member(fitem.pos) == 1){
	  fprintf(fp_txt_pos, " %u", sample[bv->rank(fitem.pos, 1)-1] - (uint32_t)j);
	  break; 
	}
	F_search(c, abt_size, &fitem); 
	fitem.pos=(uint64_t)FL(wt, fitem, dollar, rank_dollar); 
      }
    }//for
    fprintf(fp_txt_pos, "\n\n");
  
  }//while
}

/* Take a forward pass over the text and mark the sampled text positions */
void sample_bv_fwd(CWaveletTree<uchar_t> *wt_bwt, uint64_t bwt_pos, CBitVector *bv)
{
  FILE *fp_bv;
  uint64_t vector_array_size = (uint64_t)ceill((long double)(bv->vector_size)/(long double)VECTOR_UNIT_SIZE);
  uint64_t j;

  if(flag_sample_bv_file == 1){
    fp_bv = fopen(sample_bv_file, "r");
    if(fp_bv == NULL){fprintf(stderr, "sample mark bit vector file open for read fails. exit.\n"); exit(1);}
    for(j = 0; fscanf(fp_bv, "%llu\n", (unsigned long long int *)(bv->bit_vector)+j) != EOF; j++) 
      ;
    assert(j == vector_array_size);
  }
  else{
    fp_bv = fopen(sample_bv_file, "w");
    if(fp_bv == NULL){fprintf(stderr, "bwt bit vector file open for write fails. exit.\n"); exit(1);}

    uchar_t dollar = wt_bwt->text_member(bwt_pos); 
    uint64_t rank_dollar = wt_bwt->rank(bwt_pos, dollar); 
    uint64_t bwt_size = wt_bwt->root->vector_size;
    uint32_t abt_size = wt_bwt->alphabet_size;
    
    uint64_t c[abt_size]; 
    create_c_array(wt_bwt, c, dollar); 
    
    Fitem_t fitem; 
    fitem.pos = bwt_pos; 
    for(uint64_t i = 0; i < bwt_size-1; i++){
      F_search(c, abt_size, &fitem); 
      if(i % SAMPLE_INTERVAL_SIZE == 0)
	set_bit(bv->bit_vector, fitem.pos, 1);
      
      fitem.pos=(uint64_t)FL(wt_bwt, fitem, dollar, rank_dollar); 
    }
 
    for(j=0; j < vector_array_size; j++)
      fprintf(fp_bv, "%llu\n", ((unsigned long long int *)(bv->bit_vector))[j]); 
  }

  fclose(fp_bv);
  bv->create_blocks(); 
}

/* Take a backward pass over the text and mark the sampled text positions.
   The last text position appears at the first position in the F list, 
   assuming $ is the smallest symbol. 
*/
void sample_bv_bwd(CWaveletTree<uchar_t> *wt_bwt, uint64_t bwt_pos, CBitVector *bv)
{
  FILE *fp_bv;
  uint64_t vector_array_size = (uint64_t)ceill((long double)(bv->vector_size)/(long double)VECTOR_UNIT_SIZE);
  uint64_t j;

  if(flag_sample_bv_file == 1){
    fp_bv = fopen(sample_bv_file, "r");
    if(fp_bv == NULL){fprintf(stderr, "sample mark bit vector file open for read fails. exit.\n"); exit(1);}
    for(j = 0; fscanf(fp_bv, "%llu\n", (unsigned long long int *)(bv->bit_vector)+j) != EOF; j++) 
      ;
    assert(j == vector_array_size);
  }
  else{
    fp_bv = fopen(sample_bv_file, "w");
    if(fp_bv == NULL){fprintf(stderr, "bwt bit vector file open for write fails. exit.\n"); exit(1);}

    uchar_t dollar = wt_bwt->text_member(bwt_pos); 
    uint64_t rank_dollar = wt_bwt->rank(bwt_pos, dollar); 
    uint64_t bwt_size = wt_bwt->root->vector_size;
    uint32_t abt_size = wt_bwt->alphabet_size;
    
    uint64_t c[abt_size]; 
    create_c_array(wt_bwt, c, dollar); 
    
    Fitem_t fitem; 
    fitem.pos = 0; 
    for(uint64_t i = 0; i < bwt_size-1; i++){
      if(i % SAMPLE_INTERVAL_SIZE == 0)
	set_bit(bv->bit_vector, fitem.pos, 1);
      
      uchar_t l = wt_bwt->text_member(fitem.pos);  //the item in the L list at the position of 'fitem.pos'. This is the previous text element. 
      uint64_t rank = wt_bwt->rank(fitem.pos, l);
      if(dollar == l){
	assert(rank != rank_dollar);
	rank = rank > rank_dollar ? rank-1 : rank;
      }
      fitem.pos = F_search_pos(wt_bwt, c, l, rank);   //given the character and its rank, return its position in the F list. 
    }

    for(j=0; j < vector_array_size; j++)
      fprintf(fp_bv, "%llu\n", ((unsigned long long int *)(bv->bit_vector))[j]); 
  }
  
  fclose(fp_bv);
  bv->create_blocks(); 
}


/* Take a forward pass over the text and store and sampled text positions */
void sample_text_fwd(CWaveletTree<uchar_t> *wt_bwt, uint64_t bwt_pos, uint32_t *sample,  uint64_t sample_size, CBitVector *bv)
{
  FILE *fp_pos;
  uint64_t j;

  if(flag_sample_pos_file == 1){
    fp_pos = fopen(sample_pos_file, "r");
    if(fp_pos == NULL){fprintf(stderr, "sampled position  file open for read fails. exit.\n"); exit(1);}
    for(j = 0; fscanf(fp_pos, "%u\n", sample+j) != EOF; j++) 
      ;
    assert(j == sample_size);
  }
  else{
    fp_pos = fopen(sample_pos_file, "w");
    if(fp_pos == NULL){fprintf(stderr, "sampled position file open for write fails. exit.\n"); exit(1);}
    uchar_t dollar = wt_bwt->text_member(bwt_pos); 
    uint64_t rank_dollar = wt_bwt->rank(bwt_pos, dollar); 
    uint64_t bwt_size = wt_bwt->root->vector_size;
    uint32_t abt_size = wt_bwt->alphabet_size;
    
    uint64_t c[abt_size]; 
    create_c_array(wt_bwt, c, dollar); 
    
    Fitem_t fitem; 
    fitem.pos = bwt_pos; 
    for(uint64_t i = 0; i < bwt_size-1; i++){
      F_search(c, abt_size, &fitem); 
      if(i % SAMPLE_INTERVAL_SIZE == 0)
	sample[bv->rank(fitem.pos, 1)-1] = (uint32_t)i; 
      
      fitem.pos=(uint64_t)FL(wt_bwt, fitem, dollar, rank_dollar); 
    }
    for(j=0; j < sample_size; j++)
      fprintf(fp_pos, "%u\n", sample[j]); 
  }
  
  fclose(fp_pos); 
}




/* Take a backward pass over the text and store and sampled text positions.
   The last text position appears at the first position in the F list, 
   assuming $ is the smallest symbol. 
*/
void sample_text_bwd(CWaveletTree<uchar_t> *wt_bwt, uint64_t bwt_pos, uint32_t *sample,  uint64_t sample_size, CBitVector *bv)
{
  FILE *fp_pos;
  uint64_t j;

  if(flag_sample_pos_file == 1){
    fp_pos = fopen(sample_pos_file, "r");
    if(fp_pos == NULL){fprintf(stderr, "sampled position  file open for read fails. exit.\n"); exit(1);}
    for(j = 0; fscanf(fp_pos, "%u\n", sample+j) != EOF; j++) 
      ;
    assert(j == sample_size);
  }
  else{
    fp_pos = fopen(sample_pos_file, "w");
    if(fp_pos == NULL){fprintf(stderr, "sampled position file open for write fails. exit.\n"); exit(1);}

    uchar_t dollar = wt_bwt->text_member(bwt_pos); 
    uint64_t rank_dollar = wt_bwt->rank(bwt_pos, dollar); 
    uint64_t bwt_size = wt_bwt->root->vector_size;
    uint32_t abt_size = wt_bwt->alphabet_size;
    
    uint64_t c[abt_size]; 
    create_c_array(wt_bwt, c, dollar); 
    
    Fitem_t fitem; 
    fitem.pos = 0; 
    for(uint64_t i = 0; i < bwt_size-1; i++){
      if(i % SAMPLE_INTERVAL_SIZE == 0)
	sample[bv->rank(fitem.pos, 1)-1] = (uint32_t)(bwt_size - i - 1); 
      
      uchar_t l = wt_bwt->text_member(fitem.pos);  //the item in the L list at the position of 'fitem.pos'. This is the previous text element. 
      uint64_t rank = wt_bwt->rank(fitem.pos, l);
      if(dollar == l){
	assert(rank != rank_dollar);
	rank = rank > rank_dollar ? rank-1 : rank;
      }
      fitem.pos = F_search_pos(wt_bwt, c, l, rank);   //given the character and its rank, return its position in the F list. 
    }

    for(j=0; j < sample_size; j++)
      fprintf(fp_pos, "%u\n", sample[j]); 
  }

  fclose(fp_pos); 
}
