#include "huffcode.h"

int huffnode_comp(const void *p1, const void *p2)
{
  HuffNode_t *node1 = (HuffNode_t *)p1; 
  HuffNode_t *node2 = (HuffNode_t *)p2; 

  if(node1->freq < node2->freq)
    return -1; 
  
  else if(node1->freq > node2->freq)
    return 1; 

  else if(node1->height < node2->height)
    return -1; 

  else if(node1->height > node2->height)
    return 1; 

  else
    return 0; 
  
}
