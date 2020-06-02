/*
 * dcsamplesort.hpp
 *
 * Copyright 2003 Max-Planck-Institut fuer Informatik
 * Copyright 2004 Juha Ka"rkka"inen <juha.karkkainen@cs.helsinki.fi>
 *
 */

#ifndef DCSAMPLESORT_HPP
#define DCSAMPLESORT_HPP

#include "difference_cover.hpp"
#include "doubling.hpp"
#include "stringsort.hpp"
/*
*/

#include <sstream>
#include <iostream>
/*
#include <algorithm>
#include <iterator>
*/


//----------------------------------------------------------------------
// functor mark_unsorted_group
//
// When string sorting leaves a group of suffixes unsorted, 
// it marks that group by calling this functor.
//
// The marking is done by one's complementing the first and last
// entries of the group. Complemented values are assumed to become
// negative. Complementing is used instead of negation to be able
// mark the zero.
//----------------------------------------------------------------------
class mark_unsorted_group {
public:
  template <typename SArrayIterator>
  void operator() (SArrayIterator beg, SArrayIterator end) const { 
    *beg = ~*beg; 
    --end;
    *end = ~*end;  // note: group of size one stays unmarked
  }
};




template <class Sampler, class RandomAccessIterator1,
          class RandomAccessIterator2, class RandomAccessIterator3>
void
sort_dc_sample(Sampler sampler,
	       RandomAccessIterator1 str, RandomAccessIterator1 str_end,
	       RandomAccessIterator2 ranks, RandomAccessIterator2 ranks_end,
	       RandomAccessIterator3 suffixes, 
	       RandomAccessIterator3 suffixes_end)
{
  int size = sampler.samplesize();
  if ((ranks_end-ranks != size) || (suffixes_end-suffixes != size)) {
    std::ostringstream os;
    os << "sort_dc_sample:"
       << " wrong range size";
    throw std::logic_error(os.str());
  }

  // collect the sample
  RandomAccessIterator3 beyond = sampler.fill(suffixes);
  if (suffixes_end != beyond) {
    std::ostringstream os;
    os << "sort_dc_sample:"
       << " sample.fill produced " << beyond-suffixes
       << " samples but samplesize is " << size;
    throw std::logic_error(os.str());
  }
    
  // sort the sample to limited depth
  suffix_mkqsort(str, str_end, suffixes, suffixes_end, sampler.period(),
		 mark_unsorted_group());

  // compute ranks
  RandomAccessIterator3 first = suffixes;
  while (first != suffixes_end) {
    if (*first >= 0) {
      RandomAccessIterator3 last;
      ranks[sampler.pack(*first)] = first - suffixes;
      for (last=first+1; last != suffixes_end && *last >= 0; ++last) {
	ranks[sampler.pack(*last)] = last - suffixes;
      }
      *first = -(last-first);
      first = last;
    } else {
      RandomAccessIterator3 last;
      *first = ~*first;
      for (last=first+1; *last >= 0; ++last) ;
      *last = ~*last;
      for ( ; first <= last; ++first) {
	*first = sampler.pack(*first);
	ranks[*first] = last - suffixes;
      }
    }
  }

  // complete sorting with the doubling algorithm
  doubling_sort(suffixes, suffixes_end, ranks, ranks_end, 
		sampler.packed_period());

}

#endif // DCSAMPLESORT_HPP
