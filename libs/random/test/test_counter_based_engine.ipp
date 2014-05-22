
// Copyright 2010-2014, D. E. Shaw Research.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt )

#include <boost/random/uniform_int_distribution.hpp>
#include <boost/random/counter_based_engine.hpp>
#include <boost/limits.hpp>

typedef boost::random::counter_based_engine<BOOST_COUNTER_BASED_ENGINE_RESULT_TYPE, BOOST_PSEUDO_RANDOM_FUNCTION, BOOST_COUNTER_BASED_ENGINE_CTRBITS> engine_t;

#define BOOST_RANDOM_URNG engine_t

#include "test_generator.ipp"

// Now we can go on to test other aspects of engine_t...

// counter-based engines can do discards in O(1) time.  Let's
// check that many such "random" discards add up correctly.
void do_test_huge_discard(boost::uintmax_t bigjump){
    BOOST_RANDOM_URNG urng0;
    BOOST_RANDOM_URNG urng;
    BOOST_RANDOM_URNG urng2;
    BOOST_RANDOM_URNG urng3;
    BOOST_RANDOM_URNG urngn;

    BOOST_CHECK_EQUAL(urng, urng2);
    boost::uintmax_t n = 0;
    bool out_of_bits1 = false;
    bool out_of_bitsn = false;
    try{
        urng2.discard(bigjump);
    }catch(std::invalid_argument&){
        // It's ok if bigjump exceeds our sequence length.
        // It just means that we are testing a counter_based_engine
        // with a small-ish number of CounterBits.
        out_of_bits1 = true;
    }
    try{
        while(n < bigjump){
            if(!out_of_bits1)
                BOOST_CHECK_NE(urng, urng2);
            boost::random::uniform_int_distribution<boost::uintmax_t> d(1, 1+(bigjump-n)/73);;
            boost::uintmax_t smalljump = d(urng3);
            urng.discard(smalljump);
            n += smalljump;
            urngn = urng0;
            urngn.discard(n);
            BOOST_CHECK_EQUAL(urng, urngn);
        }
    }catch(std::invalid_argument&){
        out_of_bitsn = true;
    }
    BOOST_CHECK_EQUAL(out_of_bits1, out_of_bitsn);
    if(!out_of_bits1)
        BOOST_CHECK_EQUAL(urng, urng2);
}

BOOST_AUTO_TEST_CASE(test_huge_discard)
{
    // Run the discard check with numbers large enough to force us to
    // "carry" between the elements of the counter.
    do_test_huge_discard((std::numeric_limits<boost::uintmax_t>::max)() - 1);
    do_test_huge_discard(((std::numeric_limits<boost::uintmax_t>::max)()>>1) + 1);
    do_test_huge_discard(((std::numeric_limits<boost::uintmax_t>::max)()>>32) + 1);
    do_test_huge_discard((std::numeric_limits<BOOST_RANDOM_URNG::result_type>::max)());
}        

// TODO: restart, seed(key), constructor(Prf, start), limited counter width.

