/* test_counter_detail.cpp
 *
 * Copyright (c) 2014 M.A. (Thijs) van den Berg
 * Distributed under the Boost Software License, Version 1.0. (See
 * accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * See http://www.boost.org for most recent version including documentation.
 *
 * $Id$
 */
#include <istream>
#include <ostream>
#include <iomanip>
#include <string>
#include <algorithm> // std::min

#include <boost/multiprecision/cpp_int.hpp>

#include <boost/random/threefry.hpp>
#include <boost/random/philox.hpp>
#include <boost/random/counter_based_engine.hpp>

#define BOOST_TEST_MODULE counter_engine_counter
#include <boost/test/included/unit_test.hpp>

#define BOOST_COUNTER_BASED_ENGINE_RESULT_TYPE uint64_t
#define BOOST_PSEUDO_RANDOM_FUNCTION boost::random::philox<4, uint64_t>
#define BOOST_COUNTER_BASED_ENGINE_CTRBITS 128u

typedef boost::random::counter_based_engine<BOOST_COUNTER_BASED_ENGINE_RESULT_TYPE, BOOST_PSEUDO_RANDOM_FUNCTION, BOOST_COUNTER_BASED_ENGINE_CTRBITS> engine_t;
#define BOOST_RANDOM_URNG engine_t

typedef engine_t::domain_type::value_type dvalue_type;
static const unsigned ctrbits = BOOST_RANDOM_URNG::counter_bits;

static const unsigned dvalbits = std::numeric_limits<dvalue_type>::digits;
static const unsigned ndomain = BOOST_RANDOM_URNG::domain_traits::Nbits / dvalbits;

static const unsigned nrange = engine_t::results_per_counter();

typedef engine_t::key_type::value_type kvalue_type;
static const unsigned kvalbits = std::numeric_limits<kvalue_type>::digits;
static const unsigned nkey = engine_t::key_traits::Nbits/kvalbits;

using namespace boost::multiprecision;

/*
 * Extract the "sample counter" value from a counter based engine.
 * "sample_counter" tell the numer of samples that are drawn from the engine. 
 * It has a value of 1 after the first draw and increases with 1 after each draw.
 * If each encryption round generated S samples then this value is S * (2^counter_bits -1) + current_pos
 */
uint1024_t get_sample_counter(BOOST_RANDOM_URNG& urng)
{
    uint1024_t counter, digit;
    unsigned subcounter; 
    
    std::stringstream ss;
    ss << urng;
    ss >> subcounter;
   
    for (unsigned int i=0; i<ndomain; ++i) {
        ss >> digit;
        counter <<= dvalbits;
        counter += digit;
    }
    
    counter &= (uint1024_t(1)<<ctrbits) - 1;
    counter = counter * nrange + subcounter;
        
    //std::cout << "counter=" <<  counter << " (" << os.str() << ")" << std::endl;
    return counter;
}

BOOST_AUTO_TEST_CASE(test_overflow)
{
    engine_t urng;
    engine_t urng0;

    // use 'private' knowledge of the external
    // stream representation to initialize a
    // nearly maximal counter.
    std::stringstream ss;
    ss << 0;
    for(size_t i=0; i<ndomain; ++i)
        ss << ' ' << ~dvalue_type(0);
    for(size_t i=0; i<nkey; ++i)
        ss << ' ' << 0;
    ss >> urng0;
    BOOST_CHECK(ss);

    // Run the urng right up to the edge..
    urng = urng0;
    for(unsigned i=0; i<nrange; ++i){
        urng();
        BOOST_CHECK_NE(urng, urng0);
    }
    // Verify that it throws when we push over the edge
    BOOST_CHECK_THROW(urng(), std::invalid_argument);

    // Try again with discard:
    urng = urng0;
    urng.discard(nrange);
    BOOST_CHECK_NE(urng, urng0);
    BOOST_CHECK_THROW(urng(), std::invalid_argument);

    // Make sure that discard itself throws if it pushes us over the
    // edge.
    urng = urng0;
    BOOST_CHECK_THROW(urng.discard(nrange+1), std::invalid_argument);
}

BOOST_AUTO_TEST_CASE(test_counter_detail)
{
    uint1024_t expected,actual,discards;
    BOOST_RANDOM_URNG urng;
    
    // initial check
    urng();                                 // generate a single random value to trigger initialisation
    expected = uint1024_t(1);               // we expect the sample counter to now be at 1
    actual = get_sample_counter(urng);
    BOOST_CHECK_EQUAL(expected, actual);
    
    // draw 300 samples
    for (unsigned i=0; i<300; ++i) {
        urng();
        ++expected;
        actual = get_sample_counter(urng);
        BOOST_CHECK_EQUAL(expected, actual);
    }
    
    // do 300 big discards
    for (unsigned i=0; i<300; ++i) {
        discards = uint1024_t(0xFFFFFFFFFFFFFFFF);
        expected += discards;
        urng.discard(0xFFFFFFFFFFFFFFFF);
        actual = get_sample_counter(urng);
        BOOST_CHECK_EQUAL(expected, actual);
    }
}

