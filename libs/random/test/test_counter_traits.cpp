
// Copyright 2014, D. E. Shaw Research.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt )

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

    // Check make_counter with ranges of different types
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

