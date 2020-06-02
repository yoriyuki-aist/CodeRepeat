/*
 * bwt.hpp
 *
 * Copyright 2004 Juha Ka"rkka"inen <juha.karkkainen@cs.helsinki.fi>
 *
 */

#include "dcsuffixsort.hpp"
#include "dcsamplesort.hpp"
#include "stringsort.hpp"
#include "difference_cover.hpp"
#include "distribute.hpp"
#include "kmpsplit.hpp"
/*
#include "doubling.hpp"
*/

#include <iterator>
#include <vector>
#include <set>
#include <iostream>
#include <iomanip>
#include <cassert>
/*
#include <algorithm>
#include <cstdlib>
*/

#ifndef DCOVER
#define DCOVER 8
#endif

#ifndef RANDSEED
#define RANDSEED 46
#endif

#define MIN_PIVOTS 3
#define OVERSAMPLING_RATE 10.0


struct do_nothing {
  template <typename Iterator>
  void operator() (Iterator, Iterator) {}
};




template <class StringIterator, class OutputIterator>
typename std::iterator_traits<StringIterator>::difference_type
compute_bwt (StringIterator str, StringIterator str_end, 
	     OutputIterator bwt, unsigned int dcover = DCOVER)
{

  typedef typename std::iterator_traits<StringIterator>::difference_type 
                        difftype;   // signed integral type    
  typedef unsigned long sizetype;   // unsigned integral type
  typedef difftype postype;   // type for representing vector positions

  typedef typename std::vector<postype>::iterator pos_iterator;

#if DEBUG > 0
  std::cout << "BWT construction starts" << std::endl;
#endif

  //-----------------
  // setup and check
  //-----------------

  sizetype length = std::distance(str, str_end);

#if DEBUG > 1
  typedef typename std::iterator_traits<StringIterator>::value_type
    chartype;
  typedef typename std::iterator_traits<OutputIterator>::value_type
    bwttype;
  std::cout << " text memory=" << length*sizeof(chartype) << "\n"
	    << " bwt memory=" << (length+1)*sizeof(chartype) << std::endl;
#endif

  dc_sampler<postype> dcsampler(length+1, dcover);

#if DEBUG > 1
  std::cout << " dcsampler memory=" 
	    << (dcsampler.packed_period() + 2*dcsampler.period()) 
                * sizeof(unsigned)
	    << std::endl;
#endif

  sizetype blocksize = dcsampler.samplesize();
  sizetype npivots = MIN_PIVOTS;

  // make average bucket size <= blocksize/OVERSAMPLING_RATE
  while ((OVERSAMPLING_RATE*length)/(npivots+1.0) > blocksize) ++npivots;

#if DEBUG > 1
  std::cout << " length=" << length 
	    << " samplesize=" << dcsampler.samplesize()
	    << " blocksize=" << blocksize
	    << " npivots=" << npivots
	    << std::endl;
#endif

  if (dcsampler.period() + 2*npivots > length) {
    // too small text for sampling

#if DEBUG > 0
    std::cout << "small text: sort all suffixes\n";
#endif

    std::vector<postype> sa(length+1);
    for (pos_iterator i=sa.begin(); i!=sa.end(); ++i) {
      *i = i - sa.begin();
    }
    suffix_mkqsort(str, str_end, sa.begin(), sa.end(), length+1, do_nothing());
    difftype eof_position = 0;
    for (pos_iterator i=sa.begin(); i!=sa.end(); ++i) {
      if (*i != 0) {
	*bwt++ = str[*i-1];
      } else {
	*bwt++ = str[*(i-1)-1];
	eof_position = i - sa.begin();
      }
    }
    return eof_position;
  }

  // don't take too short suffixes as pivots
  unsigned int pivotrange = length-dcsampler.period()+1;

  //-----------------------------------
  // setup difference cover sample
  //-----------------------------------

#if DEBUG > 0
  std::cout << "difference cover construction ... " << std::endl;
#endif

  sizetype samplesize = dcsampler.samplesize();
  std::vector<difftype> dcranks(samplesize);
  std::vector<difftype> suffixes(samplesize);

#if DEBUG > 1
  std::cout << "dcranks memory=" 
	    << samplesize * sizeof(difftype)
	    << "\n";
  std::cout << "suffixes memory=" 
	    << samplesize * sizeof(difftype)
	    << std::endl;
#endif

  sort_dc_sample(dcsampler, str, str_end,
		 dcranks.begin(), dcranks.end(),
                 suffixes.begin(), suffixes.end());

#if DEBUG > 3
  std::cout << "sample ranks:\n";
  std::copy(dcranks.begin(), dcranks.end(),
	    std::ostream_iterator<int>(std::cout,"\n"));
  std::cout << "end of sample ranks" << std::endl;
#endif



  //-------------------  
  // setup pivots
  //-------------------

#if DEBUG > 0
  std::cout << "setting up pivots ..." << std::endl;
#endif

  std::set<difftype> pivotset;
  double scale_factor = (pivotrange * 1.0L)/(RAND_MAX+1.0L);
  std::srand(RANDSEED);
  while (pivotset.size() < npivots) {
    int r = std::rand();
    int newpivot = static_cast<difftype>(scale_factor*r);
    pivotset.insert(newpivot);
  }

#if DEBUG > 1
  std::cout << " pivotset memory = " << pivotset.size()
	    << " elements" << std::endl;
#endif

  std::vector<difftype> pivots(pivotset.begin(), pivotset.end());
  pivotset.clear();

  // sort the pivots
  dc_sort_suffixes(str, str_end, pivots.begin(), pivots.end(),
		   dcsampler, dcranks.begin(), dcranks.end());

#if DEBUG > 2
  std::cout << "pivots sorted lexicographically:\n";
  for (pos_iterator i = pivots.begin(); i != pivots.end(); ++i) {
    std::cout << std::setw(8) << *i << " ";
    std::copy(str+*i, std::min(str_end, str+*i+40),
	      std::ostream_iterator<char>(std::cout,""));
    std::cout << std::endl;
  }
#endif


#if DEBUG > 0
  std::cout << " constructing distributor ... " << std::endl;
#endif

  suffix_distributor<StringIterator, pos_iterator, 
                     dc_sampler<postype>, pos_iterator>
    distributor(str, str_end, pivots.begin(), pivots.end(), 
		dcsampler, dcranks.begin());
  
#if DEBUG > 1
  distributor.print();
#endif



  //------------------------
  // compute bucketsizes
  //------------------------

#if DEBUG > 0
  std::cout << "computing bucket sizes ... " << std::endl;
#endif

  std::vector<difftype> buckets(distributor.nbuckets());

#if DEBUG > 1
  std::cout << " number of buckets: " << distributor.nbuckets() << "\n";
  std::cout << " bucket vector memory=" 
	    << buckets.size() * sizeof(difftype)
	    << std::endl;
#endif

  for (StringIterator suf = str; suf != str_end; ++suf) {
    unsigned int bucket = distributor.find_bucket(suf);
    ++buckets[bucket];

#if DEBUG > 3
    std::cout << " into bucket " << bucket;
    std::cout << ":" << std::setw(7) << suf-str << " ";
    std::copy(suf, std::min(str_end, suf+40),
	      std::ostream_iterator<char>(std::cout,""));
    std::cout << std::endl;
#endif

  }
  // ++buckets[0]; // empty suffix

#if DEBUG > 1
  std::cout << " bucket sizes: ";
  std::copy(buckets.begin(), buckets.end(),
	    std::ostream_iterator<int>(std::cout," "));
  std::cout << std::endl;
#endif


  //-----------------------
  // build and sort blocks
  //-----------------------

#if DEBUG > 0
  std::cout << "Computing and sorting blocks ... " << std::endl;
#endif

  *bwt++ = *(str_end-1);  // empty suffix

  postype eof_position = 0;
  sizetype total_sum = 1;  

  postype bucket_begin = 0;
  while (bucket_begin < (postype)buckets.size()) {

    sizetype sum = buckets[bucket_begin];
    postype bucket_end = bucket_begin + 1;
    while (bucket_end < (postype)buckets.size() 
	   && sum+buckets[bucket_end] <= blocksize) {
      sum += buckets[bucket_end];
      ++bucket_end;
    }
    
    postype left_pivot = bucket_begin-1;
    postype right_pivot = bucket_end-1;

    short lcp = 0;
    if (left_pivot >= 0 && right_pivot < (postype)pivots.size()) {
      lcp = suffix_lcp(str+pivots[left_pivot], str+pivots[right_pivot], 
		       str_end, 0, dcsampler.period()-1);
    }

#if DEBUG > 1
    std::cout << "block [" << bucket_begin << "," << bucket_end << ")"
	      << " size=" << sum
	      << " lcp=" << lcp
	      << std::endl;
#endif

    if (sum > blocksize) {
      blocksize = sum;
      suffixes.resize(blocksize);

#if DEBUG > 1
      std::cout << " blocksize increased to "
		<< blocksize << "\n";
      std::cout << " suffixes memory resized to "
		<< blocksize*sizeof(postype) 
		<< std::endl;
#endif

    }

    pos_iterator i = suffixes.begin();
    //if (bucket_begin == 0) { *i++ = length;       // empty suffix } 

    postype left_pivot_not_beyond = left_pivot<0 ? 0 : left_pivot;
    kmp_split<StringIterator, dc_sampler<postype>, pos_iterator> 
      left_cmp(str, str_end, str+pivots[left_pivot_not_beyond],
		   dcsampler, dcranks.begin());

    postype right_pivot_not_beyond = 
      right_pivot==(postype)pivots.size() ? right_pivot-1 : right_pivot;
    kmp_split<StringIterator, dc_sampler<postype>, pos_iterator> 
      right_cmp(str, str_end, str+pivots[right_pivot_not_beyond],
		   dcsampler, dcranks.begin());

    for (StringIterator suf = str; suf != str_end; ++suf) {
      
#if DEBUG > 3
      std::cout << ":" << std::setw(3) << suf-str << " ";
      std::copy(suf, std::min(str_end, suf+40),
		std::ostream_iterator<char>(std::cout,""));
      std::cout << std::endl;
#endif

      /*
      // is the suffix left of the block
      if (left_pivot >=0) {
	postype pivotpos = pivots[left_pivot];
	if (*suf < str[pivotpos]) continue;
	int lcp = suffix_lcp(suf, str+pivotpos, 
			     str_end, 0, dcsampler.period()-1);
	if (lcp < dcsampler.period()-1) {
	  if (suf+lcp == str_end || suf[lcp] < (str+pivotpos)[lcp]) {
	    continue;
	  }
	} else {
	  postype sufpos = suf-str;
	  std::pair<postype,postype> result 
	    = dcsampler.pair(sufpos, pivotpos);
	  if (dcranks[result.first] < dcranks[result.second]){
	    continue;
	  }
	}
      }

      // is the suffix right of the block
      if (right_pivot < (postype)pivots.size()) {
	postype pivotpos = pivots[right_pivot];
	if (*suf > str[pivotpos]) continue;
	int lcp = suffix_lcp(str+pivotpos, suf,
			     str_end, 0, dcsampler.period()-1);
	if (lcp < dcsampler.period()-1) {
	  if (suf+lcp < str_end && (str+pivotpos)[lcp] < suf[lcp]) {
	    continue;
	  }
	} else {
	  postype sufpos = suf-str;
	  std::pair<postype,postype> result 
	    = dcsampler.pair(pivotpos, sufpos);
	  if (dcranks[result.first] <= dcranks[result.second]){
	    continue;
	  }
	}
      }
      */

      //postype bucket = distributor.find_bucket(suf);

      bool to_left = left_pivot>0 && left_cmp.is_next_less(); 
      bool to_right = right_pivot<(postype)pivots.size() && !right_cmp.is_next_less() ;

      if (to_left || to_right) {
	//assert (bucket_begin > bucket || bucket >= bucket_end);
	continue;
      }

      // it is in the bucket
      //postype bucket = distributor.find_bucket(suf);

      //if (bucket_begin > bucket || bucket >= bucket_end) continue;

      //assert (bucket_begin <= bucket && bucket < bucket_end);

      *i++ = suf-str;
#if DEBUG > 3
      //std::cout << "into bucket " << bucket;
      std::cout << ":" << std::setw(3) << suf-str << " ";
      std::copy(suf, std::min(str_end, suf+40),
		std::ostream_iterator<char>(std::cout,""));
      std::cout << std::endl;
#endif
      
    }

    assert(i == suffixes.begin()+sum);    

    dc_sort_suffixes(str, str_end, suffixes.begin(), i,
		     dcsampler, dcranks.begin(), dcranks.end());

#if DEBUG > 3
    std::cout << "sorted bucket:\n";
    for (pos_iterator j = suffixes.begin(); j != i; ++j) {
      std::cout << std::setw(3) << *j << " ";
      std::copy(str+*j, std::min(str_end, str+*j+40),
		std::ostream_iterator<char>(std::cout,""));
      std::cout << std::endl;
    }
#endif

    for (pos_iterator j=suffixes.begin(); j != i; ++j) {
      if (*j != 0) {
	*bwt++ = str[*j-1];
      } else {
	*bwt++ = str[*(j-1)-1];
	eof_position = total_sum + (j - suffixes.begin());
      }
    }
    
    total_sum += sum;

    bucket_begin = bucket_end;
  }

  assert(total_sum == length+1);

  return eof_position;
}
