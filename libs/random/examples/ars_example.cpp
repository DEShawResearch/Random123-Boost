#include "m128_traits.hpp"
#include <boost/random/counter_based_engine.hpp>
#include <iostream>

using namespace boost::random;

struct ars_prf{
    typedef __m128i domain_type;
    typedef __m128i range_type;
    typedef __m128i key_type ;

    range_type operator()(domain_type v){
        return v;
#if 0
        unsigned Nrounds = 7;
        if( Nrounds>1 ){
            kk = _mm_add_epi64(kk, kweyl);
            v = _mm_aesenc_si128(v, kk);
        }
        if( Nrounds>2 ){
            kk = _mm_add_epi64(kk, kweyl);
            v = _mm_aesenc_si128(v, kk);
        }
        if( Nrounds>3 ){
            kk = _mm_add_epi64(kk, kweyl);
            v = _mm_aesenc_si128(v, kk);
        }
        if( Nrounds>4 ){
            kk = _mm_add_epi64(kk, kweyl);
            v = _mm_aesenc_si128(v, kk);
        }
        if( Nrounds>5 ){
            kk = _mm_add_epi64(kk, kweyl);
            v = _mm_aesenc_si128(v, kk);
        }
        if( Nrounds>6 ){
            kk = _mm_add_epi64(kk, kweyl);
            v = _mm_aesenc_si128(v, kk);
        }
        if( Nrounds>7 ){
            kk = _mm_add_epi64(kk, kweyl);
            v = _mm_aesenc_si128(v, kk);
        }
        if( Nrounds>8 ){
            kk = _mm_add_epi64(kk, kweyl);
            v = _mm_aesenc_si128(v, kk);
        }
        if( Nrounds>9 ){
            kk = _mm_add_epi64(kk, kweyl);
            v = _mm_aesenc_si128(v, kk);
        }
        kk = _mm_add_epi64(kk, kweyl);
        v = _mm_aesenclast_si128(v, kk);
        ret.v[0].m = v;
        return ret;
        return d;
#endif
    }

    m128prf_common(key_type _k) : k(_k){
        //std::cerr << "m128prf_common(key_type)\n";
    }

    m128prf_common() { k = key_type(); }

    m128prf_common(const m128prf_common& v) : k(v.k) {
        //std::cerr << "m128prf_common(const m128prf_common&)\n";
    }

    void setkey(key_type _k){ k = _k; }
    key_type getkey() const { return k; }

protected:
    key_type k;
};

int main(int argc, char **argv){
    counter_based_engine<detail::ars, 64, uint32_t> e;
    std::cout << e() << "\n";
    std::cout << e() << "\n";
    std::cout << e() << "\n";
    std::cout << e() << "\n";
    std::cout << e() << "\n";
    std::cout << e() << "\n";
    std::cout << e() << "\n";
    std::cout << e() << "\n";

    std::cout << e << "\n";
    return 0;
}
