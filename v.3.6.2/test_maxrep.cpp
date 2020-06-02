#include "maxrep.h"

using namespace std; 


int main(int argc, char **argv)
{
  if(argc != 5){
    cout << "Usage: " << argv[0] << " bwt_file bwt_pos_file lcp_file ml" << endl; 
    exit(1); 
  }

  uint32_t ml = (uint32_t)atoi(argv[4]);

  maxrep(argv[1], argv[2], argv[3], ml); 

  return 0; 
}
