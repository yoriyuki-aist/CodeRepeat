/*
  Given the Burrows-Wheeler Transform (BWT) of a text, computer the
  Longest Common Prefix (LCP) array of the text.

  This is a component of finding maximum repeats in the text. 
  
  Program by: Bojian Xu, bojianxu@tamu.edu
*/

#include <stdio.h>
#include <math.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>

#include "wt.h"
#include "uti.h"
#include "bwt_2_lcp.h"

using namespace std; 


/*
  1. bwt size is one character larger the original text. 
  2. $ MUST be replace by a normal character at the position stored in the file 'pos_file' !
  3. positions start from 0.
*/
void bwt_2_(char * bwt_file, char* pos_file)
{
  char filename[64];
  
  FILE *fp_bwt, *fp_pos; 
  if((fp_bwt=fopen(bwt_file, "rb"))==NULL){   // binary because EOF gets interpreted on windows in text mode
    cout << "BWT file open fails. Exit." << endl;
    exit(1); 
  }
  if((fp_pos=fopen(pos_file, "r"))==NULL){
    cout << "POS file open fails. Exit." << endl;
    exit(1); 
  }

  /* Create a weight balanced wavelet tree for the bwt of the text. 
     Note: the papameter 'text_size', which is arbitrarily assigned 0 here, 
           is useless if the data source is a disk file   
  */
  CWaveletTree<uchar_t> *wt = new CWaveletTree<uchar_t>((void*)fp_bwt, DISK, WBWT, TXT, 0);  
  fclose(fp_bwt);

  /* read in the position of $ in the BWT */
  unsigned long long int pos; //The position of $ in the bwt. Position starts from 0. 
  if(fscanf(fp_pos, "%llu\n", &pos)==EOF){ cout << "pos file reading error. exit" << endl; exit(1); }
  fclose(fp_pos); 



  /* compute the original text */
  
  FILE *fp_txt;
  sprintf(filename, "%s.txt", bwt_file); 
  if((fp_txt=fopen(filename, "w"))==NULL){ 
    cout << "txt file open for writing fails. Exit." << endl; 
    exit(1); 
  }
  bwt_2_text(wt, pos, fp_txt); 
  fclose(fp_txt);
  

  /* compute the lcp array and store it in a disk file. */
  FILE *fp_lcp;
  sprintf(filename, "%s.lcp", bwt_file); 
  if((fp_lcp=fopen(filename, "w"))==NULL){ 
    cout << "lcp file open for writing fails. Exit." << endl; 
    exit(1); 
  }
  //bwt_2_lcp_slow(wt, pos, fp_lcp); 
  bwt_2_lcp_fast(wt, pos, fp_lcp); 
  fclose(fp_lcp); 

  /* delete the wavelet tree */
  delete wt;     
}


/*
  Given the wavelet tree over the bwt of the text, recover the text. 
  The $ symbol in the bwt is replaced by a normal symbol. It's position in bwt is stored in 'pos'. 
  the $ symbol is asssumed to be the smallest and appear only once in the bwt. 
*/
void bwt_2_text(CWaveletTree<uchar_t> *wt, uint64_t pos, FILE *fp_txt)
{
  uchar_t dollar = wt->text_member(pos); 
  uint64_t rank_dollar = wt->rank(pos, dollar); 
  uint64_t bwt_size = wt->root->vector_size;
  uint32_t abt_size = wt->alphabet_size; 

  uint64_t c[abt_size]; 
  create_c_array(wt, c, dollar); 

  Fitem_t fitem; 
  fitem.pos = pos; 
  for(uint64_t i = 0; i < bwt_size-1; i++){
    F_search(c, abt_size, &fitem); 
    fputc(wt->alphabet[fitem.abt_pos], fp_txt); 
    fitem.pos=(uint64_t)FL(wt, fitem, dollar, rank_dollar); 
  }
}



