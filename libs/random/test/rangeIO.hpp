
// Copyright 2010-2014, D. E. Shaw Research.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt )

#ifndef __rangeIO__dot_h_
#define __rangeIO__dot_h_

#include <iterator>
#include <iostream>

// This *almost* exists as a friend of iterator_range Boost.Range, but
// iterator_range does insertion (output) without a separator and has
// no extractor at all.
template  <typename ITER>
class rangeInserter_helper {
    ITER b;
    ITER e;
    const char *sep;
public:
    rangeInserter_helper(ITER _b, ITER _e, const char* _sep=" ") : b(_b), e(_e), sep(_sep){}
    friend std::ostream& operator<<(std::ostream& s, const rangeInserter_helper&r){
        std::ostream::sentry sentry(s);
        if(sentry)
            copy(r.b, r.e, std::ostream_iterator<typename std::iterator_traits<ITER>::value_type>(s, r.sep));
        return s;
    }
};

// The extra level of indirection - a function that calls the
// constructor of the _helper class allows the user to write
// rangeExtractor(b,e) instead of rangeExtractor<itertype>(b,e).
template <typename ITER>
static rangeInserter_helper<ITER> 
rangeInserter(ITER b, ITER e, const char *sep=" "){
    return rangeInserter_helper<ITER>(b, e, sep);
}



template  <typename ITER>
class rangeExtractor_helper {
    ITER b;
    ITER e;
public:
    rangeExtractor_helper(ITER _b, ITER _e) : b(_b), e(_e){}
    friend std::istream& operator>>(std::istream& s, const rangeExtractor_helper&r){
        std::istream::sentry sentry(s);
        if(sentry){
            ITER bb = r.b;
            while(bb!=r.e)
                s >> *bb++;
        }
        return s;
    }
};

// The extra level of indirection - a function that calls the
// constructor of the _helper class allows the user to write
// rangeExtractor(b,e) instead of rangeExtractor<itertype>(b,e).
template <typename ITER>
static rangeExtractor_helper<ITER> 
rangeExtractor(ITER b, ITER e){
    return rangeExtractor_helper<ITER>(b, e);
}

#endif
