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
#include <boost/random/counter_based_engine.hpp>

#define BOOST_TEST_MODULE counter_engine_counter
#include <boost/test/included/unit_test.hpp>

#define BOOST_COUNTER_BASED_ENGINE_CTRBITS 128
#define BOOST_COUNTER_BASED_ENGINE_DVALBITS 64
#define BOOST_COUNTER_BASED_ENGINE_NDOMAIN 4

//#define BOOST_COUNTER_BASED_ENGINE_DSIZE 4
#define BOOST_PSEUDO_RANDOM_FUNCTION boost::random::threefry<4, uint64_t>
typedef boost::random::counter_based_engine<BOOST_PSEUDO_RANDOM_FUNCTION, BOOST_COUNTER_BASED_ENGINE_CTRBITS> engine_t;
#define BOOST_RANDOM_URNG engine_t

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
   
    for (unsigned int i=0; i<BOOST_COUNTER_BASED_ENGINE_NDOMAIN; ++i) {
        ss >> digit;
        counter <<= BOOST_COUNTER_BASED_ENGINE_DVALBITS;
        counter += digit;
    }
    --counter; // the counter starts at one, here we make it zero based
    
    counter &= (uint1024_t(1)<<BOOST_COUNTER_BASED_ENGINE_CTRBITS) - 1;
    counter = counter * BOOST_COUNTER_BASED_ENGINE_NDOMAIN + subcounter;
        
    //std::cout << "counter=" <<  counter << " (" << os.str() << ")" << std::endl;
    return counter;
}


/*
 * Set the sample counter value
 */
void set_sample_counter(BOOST_RANDOM_URNG& urng, uint1024_t counter)
{
    uint1024_t digit;
    unsigned subcounter;
    
    std::stringstream oldstate;
    oldstate << urng;

    // pop the counter from the captured state
    oldstate >> subcounter;
    for (unsigned i=0; i<BOOST_COUNTER_BASED_ENGINE_NDOMAIN; ++i)
        oldstate >> digit;
        
    // store the remaining state in a string
    std::string remainder;
    std::getline(oldstate, remainder);
    
    // construct the new counter part of the state
    std::stringstream newstate;
    newstate << (counter % BOOST_COUNTER_BASED_ENGINE_NDOMAIN);

    counter /= BOOST_COUNTER_BASED_ENGINE_NDOMAIN;
    ++counter;
    
    int bits_already_set = 0;
    for (int i=BOOST_COUNTER_BASED_ENGINE_NDOMAIN-1; i>=0; --i) {
    
        unsigned bits_this_round = std::min(BOOST_COUNTER_BASED_ENGINE_DVALBITS, BOOST_COUNTER_BASED_ENGINE_CTRBITS - bits_already_set);
        
        digit = counter >> (i*BOOST_COUNTER_BASED_ENGINE_DVALBITS);
        digit &=  (uint1024_t(1)<<bits_this_round) - 1;
        newstate << ' ' << digit;
        
        bits_already_set += bits_this_round;
    }

    newstate << ' ' << remainder;
    
    // set the new state
    newstate >> urng;
}

/*
 * The max sample counter value
 */
uint1024_t max_sample_counter()
{
    uint1024_t ret(1);
    ret <<= BOOST_COUNTER_BASED_ENGINE_CTRBITS;
    --ret;
    --ret;
    ret *= BOOST_COUNTER_BASED_ENGINE_NDOMAIN;
    ret += BOOST_COUNTER_BASED_ENGINE_NDOMAIN - 1;
    return ret;
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
    
    // advance to the larget posible counter value
    set_sample_counter(urng, max_sample_counter() );

    
    // trigger an overflow?
    std::cout << "before overflow: counter=" << get_sample_counter(urng) << " state=" << urng <<  std::endl;
    urng();
    std::cout << "after overflow: counter=" << get_sample_counter(urng) << " state=" << urng <<  std::endl;
    urng();
    std::cout << "after overflow: counter=" << get_sample_counter(urng) << " state=" << urng <<  std::endl;
}

