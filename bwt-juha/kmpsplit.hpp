
#include <vector>
#include <iostream>
#include <iterator>

template <class Iterator, class DCSampler, class RankIterator>
class kmp_split {
public:

  kmp_split(Iterator s, Iterator se, Iterator p, 
	    const DCSampler& d, RankIterator r)
    : str(s), str_end(se), suf(str), pivot(p), pivot_end(p+d.period()-1), 
      last(1), last_lcp(0), dcsampler(d), dcranks(r)
  { 
    preprocess(); 
  }

private:

  bool dc_less(Iterator a, Iterator b) const {
    std::pair<int,int> rep = dcsampler.pair(a-str, b-str);
    return dcranks[rep.first] <= dcranks[rep.second];
  }

public:

  bool is_next_less() {
    // state:
    // suf == str_end ||
    // last_lcp == lcp((suf-last), pivot) &&
    // last_less == (suf-last)+last_lcp == str_end || 
    //              (suf-last)[last_lcp] < pivot[last_lcp]

    bool result = true;
    if (suf==str_end) return true;
    if (suf==pivot) {
      last_lcp = pivot_end-pivot;
      last_less = false;
      ++suf; last=1;
      return false;
    }

    /*
    std::cout << " last=" << last
	      << " last_lcp=" << last_lcp
	      << " last+lcp[last]=" << last+lcp[last]
	      << " last_less=" << last_less
	      << " less[last]=" << bool(less[last]);
    */

    if (last_lcp < last || last_lcp == last+lcp[last]) {
      int lcp;
      if (last_lcp < last) lcp = 0;
      else lcp = last_lcp - last;
      while (suf+lcp != str_end && pivot+lcp != pivot_end
	     && suf[lcp]==pivot[lcp]) {
	++lcp;
      }
      /*
      std::cout << " lcp=" << lcp
		<< " suf+lcp-str=" << suf+lcp-str
		<< " str_end-str=" << str_end-str
		<< " suf+lcp==str=" << (suf+lcp==str);
      */
      result = (suf+lcp==str_end) 
	|| (pivot+lcp==pivot_end ? dc_less(suf, pivot) : suf[lcp] < pivot[lcp]);
      //std::cout << " result=" << result << std::endl;
      last_lcp = lcp;
      last_less = result;
      ++suf; last = 1;
    } else if (last_lcp > last+lcp[last]) {
      result = !less[last];
      ++suf; ++last;
    } else if (last_lcp < last+lcp[last]) {
      result = last_less;
      last_lcp -= last;
      ++suf; last = 1;
    }

    //std::cout << " result=" << result << std::endl;
    return result;
  }

  //81   0      212 cabcbacacabcbacacabcbacacabcbacacabcbaca

private:
  Iterator str;
  Iterator str_end;
  Iterator suf;
  Iterator pivot;
  Iterator pivot_end;

  int last;
  int last_lcp;
  bool last_less;

  std::vector<int> lcp;
  std::vector<char> less;

  const DCSampler& dcsampler;
  RankIterator dcranks;

  void preprocess() {

    int length = pivot_end-pivot;

    int i = 0;
    int j = 1;

    /*
    std::vector<int> shift(length+1);
    shift[0] = -1;

    while (true) {
      while ( j < length && pivot[i] == pivot[j] ) {
	shift[j] = shift[i];
	++i; ++j;
      }
      shift[j] = i;
      if (j == length) break;
      do {
	i = shift[i];
      } while ( i != -1 && pivot[i] != pivot[j] );
      if (i==-1) { ++i; ++j; }
    }

    std::copy (shift.begin(), shift.end(),
	       std::ostream_iterator<int>(std::cout, " "));
    std::cout << std::endl;
    */

    lcp.resize(length+1);
    less.resize(length+1);

    lcp[0] = length;
    less[0] = 0;
    i = 0; j = 1;

    while (j <= length) {
      while ( j < length && pivot[i] == pivot[j] ) {
	++i; ++j;
      }
      if ( j == length || pivot[i] < pivot[j] ) {
	lcp[j-i] = i;
	less[j-i] = 1;
      } else {
	lcp[j-i] = i;
	less[j-i] = 0;
      }
      int k = i;
      do {
	--i;
	if (i == -1) {
	  ++i; ++j;
	  break;
	} else if (lcp[k-i] == i) {
	  break;
	} else if (lcp[k-i] < i) {
	  lcp[j-i] = lcp[k-i];
	  less[j-i] = less[k-i];
	} else {
	  lcp[j-i] = i; // i == lcp[j-k]
	  less[j-i] = less[j-k];
	}
      } while (true);
    }

    /*
    std::copy (lcp.begin(), lcp.end(),
	       std::ostream_iterator<int>(std::cout, " "));
    std::cout << std::endl;
    std::copy (less.begin(), less.end(),
	       std::ostream_iterator<int>(std::cout, " "));
    std::cout << std::endl;
    */  
  }


};


/*

gagcgag
gagcgag 7 0
 g      0 0
  ga    1 1
   g    0 0
    gag 3 1
     g  0 0
      g 1 1
        0 1

aagagcgagta
             
*/
