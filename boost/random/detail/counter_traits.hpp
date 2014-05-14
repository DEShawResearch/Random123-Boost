/** @page LICENSE
Copyright 2014, D. E. Shaw Research.
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
#ifndef BOOST_RANDOM_DETAIL_COUNTER_TRAITS_HPP
#define BOOST_RANDOM_DETAIL_COUNTER_TRAITS_HPP

#include <boost/throw_exception.hpp>
#include <boost/random/detail/seed_impl.hpp>
#include <boost/array.hpp>
#include <boost/limits.hpp>
#include <boost/type_traits/is_same.hpp>
#include <boost/type_traits/is_integral.hpp>
#include <boost/type_traits/is_class.hpp>
#include <boost/utility/enable_if.hpp>
#include <boost/cstdint.hpp>
#include <boost/mpl/has_xxx.hpp>

namespace boost{
namespace random{
namespace detail{

// counter_traits - static functions that allow counter_based_engine
//  to "do its thing" with the data types used by a Prf.  The idea is
//  that counter_based_engine will be declared something like:
//
//template<typename Prf, 
//         unsigned CtrBits = static_unsigned_min<64u, detail::counter_traits<typename Prf::domain_type>::Nbits/2>::value,
//         typename UintType = typename detail::counter_traits<typename Prf::range_type>::preferred_result_type,
//         unsigned w = std::numeric_limits<UintType>::digits,
//         typename DomainTraits = detail::counter_traits<typename Prf::domain_type>,
//         typename RangeTraits = detail::counter_traits<typename Prf::range_type>,
//         typename KeyTraits = detail::counter_traits<typename Prf::key_type>
//>
// struct counter_based_engine{...};
//
// And it will use the static functions provided by the traits classes
// to increment counters in the domain, extract results from the
// range, etc.
//
// In addition, we provide a specialization that matches any
//   counter_based_engine< boost::array<T, N> >;
// which satisfies the requirements for threefry and philox.
//
// To introduce a new Prf that works with different types, one would
// have to provide specializations of counter_based_engine for the new
// Prf's domain, range and key types.

template <typename CtrType, typename Enable = void>
struct counter_traits{
    BOOST_STATIC_CONSTANT(unsigned, Nbits = 0);

    template<class CharT, class Traits>
    static std::basic_ostream<CharT, Traits>& insert(std::basic_ostream<CharT, Traits>& os, const CtrType& a);

    template<class CharT, class Traits>
    static std::basic_istream<CharT, Traits>& extract(std::basic_istream<CharT, Traits>& is, CtrType& a);

    static bool is_equal(const CtrType& rhs, const CtrType& lhs);

    // key_from_{value,range,seedseq} - construct a key from the
    //   argument.  These functions make no effort to check or set the
    //   high bits of the key.  They should be passed through either
    //   chk_highkeybits or set_highkeybits before being passed on to
    //   a Prf constructor or Prf::setkey
    static CtrType key_from_value(uintmax_t v);

    template <typename SeedSeq>
    static CtrType key_from_seedseq(SeedSeq& seq);

    template <typename It>
    static CtrType key_from_range(It& first, It last);

    // clr_highbits - clear the HighBits of c, return true if the
    //  bits of c were not clear on input.
    template <unsigned HighBits>
    static bool clr_highbits(CtrType& c);

    template <unsigned HighBits>
    static CtrType incr(CtrType d);

    template <unsigned HighBits>
    static CtrType incr(CtrType d, boost::uintmax_t n);

    template <typename result_type, unsigned w>
    static result_type nth_result(unsigned n, CtrType v);
};

// We want a counter_traits type, so we can say:
//  counter_traits<Prf::domain_type>::incr(d)
// in counter_based_engine.  

// How can we write a specialization that works for any 
// CtrType that happens to be a boost::array<T,N>.

// 1 - Create a predicate class tells us whether we're
// looking at is going to supply all the methods and
// members needed by array_counter_traits.  We
// approximate that with a test for is_class
// and a data member named 'elems',  See:
//    http://en.wikibooks.org/wiki/More_C%2B%2B_Idioms/Member_Detector
// Isn't there an Boost.MPL-way to do this??

template <typename T, bool IS_CLASS>
struct class_has_elems{
    struct Fallback { int elems; };
    struct Derived : T, Fallback {};
    template<typename U, U> struct Check;
    typedef char ArrayOfOne[1];
    typedef char ArrayOfTwo[2];
    template<typename U>
    static ArrayOfOne& func(Check<int Fallback::*, &U::elems> *);
    
    template<typename U>
    static ArrayOfTwo& func(...);

public:
    static const bool value = sizeof( func<Derived>(0) ) == 2 ;
};
template <typename T>
struct class_has_elems<T, false>{
    static const bool value = false;
};

template <typename T>
struct isBoostArray{
    static const bool value = class_has_elems<T, is_class<T>::value>::value;
};

// 2 - an array_counter_traits that "works" provides the traits
//    for a boost::array<T, N>

template <typename T, unsigned N>
struct array_counter_traits{
protected:
    typedef boost::array<T, N> a_type;
    BOOST_STATIC_CONSTANT(unsigned, value_bits = std::numeric_limits<T>::digits);
public:
    BOOST_STATIC_CONSTANT(unsigned, Nbits = N*value_bits);

    template<class CharT, class Traits>
    static std::basic_ostream<CharT, Traits>& insert(std::basic_ostream<CharT, Traits>& os, const a_type& a){
        for(unsigned i=0; i<N; ++i)
            os << a[i] << ' ';
        return os;
    }

    template<class CharT, class Traits>
    static std::basic_istream<CharT, Traits>& extract(std::basic_istream<CharT, Traits>& is, a_type& a){
        for(unsigned i=0; i<N; ++i)
            is >> a[i] >> std::ws;
        return is;
    }

    static bool is_equal(const a_type& rhs, const a_type& lhs){
        return rhs == lhs;
    }

    // key_from_{value,range,seedseq} - construct a key from the
    //   argument.  These functions make no effort to check or set the
    //   high bits of the key.  They should be passed through either
    //   chk_highkeybits or set_highkeybits before being passed on to
    //   a Prf constructor or Prf::setkey
    static a_type key_from_value(uintmax_t v){
        a_type ret = {{T(v)}};
        return ret;
    }

    template <typename SeedSeq>
    static a_type key_from_seedseq(SeedSeq& seq){
        a_type ret;
        detail::seed_array_int<Nbits>(seq, ret.elems);
        return ret;
    }

    template <typename It>
    static a_type key_from_range(It& first, It last){
        a_type ret;
        detail::fill_array_int<Nbits>(first, last, ret.elems);
        return ret;
    }

    template <unsigned HighBits>
    static bool clr_highbits(a_type& c){
        BOOST_STATIC_CONSTANT(unsigned, incr_idx = (Nbits - HighBits)/value_bits);
        BOOST_STATIC_CONSTANT(T, incr_stride = T(1)<<((Nbits - HighBits)%value_bits));
        BOOST_STATIC_CONSTANT(T, Mask = low_bits_mask_t<(Nbits - HighBits)%value_bits>::sig_bits);
        bool bad = false;
        typename a_type::iterator p = c.begin() + incr_idx;
        if( *p >= incr_stride )
            bad = true;
        *p++ &= Mask;
        for( ; p != c.end(); ++p){
            if(*p) bad = true;
            *p = 0;
        }
        return bad;
    }

    template <unsigned HighBits>
    static a_type incr(a_type d){
        BOOST_STATIC_ASSERT(HighBits <= Nbits);
        BOOST_STATIC_ASSERT(HighBits > 0);
        BOOST_STATIC_CONSTANT(T, incr_stride = T(1)<<((Nbits - HighBits)%value_bits));
        BOOST_STATIC_CONSTANT(unsigned, FullCtrWords = HighBits/value_bits);
        typename a_type::reverse_iterator p = d.rbegin();
        for(unsigned i=0; i<FullCtrWords; ++i){
            *p += 1;
            if(*p++)
                return d;
        }
        *p += incr_stride;
        if(*p < incr_stride)
            BOOST_THROW_EXCEPTION(std::invalid_argument("counter_traits::incr(): ran out of counters"));
        return d;
    }

    template <unsigned HighBits>
    static a_type incr(a_type d, boost::uintmax_t n){
        BOOST_STATIC_ASSERT(HighBits <= Nbits);
        BOOST_STATIC_ASSERT(HighBits > 0);
        BOOST_STATIC_CONSTANT(T, incr_stride = T(1)<<((Nbits - HighBits)%value_bits));
        BOOST_STATIC_CONSTANT(unsigned, FullCtrWords = HighBits/value_bits);
        typename a_type::reverse_iterator p = d.rbegin();
        for(unsigned i=0; i<FullCtrWords; ++i){
            *p += T(n);
            bool carry = (*p++ < T(n));
            n >>= value_bits-1; n>>=1;
            if(carry)
                n++;
            if(n==0)
                return d;
        }
        // ??? - does this correctly detect all the overflow cases ???
        if(n==0)
            return d;
        if(n*incr_stride < incr_stride)
            BOOST_THROW_EXCEPTION(std::invalid_argument("counter_traits::incr(): ran out of counters"));
        *p += n*incr_stride;
        if(*p < incr_stride)
            BOOST_THROW_EXCEPTION(std::invalid_argument("counter_traits::incr(): ran out of counters"));
        return d;
    }

    template <typename result_type, unsigned w>
    static result_type nth_result(unsigned n, a_type v){
        result_type r;
        if( w == value_bits ){
            return v[n];
        }else if( w < value_bits ){
            const unsigned  results_per_rangeval = value_bits/w;
            const unsigned idx = n/results_per_rangeval;
            const unsigned shift = (n%results_per_rangeval)*w;
            const typename a_type::value_type r = v[ idx ];
            const result_type wmask = low_bits_mask_t<w>::sig_bits;
            return (r >> shift)&wmask;
        }else{
            unsigned idx = (n*w)/Nbits;
            r = v[idx++];
            for(int i=1; i<w/value_bits; ++i){
                r = (r<<w) | v[idx++];
            }
            return r;
        }
    }
};

// 3 - And finally, we can specialize counter_traits, publicly inheriting
// from array_counter_traits if CtrType really is a Boost.Array.
template <typename CtrType>
struct counter_traits<CtrType, typename enable_if_c<isBoostArray<CtrType>::value>::type >
    : public array_counter_traits<typename CtrType::value_type, CtrType::static_size>
{};

} // namespace detail
} // namespace random
} // namespace boost

#endif
