
// Copyright 2010-2014, D. E. Shaw Research.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt )

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
