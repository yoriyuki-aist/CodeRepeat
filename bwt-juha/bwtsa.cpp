/*
 * bwt.cpp
 *
 * Copyright 2006 Juha K"arkk"ainen <juha.karkkainen@cs.helsinki.fi>
 *
 */


#define DCOVER 5

#include "suffixsort.hpp"

#include <algorithm>
#include <iostream>
#include <fstream>
#include <iterator>
#include <streambuf>
#include <vector>
#include <cstdio>
#include <cstdlib>



int main(int argc, char* argv[])
{

  // Check Arguments
  if (argc != 2 && argc != 3) {
    std::cerr << "Usage: " << argv[0] << " textfile [bwtfile]" << std::endl;
    std::exit(EXIT_FAILURE);
  }

  //int dcover = DCOVER;

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
  

  std::vector<int> sa(length+1);
  sa[0] = length;
  construct_suffix_array(text.begin(), text.end(), sa.begin()+1, sa.end());

  std::ostreambuf_iterator<char> bwtiter(bwtbuf);
  std::vector<int>::iterator i;
  for (i = sa.begin(); i != sa.end(); ++i) {
    if (*i != 0) {
      *bwtiter++ = text[*i-1];
    } else {
      *bwtiter++ = text[*(i-1)-1];
      eof_pos = i - sa.begin();
    }
  }

  /*
  eof_pos = compute_bwt(text.begin(), text.end(), 
			std::ostreambuf_iterator<char>(bwtbuf), dcover);
  */

  bwtbuf->pubseekpos(4);
  bwtfile << std::setw(posstr_size) << eof_pos;

}
