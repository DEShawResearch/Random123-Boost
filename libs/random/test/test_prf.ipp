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
#include <boost/random/uniform_int_distribution.hpp>
#include <boost/random/counter_based_engine.hpp>
#include <boost/random/counter_based_urng.hpp>

// Terminology is *very* confusing.  The Boost1.48
// UniformRandomNumberGenerator concept appears to be very close to
// the C++1x Uniform Random Number Generator (rand.req.urng,
// 26.5.1.3).  

// The Boost1.48 PseudoRandomNumberGenerator is very close to the
// C++1x Random Number Engine (rand.req.eng 26.5.1.4).  In fact,
// Boost.PRNG is a little more general (allowing for signed
// result_types, and a few more constructors), but they are clearly
// analogs.  Furthermore, the concept class in random/test/concepts is
// called RandomNumberEngine.

// It would be less confusing if the Boost documentation replaced the
// name Pseudo-random Number Generator with Random Number Engine.

// Things get more confusing in test_generator.ipp.  It's clearly a
// test suite for Pseudo-Random Number Generators.  But it uses the
// shorthand URNG for objects of the type that it tests, and that
// shorthand appears throughout the other test_*.cpp files even though
// the Boost documentation draws a clear and unambiguous distinction
// between Uniform Random Number Generators and Pseudo-Random Number
// Generators.  Giving all the Pseudo-Random Number Generators an
// abbreviated name with the mnemonic URNG is terribly confusing.  It
// would be less confusing to use the abbreviation PRNG instead.  But
// (see above) PRNG looks like an anachronism as well.  The C++1x name
// is "Random Number Engine".

// So, in this directory, I've created 
//   test_engine.ipp
// which is test_generator.ipp with a case-preserving 
// string-replacement of s/urng/rne/.

// Finally, the boost documentation (through at least 1.55) makes a
// special note of the fact that the Boost URNG does not provide
// operator()(long), but so what?  Neither does C++1x URNG (26.5.1.3
// [rand.req.urng]).  Neither Boost's nor C++1x's URNG can be used
// with std::random_shuffle, but both *can* be used with std::shuffle
// (both in std::25.2.11[lib.alg.random.shuffle]).  The highlighted
// note isn't wrong, but it is a confusing distraction.

// If we've got a BOOST_PSEUDO_RANDOM_FUNCTION, we should be able to turn it
// into a bona fide URNG with the counter_based_engine template.  Let's
// check that:

typedef boost::random::counter_based_engine<BOOST_PSEUDO_RANDOM_FUNCTION> engine_t;

#define BOOST_RANDOM_RNE engine_t

#include "test_engine.ipp"

// Now we can go on to test other aspects of engine_t...

// counter-based engines can do discards in O(1) time.  Let's
// check that many such "random" discards add up correctly.
void do_test_huge_discard(boost::uintmax_t bigjump){
    BOOST_RANDOM_RNE rne;
    BOOST_RANDOM_RNE rne2;
    BOOST_RANDOM_RNE rne3;

    BOOST_CHECK_EQUAL(rne, rne2);
    boost::uintmax_t n = 0;
    rne2.discard(bigjump);
    while(n < bigjump){
        BOOST_CHECK_NE(rne, rne2);
        boost::random::uniform_int_distribution<boost::uintmax_t> d(1, 1+(bigjump-n)/73);;
        boost::uintmax_t smalljump = d(rne3);
        rne.discard(smalljump);
        n += smalljump;
    }
    BOOST_CHECK_EQUAL(rne, rne2);
}

BOOST_AUTO_TEST_CASE(test_huge_discard)
{
    // Run the discard check with numbers large enough to force us to
    // "carry" between the elements of the counter.
    do_test_huge_discard((std::numeric_limits<boost::uintmax_t>::max)() - 1);
    do_test_huge_discard(((std::numeric_limits<boost::uintmax_t>::max)()>>1) + 1);
    do_test_huge_discard(((std::numeric_limits<boost::uintmax_t>::max)()>>32) + 1);
    do_test_huge_discard((std::numeric_limits<BOOST_RANDOM_RNE::result_type>::max)());
}        

// TODO: seek/tell, seed(key) and getseed()

// Now let's test the counter_based_urng template:
typedef boost::random::counter_based_urng<BOOST_PSEUDO_RANDOM_FUNCTION> urng_t;

#undef BOOST_RANDOM_URNG
#define BOOST_RANDOM_URNG urng_t
BOOST_AUTO_TEST_CASE(test_min_max_urng)
{
    BOOST_PSEUDO_RANDOM_FUNCTION::domain_type c0 = {};
    BOOST_PSEUDO_RANDOM_FUNCTION::key_type k0 = {};
    BOOST_RANDOM_URNG urng(BOOST_PSEUDO_RANDOM_FUNCTION(k0), c0);
    for(int i = 0; i < 10000; ++i) {
        result_type value = urng();
        BOOST_CHECK_GE(value, (BOOST_RANDOM_URNG::min)());
        BOOST_CHECK_LE(value, (BOOST_RANDOM_URNG::max)());
    }
}

