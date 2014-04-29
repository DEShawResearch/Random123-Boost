/** @page LICENSE
Copyright 2010-2012, D. E. Shaw Research.
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:

* Redistributions of source code must retain the above copyright
  notice, this list of conditions, and the following disclaimer.

* Redistributions in binary form must reproduce the above copyright
  notice, this list of conditions, and the following disclaimer in the
  documentation and/or other materials provided with the distribution.

* Neither the name of D. E. Shaw Research nor the names of its
  contributors may be used to endorse or promote products derived from
  this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
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
