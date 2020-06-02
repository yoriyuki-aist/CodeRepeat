#include <stdio.h>
#include <math.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

#include "arg.h"


int wt_type = 1;               //-T
int flag_wt_type = 0;

int tradeoff = 0;                 //-s
int flag_tradeoff = 0;

int output_txt = 1;               //-t
int flag_output_txt = 0;

int output_pos = 1;               //-p
int flag_output_pos = 0;

uint32_t min_size = 1;                 //-m
int flag_min_size = 0;

uint32_t max_size = 0;                 //-M   need to assign the bwt file size as the default value if it is not given !!!
int flag_max_size = 0;

unsigned long long int min_occ = 2;                  //-r
int flag_min_occ = 0;

unsigned long long int max_occ = 0;                  //-R   need to assign the bwt file size as the default value if it is not given !!!
int flag_max_occ = 0;

char bwt_file[256];               //-i
int flag_bwt_file = 0; 

char bwt_pos_file[256];           //-P
int flag_bwt_pos_file = 0;

char output_file[256];            //-o  
int flag_output_file = 0;

char lcp_file[256];
int flag_lcp_file = 0;        //-L

char sample_bv_file[256];
int flag_sample_bv_file = 0;  //-b

char sample_pos_file[256];
int flag_sample_pos_file = 0; //-B

char bwt_bv_file[256];
int flag_bwt_bv_file = 0; //-B

void usage(FILE *f, char *s) 
{
  fprintf(f,
	  "\nUsage:\t%s  -i <BWTfile> -P <TextPositionFile>\n"
	  "                     [-L <LCPfile>] [-o <OutputFile>]\n"
          "                     [-T <0|1|2>] [-s <0|1>] [-t <0|1>] [-p <0|1>]\n"
          "                     [-m <size>] [-M <size>] [-r <number>] [-R <number>]\n"
          "                     [-b <sampled bit vector file>] [-B <sampled positions file>] \n"
	  "                     [-w <BWT bit vector file>]\n"
	  "                     [-h]\n\n"
	  "\t-i\tThe input bwt file name.\n"
          "\t-P\tThe file that stores the position of the original text in the bwt.\n"
          "\t-L\tThe file that contains the LCP array of the text.\n"
          "\t-o\tOutput file name. '$(BWTfilename).output' by default\n"
          "\t-T\tWavlettree type. 0: normal wavelet tree; 1: weight balanced wavelet tree(default); 2: Huffman shaped wavelet tree.\n"
          "\t-s\t(not available so far)Time and space tradeoff. \n"
          "\t\t0: larger space, faster processing(default); 1: smaller space, slower processing.\n"
          "\t-t\tOutput the repeats in the output; 0: no; 1: yes(default).\n"
          "\t-p\tOutput the text positions where the repeats occur. 0: no; 1: yes(default).\n"
          "\t\tWarning: this operation is very slow and yields a large amount of data\n"
          "\t-m\tminimum repeat size, must be larger than 0. The default value is 1.\n"
          "\t-M\tmaximum repeat size, must be no smaller than minimum repeat size. The default value if infinite.\n"
          "\t-r\tminimum number of occurances of the repeats, must be larer than 1. The default value is 2.\n"
          "\t-R\tmaximum number of occurances of the repeats, must be no smaller than the minimum number of occurances.\n"
          "\t\tThe default value is infinite.\n"
          "\t-b\tThe file that contains the mark bit vector for the sampled positions.\n"
	  "\t\tThis file will be created by the program if it is not provided.\n"
	  "\t-B\tThe file that contains the sampled text positions.\n"
	  "\t\tThis file will be created by the program if it is not provided.\n"
	  "\t-w\tBit vector for the BWT.\n"
	  "\t\tThis file will be created by the program if it is not provided.\n"
	  "\t-h\tThis help message.\n\n"
	  , s);
}