/*
  1. Naive computation of the lcp array. 
  2. Array element is computed in the order of lcp[0], lcp[2], ...
  3. The lcp array is directly write into the file pointed by the parameter fp_lcp.
  4. The lcp array size is equal to the bwt size including $
     (so it is one character larger than the text size)
*/
void bwt_2_lcp_slow(CWaveletTree<uchar_t> *wt, uint64_t pos, FILE *fp_lcp)
{
  uchar_t dollar = wt->text_member(pos); 
  uint64_t rank_dollar = wt->rank(pos, dollar); 
  uint64_t bwt_size = wt->root->vector_size;
  uint32_t abt_size = wt->alphabet_size; 
  Fitem_t fitem_down, fitem_up;
  uint64_t c[abt_size]; 
  int64_t ret; 
  int h; 

  create_c_array(wt, c, dollar);

  rewind(fp_lcp); 
  fprintf(fp_lcp, "0\n0\n");  //the first lcp value is not defined. the second lcp value is 0. 

  for(uint64_t i = 2; i < bwt_size; i++){
    h = 0; 
    
    fitem_up.pos = i-1; 
    fitem_down.pos = i; 
    F_search(c, wt->alphabet_size, &fitem_up);
    F_search(c, wt->alphabet_size, &fitem_down);
    
    while(fitem_down.abt_pos == fitem_up.abt_pos){
      h++;

      ret=FL(wt, fitem_up, dollar, rank_dollar);
      assert(ret >= 0); 
      fitem_up.pos = (uint64_t)ret; 
      if(fitem_up.pos == 0)  //reach the end of the text
	break; 
     
      ret=FL(wt, fitem_down, dollar, rank_dollar);
      assert(ret >= 0); 
      fitem_down.pos = (uint64_t)ret; 
      // if(fitem_down.pos == 0)  // won't have the chance to reach the end of the text before 'up' suffix reaches
      //	break; 

      assert(fitem_up.pos != fitem_down.pos);

      F_search(c, wt->alphabet_size, &fitem_up);
      F_search(c, wt->alphabet_size, &fitem_down);
    }//while
    fprintf(fp_lcp, "%d\n", h);  //write the lcp value into the file
  }//for
}

