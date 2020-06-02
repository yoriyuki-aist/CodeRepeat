

#include <vector>
#include <algorithm>
#include <iterator>
#include <cassert>
#include <iostream>
#include <numeric>






template <class BwtIterator, class OutputIterator>
void
inverse_bwt (BwtIterator bwt, BwtIterator bwt_end, 
	     OutputIterator str, size_t eof_pos)
{

  typedef typename
    std::iterator_traits<BwtIterator>::value_type char_type;

  std::vector<size_t> count(257, 0);

  BwtIterator it;
  size_t pos;

  for (it = bwt; it != bwt_end; ++it) {
    size_t pos = *it;
    ++count[pos+1];
  }
  pos = bwt[eof_pos];
  --count[pos+1];
  ++count[0];
  std::partial_sum(count.begin(), count.end(), count.begin());

  std::vector<size_t> permutation(bwt_end-bwt);
  for (it = bwt; it != bwt+eof_pos; ++it) {
    size_t pos = *it;
    permutation[count[pos]++] = it-bwt;
  }
  permutation[0] = eof_pos; ++it;
  for ( ; it != bwt_end; ++it) {
    size_t pos = *it;
    permutation[count[pos]++] = it-bwt;
  }


  /*
  std::copy (permutation.begin(), permutation.end(),
	     std::ostream_iterator<size_t>(std::cerr, " "));
  std::cerr << std::endl;

  */

  pos = permutation[permutation[0]];
  for (size_t n = bwt_end-bwt-1; n>0; --n) {
    //for (k=permutation[eof_pos]; k!=eof_pos; k=permutation[eof_pos]) {
    //std::cerr << k << " ";
    //std::cerr << bwt[k] << '\n';
    *str++ = bwt[pos];
    pos = permutation[pos];
  }

  assert(pos == eof_pos);

}
