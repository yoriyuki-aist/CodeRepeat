/*
 * bwt.cpp
 *
 * Copyright 2006 Juha K"arkk"ainen <juha.karkkainen@cs.helsinki.fi>
 *
 */


#define DCOVER 8

#include "dnavector.hpp"
#include "bwt.hpp"

#include <algorithm>
#include <iostream>
#include <fstream>
#include <iterator>
#include <streambuf>
#include <vector>
#include <cstdio>
#include <cstdlib>

#define BUFSIZE (1<<18)

int main(int argc, char* argv[])
{

  // Check Arguments
  if (argc != 2 && argc != 3) {
    std::cerr << "Usage: " << argv[0] << " dnafile [bwtfile]" << std::endl;
    std::exit(EXIT_FAILURE);
  }

  int dcover = DCOVER;

  std::string infilename (argv[1]);
  std::string outfilename = infilename;
  outfilename += ".bwt";
  if (argc==3) { outfilename = argv[2]; }

  // Open text file 
  std::FILE *textfile;
  textfile = std::fopen(infilename.c_str(), "rb");   // binary because EOF gets interpreted on windows in text mode
  if (!textfile) {
    std::cerr << "ERROR: Unable to open text file " << infilename << std::endl;
    std::exit(EXIT_FAILURE);
  }
  
  // Read text
  std::fseek(textfile, 0, SEEK_END);
  std::size_t length = std::ftell(textfile);
  std::rewind(textfile);

  /*
  std::vector<unsigned char> text(length);
  length = std::fread((void *)&text[0], sizeof(char), length, textfile);
  std::fclose(textfile);
  if ( length != text.size() ) {
    std::cerr << "ERROR: wrong number of bytes read" << std::endl;
    exit(EXIT_FAILURE);
  }
  */

  std::vector<char> buffer(BUFSIZE);
  dna_vector data(length);
  
  size_t readcount = 0;
  while (readcount + BUFSIZE < length) {
    size_t read = std::fread((void *)&buffer[0], sizeof(char), BUFSIZE, textfile);
    if ( read != BUFSIZE ) {
      std::cerr << "ERROR: wrong number of bytes read" << std::endl;
      std::fclose(textfile);
      exit(EXIT_FAILURE);
    }
    data.append(buffer.begin(), buffer.end());
    readcount += read;
  }
  if (readcount < length) {
    size_t read = std::fread((void *)&buffer[0], sizeof(char), 
			      length-readcount, textfile);
    readcount += read;
    if ( readcount != length ) {
      std::cerr << "ERROR: wrong number of bytes read" << std::endl;
      std::fclose(textfile);
      exit(EXIT_FAILURE);
    }
    data.append(buffer.begin(), buffer.begin()+read);
  }

  std::fclose(textfile);
  buffer.clear();

#ifdef DEBUG
  std::cout << "Read dna sequence of length " << length 
	    << "=" << data.end()-data.begin()
	    << "=" << (data.end().ptr()-data.begin().ptr())/2
	    << " from " << infilename << "\n";
  //	    << " data.begin()=" << data.begin().ptr()
  //    << " data.end()=" << data.begin().ptr() << "\n";
#endif

  size_t eof_pos = length + 1;
  std::ostringstream posstr;
  posstr << eof_pos;
  size_t posstr_size = posstr.str().size();
  //std::cerr << posstr.str() << ' ' << posstr_size << '\n';

  std::ofstream bwtfile(outfilename.c_str());
  bwtfile << "BWT " << posstr.str() << '\n';

  std::streambuf* bwtbuf = bwtfile.rdbuf();
  
  eof_pos = compute_bwt(data.begin(), data.end(), 
			std::ostreambuf_iterator<char>(bwtbuf), dcover);

  /*
  std::copy (text.begin(), text.end(),
	     std::ostreambuf_iterator<char>(bwtbuf) );
  */

  bwtbuf->pubseekpos(4);
  bwtfile << std::setw(posstr_size) << eof_pos;

}