/*
  1. Faster computation of the lcp array. 
  2. Array element is computed in the order of text position
  3. lcp array is stored in an array first, then recorded into a disk file 'fp_lcp'. 
  4. The lcp array size is equal to the bwt size including $
     (so it is one character larger than the text size)
*/
void bwt_2_lcp_fast(CWaveletTree<uchar_t> *wt, uint64_t pos, FILE *fp_lcp)
{
  assert(pos > 0); 

  uchar_t dollar = wt->text_member(pos); 
  uint64_t rank_dollar = wt->rank(pos, dollar); 
  uint64_t bwt_size = wt->root->vector_size;
  uint32_t abt_size = wt->alphabet_size; 
  Fitem_t fitem_up, fitem_down;
  uint64_t new_pos_down; 
  int64_t ret; 
  int h; 
  
  map<uint32_t, uint16_t> map_lcp_16; //store lcp values that are > 254
  map<uint32_t, uint32_t> map_lcp_32;

  uint64_t c[abt_size]; 
  create_c_array(wt, c, dollar);
  
  /* queue 'q' stores the next position in the down suffix to start to compare 
     till the first position where there was no match happened in the last round */
  queue<uint32_t> q;   
  q.push((uint32_t)pos);   //init the queue. 

  /* lcp array  */
  uint8_t * lcp = new uint8_t[bwt_size]; 
  lcp[0] = lcp[1] = 0; //first lcp value is not defined. second lcp value is 0, because the first suffix is $
  for(uint64_t i = 2; i < bwt_size; i++) //init
    lcp[i] = 255;

  while(q.front() > 0){//reach the end of the text; 
    new_pos_down = q.front(); 

    //this lcp value is 0 and has been assinged before the while loop.
    if(new_pos_down == 1){
      q.pop(); 
      if(q.empty()) 
	q_add_next(c, wt, q, new_pos_down, dollar, rank_dollar); 

      continue; 
    }

    /* get the start position of the 'up' and 'down' suffix to compare */
    fitem_down.pos =(uint64_t)(q.back()); //position starts to compare for the 'lower' suffix
    fitem_up.pos = new_pos_down - 1;  //position starts to 'go through' for the 'up' suffix

    //check if bwt's are the same. if yes, quick computation ... 
    //    if(new_pos_down != pos && wt->text_member(new_pos_down)==wt->text_member(new_pos_down-1) && h > 0){
    if(new_pos_down != pos && new_pos_down-1 != pos && wt->text_member(new_pos_down)==wt->text_member(new_pos_down-1) ){
      h--;
      insert_lcp(lcp, map_lcp_16, map_lcp_32, new_pos_down, h);
  
      q.pop();
      if(q.empty())
	q_add_next(c, wt, q, new_pos_down, dollar, rank_dollar); 

      continue; 
    }
    

    /*lcp value is at least this h*/
    q.pop(); 
    h = q.size();

    /*let the fitem_up scan over h characters */
    for(int j=0; j<h; j++){
      F_search(c, wt->alphabet_size, &fitem_up);
      fitem_up.pos = (uint64_t)FL(wt, fitem_up, dollar, rank_dollar); 
    }
    if(fitem_up.pos == 0){
      insert_lcp(lcp, map_lcp_16, map_lcp_32, new_pos_down, h); 
      continue; 
    }

    F_search(c, wt->alphabet_size, &fitem_up);
    F_search(c, wt->alphabet_size, &fitem_down);
    while(fitem_down.abt_pos == fitem_up.abt_pos){
      h++;

      fitem_up.pos = (uint64_t)FL(wt, fitem_up, dollar, rank_dollar);
      fitem_down.pos = (uint64_t)FL(wt, fitem_down, dollar, rank_dollar);
      q.push((uint32_t)(fitem_down.pos)); 

      //note: fitem_down.pos == 0 cannot happen before fitem_up.pos==0 happens
      if(fitem_up.pos == 0)
      	break; 

      assert(fitem_up.pos != fitem_down.pos);

      F_search(c, wt->alphabet_size, &fitem_up);
      F_search(c, wt->alphabet_size, &fitem_down);
    }//while
	
    insert_lcp(lcp, map_lcp_16, map_lcp_32, new_pos_down, h);
    
    if(q.empty())
      q_add_next(c, wt, q, new_pos_down, dollar, rank_dollar); 
    
  } //while
  
  rewind(fp_lcp); 
  for(uint64_t i = 0; i < bwt_size; i++){
    if(lcp[i] < 255) 
      fprintf(fp_lcp, "%d\n", (int)lcp[i]);  
    else{
      map<uint32_t, uint16_t>::iterator it; 
      it = map_lcp_16.find((uint32_t)i);
      if(it != map_lcp_16.end())
	fprintf(fp_lcp, "%d\n", (int)(it->second));
      else
	fprintf(fp_lcp, "%d\n", (int)(map_lcp_32.find((uint32_t)i)->second));
    }
  }

  delete[] lcp;
  
  return; 
}


/*
  C[i] contains the number of symbols that are smaller than or equal to alphabet[i], exclusing $. 
*/
void create_c_array(CWaveletTree<uchar_t> *wt, uint64_t *c, uchar_t dollar)
{
  for(uint32_t i = 0; i < wt->alphabet_size; i++){
    c[i] = (i>0? c[i-1]+wt->freqs[i] : wt->freqs[i]);
    c[i] -= (wt->alphabet[i]==dollar? 1:0); 
  }
}


/*
  Given the position in the F list, find the corresponding position in the L list. 
*/
int64_t FL(CWaveletTree<uchar_t> *wt, Fitem_t fitem, uchar_t dollar, uint64_t rank_dollar)
{
  uint64_t rank;

  if(wt->alphabet[fitem.abt_pos] == dollar && fitem.rank >= rank_dollar) 
    rank = fitem.rank + 1; //take off of the effect of $ being replaced by a normal char. 
  else
    rank = fitem.rank; 

  return wt->select(rank, wt->alphabet[fitem.abt_pos]); 
}

  
/*
Input: 1) c array: c[i] records the total number copies of characters in alphabet[0...i]
       2) abt_size: alphabet size
       3) Fitem->pos, the position in the F list. 

Ouput: assign the valuses to Fitem->abt_pos and Fitem->rank.
*/

