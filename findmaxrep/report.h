#ifndef _report_h_
#define _report_h_

#include "wt.h"
#include "bv.h"
#include "uti.h"

#define SAMPLE_INTERVAL_SIZE 32   //record the text positions at every 32 symbols 

void report(char *bwt_file, char *bwt_pos_file, char *reps_sa_file); 

void sample_bv_fwd(CWaveletTree<uchar_t> *wt, uint64_t bwt_pos, CBitVector *bv); 
void sample_bv_bwd(CWaveletTree<uchar_t> *wt_bwt, uint64_t bwt_pos, CBitVector *bv);

void sample_text_fwd(CWaveletTree<uchar_t> *wt, uint64_t pos, uint32_t *sample,  uint64_t sample_size, CBitVector *bv);
void sample_text_bwd(CWaveletTree<uchar_t> *wt_bwt, uint64_t bwt_pos, uint32_t *sample,  uint64_t sample_size, CBitVector *bv);

void find_text(CWaveletTree<uchar_t> *wt, uint64_t bwt_pos, uint32_t *sample, CBitVector *bv, FILE *fp_sa, FILE *fp_txt_pos);



#endif

