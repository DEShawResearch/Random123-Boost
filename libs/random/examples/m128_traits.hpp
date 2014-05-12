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

namespace boost{
namespace random{
namespace detail{

template<>
struct counter_traits<__m128i>{
#if 0
    BOOST_RANDOM_DETAIL_EQUALITY_OPERATOR(m128prf_common, lhs, rhs){ 
        return 0xf==_mm_movemask_ps(_mm_castsi128_ps(_mm_cmpeq_epi32(lhs.k, rhs.k)));
    }

    BOOST_RANDOM_DETAIL_INEQUALITY_OPERATOR(m128prf_common)
#endif

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

    template <unsigned CtrBits>
    static void chk_highbits(__m128i c){
    }

    // key_from_{value,range,seedseq} - construct a key from the
    //   argument.  These functions make no effort to check or set the
    //   high bits of the key.  They should be passed through either
    //   chk_highkeybits or set_highkeybits before being passed on to
    //   a Prf constructor or Prf::setkey
    static __m128i key_from_value(uintmax_t v){
        __m128i ret = _mm_set_epi32(v, 0, 0, 0);
        return ret;
    }

    template <typename SeedSeq>
    static __m128i key_from_seedseq(SeedSeq& seq){
        __m128i ret;
        detail::seed_array_int<128>(seq, &ret);
        return ret;
    }

    template <typename It>
    static __m128i key_from_range(It& first, It last){
        __m128i ret;
        detail::fill_array_int<128>(first, last, &ret);
        return ret;
    }

    // We avoid collisions between engines with different CtrBits by
    // embedding the value CtrBits-1 in the high few bits of the last
    // element of Prf's key.  If we didn't do this, it would be too
    // easy for counter_based_engine<Prf, N> and
    // counter_based_engine<Prf, M> to "collide", producing
    // overlapping streams.
    //
    // When we accept a key from the user, e.g., seed(arithmetic) or
    // seed(__m128i), it is an error if the specified value has any
    // high bits set.  On the other hand, it's not an error if
    // seed_seq.generate() sets those bits -- we just ignore them.

    // chk_highkeybits - first check that the high CtrBitsBits of k
    //  are 0.  If they're not, throw an out_of_range exception.  If
    //  they are, then call set_highkeybits.
    template<unsigned CtrBits, unsigned CtrBitsBits>
    static __m128i chk_highkeybits(__m128i k){
        BOOST_STATIC_ASSERT(CtrBitsBits <= 32);
        BOOST_STATIC_CONSTANT(uint32_t, CtrBitsMask = low_bits_mask_t<32-CtrBitsBits>::sig_bits );
        uint32_t k3 = _mm_extract_epi32(k, 3);
        if( k3 & ~CtrBitsMask ){
            //std::cerr << "k[Nkey-1] = " << std::hex <<  k[Prf::Nkey-1] << std::dec << std::endl;
            BOOST_THROW_EXCEPTION(std::domain_error("high bits of key are reserved for internal use by counter_based_engine"));
        }
        return set_highkeybits<CtrBits, CtrBitsBits>(k);
    }

    // set_highkeybits - set the high CtrBitsBits of k to CtrBits-1,
    //  regardless of their original contents.
    template<unsigned CtrBits, unsigned CtrBitsBits>
    static __m128i set_highkeybits(__m128i k){
        BOOST_STATIC_ASSERT(CtrBitsBits <= 32);
        BOOST_STATIC_CONSTANT(uint32_t, CtrBitsMask = low_bits_mask_t<32-CtrBitsBits>::sig_bits );
        __m128i mask = _mm_set_epi32(CtrBitsMask, 0, 0, 0);
        k = _mm_and_si128(k, mask);
        return _mm_or_si128(k, _mm_set_epi32(CtrBits-1, 0, 0, 0));
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
        BOOST_STATIC_ASSERT(w==32);
        switch(n){
        case 0: return _mm_extract_epi32(v, 0);
        case 1: return _mm_extract_epi32(v, 1);
        case 2: return _mm_extract_epi32(v, 2);
        case 3: return _mm_extract_epi32(v, 3);
        }
        BOOST_THROW_EXCEPTION(std::out_of_range("nth_result(n)"));
    }
};

} // namespace detail
} // namespace random
} // namespace boost

#endif