void F_search(uint64_t *c, uint32_t abt_size, Fitem_t *fitem)
{
  assert(fitem->pos > 0); 
  assert(fitem->pos - 1 < c[abt_size-1]);   //pos must be smaller than the F list size. Note: pos starts from 0.
  uint64_t pos = fitem->pos - 1; //take off $ at the beginning of F list. 

  /*
  for(int i = 0; i < abt_size; i++){
    if(pos+1 <= c[i]){
      fitem->abt_pos = i; 
      fitem->rank = (i>0? pos+1-c[i-1] : pos+1);
      return; 
    }
  }
  */

  uint32_t low = 0;
  uint32_t high = abt_size - 1; 
  uint32_t mid; 

  while(low <= high){
    mid = low + (high-low)/2;
    if(c[mid] < pos+1){
      if(c[mid+1] >= pos+1){
	fitem->abt_pos = mid + 1;
 	fitem->rank = pos + 1 - c[mid]; 
	return; 
      } 
      else 
	low = mid; 
    } 
    else if(mid == 0){
      fitem->abt_pos = mid; 
      fitem->rank = pos + 1; 
      return;       
    } 
    else if(c[mid-1] < pos+1){ 
      fitem->abt_pos = mid; 
      fitem->rank = pos + 1 -  c[mid-1]; 
      return; 
    } 
    else  
      high = mid; 
  } 
}

/*
  Given the character and its rank, return its position in the F list. 
  This code assumes that 'l' is in the alphabet. 
*/

uint64_t F_search_pos(CWaveletTree<uchar_t> *wt_bwt, uint64_t *c, uchar_t l, uint64_t rank)
{
  uint32_t abt_size = wt_bwt->alphabet_size; 
  uchar_t *abt = wt_bwt->alphabet;   //characters are stord in the alphabet in ascending order. 

  //binary search to find the position of 'l' in the alphabet. 

  uint32_t low = 0;
  uint32_t high = abt_size - 1; 
  uint32_t mid; 
  
  while(low <= high){  
    mid = low + (high-low)/2;
    if(abt[mid] == l)
      return mid == 0 ? rank : c[mid-1] + rank;   //need to take accoun the '$' at the top of F list. 

    if(abt[mid] < l)
      low = mid + 1; 
    else 
      high = mid; 
  }
}



/*
  Insert the lcp value lcp[pos]==h into the lcp array if it is < 255;
  Otherwise, insert <pos, h> into a STL map and insert 255 into lcp[pos].
*/
void insert_lcp(uint8_t *lcp, map<uint32_t, uint16_t> &map_lcp_16, map<uint32_t, uint32_t> &map_lcp_32, uint64_t pos, int h)
{
  if(h < LCP_THRESHOLD_8)
    lcp[pos] = (uint8_t)h; 
  else if (h <= LCP_THRESHOLD_16){
    lcp[pos] = LCP_THRESHOLD_8; 
    map_lcp_16.insert(pair<uint32_t, uint16_t>((uint32_t)pos, (uint16_t)h)); 
  }
  else{
    lcp[pos] = LCP_THRESHOLD_8; 
    map_lcp_32.insert(pair<uint32_t, uint32_t>((uint32_t)pos, (uint32_t)h)); 
  }
}


/* 
   add the F list position of the next text symbol 
   (following the one at the 'pos' postion) into the queue 
*/
void q_add_next(uint64_t *c, CWaveletTree<uchar_t> *wt, queue<uint32_t> &q, uint64_t pos, uchar_t dollar, uint64_t rank_dollar)
{
  assert(pos > 0); 
  Fitem_t fitem;
  fitem.pos = pos;
  F_search(c, wt->alphabet_size, &fitem);
  q.push((uint32_t)FL(wt, fitem, dollar, rank_dollar));
}
