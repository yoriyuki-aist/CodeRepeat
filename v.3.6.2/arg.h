#ifndef _arg_h_
#define _arg_h_

#include <stdio.h>
#include <stdint.h>

extern int wt_type;
extern int flag_wt_type;

extern int tradeoff;
extern int flag_tradeoff;

extern int output_txt; 
extern int flag_output_txt;

extern int output_sa; 
extern int flag_output_sa; 

extern int output_pos; 
extern int flag_output_pos; 

extern uint32_t min_size; 
extern int flag_min_size; 

extern uint32_t max_size;
extern int flag_max_size;

extern unsigned long long int min_occ; 
extern int flag_min_occ; 

extern unsigned long long int max_occ;
extern int flag_max_occ; 

extern char bwt_file[]; 
extern int flag_bwt_file;

extern char bwt_pos_file[];   
extern int flag_bwt_pos_file; 

extern char output_file[]; 
extern int flag_output_file; 

extern char lcp_file[];
extern int flag_lcp_file;      

extern char sample_bv_file[];
extern int flag_sample_bv_file;  //-b

extern char sample_pos_file[];
extern int flag_sample_pos_file; //-B

extern char bwt_bv_file[];
extern int flag_bwt_bv_file; //-B


void usage(FILE *f, char *s);
extern int get_args(int argc, char **argv);


#endif
