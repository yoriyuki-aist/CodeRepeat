/*
 * bwt.cpp
 *
 * Copyright 2006 Juha K"arkk"ainen <juha.karkkainen@cs.helsinki.fi>
 *
 */


#define DCOVER 7
#define HEADERSIZE 20

#include "bwt.hpp"

#include <algorithm>
#include <iostream>
#include <fstream>
#include <iterator>
#include <streambuf>
#include <vector>
#include <cstdio>
#include <cstdlib>

//start: added by Bojian Xu
#include <time.h>
#include <sys/time.h>

using namespace std; 
//end: added by Bojian Xu


int main(int argc, char* argv[])
{

//start: added by Bojian Xu
  struct timeval start, end; 
  gettimeofday(&start, NULL);
//end: added by Bojian Xu

  // Check Arguments
  if (argc != 2 && argc != 3) {
    std::cerr << "Usage: " << argv[0] << " textfile [bwtfile]" << std::endl;
    std::exit(EXIT_FAILURE);
  }

  int dcover = DCOVER;

  std::string infilename (argv[1]);
  std::string outfilename = infilename;
  outfilename += ".bwt";
  if (argc==3) { outfilename = argv[2]; }

  // Open text file 
  std::FILE *textfile;
  textfile = std::fopen(infilename.c_str(), "r");
  if (!textfile) {
    std::cerr << "ERROR: Unable to open text file " << infilename << std::endl;
    std::exit(EXIT_FAILURE);
  }
  
  // Read text
  std::fseek(textfile, 0, SEEK_END);
  std::size_t length = std::ftell(textfile);
  std::rewind(textfile);
  std::vector<unsigned char> text(length);
  length = std::fread((void *)&text[0], sizeof(char), length, textfile);
  std::fclose(textfile);
  if ( length != text.size() ) {
    std::cerr << "ERROR: wrong number of bytes read" << std::endl;
    exit(EXIT_FAILURE);
  }

#ifdef DEBUG
  std::cout << "Read text of length " << length 
	    << " from " << infilename << "\n"; 
#endif

  size_t eof_pos = text.size() + 1;
  std::ostringstream posstr;
  posstr << eof_pos;
  size_t posstr_size = posstr.str().size();
  //std::cerr << posstr.str() << ' ' << posstr_size << '\n';

  std::ofstream bwtfile(outfilename.c_str());
  bwtfile << "BWT " << posstr.str() << '\n';

  std::streambuf* bwtbuf = bwtfile.rdbuf();
  
  eof_pos = compute_bwt(text.begin(), text.end(), 
			std::ostreambuf_iterator<char>(bwtbuf), dcover);

  /*
  std::copy (text.begin(), text.end(),
	     std::ostreambuf_iterator<char>(bwtbuf) );
  */

  bwtbuf->pubseekpos(4);
  bwtfile << std::setw(posstr_size) << eof_pos;

  //start: added by Bojian Xu
  gettimeofday(&end, NULL);
  cout << "total time: " <<(long double)((end.tv_sec*1000000+end.tv_usec) - (start.tv_sec*1000000+start.tv_usec))/1000000.0 <<endl;
  //end: added by Bojian Xu
}
