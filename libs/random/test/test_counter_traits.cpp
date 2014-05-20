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
#include <boost/random/detail/counter_traits.hpp>
#include <sstream>
#include "printlogarray.hpp"

#define BOOST_TEST_MAIN
#include <boost/test/included/unit_test.hpp>

using namespace boost;

namespace /*anon*/ {

template <typename T, typename Traits>
void chk_insert_extract(T v){
    T w;
    std::stringstream oss;
    Traits::insert(oss, v);
    Traits::extract(oss, w);
    BOOST_CHECK(Traits::is_equal(v, w));
}

template <typename UintType, unsigned N>
void test_core(){  // constructors, insertion, extraction, equality, typedefs
    typedef array<UintType, N> CtrType;
    typedef random::detail::counter_traits<CtrType> Traits;

    CtrType a = {};
    chk_insert_extract<CtrType, Traits>(a);
    CtrType b = Traits::make_counter();
    BOOST_CHECK(Traits::is_equal(a, b));
    a[0] = 1;
    BOOST_CHECK(!Traits::is_equal(a, b));

    b = Traits::make_counter(1);
    BOOST_CHECK(Traits::is_equal(a, b));
    chk_insert_extract<CtrType, Traits>(b);

    CtrType c;
    for(unsigned i=0; i<N; ++i){
        c[i] = i+99;
    }
    chk_insert_extract<CtrType, Traits>(c);
}

template <typename UintType, unsigned N, unsigned HighBits>
void test_highbits(){
    // test incr and clrhighbits
    BOOST_CHECK_EQUAL(1, 1);
}

template <typename UintType, unsigned N, typename result_type, unsigned w>
void test_nth(){
    BOOST_CHECK_EQUAL(1, 1);
}

void core_varietypack(){
    test_core<uint32_t, 4>();
    test_core<uint64_t, 4>();
    test_core<uint8_t, 16>();
    test_core<uint16_t, 8>();
}

void highbit_varietypack(){
    test_highbits<uint32_t, 4, 32>();
    test_highbits<uint64_t, 4, 32>();
}

void nth_varietypack(){
    test_nth<uint32_t, 4, uint32_t, 32>();
    test_nth<uint32_t, 4, uint64_t, 32>();
    test_nth<uint64_t, 4, uint32_t, 32>();
    test_nth<uint64_t, 4, uint64_t, 32>();

    test_nth<uint32_t, 4, uint64_t, 64>();
    test_nth<uint64_t, 4, uint64_t, 64>();

    test_nth<uint32_t, 2, uint32_t, 32>();
    test_nth<uint32_t, 2, uint64_t, 32>();
    test_nth<uint64_t, 2, uint32_t, 32>();
    test_nth<uint64_t, 2, uint64_t, 32>();

    test_nth<uint32_t, 2, uint64_t, 64>();
    test_nth<uint64_t, 2, uint64_t, 64>();
}
} // namespace anon
        

BOOST_AUTO_TEST_CASE(test_counter_traits)
{
    core_varietypack();
    highbit_varietypack();
    nth_varietypack();
}

