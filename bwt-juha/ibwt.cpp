/*
 * bwt.cpp
 *
 * Copyright 2006 Juha K"arkk"ainen <juha.karkkainen@cs.helsinki.fi>
 *
 */


#define HEADERSIZE 20

#include "ibwt.hpp"

#include <algorithm>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <iterator>
#include <streambuf>
#include <vector>
#include <cstdio>
#include <cstdlib>



int main(int argc, char* argv[])
{

  // Check Arguments
  if (argc != 2 && argc != 3) {
    std::cerr << "Usage: " << argv[0] << " bwtfile [textfile]" << std::endl;
    std::exit(EXIT_FAILURE);
  }

  std::string infilename (argv[1]);
  std::string::size_type suffix = infilename.find(".bwt");
  std::string outfilename;
  if (argc==3) { outfilename = argv[2]; }
  else if (suffix==std::string::npos) {
    std::cerr << "ERROR: Refusing to overwrite the input file" << std::endl;
    exit(EXIT_FAILURE);
  } else if (suffix != infilename.length()-4) {
    std::cerr << "ERROR: bwtfile does not have the suffix .bwt" << std::endl;
    exit(EXIT_FAILURE);
  } else {
    outfilename = infilename.substr(0, suffix);
  }

  // Open bwt file 
  std::FILE *bwtfile;
  bwtfile = std::fopen(infilename.c_str(), "rb");   // binary because EOF gets interpreted on windows in text mode
  if (!bwtfile) {
    std::cerr << "ERROR: Unable to open bwt file " << infilename << std::endl;
    std::exit(EXIT_FAILURE);
  }
  
  // Read bwt
  std::fseek(bwtfile, 0, SEEK_END);
  std::size_t length = std::ftell(bwtfile);
  std::rewind(bwtfile);

  std::vector<unsigned char> bwt(length);
  typedef std::vector<unsigned char>::iterator bwt_iterator;

  length = std::fread((void *)&bwt[0], sizeof(char), length, bwtfile);
  std::fclose(bwtfile);
  if ( length != bwt.size() ) {
    std::cerr << "ERROR: wrong number of bytes read" << std::endl;
    exit(EXIT_FAILURE);
  }

  bwt_iterator line_end = std::find (bwt.begin(), bwt.end(), '\n');
  size_t bwt_size = bwt.end() - line_end - 1;

  std::istringstream header (std::string(bwt.begin(), line_end));
  std::string id;
  size_t eof_pos = bwt_size+1;
  header >> id >> eof_pos;
  if ( id != "BWT" ) {
    std::cerr << "ERROR: Not a BWT file" << std::endl;
    exit(EXIT_FAILURE);
  }
  if ( eof_pos >= bwt_size) {
    std::cerr << "ERROR: Incorrect eof position" << std::endl;
    exit(EXIT_FAILURE);
  }
  
#ifdef DEBUG
  std::cout << "Read bwt of length " << bwt_size 
	    << " from " << infilename << "\n"; 
  std::cout << "Writing text of length " << bwt_size-1
	    << " to " << outfilename << "\n"; 
#endif

  std::ofstream textfile(outfilename.c_str());
  std::streambuf* textbuf = textfile.rdbuf();

  inverse_bwt(line_end+1, bwt.end(), 
	      std::ostreambuf_iterator<char>(textbuf), eof_pos);

}
