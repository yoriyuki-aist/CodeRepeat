/*
  Given the Burrows-Wheeler Transform (BWT) of a text, computer the
  Longest Common Prefix (LCP) array of the text.

  This is a component of finding maximum repeats in the text. 
  
  Program by: Bojian Xu, bojianxu@tamu.edu
*/

#ifndef _bwt_2_lcp_h_
#define _bwt_2_lcp_h_

#include "uti.h"

#define LCP_THRESHOLD_8 255
#define LCP_THRESHOLD_16 65535

typedef struct{
  uint64_t pos; //the F list position where the queried character occurs, i.e., F[text_pos] == queried character 
  uint32_t abt_pos;  //the alphabet position of the queried character, i.e., alphabet[abt_pos] == queried character
  uint64_t rank;  //the number of occurrences of the queried character upon position text_pos in the F list, i.e., there are 'rank' copies of the queried character in F[0...text_pos]
}Fitem_t;  


void bwt_2_(char *bwt_file, char *pos_file); 
void bwt_2_text(CWaveletTree<uchar_t> *wt, uint64_t pos, FILE *fp_txt);
void bwt_2_lcp_fast(CWaveletTree<uchar_t> *wt, uint64_t pos, FILE *fp_lcp);
void bwt_2_lcp_slow(CWaveletTree<uchar_t> *wt, uint64_t pos, FILE *fp_lcp);
void F_search(uint64_t *c, uint32_t abt_size, Fitem_t *Fitem);
uint64_t F_search_pos(CWaveletTree<uchar_t> *wt_bwt, uint64_t *c, uchar_t l, uint64_t rank);
int64_t FL(CWaveletTree<uchar_t> *wt, Fitem_t fitem, uchar_t dollar, uint64_t rank_dollar);
void create_c_array(CWaveletTree<uchar_t> *wt, uint64_t *c, uchar_t dollar);
void insert_lcp(uint8_t *lcp, map<uint32_t, uint16_t> &lcp_map_16, map<uint32_t, uint32_t> &lcp_map_32, uint64_t pos, int h);
void q_add_next(uint64_t *c, CWaveletTree<uchar_t> *wt, queue<uint32_t> &q, uint64_t pos, uchar_t dollar, uint64_t dollar_rank);

#endif
