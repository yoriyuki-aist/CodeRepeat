#ifndef _main_h_
#define _main_h_

static const bool emitUtf8 = emitUTF8;

void master(CWaveletTree<uchar_t> *wt_bwt, CWaveletTree<uint32_t> *wt_lcp, uint64_t bwt_pos,
            CBitTree *bt_lcp, CBitVector *bv_bwt, CBitVector *sam_bv, uint32_t *sample, FILE *fp_output);

void output(CWaveletTree<uchar_t> *wt_bwt, CBitVector *sam_bv, uint32_t *sample, 
            FILE **fp_output, uint32_t rep_size, unsigned long long int suf_start, unsigned long long int suf_end,
	    uchar_t dollar, uint64_t rank_dollar, uint64_t bwt_size, uint32_t abt_size, uint64_t *c, uint64_t bwt_pos);


#endif
