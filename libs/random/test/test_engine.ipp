// This is the original boost test_generator.ipp, modified to use the
// abbreviation RNE or rne for Random Number Engine rather URNG or
// urng.  See the comment in test_prf.ipp for why the URNG
// abbreviation is so confusing.

/* test_generator.ipp
 *
 * Copyright Steven Watanabe 2011
 * Distributed under the Boost Software License, Version 1.0. (See
 * accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * $Id: test_generator.ipp 71018 2011-04-05 21:27:52Z steven_watanabe $
 *
 */

#include "concepts.hpp"
#include <boost/random/seed_seq.hpp>

#define BOOST_TEST_MAIN
#include <boost/test/unit_test.hpp>

using boost::random::test::RandomNumberEngine;
BOOST_CONCEPT_ASSERT((RandomNumberEngine< BOOST_RANDOM_RNE >));

typedef BOOST_RANDOM_RNE::result_type result_type;
typedef boost::random::detail::seed_type<result_type>::type seed_type;

#ifdef BOOST_MSVC
#pragma warning(push)
#pragma warning(disable:4244)
#endif

template<class Converted, class RNE, class T>
void test_seed_conversion(RNE & rne, const T & t)
{
    Converted c = static_cast<Converted>(t);
    if(static_cast<T>(c) == t) {
        RNE rne2(c);
        std::ostringstream msg;
        msg << "Testing seed: type " << typeid(Converted).name() << ", value " << c;
        BOOST_CHECK_MESSAGE(rne == rne2, msg.str());
        rne2.seed(c);
        BOOST_CHECK_MESSAGE(rne == rne2, msg.str());
    }
}

#ifdef BOOST_MSVC
#pragma warning(pop)
#endif

void test_seed(seed_type value)
{
    BOOST_RANDOM_RNE rne(value);

    // integral types
    test_seed_conversion<char>(rne, value);
    test_seed_conversion<signed char>(rne, value);
    test_seed_conversion<unsigned char>(rne, value);
    test_seed_conversion<short>(rne, value);
    test_seed_conversion<unsigned short>(rne, value);
    test_seed_conversion<int>(rne, value);
    test_seed_conversion<unsigned int>(rne, value);
    test_seed_conversion<long>(rne, value);
    test_seed_conversion<unsigned long>(rne, value);
#if !defined(BOOST_NO_INT64_T)
    test_seed_conversion<boost::int64_t>(rne, value);
    test_seed_conversion<boost::uint64_t>(rne, value);
#endif

    // floating point types
    test_seed_conversion<float>(rne, value);
    test_seed_conversion<double>(rne, value);
    test_seed_conversion<long double>(rne, value);
}

BOOST_AUTO_TEST_CASE(test_default_seed)
{
    BOOST_RANDOM_RNE rne;
    BOOST_RANDOM_RNE rne2;
    rne2();
    BOOST_CHECK_NE(rne, rne2);
    rne2.seed();
    BOOST_CHECK_EQUAL(rne, rne2);
}

BOOST_AUTO_TEST_CASE(test_arithmetic_seed)
{
    test_seed(static_cast<seed_type>(0));
    test_seed(static_cast<seed_type>(127));
    test_seed(static_cast<seed_type>(539157235));
    test_seed(static_cast<seed_type>(~0u));
}
   
BOOST_AUTO_TEST_CASE(test_iterator_seed)
{
    const std::vector<int> v((std::max)(std::size_t(9999u), sizeof(BOOST_RANDOM_RNE) / 4), 0x41);
    std::vector<int>::const_iterator it = v.begin();
    std::vector<int>::const_iterator it_end = v.end();
    BOOST_RANDOM_RNE rne(it, it_end);
    BOOST_CHECK(it != v.begin());
    std::iterator_traits<std::vector<int>::const_iterator>::difference_type n_words = (it - v.begin());
    BOOST_CHECK_GT(n_words, 0);
    BOOST_CHECK_EQUAL(n_words, BOOST_RANDOM_SEED_WORDS);

    it = v.begin();
    BOOST_RANDOM_RNE rne2;
    rne2.seed(it, it_end);
    std::iterator_traits<std::vector<int>::const_iterator>::difference_type n_words2 = (it - v.begin());
    BOOST_CHECK_EQUAL(n_words, n_words2);
    BOOST_CHECK_EQUAL(rne, rne2);

    it = v.end();
    BOOST_CHECK_THROW(BOOST_RANDOM_RNE(it, it_end), std::invalid_argument);
    BOOST_CHECK_THROW(rne.seed(it, it_end), std::invalid_argument);

    if(n_words > 1) {
        it = v.end();
        --it;
        BOOST_CHECK_THROW(BOOST_RANDOM_RNE(it, it_end), std::invalid_argument);
        it = v.end();
        --it;
        BOOST_CHECK_THROW(rne.seed(it, it_end), std::invalid_argument);
    }
}

