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
#include <boost/random/detail/mulhilo.hpp>
#include <cassert>
#include <iostream>
#include <typeinfo>
#include <boost/random/uniform_int_distribution.hpp>
#include <boost/random/mersenne_twister.hpp>

using boost::random::uniform_int_distribution;
using boost::random::mt11213b;
using boost::random::detail::mulhilo;
using boost::random::detail::mulhilo_halfword;

#define BOOST_TEST_MAIN
#include <boost/test/unit_test.hpp>

template <typename UINT>
void doit(){
    uniform_int_distribution<UINT> D;
    mt11213b mt;
    for(int i=0; i<1000000; ++i){
        UINT a = D(mt);
        UINT b = D(mt);

        UINT hi, lo;
        lo = mulhilo(a, b, hi);
        BOOST_CHECK_EQUAL(lo, (UINT)(a*b) );
        // Can't we say something about hi/a and b 
        // and hi/b and a?

        UINT hi_hw, lo_hw;
        lo_hw = mulhilo_halfword(a, b, hi_hw);
        BOOST_CHECK_EQUAL(lo_hw, lo);
        BOOST_CHECK_EQUAL(hi_hw, hi);
        //std::cout << a << " * " << b << " = " << hi << "." << lo << "\n";
    }
}

BOOST_AUTO_TEST_CASE(test_mulhilo)
{
    doit<uint8_t>();
    doit<uint16_t>();
    doit<uint32_t>();
    doit<uint64_t>();
}
