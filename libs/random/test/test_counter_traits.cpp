
// Copyright 2014, D. E. Shaw Research.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt )

#include <boost/random/detail/counter_traits.hpp>
#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_int_distribution.hpp>
#include <sstream>
#include "printlogarray.hpp"

#define BOOST_TEST_MAIN
#include <boost/test/included/unit_test.hpp>

using namespace boost;

namespace /*anon*/ {

// It's convenient to test the increment functions with
// 'random' increments.
random::mt19937_64 mt;

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
    typedef array<UintType, N> CtrType;
    typedef random::detail::counter_traits<CtrType> Traits;
    CtrType a;
    UintType fff = ~UintType(0);
    static const unsigned uintmaxbits = std::numeric_limits<uintmax_t>::digits;

    // Check that clr_highbits does something.
    a.fill(fff);
    CtrType b = a;
    BOOST_CHECK( Traits:: template clr_highbits<HighBits>(b) );
    BOOST_CHECK_NE(b, a);
    // And check that it doesn't do anything when the bits are
    // already clear.
    BOOST_CHECK( !Traits:: template clr_highbits<HighBits>(b) );

    // Check that if we increment from fff we overflow
    BOOST_CHECK_THROW( Traits::template incr<HighBits>(a), std::invalid_argument );
    BOOST_CHECK_THROW( Traits::template incr<HighBits>(a, 2), std::invalid_argument );
    BOOST_CHECK_THROW( Traits::template incr<HighBits>(a, fff), std::invalid_argument );

    // Check that incr twice is the same as incr(, 2)
    CtrType c = Traits::template incr<HighBits>(b);
    if( HighBits > 1 ){
        c = Traits::template incr<HighBits>(c);
        b = Traits::template incr<HighBits>(b, 2);
        BOOST_CHECK_EQUAL(b, c);
        BOOST_CHECK( Traits:: template clr_highbits<HighBits>(b) );
    }else{
        BOOST_CHECK_THROW(Traits::template incr<HighBits>(c), std::invalid_argument);
        BOOST_CHECK_THROW(Traits::template incr<HighBits>(b, 2), std::invalid_argument);
    }

    // incr(big/2); incr(big-big/2) is the same as incr(big)
    // and that overflow happens if we go past 'big'
    CtrType b0 = b;
    if( HighBits < uintmaxbits ){
        const unsigned HB = HighBits<uintmaxbits ? HighBits : 1;
        const uintmax_t hbmask = low_bits_mask_t<HB>::sig_bits;
        b = Traits::template incr<HighBits>(b0, hbmask);
        CtrType chalf = Traits::template incr<HighBits>(b0, hbmask/2);
        BOOST_CHECK_NE(b, chalf);
        uintmax_t remaining = hbmask - hbmask/2;
        c = Traits::template incr<HighBits>(chalf, remaining);
        BOOST_CHECK_EQUAL(b, c);
        BOOST_CHECK_THROW( Traits::template incr<HighBits>(c), std::invalid_argument );
        BOOST_CHECK_THROW( Traits::template incr<HighBits>(chalf, 1 + remaining), std::invalid_argument );
    }

