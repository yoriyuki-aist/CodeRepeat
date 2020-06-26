/* Convert a text DNA base (ACGT) sequence file into 2-bit based representation. */

#include <stdio.h>
#include <stdlib.h>

int main()
{
  int d; 
  unsigned char base; 
  unsigned char four_bases = 0;
  unsigned long long int i; 

  for(i = 0; (d=fgetc(stdin)) != EOF; i++){
    base = (unsigned char)d;
    switch (base){
      case 'A': base = 0; break; 
      case 'C': base = 1; break;
      case 'G': base = 2; break;
      case 'T': base = 3; break;
      default: fprintf(stderr, "Not ACGT. EXIT. \n"); exit(1);
    }

    base <<= (6 - (i % 4) * 2); 
    four_bases |= base; 

    if(i%4 == 3){
      if(fwrite((const void*)(&four_bases), sizeof(unsigned char), 1, stdout) != 1){
	fprintf(stderr, "fwrite error. exit. \n");
	exit(1);
      }
      four_bases = 0; 
    }

  }
  
  if(i%4 != 0)
    if(fwrite((const void*)(&four_bases), sizeof(unsigned char), 1, stdout) != 1){
      fprintf(stderr, "fwrite error. exit. \n");
      exit(1);
    }

  return 0;
}


