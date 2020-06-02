#ifndef _maxrep_h_
#define _maxrep_h_

#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include <assert.h>
#include <string.h>
#include <map>
#include <queue>

#include "uti.h"
#include "bt.h"
#include "bv.h"
#include "wt.h"
#include "wt_node.h"
#include "bwt_2_lcp.h" 

void maxrep(char *bwt, char *bwt_pos, char *lcp, uint32_t ml);
void set_bv(CBitVector *bv, FILE *fp);
void find_maxrep(CWaveletTree<uint32_t> *wt, CBitTree * bt, CBitVector *bv, uint64_t bwt_pos, uint32_t ml, FILE *fp_reps);

#endif
