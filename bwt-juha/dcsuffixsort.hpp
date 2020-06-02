/*
 * dcsuffixsort.hpp
 *
 * Copyright 2003 Max-Planck-Institut fuer Informatik
 * Copyright 2004 Juha Ka"rkka"inen <juha.karkkainen@cs.helsinki.fi>
 *
 */

#ifndef DCSUFFIXSORT_HPP
#define DCSUFFIXSORT_HPP

#include "stringsort.hpp"
#include "difference_cover.hpp"
#include "partition.hpp"
/*
#include "doubling.hpp"
#include "stringsort.hpp"
#include "checker.hpp"
#include "timing.hpp"
*/

#include <utility>
#include <iterator>
#include <stdexcept>
#include <sstream>
/*
#include <algorithm>
#include <vector>
#include <string>
#include <functional>
*/

//----------------------------------------------------------------------
// dc_suffix_compare
//
// compare two suffixes using difference covers
//----------------------------------------------------------------------
template <typename RankIterator>
class dc_suffix_compare {
private:
  typedef typename std::iterator_traits<RankIterator>::value_type 
          rank_type;
  const RankIterator sample_ranks;
  const dc_sampler<rank_type>& sample;
public:
  dc_suffix_compare(RankIterator beg, const dc_sampler<rank_type>& s) 
    : sample_ranks(beg), sample(s) {}
  template <typename StringPosType>
  bool operator() (StringPosType a, StringPosType b) const {
    std::pair<rank_type,rank_type> rep = sample.pair(a, b);
    return sample_ranks[rep.first] < sample_ranks[rep.second];
  }
};


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
class mark_unsorted_group2 {
public:
  template <typename SArrayIterator>
  void operator() (SArrayIterator beg, SArrayIterator end) const { 
    *beg = ~*beg; 
    --end;
    *end = ~*end;  // note: group of size one stays unmarked
  }
};

/*
//----------------------------------------------------------------------
// functor do_nothing
//
// Replaces mark_unsorted_group when nothing needs to be done
// to unsorted groups (this happens when doing stringsort only)
//----------------------------------------------------------------------
class do_nothing {
public:
  template <typename Iterator>
  void operator() (Iterator, Iterator) {}
};
*/


//----------------------------------------------------------------------
// function dc_sort_suffixes
// 
// Sort a set of suffixes using a difference cover sample.
//----------------------------------------------------------------------
template <typename StringIterator, typename SArrayIterator,
	  typename DCSampler, typename RankIterator>
void
dc_sort_suffixes(StringIterator str, StringIterator str_end,
	      SArrayIterator sa, SArrayIterator sa_end,
	      const DCSampler& sampler,
	      RankIterator ranks, RankIterator ranks_end)
{
  if (ranks_end-ranks != sampler.samplesize()) {
    std::ostringstream os;
    os << "dc_suffix_sort:"
       << " wrong range size";
    throw std::logic_error(os.str());
  }
      
  suffix_mkqsort(str, str_end, sa, sa_end, sampler.period(),
			 mark_unsorted_group2());
      
  SArrayIterator first = sa;
  while (true) {
    while (first != sa_end && *first >= 0) ++first;
    if (first == sa_end) break;
    *first = ~*first;
    SArrayIterator last = first + 1;
    while (*last >= 0) ++last;
    *last = ~*last;
    ++last;
    quicksort(first, last, 
	      dc_suffix_compare<RankIterator>(ranks, sampler));
    first = last;
  }
  
}

#endif // DCSUFFIXSORT_HPP
