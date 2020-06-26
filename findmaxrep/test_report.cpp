#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include <assert.h>
#include <string.h>
#include "uti.h"
#include "wt_node.h"
#include "report.h"

int main(int argc, char **argv)
{
  if(argc != 4){
    fprintf(stderr, "Usage: %s bwt_file  bwt_pos_file repeat_suffix_interval_file\n", argv[0]); 
    exit(1); 
  }

  report(argv[1], argv[2], argv[3]); 

  return 0; 
}