    // Increment by geometrically increasing amounts, until we
    // overflow.  Initialize the counter with 0xff..., and then clear
    // min(uintmaxbits , HighBits) of the highest bits.  Thus, the
    // counter will overflow and the loop will terminate when we
    // accumulate increments of 2^min(uintmaxbits, HighBits).
    // Increments are chosen with a randomly increasing geometric
    // progression.  I.e., 
    //    delta = uinform_int_distribution(delta, (9/8) * delta )
    //
    // Since uintmaxbits is unlikely to be larger than 128,
    // the loop will terminate after a few hundred iterations.
    const unsigned clrbits = HighBits>uintmaxbits ? uintmaxbits : HighBits;
    a.fill(fff);
    Traits::template clr_highbits<clrbits>(a);
    b = a;
    c = a;
    uintmax_t delta = 1;
    int count = 0;
    bool threw = false;
    while(!threw){
        uintmax_t ub = delta + delta/8 + 1;
        count++;
        delta = random::uniform_int_distribution<uintmax_t>(delta, ub)(mt);
        // increment a by delta in one step
        try{
            a = Traits::template incr<HighBits>(a, delta);
        }catch(std::invalid_argument&){
            threw = 1;
        }

        // increment b by delta in two steps
        try{
            uintmax_t partial = random::uniform_int_distribution<uintmax_t>(0, delta)(mt);
            b = Traits::template incr<HighBits>(b, partial);
            b = Traits::template incr<HighBits>(b, delta-partial);
        }catch(std::invalid_argument&){
            BOOST_CHECK(threw);
        }

        // increment c by delta with (up to) 10 unit steps
        // plus one more to finish the job.
        try{
            unsigned j;
            for(j=0; j<10 && j<delta; ++j)
                c = Traits::template incr<HighBits>(c);
            c = Traits::template incr<HighBits>(c, delta-j);
        }catch(std::invalid_argument&){
            BOOST_CHECK(threw);
        }
        if(!threw){
            BOOST_CHECK_EQUAL(a, b);
            BOOST_CHECK_EQUAL(a, c);
        }
    }
    //std::cerr << "count : " << count << "\n";
}

template <typename UintType, unsigned N, typename result_type, unsigned w>
void test_nth(){
    BOOST_CHECK_EQUAL(1, 1);
}
} // namespace anon

BOOST_AUTO_TEST_CASE(core_varietypack)
{
    test_core<uint32_t, 4>();
    test_core<uint64_t, 4>();
    test_core<uint8_t, 16>();
    test_core<uint16_t, 8>();
}

BOOST_AUTO_TEST_CASE(highbit_varietypack)
{
    test_highbits<uint8_t, 2, 1>();
    test_highbits<uint8_t, 2, 2>();
    test_highbits<uint8_t, 2, 16>();

    test_highbits<uint8_t, 6, 1>();
    test_highbits<uint8_t, 6, 7>();
    test_highbits<uint8_t, 6, 8>();
    test_highbits<uint8_t, 6, 9>();
    test_highbits<uint8_t, 6, 15>();
    test_highbits<uint8_t, 6, 16>();
    test_highbits<uint8_t, 6, 17>();
    test_highbits<uint8_t, 6, 23>();
    test_highbits<uint8_t, 6, 24>();
    test_highbits<uint8_t, 6, 25>();
    test_highbits<uint8_t, 6, 31>();
    test_highbits<uint8_t, 6, 32>();
    test_highbits<uint8_t, 6, 33>();
    test_highbits<uint8_t, 6, 39>();
    test_highbits<uint8_t, 6, 40>();
    test_highbits<uint8_t, 6, 41>();
    
    test_highbits<uint32_t, 1, 1>();
    test_highbits<uint32_t, 1, 2>();
    test_highbits<uint32_t, 1, 16>();
    test_highbits<uint32_t, 1, 31>();
    test_highbits<uint32_t, 1, 32>();

    test_highbits<uint32_t, 2, 1>();
    test_highbits<uint32_t, 2, 2>();
    test_highbits<uint32_t, 2, 16>();
    test_highbits<uint32_t, 2, 31>();
    test_highbits<uint32_t, 2, 32>();
    test_highbits<uint32_t, 2, 33>();
    test_highbits<uint32_t, 2, 63>();
    test_highbits<uint32_t, 2, 64>();

    test_highbits<uint64_t, 4, 1>();
    test_highbits<uint64_t, 4, 2>();
    test_highbits<uint64_t, 4, 32>();
    test_highbits<uint64_t, 4, 63>();
    test_highbits<uint64_t, 4, 64>();
    test_highbits<uint64_t, 4, 65>();
    test_highbits<uint64_t, 4, 127>();
    test_highbits<uint64_t, 4, 128>();
    test_highbits<uint64_t, 4, 129>();
    test_highbits<uint64_t, 4, 191>();
    test_highbits<uint64_t, 4, 192>();
    test_highbits<uint64_t, 4, 193>();
    test_highbits<uint64_t, 4, 254>();
    test_highbits<uint64_t, 4, 255>();
    test_highbits<uint64_t, 4, 256>();
}

BOOST_AUTO_TEST_CASE(nth_varietypack)
{
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
        