BOOST_AUTO_TEST_CASE(test_seed_seq_seed)
{
    boost::random::seed_seq q;
    BOOST_RANDOM_RNE rne(q);
    BOOST_RANDOM_RNE rne2;
    BOOST_CHECK_NE(rne, rne2);
    rne2.seed(q);
    BOOST_CHECK_EQUAL(rne, rne2);
}

template<class CharT>
void do_test_streaming(const BOOST_RANDOM_RNE& rne)
{
    BOOST_RANDOM_RNE rne2;
    std::basic_ostringstream<CharT> output;
    output << rne;
    BOOST_CHECK_NE(rne, rne2);
    // restore old state
    std::basic_istringstream<CharT> input(output.str());
    input >> rne2;
    BOOST_CHECK_EQUAL(rne, rne2);
}

BOOST_AUTO_TEST_CASE(test_streaming)
{
    BOOST_RANDOM_RNE rne;
    rne.discard(9307);
    do_test_streaming<char>(rne);
#if !defined(BOOST_NO_STD_WSTREAMBUF) && !defined(BOOST_NO_STD_WSTRING)
    do_test_streaming<wchar_t>(rne);
#endif
}

BOOST_AUTO_TEST_CASE(test_discard)
{
    BOOST_RANDOM_RNE rne;
    BOOST_RANDOM_RNE rne2;
    BOOST_CHECK_EQUAL(rne, rne2);
    for(int i = 0; i < 9307; ++i)
        rne();
    BOOST_CHECK_NE(rne, rne2);
    rne2.discard(9307);
    BOOST_CHECK_EQUAL(rne, rne2);
}

BOOST_AUTO_TEST_CASE(test_copy)
{
    BOOST_RANDOM_RNE rne;
    rne.discard(9307);
    {
        BOOST_RANDOM_RNE rne2 = rne;
        BOOST_CHECK_EQUAL(rne, rne2);
    }
    {
        BOOST_RANDOM_RNE rne2(rne);
        BOOST_CHECK_EQUAL(rne, rne2);
    }
    {
        BOOST_RANDOM_RNE rne2;
        rne2 = rne;
        BOOST_CHECK_EQUAL(rne, rne2);
    }
}

BOOST_AUTO_TEST_CASE(test_min_max)
{
    BOOST_RANDOM_RNE rne;
    for(int i = 0; i < 10000; ++i) {
        result_type value = rne();
        BOOST_CHECK_GE(value, (BOOST_RANDOM_RNE::min)());
        BOOST_CHECK_LE(value, (BOOST_RANDOM_RNE::max)());
    }
}

BOOST_AUTO_TEST_CASE(test_comparison)
{
    BOOST_RANDOM_RNE rne;
    BOOST_RANDOM_RNE rne2;
    BOOST_CHECK(rne == rne2);
    BOOST_CHECK(!(rne != rne2));
    rne();
    BOOST_CHECK(rne != rne2);
    BOOST_CHECK(!(rne == rne2));
}

BOOST_AUTO_TEST_CASE(validate)
{
    BOOST_RANDOM_RNE rne;
    for(int i = 0; i < 9999; ++i) {
        rne();
    }
    BOOST_CHECK_EQUAL(rne(), BOOST_RANDOM_VALIDATION_VALUE);
}

BOOST_AUTO_TEST_CASE(validate_seed_seq)
{
    boost::random::seed_seq seed;
    BOOST_RANDOM_RNE rne(seed);
    for(int i = 0; i < 9999; ++i) {
        rne();
    }
    BOOST_CHECK_EQUAL(rne(), BOOST_RANDOM_SEED_SEQ_VALIDATION_VALUE);
}

BOOST_AUTO_TEST_CASE(validate_iter)
{
    const std::vector<int> v((std::max)(std::size_t(9999u), sizeof(BOOST_RANDOM_RNE) / 4), 0x41);
    std::vector<int>::const_iterator it = v.begin();
    std::vector<int>::const_iterator it_end = v.end();
    BOOST_RANDOM_RNE rne(it, it_end);
    for(int i = 0; i < 9999; ++i) {
        rne();
    }
    BOOST_CHECK_EQUAL(rne(), BOOST_RANDOM_ITERATOR_VALIDATION_VALUE);
}

BOOST_AUTO_TEST_CASE(test_generate)
{
    BOOST_RANDOM_RNE rne;
    boost::uint32_t expected[] = BOOST_RANDOM_GENERATE_VALUES;
    static const std::size_t N = sizeof(expected)/sizeof(expected[0]); 
    boost::uint32_t actual[N];
    rne.generate(&actual[0], &actual[0] + N);
    BOOST_CHECK_EQUAL_COLLECTIONS(actual, actual + N, expected, expected + N);
}
