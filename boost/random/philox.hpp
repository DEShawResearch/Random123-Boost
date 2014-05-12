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
#ifndef BOOST_RANDOM_PHILOX_HPP
#define BOOST_RANDOM_PHILOX_HPP
#include <boost/array.hpp>
#include <boost/static_assert.hpp>
#include <boost/cstdint.hpp>
#include <boost/limits.hpp>
#include <boost/random/detail/mulhilo.hpp>
#include <boost/random/detail/prf_common.hpp>
#include <boost/mpl/for_each.hpp>
#include <boost/mpl/range_c.hpp>

namespace boost{
namespace random{

template <unsigned _N, typename Uint>
struct philox_constants{
    // specializations will hold the Mutlipliers: M0, M1
    // and the Weyl constants:  W0, W1
};

template <>
struct philox_constants<2, uint64_t>{
    static const uint64_t M0 = UINT64_C(0xD2B74407B1CE6E93);
    static const uint64_t W0 = UINT64_C(0x9E3779B97F4A7C15);
};

template <>
struct philox_constants<2, uint32_t>{
    static const uint32_t M0 = UINT32_C(0xD256D193);
    static const uint32_t W0 = UINT32_C(0x9E3779B9);
};

template <>
struct philox_constants<4, uint64_t>{
    static const uint64_t M0 = UINT64_C(0xD2E7470EE14C6C93);
    static const uint64_t M1 = UINT64_C(0xCA5A826395121157);
    static const uint64_t W0 = UINT64_C(0x9E3779B97F4A7C15);  /* golden ratio */
    static const uint64_t W1 = UINT64_C(0xBB67AE8584CAA73B);  /* sqrt(3)-1 */
};

template <>
struct philox_constants<4, uint32_t>{
    static const uint32_t M0 = UINT32_C(0xD2511F53);
    static const uint32_t M1 = UINT32_C(0xCD9E8D57);
    static const uint32_t W0 = UINT64_C(0x9E3779B9);  /* golden ratio */
    static const uint32_t W1 = UINT64_C(0xBB67AE85);  /* sqrt(3)-1 */
};

template <unsigned N, typename Uint, unsigned R=10, typename Constants = philox_constants<N, Uint> >
struct philox{
    BOOST_STATIC_ASSERT( N%2 == 0 );
};

template <typename Uint, unsigned R, typename Constants>
struct philox<2, Uint, R, Constants> {
    typedef array<Uint, 2> domain_type;
    typedef array<Uint, 2> range_type;
    typedef array<Uint, 1> key_type ;

    philox() : k(){}
    philox(key_type _k) : k(_k) {}
    philox(const philox& v) : k(v.k){}

    void setkey(key_type _k){
        k = _k;
    }

    key_type getkey() const{
        return k;
    }

    bool operator==(const philox& rhs) const{
        return k == rhs.k;
    }

    bool operator!=(const philox& rhs) const{
        return k != rhs.k;
    }

    range_type operator()(domain_type c){
        key_type kcopy = k;
#if 0   // using mpl to unroll the loop doesn't seem to help much.
        _roundapplyer ra(c, kcopy);
        mpl::for_each<mpl::range_c<unsigned, 0, R> >(ra);
#else
        for(unsigned r=0; r<R; ++r)
            round(c, kcopy);
#endif
        return c;
    }
protected:
    static inline void round(domain_type& ctr, key_type& key){
        Uint hi;
        Uint lo = detail::mulhilo(Constants::M0, ctr[0], hi);
        domain_type out = {{hi^key[0]^ctr[1], lo}};
        ctr = out;
        key[0] += Constants::W0;
    }

    struct _roundapplyer{
        domain_type& c;
        key_type& k;
        _roundapplyer(domain_type& _c, key_type& _k): c(_c), k(_k){}
        void operator()(unsigned){ round(c, k); }
    };
    key_type k;
};

template<typename Uint, unsigned R, typename Constants>
struct philox<4, Uint, R, Constants> {
    typedef array<Uint, 4> domain_type;
    typedef array<Uint, 4> range_type;
    typedef array<Uint, 2> key_type ;
public:

    philox() : k(){}
    philox(key_type _k) : k(_k) {}
    philox(const philox& v) : k(v.k){}

    void setkey(key_type _k){
        k = _k;
    }

    key_type getkey() const{
        return k;
    }

    bool operator==(const philox& rhs) const{
        return k == rhs.k;
    }

    bool operator!=(const philox& rhs) const{
        return k != rhs.k;
    }

    range_type operator()(domain_type c){
        key_type kcopy = k;
#if 0   // using mpl to unroll the loop doesn't seem to help much.
        _roundapplyer ra(c, kcopy);
        mpl::for_each<mpl::range_c<unsigned, 0, R> >(ra);
#else
        for(unsigned r=0; r<R; ++r)
            round(c, kcopy);
#endif
        return c;
    }

protected:
    static inline void round(domain_type& ctr, key_type& key){
        Uint hi0;
        Uint hi1;
        Uint lo0 = detail::mulhilo(Constants::M0, ctr[0], hi0);
        Uint lo1 = detail::mulhilo(Constants::M1, ctr[2], hi1);
        domain_type out = {{hi1^ctr[1]^key[0], lo1,
                                      hi0^ctr[3]^key[1], lo0}};
        ctr = out;
        key[0] += Constants::W0;
        key[1] += Constants::W1;
    }

    struct _roundapplyer{
        domain_type& c;
        key_type& k;
        _roundapplyer(domain_type& _c, key_type& _k): c(_c), k(_k){}
        void operator()(unsigned){ round(c, k); }
    };

    key_type k;
};

} // namespace random
} // namespace boost

#endif // BOOST_RANDOM_PHILOX_HPP
