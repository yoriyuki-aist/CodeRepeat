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
#include <queue>
#include <set>

#include "wt.h"
#include "uti.h"
#include "bwt_2_lcp.h"

using namespace std; 


int main(int argc, char**argv)
{
  if(argc != 3){
    cout << endl << "Usage: " <<  argv[0] << " <bwt>  <pos>" << endl << endl;
    cout << "Notes: 1) \"bwt\" is the file that contains the Burrows-Wheller transform of the text (no special terminate symbol added)";
    cout << endl; 
    cout << "       2) \"pos\" is the file that contain the position of the last character of the original text in the bwt (position starts from 0)";
    cout << endl; 
  }

  bwt_2_(argv[1], argv[2]);

  return 0; 
}

