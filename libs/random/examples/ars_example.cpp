
// Copyright 2014, D. E. Shaw Research.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt )

#include "m128_traits.hpp"
#include <boost/random/counter_based_engine.hpp>
#include <boost/timer.hpp>
#include <iostream>

using namespace boost::random;

// This is the ARS pseudo-random function from Salmon et al. 2011,
// implemented using the AESNI _mm_aesenc_si128 hardware intrinsics.
// On Intel hardware with the AESNI instructions, it's the fastest
// Crush-resistant RNG we know of.  It runs at 2.7nsec/uint64 
// on a 3.07GHz Xeon, i.e., just about 1 cycle-per-byte.
//
// This is in examples/ rather than boost/random because these
// intrinsics (_mm_aesenc_si128) and data types (__m128i) are too
// machine/compiler-specific for the main 'boost' tree.  Nevertheless,
// this example demonstrates that it's possible (and not that hard) to
// create hardware-dependent Prfs  that can be used by the standard
// counter_based_engine.
template <unsigned R=7>
struct ars_prf{
    typedef __m128i domain_type;
    typedef __m128i range_type;
    typedef __m128i key_type ;

    range_type operator()(domain_type v){
        __m128i kweyl = _mm_set_epi64x(UINT64_C(0xBB67AE8584CAA73B), /* sqrt(3) - 1.0 */
                                       UINT64_C(0x9E3779B97F4A7C15)); /* golden ratio */
        __m128i kk = k;
        if( R>1 ){
            kk = _mm_add_epi64(kk, kweyl);
            v = _mm_aesenc_si128(v, kk);
        }
        if( R>2 ){
            kk = _mm_add_epi64(kk, kweyl);
            v = _mm_aesenc_si128(v, kk);
        }
        if( R>3 ){
            kk = _mm_add_epi64(kk, kweyl);
            v = _mm_aesenc_si128(v, kk);
        }
        if( R>4 ){
            kk = _mm_add_epi64(kk, kweyl);
            v = _mm_aesenc_si128(v, kk);
        }
        if( R>5 ){
            kk = _mm_add_epi64(kk, kweyl);
            v = _mm_aesenc_si128(v, kk);
        }
        if( R>6 ){
            kk = _mm_add_epi64(kk, kweyl);
            v = _mm_aesenc_si128(v, kk);
        }
        if( R>7 ){
            kk = _mm_add_epi64(kk, kweyl);
            v = _mm_aesenc_si128(v, kk);
        }
        if( R>8 ){
            kk = _mm_add_epi64(kk, kweyl);
            v = _mm_aesenc_si128(v, kk);
        }
        if( R>9 ){
            kk = _mm_add_epi64(kk, kweyl);
            v = _mm_aesenc_si128(v, kk);
        }
        kk = _mm_add_epi64(kk, kweyl);
        v = _mm_aesenclast_si128(v, kk);
        return v;
    }

    ars_prf(key_type _k) : k(_k){
        //std::cerr << "ars_prf(key_type)\n";
    }

    ars_prf() { k = key_type(); }

    ars_prf(const ars_prf& v) : k(v.k) {
        //std::cerr << "ars_prf(const ars_prf&)\n";
    }

    void setkey(key_type _k){ k = _k; }
    key_type getkey() const { return k; }

    bool operator==(const ars_prf& rhs) const{
        return boost::random::detail::counter_traits<__m128i>::is_equal(k, rhs.k);
    }

    bool operator!=(const ars_prf& rhs) const{
        return !(k == rhs.k);
    }
protected:
    key_type k;
};

static const double cpu_frequency = 3.07e9;

void show_elapsed(double end, int iter, const std::string & name)
{
  double usec = end/iter*1e6;
  double cycles = usec * cpu_frequency/1e6;
  std::cout << name << ": " 
            << usec*1e3 << " nsec/loop = " 
            << cycles << " CPU cycles"
            << std::endl;
}

int main(int argc, char **argv){
    counter_based_engine<uint32_t, ars_prf<5> > e;
    std::cout << std::hex;
    std::cout << e() << "\n";
    std::cout << e() << "\n";
    std::cout << e() << "\n";
    std::cout << e() << "\n";
    std::cout << e() << "\n";
    std::cout << e() << "\n";
    std::cout << e() << "\n";
    std::cout << e() << "\n";

    std::cout << std::dec;
    std::cout << "state of the generator: " << e << "\n";

    counter_based_engine<uint32_t, ars_prf<5> > f;
    BOOST_ASSERT(e != f);

    // Aggressive parameters:  5 rounds is "Crush-resistant"
    // but with no safety margin.
    // 64-bit output minimizes loop-overhead.
    counter_based_engine<uint64_t, ars_prf<5> > g;
    uint32_t ss[4] = {1, 2, 3, 4};
    uint32_t *ss0 = &ss[0];
    g.seed(ss0, &ss[4]);
    size_t N = 100000000;
    boost::timer t;
    uint64_t tmp = 0;
    for(size_t i=0; i<N; ++i){
        tmp ^= g();
    }
    show_elapsed(t.elapsed(), N, "ars<5/64>");
    if(tmp == 0)
        std::cout << "tmp==0.  That's odd...";

    std::cout << "state of the generator: " << g << "\n";
    return 0;
}
