#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <assert.h>
#include <set>
#include <sys/time.h>
#include <sys/resource.h>
#include <unistd.h>
#include <stdint.h>

#include "uti.h"
#include "huffcode.h"

int main()
{

  uchar_t abt[6] = {'a','b','c','d','e','f'};
  uint64_t freq[6] = {6,13,7,3,8,12};  
  uint32_t abt_size = 6; 
  
  CHuffCode<uchar_t> *huff = new CHuffCode<uchar_t>(abt, freq, abt_size);
  delete huff; 



  return 0; 
}

