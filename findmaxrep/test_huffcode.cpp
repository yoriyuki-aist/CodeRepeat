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

#define abt_size 5

int main()
{
  uchar_t abt[abt_size] = {'a','b', 'c', 'd', 'e'};
  uint64_t freq[abt_size] = {1,1,6,6,2};  
  
  CHuffCode<uchar_t> *codes = new CHuffCode<uchar_t>(abt, freq, abt_size);

  for(int i = 0; i < abt_size; i++){
    uint64_t code = codes->code_table.find(abt[i])->second->code;
    uint8_t length = codes->code_table.find(abt[i])->second->length;

    cout << "huffman code of " << abt[i] << ": ";
    for(uint32_t j = 0; j < length; j ++){
      cout << get_bit(&code, (uint64_t)j) << " ";
    }
    cout << endl;
  }

  delete codes; 

  return 0; 
}

