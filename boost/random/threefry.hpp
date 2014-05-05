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
#ifndef BOOST_RANDOM_THREEFRY_HPP
#define BOOST_RANDOM_THREEFRY_HPP

#include <boost/array.hpp>
#include <boost/static_assert.hpp>
#include <boost/cstdint.hpp>
#include <boost/limits.hpp>
#include <boost/random/detail/prf_common.hpp>
#include <boost/random/detail/rotl.hpp>
#include <boost/mpl/range_c.hpp>
#include <boost/mpl/for_each.hpp>

namespace boost{
namespace random{

// threefry_constants is an abstract template that will be
// specialized with the KS_PARITY and Rotation constants
// of the threefry generators.  These constants are carefully
// chosen to achieve good randomization.  
//  threefry_constants<2, uint32_t>
//  threefry_constants<2, uint64_t>
//  threefry_constants<4, uint32_t>
//  threefry_constants<4, uint64_t>
// The constants here are from Salmon et al <FIXME REF>.
//
// See Salmon et al, or Schneier's original work on Threefish <FIXME
// REF> for information about how the constants were obtained.
template <unsigned _N, typename Uint>
struct threefry_constants{
};

// 2x32 constants
template <>
struct threefry_constants<2, uint32_t>{
    static const uint32_t KS_PARITY = UINT32_C(0x1BD11BDA);
    static const unsigned Rotations[8];
};
const unsigned
threefry_constants<2, uint32_t>::Rotations[]  =
    {13, 15, 26, 6, 17, 29, 16, 24};

// 4x32 contants
template <>
struct threefry_constants<4, uint32_t>{
    static const uint32_t KS_PARITY = UINT32_C(0x1BD11BDA);
    static const unsigned Rotations0[8];
    static const unsigned Rotations1[8];
};
const unsigned
threefry_constants<4, uint32_t>::Rotations0[]  = 
    {10, 11, 13, 23, 6, 17, 25, 18};

const unsigned
threefry_constants<4, uint32_t>::Rotations1[]  = 
    {26, 21, 27, 5, 20, 11, 10, 20};

// 2x64 constants
template <>
struct threefry_constants<2, uint64_t>{
    static const uint64_t KS_PARITY = UINT64_C(0x1BD11BDAA9FC1A22);
    static const unsigned Rotations[8];
};
const unsigned
threefry_constants<2, uint64_t>::Rotations[]  =
    {16, 42, 12, 31, 16, 32, 24, 21};

// 4x64 constants
template <>
struct threefry_constants<4, uint64_t>{
    static const uint64_t KS_PARITY = UINT64_C(0x1BD11BDAA9FC1A22);
    static const unsigned Rotations0[8];
    static const unsigned Rotations1[8];
};
const unsigned
threefry_constants<4, uint64_t>::Rotations0[]  = 
    {14, 52, 23, 5, 25, 46, 58, 32};

const unsigned
threefry_constants<4, uint64_t>::Rotations1[]  = {
    16, 57, 40, 37, 33, 12, 22, 32};

template <unsigned N, typename Uint, unsigned R=20, typename Constants=threefry_constants<N, Uint> >
struct threefry{
    BOOST_STATIC_ASSERT( N==2 || N==4 );
    // should never be instantiated.
    // Only the specializations on N=2 and 4 are
    // permitted/implemented.
};

// specialize threefry<2, Uint, R>
template<typename Uint, unsigned R, typename Constants>
struct threefry<2, Uint, R, Constants> : public detail::prf_common<2, 2, 2, Uint>{
private:
    typedef detail::prf_common<2, 2, 2, Uint> common_type;
    typedef typename common_type::domain_type _ctr_type;
    typedef typename common_type::key_type _key_type;
    struct _roundapplyer{
        _ctr_type& c;
        Uint* ks;
        _roundapplyer(_ctr_type& _c, Uint* _ks) : c(_c), ks(_ks){}
        void operator()(unsigned r){
            c[0] += c[1]; c[1] = detail::rotl(c[1],Constants::Rotations[r%8]); c[1] ^= c[0];
            unsigned rplus = r+1;
            if((rplus&3)==0){
                unsigned r4 = rplus>>2;
                c[0] += ks[r4%3]; 
                c[1] += ks[(r4+1)%3] + r4;
            }
        }
    };
public:
    threefry() : common_type(){}
    threefry(_key_type _k) : common_type(_k){}

    template<class It>  threefry(It& first, It last) 
        : common_type(first, last)
    {}

    threefry(threefry& v) : common_type(static_cast<common_type &>(v)) {}
    threefry(const threefry& v) : common_type(static_cast<const common_type &>(v)){}

    threefry(Uint v) : common_type(v){}

    BOOST_RANDOM_DETAIL_SEED_SEQ_CONSTRUCTOR(threefry, SeedSeq, seq): 
        common_type(seq)
    { }

    BOOST_RANDOM_DETAIL_CONST_SEED_SEQ_CONSTRUCTOR(threefry, SeedSeq, seq)
        : common_type(seq)
    { }