int get_args(int argc, char **argv)
{
  char buf[16];
  int i; 

  if(argc < 5)
    return 1;

  while (argc > 1) {
    if(argv[1][0] != '-' || strlen(argv[1]) != 2)
      return 1; 
    if(argv[1][1] != 'h' && argc < 3)
      return 1;
    if(argv[1][1] == 'h')
      return 1; 

    switch (argv[1][1]) {
    case 'i':
      if(flag_bwt_file == 1){
	fprintf(stderr, "Error: parameter duplicates.\n");
	return 1;
      }
      flag_bwt_file = 1;
      strcpy(bwt_file, argv[2]);
      break;

    case 'P':
      if(flag_bwt_pos_file == 1){
	fprintf(stderr, "Error: parameter duplicates.\n");
	return 1;
      }
      flag_bwt_pos_file = 1;
      strcpy(bwt_pos_file, argv[2]);
      break;

    case 'L':
      if(flag_lcp_file == 1){
	fprintf(stderr, "Error: parameter duplicates.\n");
	return 1;
      }
      flag_lcp_file = 1;
      strcpy(lcp_file, argv[2]);
      break;

    case 'o':
      if(flag_output_file == 1){
	fprintf(stderr, "Error: parameter duplicates.\n");
	return 1;
      }
      flag_output_file = 1;
      strcpy(output_file, argv[2]);
      break;

    case 'T':
      if(flag_wt_type == 1){
	fprintf(stderr, "Error: parameter duplicates.\n");
	return 1;
      }
      flag_wt_type = 1;
      if(strcmp(argv[2],"0")!=0 && strcmp(argv[2],"1") != 0 && strcmp(argv[2],"2") != 0) return 1; 
      wt_type = atoi(argv[2]);
      break;

    case 's':
      if(flag_tradeoff == 1){
	fprintf(stderr, "Error: parameter duplicates.\n");
	return 1;
      }
      flag_tradeoff = 1;
      if(strcmp(argv[2],"0")!=0 && strcmp(argv[2],"1") != 0) return 1; 
      tradeoff = atoi(argv[2]);
      break;

    case 't':
      if(flag_output_txt == 1){
	fprintf(stderr, "Error: parameter duplicates.\n");
	return 1;
      }
      flag_output_txt = 1;
      if(strcmp(argv[2],"0")!=0 && strcmp(argv[2],"1") != 0) return 1; 
      output_txt = atoi(argv[2]);
      break;

    case 'p':
      if(flag_output_pos == 1){
	fprintf(stderr, "Error: parameter duplicates.\n");
	return 1;
      }
      flag_output_pos = 1;
      if(strcmp(argv[2],"0")!=0 && strcmp(argv[2],"1") != 0) return 1; 
      output_pos = atoi(argv[2]);
      break;

    case 'm':
      if(flag_min_size == 1){
	fprintf(stderr, "Error: parameter duplicates.\n");
	return 1;
      }
      flag_min_size = 1;
      min_size = atoi(argv[2]);
      sprintf(buf,"%u", min_size);
      for(i=0; i<strlen(argv[2]); i++)
	if(argv[2][i] !='0')
	  break;
      if(strcmp(buf, argv[2]+i)!=0) return 1;
      break;

    case 'M':
      if(flag_max_size == 1){
	fprintf(stderr, "Error: parameter duplicates.\n");
	return 1;
      }
      flag_max_size = 1;
      max_size = atoi(argv[2]);
      sprintf(buf,"%u", max_size);
      for(i=0; i<strlen(argv[2]); i++)
	if(argv[2][i] !='0')
	  break;
      if(strcmp(buf, argv[2]+i)!=0) return 1;
      break;

    case 'r':
      if(flag_min_occ == 1){
	fprintf(stderr, "Error: parameter duplicates.\n");
	return 1;
      }
      flag_min_occ = 1;
      min_occ = atoi(argv[2]);
      sprintf(buf,"%llu", min_occ);
      for(i=0; i<strlen(argv[2]); i++)
	if(argv[2][i] !='0')
	  break;
      if(strcmp(buf, argv[2]+i)!=0) return 1;
      break;

    case 'R':
      if(flag_max_occ == 1){
	fprintf(stderr, "Error: parameter duplicates.\n");
	return 1;
      }
      flag_max_occ = 1;
      max_occ = atoi(argv[2]);
      sprintf(buf,"%llu", max_occ);
      for(i=0; i<strlen(argv[2]); i++)
	if(argv[2][i] !='0')
	  break;
      if(strcmp(buf, argv[2]+i)!=0) return 1;
      break;

    case 'b':
      if(flag_sample_bv_file == 1){
	fprintf(stderr, "Error: parameter duplicates.\n");
	return 1;
      }
      flag_sample_bv_file = 1;
      strcpy(sample_bv_file, argv[2]);
      break;

    case 'B':
      if(flag_sample_pos_file == 1){
	fprintf(stderr, "Error: parameter duplicates.\n");
	return 1;
      }
      flag_sample_pos_file = 1;
      strcpy(sample_pos_file, argv[2]);
      break;

    case 'w':
      if(flag_bwt_bv_file == 1){
	fprintf(stderr, "Error: parameter duplicates.\n");
	return 1;
      }
      flag_bwt_bv_file = 1;
      strcpy(bwt_bv_file, argv[2]);
      break;

    default:
	fprintf(stderr, "Error: unknown parameters.\n");
	return 1;
    }//switch
    argc -= 2;
    argv += 2;
  }//while

  int flag = 0;
  if(flag_bwt_file == 0){
    fprintf(stderr, "Error: BWT file is not given.\n");
    flag = 1; 
  }

  if(flag_bwt_pos_file == 0){
    fprintf(stderr, "Error: Text position file is not given.\n");
    flag = 1;
  }

  if(min_size < 1){
    fprintf(stderr, "Error: minimum repeat size must be > 0.\n");
    flag = 1;
  }

  if(max_size < min_size && flag_max_size == 1){
    fprintf(stderr, "Error: maximum repeat size must be no smaller than minimum repeat size.\n");
    flag = 1;
  }

  if(min_occ < 2){
    fprintf(stderr, "Error: minimum repeat occurance must be larger than 1.\n");
    flag = 1;
  }

  if(max_occ < min_occ && flag_max_occ == 1){
    fprintf(stderr, "Error: maximum repeat occurance must be no smaller than minimum repeat occurance.\n");
    flag = 1;
  }
  
  if(flag_output_file == 0)
    sprintf(output_file, "%s.output", bwt_file);

  if(flag_lcp_file == 0)
    sprintf(lcp_file, "%s.lcp", bwt_file);

  if(flag_sample_bv_file == 0)
    sprintf(sample_bv_file, "%s.sm_bv", bwt_file);

  if(flag_sample_pos_file == 0)
    sprintf(sample_pos_file, "%s.sm_pos", bwt_file);

  if(flag_bwt_bv_file == 0)
    sprintf(bwt_bv_file, "%s.bwt_bv", bwt_file);


  if(flag == 1)
    return 1;

  return 0;
}

