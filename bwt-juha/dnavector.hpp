/*
 * dnavector.cpp
 *
 * Copyright 2006 Juha K"arkk"ainen <juha.karkkainen@cs.helsinki.fi>
 *
 */


#include <vector>
#include <cassert>
#include <iterator>
#include <iostream>

class dna_vector {
public:
  dna_vector (size_t size = 0) : data(2*size), length(0) {}
  // implicit copy constructor, copy assignment and destructor

  template <class Iterator>
  void append (Iterator begin, Iterator end) {
    size_t size = end - begin;
    assert (size <= data.size() - length);

    bit_iterator i = data.begin() + 2*length;
    length += size;
#if DEBUG > 1
    std::cerr << "added " << size << " chars. length=" 
	      << length << std::endl;
#endif
    for ( ; begin != end; ++begin) {
      char ch = *begin;
      if (ch < 'a') ch += 'a'-'A';
      if (ch < 'g') {
	*i++ = false;
	if (ch == 'a') {
	  *i++ = false;
	} else {
	  *i++ = true;
	}
      } else {
	*i++ = true;
	if (ch == 'g') {
	  *i++ = false;
	} else {
	  *i++ = true;
	}
      }
    }
  }

  class read_iterator 
    : public std::iterator<std::random_access_iterator_tag, char,
			   int, int*, int&>
  {
  public:
    read_iterator () { }
    read_iterator (std::vector<bool>::iterator i) : p(i) {}
    
    char operator* () { 
      bool bit0 = *p;
      bool bit1 = *(p+1);
      return bit0 ? (bit1 ? 'T' : 'G') : (bit1 ? 'C' : 'A');
    }

    std::vector<bool>::iterator
    ptr() { return p; }

    read_iterator& operator++ () { 
      p+=2; return *this; 
    }
    read_iterator operator++ (int) { 
      read_iterator tmp = *this; ++*this; return tmp; 
    }

    read_iterator& operator-- () { p-=2; return *this; }
    read_iterator operator-- (int) { 
      read_iterator tmp = *this; --*this; return tmp; 
    }

    read_iterator& operator+= (difference_type n) {
      p += 2*n; return *this; 
    }
    read_iterator& operator-= (difference_type n) {
      p -= 2*n; return *this; 
    }

    friend
    read_iterator operator+ (read_iterator iter, difference_type n) { 
      return iter += n;
    }

    friend
    read_iterator operator+ (difference_type n, read_iterator iter) { 
      return iter += n;
    }

    friend
    read_iterator operator- (read_iterator iter, difference_type n) { 
      return iter -= n;
    }

    friend
    difference_type operator- (read_iterator iter1, 
			       read_iterator iter2) {
      return (iter1.p - iter2.p) / 2;
    }
  
    char operator[](difference_type n) const { 
      read_iterator tmp = *this + n;
      return *tmp; 
    }

    bool operator== (const read_iterator& other) const {
      return p == other.p;
    }  

    bool operator!= (const read_iterator& other) const {
      return p != other.p;
    }

    bool operator<(const read_iterator& other) const {
      return p < other.p;
    }

    bool operator>(const read_iterator& other) const {
      return p > other.p;
    }
    
    bool operator<=(const read_iterator& other) const {
      return p <= other.p;
    }
    
    bool operator>=(const read_iterator& other) const {
      return p >= other.p;
    }
    
  private:
    std::vector<bool>::iterator p;
  };

  typedef read_iterator iterator;

  read_iterator begin() {
    return read_iterator(data.begin());
  }
  read_iterator end() {
    return read_iterator(data.begin())+length;
  }

  

private:
  typedef std::vector<bool>::iterator bit_iterator;

  std::vector<bool> data;
  size_t length;

};