BOOST_AUTO_TEST_CASE(test_comparison_urng)
{
    BOOST_PSEUDO_RANDOM_FUNCTION::domain_type c0 = {};
    BOOST_PSEUDO_RANDOM_FUNCTION::key_type k0 = {};
    BOOST_RANDOM_URNG urng(BOOST_PSEUDO_RANDOM_FUNCTION(k0), c0);
    BOOST_RANDOM_URNG urng2(BOOST_PSEUDO_RANDOM_FUNCTION(k0), c0);
    BOOST_CHECK(urng == urng2);
    BOOST_CHECK(!(urng != urng2));
    urng();
    BOOST_CHECK(urng != urng2);
    BOOST_CHECK(!(urng == urng2));

    BOOST_PSEUDO_RANDOM_FUNCTION::domain_type c1 = {{1}};
    BOOST_PSEUDO_RANDOM_FUNCTION::key_type k1 = {{1}};
    BOOST_RANDOM_URNG urng3(BOOST_PSEUDO_RANDOM_FUNCTION(k1), c0);
    BOOST_RANDOM_URNG urng4(BOOST_PSEUDO_RANDOM_FUNCTION(k0), c1);
    BOOST_RANDOM_URNG urng5(BOOST_PSEUDO_RANDOM_FUNCTION(k1), c1);
    BOOST_CHECK_NE(urng, urng3);
    BOOST_CHECK_NE(urng, urng4);
    BOOST_CHECK_NE(urng, urng5);
    BOOST_CHECK_NE(urng3, urng4);
    BOOST_CHECK_NE(urng3, urng5);
    BOOST_CHECK_NE(urng4, urng5);
}

BOOST_AUTO_TEST_CASE(test_copy_urng)
{
    BOOST_PSEUDO_RANDOM_FUNCTION::domain_type c0 = {};
    BOOST_PSEUDO_RANDOM_FUNCTION::key_type k0 = {};
    BOOST_RANDOM_URNG urng(BOOST_PSEUDO_RANDOM_FUNCTION(k0), c0);
    {
        BOOST_RANDOM_URNG urng2 = urng;
        BOOST_CHECK_EQUAL(urng, urng2);
    }
    {
        BOOST_RANDOM_URNG urng2(urng);
        BOOST_CHECK_EQUAL(urng, urng2);
    }
    {
        BOOST_RANDOM_URNG urng2 = urng;
        BOOST_CHECK_EQUAL(urng, urng2);
    }
}

template<class CharT>
void do_test_streaming_urng(const BOOST_RANDOM_URNG& urng)
{
    BOOST_PSEUDO_RANDOM_FUNCTION::domain_type c0 = {};
    BOOST_PSEUDO_RANDOM_FUNCTION::key_type k0 = {};
    BOOST_RANDOM_URNG urng2(BOOST_PSEUDO_RANDOM_FUNCTION(k0), c0);
    std::basic_ostringstream<CharT> output;
    output << urng;
    BOOST_CHECK_NE(urng, urng2);
    // restore old state
    std::basic_istringstream<CharT> input(output.str());
    input >> urng2;
    BOOST_CHECK_EQUAL(urng, urng2);
}

BOOST_AUTO_TEST_CASE(test_streaming_urng)
{
    BOOST_PSEUDO_RANDOM_FUNCTION::domain_type c0 = {};
    BOOST_PSEUDO_RANDOM_FUNCTION::key_type k0 = {};
    BOOST_RANDOM_URNG urng(BOOST_PSEUDO_RANDOM_FUNCTION(k0), c0);
    urng.discard(9307);
    do_test_streaming_urng<char>(urng);
#if !defined(BOOST_NO_STD_WSTREAMBUF) && !defined(BOOST_NO_STD_WSTRING)
    do_test_streaming_urng<wchar_t>(urng);
#endif
}

BOOST_AUTO_TEST_CASE(test_discard_urng)
{
    BOOST_PSEUDO_RANDOM_FUNCTION::domain_type c0 = {};
    BOOST_PSEUDO_RANDOM_FUNCTION::key_type k0 = {};
    BOOST_RANDOM_URNG urng(BOOST_PSEUDO_RANDOM_FUNCTION(k0), c0);
    BOOST_RANDOM_URNG urng2(BOOST_PSEUDO_RANDOM_FUNCTION(k0), c0);
    BOOST_CHECK_EQUAL(urng, urng2);
    for(int i = 0; i < 9307; ++i){
        urng();
        BOOST_CHECK_NE(urng, urng2);
    }
    urng2.discard(9307);
    BOOST_CHECK_EQUAL(urng, urng2);
}

// TODO: reset.