    _ctr_type operator()(_ctr_type c){ 
        Uint ks[3];
        ks[2] = Constants::KS_PARITY;
        ks[0] = this->k[0]; ks[2] ^= this->k[0]; c[0] += this->k[0];
        ks[1] = this->k[1]; ks[2] ^= this->k[1]; c[1] += this->k[1];
#if 1
        _roundapplyer ra(c, ks);
        mpl::for_each<mpl::range_c<unsigned, 0, R> >( ra );
        return c;
#else
        for(unsigned r=0; r<R; ){
            c[0] += c[1]; c[1] = detail::rotl(c[1],Constants::Rotations[r%8]); c[1] ^= c[0];
            ++r;
            if((r&3)==0){
                unsigned r4 = r>>2;
                c[0] += ks[r4%3]; 
                c[1] += ks[(r4+1)%3] + r4;
            }
        }
        return c; 
#endif
    }
};

// specialize threefry<4, Uint, R>
template<typename Uint, unsigned R, typename Constants>
struct threefry<4, Uint, R, Constants> : public detail::prf_common<4, 4, 4, Uint>{
private:
    typedef detail::prf_common<4, 4, 4, Uint> common_type;
    typedef typename common_type::domain_type _ctr_type;
    typedef typename common_type::key_type _key_type;
    struct _roundapplyer{
        _ctr_type& c;
        Uint* ks;
        _roundapplyer(_ctr_type& _c, Uint* _ks) : c(_c), ks(_ks){}
        void operator()(unsigned r){
            if((r&1)==0){
                c[0] += c[1]; c[1] = detail::rotl(c[1],Constants::Rotations0[r%8]); c[1] ^= c[0];
                c[2] += c[3]; c[3] = detail::rotl(c[3],Constants::Rotations1[r%8]); c[3] ^= c[2];
            }else{
                c[0] += c[3]; c[3] = detail::rotl(c[3],Constants::Rotations0[r%8]); c[3] ^= c[0];
                c[2] += c[1]; c[1] = detail::rotl(c[1],Constants::Rotations1[r%8]); c[1] ^= c[2];
            }
            ++r;
            if((r&3)==0){
                unsigned r4 = r>>2;
                c[0] += ks[(r4+0)%5]; 
                c[1] += ks[(r4+1)%5];
                c[2] += ks[(r4+2)%5];
                c[3] += ks[(r4+3)%5] + r4;
            }
        }
    };

public:
    threefry() : common_type(){}
    threefry(_key_type _k) : common_type(_k){}

    template<class It> 
    threefry(It& first, It last) : common_type(first, last) {}

    threefry(threefry& v) : common_type(static_cast<common_type &>(v)) {}
    threefry(const threefry& v) : common_type(static_cast<const common_type &>(v)){}

    threefry(Uint v) : common_type(v){}

    BOOST_RANDOM_DETAIL_SEED_SEQ_CONSTRUCTOR(threefry, SeedSeq, seq)
        : common_type(seq)
    { }

    BOOST_RANDOM_DETAIL_CONST_SEED_SEQ_CONSTRUCTOR(threefry, SeedSeq, seq)
        : common_type(seq)
    { }

    _ctr_type operator()(_ctr_type c){ 
        Uint ks[5];
        ks[4] = Constants::KS_PARITY;
        ks[0] = this->k[0]; ks[4] ^= this->k[0]; c[0] += this->k[0];
        ks[1] = this->k[1]; ks[4] ^= this->k[1]; c[1] += this->k[1];
        ks[2] = this->k[2]; ks[4] ^= this->k[2]; c[2] += this->k[2];
        ks[3] = this->k[3]; ks[4] ^= this->k[3]; c[3] += this->k[3];

#if 1
        _roundapplyer ra(c, ks);
        mpl::for_each<mpl::range_c<unsigned, 0, R> >( ra );
        return c;
#else
        for(unsigned r=0; r<R; ){
            if((r&1)==0){
                c[0] += c[1]; c[1] = detail::rotl(c[1],Constants::Rotations0[r%8]); c[1] ^= c[0];
                c[2] += c[3]; c[3] = detail::rotl(c[3],Constants::Rotations1[r%8]); c[3] ^= c[2];
            }else{
                c[0] += c[3]; c[3] = detail::rotl(c[3],Constants::Rotations0[r%8]); c[3] ^= c[0];
                c[2] += c[1]; c[1] = detail::rotl(c[1],Constants::Rotations1[r%8]); c[1] ^= c[2];
            }
            ++r;
            if((r&3)==0){
                unsigned r4 = r>>2;
                c[0] += ks[(r4+0)%5]; 
                c[1] += ks[(r4+1)%5];
                c[2] += ks[(r4+2)%5];
                c[3] += ks[(r4+3)%5] + r4;
            }
        }
        return c; 
#endif
    }

};


} // namespace random
} // namespace boost

#endif // BOOST_RANDOM_THREEFRY_HPP
