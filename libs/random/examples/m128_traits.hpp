
// Copyright 2010-2014, D. E. Shaw Research.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt )

#ifndef BOOST_RANDOM_M128COMMON_PRF_HPP
#define BOOST_RANDOM_M128COMMON_PRF_HPP

#include <x86intrin.h>
#include <boost/throw_exception.hpp>
#include <boost/random/detail/operators.hpp>
#include <boost/random/detail/seed.hpp>
#include <boost/random/detail/seed_impl.hpp>
#include <boost/random/detail/counter_traits.hpp>
#include <boost/limits.hpp>

#include <stdexcept>
#if !defined(BOOST_NO_CXX11_HDR_INITIALIZER_LIST)
#include <initializer_list>
#endif

namespace boost{
namespace random{
namespace detail{

// This is a SKETCH of a possible specialization of the
// counter_traits<__m128i> for the Intel SSE __m128i data type.  It's
// only complete enough to demonstrate a hardware-specific
// implementation of the ARS generator in ars_example.cpp.  IT IS NOT
// WELL TESTED AND HAS KNOWN DEFECTS (e.g., overflows and carries are
// not detected in incr()!!).  

template<>
struct counter_traits<__m128i>{
    BOOST_STATIC_CONSTANT(unsigned, Nbits = 128u);

    template<class CharT, class Traits>
    static std::basic_ostream<CharT, Traits>& insert(std::basic_ostream<CharT, Traits>& os, const __m128i& f){
        union{
            uint64_t u64[2];
            __m128i m;
        }u;
        _mm_storeu_si128(&u.m, f);
        return os << u.u64[0] << " " << u.u64[1];
    }

    template<class CharT, class Traits>
    static std::basic_istream<CharT, Traits>& extract(std::basic_istream<CharT, Traits>& is, __m128i& f){
        uint64_t u64[2];
        is >> u64[0] >> u64[1];
        f = _mm_set_epi64x(u64[1], u64[0]);
        return is;
    }

    static bool is_equal(const __m128i& rhs, const __m128i& lhs){
        return 0xf==_mm_movemask_ps(_mm_castsi128_ps(_mm_cmpeq_epi32(lhs, rhs)));
    }

    static __m128i make_counter(){
        return _mm_setzero_si128();
    }

    static __m128i make_counter(uintmax_t v){
        __m128i ret = _mm_set_epi32(v, 0, 0, 0);
        return ret;
    }

    template <typename It>
    static __m128i make_counter(It& first, It last){
        union{
            uint32_t a[4];
            __m128i m;
        } u;
        detail::fill_array_int<32>(first, last, u.a);
        return u.m;
    }

    template <typename It>
    static __m128i make_counter(const It& first, It last){
        It _first = first;
        return make_counter(_first, last);
    }

#if !defined(BOOST_NO_CXX11_HDR_INITIALIZER_LIST)
    template <typename V>
    static __m128i make_counter(std::initializer_list<V> il){
        return make_counter(il.begin(), il.end());
    }
#endif

private:
    template <typename SeedSeq>
    static __m128i _make_counter_from_seed_seq(SeedSeq& seq){
        union{
            uint32_t a[4];
            __m128i m;
        } u;
        detail::seed_array_int<32>(seq, u.a);
        return u.m;
    }
    template <typename Tt, typename Prf, unsigned CtrBits, unsigned w, typename Dtraits, typename Rtraits, typename Ktrats>
    friend class counter_based_engine;

public:

    template<unsigned CtrBitsBits>
    static bool clr_highbits(__m128i& k){
        BOOST_STATIC_ASSERT(CtrBitsBits <= 32);
        BOOST_STATIC_CONSTANT(uint32_t, CtrBitsMask = low_bits_mask_t<32-CtrBitsBits>::sig_bits );
        uint32_t k3 = _mm_extract_epi32(k, 3);
        bool ret = !!( k3 & ~CtrBitsMask );
        k3 &= CtrBitsMask;
        k = _mm_insert_epi32(k, k3, 3);
        return ret;
    }

    template <unsigned CtrBits>
    static __m128i incr(__m128i c){
        BOOST_STATIC_ASSERT(CtrBits <= 128);
        BOOST_STATIC_ASSERT(CtrBits > 0);
        __m128i zeroone = _mm_set_epi64x(UINT64_C(0), UINT64_C(1));
        c = _mm_add_epi64(c, zeroone);
        return c;
    }

    template <unsigned CtrBits>
    static __m128i incr(__m128i d, boost::uintmax_t n){
        BOOST_STATIC_ASSERT(CtrBits <= 128);
        BOOST_STATIC_ASSERT(CtrBits > 0);
        __m128i incr128 = _mm_set_epi64x(0, n);
        d = _mm_add_epi64(d, incr128);
        return d;
    }

    template <typename result_type, unsigned w>
    static result_type nth_result(unsigned n, __m128i v){
        BOOST_STATIC_ASSERT(w==32 || w==64);
        switch(w){
        case 32:
            switch(n){
            case 0: return _mm_extract_epi32(v, 0);
            case 1: return _mm_extract_epi32(v, 1);
            case 2: return _mm_extract_epi32(v, 2);
            case 3: return _mm_extract_epi32(v, 3);
            }
            BOOST_THROW_EXCEPTION(std::out_of_range("nth_result(n)"));
        case 64:
            switch(n){
            case 0: return _mm_extract_epi64(v, 0);
            case 1: return _mm_extract_epi64(v, 1);
            }
            BOOST_THROW_EXCEPTION(std::out_of_range("nth_result(n)"));
        }
    }
};

} // namespace detail
} // namespace random
} // namespace boost

#endif
