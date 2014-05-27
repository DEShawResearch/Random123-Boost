
// Copyright 2014, D. E. Shaw Research.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt )

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

#if !defined(BOOST_NO_CXX11_HDR_INITIALIZER_LIST)
#include <initializer_list>
#endif

namespace boost{
namespace random{
template <typename, typename, unsigned, unsigned, typename, typename, typename>
struct counter_based_engine;
namespace detail{

// counter_traits - static functions that allow counter_based_engine
//  to "do its thing" with the data types used by a Prf.  The idea is
//  that counter_based_engine will be declared something like:
//
//template<typename UintType,
//         typename Prf, 
//         unsigned CtrBits = static_unsigned_min<64u, detail::counter_traits<typename Prf::domain_type>::Nbits/2>::value,
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
// In addition, we provide a specializations of the form:
//   counter_traits< boost::array<UintType, N> >;
// Since the domain_types, key_types and range_types of threefry
// and philox are all boost::arrays, this allows us to wrap them
// (threefry and philox) with counter_based_engine.
//
// To introduce a new Prf that works with different types, one would
// provide specializations of counter_traits for the new Prf's domain,
// range and key types.  For example, hardware-and-compiler-specific
// code that specializes counter_traits for the Intel SSE __m128i type
// is in the libs/random/examples directory.

template <typename CtrType, typename Enable = void>
struct counter_traits{
    BOOST_STATIC_CONSTANT(unsigned, Nbits = 0);

    template<class CharT, class Traits>
    static std::basic_ostream<CharT, Traits>& insert(std::basic_ostream<CharT, Traits>& os, const CtrType& a);

    template<class CharT, class Traits>
    static std::basic_istream<CharT, Traits>& extract(std::basic_istream<CharT, Traits>& is, CtrType& a);

    static bool is_equal(const CtrType& rhs, const CtrType& lhs);

    // make_counter - construct a counter from the given arguments.
    // N.B.  counter_based_engine also assumes that CtrTypes are
    // DefaultConstructable and CopyAssignable and CopyConstructable.
    static CtrType make_counter(uintmax_t v);

    template <typename It>
    static CtrType make_counter(It first, It last, It *endp);

#if !defined(BOOST_NO_CXX11_HDR_INITIALIZER_LIST)
    template <typename V>
    static CtrType make_counter(std::initializer_list<V>);
#endif

    // clr_highbits - clear the HighBits of c, return true if the
    //  bits of c were not clear on input.
    template <unsigned HighBits>
    static bool clr_highbits(CtrType& c);

    template <unsigned HighBits>
    static CtrType incr(CtrType d);

    template <unsigned HighBits>
    static CtrType incr(CtrType d, boost::uintmax_t n);

    template <unsigned w>
    static std::size_t size();

    template <typename result_type, unsigned w>
    static result_type at(std::size_t n, CtrType v);

    // _make_counter_from_seedseq - used by counter_based_engine
    //   because counter_based_engine is obliged to provide SeedSeq
    //   methods.  It is discouraged for use by applications because
    //   SeedSeq output is subject to birthday-paradox collisions.
    //   Applications that want to use counter_traits::make_counter to
    //   manufacture counters should the range-based
    //   make_counter(first, last) and take responsibility for
    //   avoiding collisions themselves rather than relying on the
    //   dubious statistical properties of SeedSeq.  Applications
    //   that *really* want to iniialize with a SeedSeq can
    //   create derived class.
protected:
    template <typename SeedSeq>
    static CtrType _make_counter_from_seedseq(SeedSeq& s);
    template <typename, typename, unsigned, unsigned, typename, typename, typename>
    friend struct ::boost::random::counter_based_engine;
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
    BOOST_STATIC_ASSERT(N>0);
    BOOST_STATIC_ASSERT(std::numeric_limits<T>::is_specialized); 
    BOOST_STATIC_ASSERT(!std::numeric_limits<T>::is_signed);
    BOOST_STATIC_ASSERT(std::numeric_limits<T>::is_modulo);
    BOOST_STATIC_ASSERT(std::numeric_limits<T>::radix == 2);
protected:
    typedef boost::array<T, N> CtrType;
    BOOST_STATIC_CONSTANT(unsigned, value_bits = std::numeric_limits<T>::digits);
public:
    BOOST_STATIC_CONSTANT(unsigned, Nbits = N*value_bits);

    template<class CharT, class Traits>
    static std::basic_ostream<CharT, Traits>& insert(std::basic_ostream<CharT, Traits>& os, const CtrType& a){
        for(unsigned i=0; i<N; ++i)
            os << a[i] << ' ';
        return os;
    }

    template<class CharT, class Traits>
    static std::basic_istream<CharT, Traits>& extract(std::basic_istream<CharT, Traits>& is, CtrType& a){
        for(unsigned i=0; i<N; ++i)
            is >> a[i] >> std::ws;
        return is;
    }

    static bool is_equal(const CtrType& rhs, const CtrType& lhs){
        return rhs == lhs;
    }

    static CtrType make_counter(uintmax_t v = 0){
        // N.B.  will throw if v has non-zero bits that
        // don't 'fit' in CtrType.
        return make_counter(&v, &v+1);
    }

    // ??? What should the semantics of make_counter be???  The
    // primary consideration has to be avoidance of unintended
    // collisions.  If the caller assembles two ranges that *look*
    // different, and invokes make_counter on them, then they *must*
    // produce different CounterTypes (or at least one should throw an
    // exception).  Let's define what we mean by two ranges that 'look
    // different':
    //
    //    First, conceptually pad both ranges with zeros up to the
    //    length required for the CounterType.
    // 
    //    Then, compare them  element-by-element.  If any elements
    //    differ, then they 'look different'.
    //
    // Notice that this rule demands that make_counter consider all
    // the bits in the range.  The caller controls the iterator's
    // value_type and is free to choose it to suit his needs.  If the
    // caller finds it convenient to have 64-bit value_types, then
    // make_counter must not silently map ranges that 'look different'
    // to the caller into identical CtrTypes.  In particular,
    // make_counter must not gratuitously mask off high bits (above
    // the 32nd) of the range's value_type.
    //
    // By the above definition, two ranges don't 'look different' if
    // they differ only in elements that are unconsumed by
    // make_counter.  The caller can detect this by looking at the
    // value stored in *endp.  Whether the caller does or not is out
    // of our control, but existing practice seems to be to encourage
    // the dubious practice of providing extra-long ranges when
    // seeding engines, e.g., in test_seed_iterator.
    //
    // On the other hand, if invoked with endp==0, then the caller
    // can't detect unconsumed values, so in that case, make_counter
    // throws an invalid_argument if there are non-zero elements in
    // the unconsumed part of the range.  Use of the default endp==0
    // is much safer and is strongly encouraged.
    //
    template <typename It>
    static CtrType make_counter(It first, It last, It* endp=0){
        typedef typename std::iterator_traits<It>::value_type it_value_type;
        BOOST_STATIC_ASSERT(std::numeric_limits<it_value_type>::is_integer);
        BOOST_STATIC_ASSERT(std::numeric_limits<it_value_type>::radix == 2);
        const unsigned it_value_bits = std::numeric_limits<it_value_type>::digits + std::numeric_limits<it_value_type>::is_signed;
        BOOST_STATIC_ASSERT(value_bits%it_value_bits==0 || it_value_bits%value_bits==0);
        CtrType ret;
        it_value_type itv;
        if( it_value_bits == value_bits ){
            for(std::size_t j = 0; j < N; j++) {
                itv = (first != last) ? *first++ : 0;
                ret[j] = static_cast<T>(itv);
            }
        }else if( it_value_bits < value_bits ){
            for(std::size_t j = 0; j < N; j++) {
                T val = 0;
                unsigned lshift = 0;
                for(std::size_t k = 0; k < value_bits/it_value_bits; ++k) {
                    itv = (first != last) ? *first++ : 0;
                    val |= static_cast<T>(itv) << lshift;
                    lshift += it_value_bits;
                }
                ret[j] = val;
            }
        }else{
            const unsigned itvals_per_Tval = it_value_bits/value_bits;
            itv = (first != last) ? *first++ : 0;
            unsigned jj = 0;
            for(std::size_t j=0; j<N; ++j){
                if( jj++ == itvals_per_Tval ){
                    itv = (first != last) ? *first++ : 0;
                    jj = 0;
                }
                ret[j] = static_cast<T>(itv) & low_bits_mask_t<value_bits>::sig_bits;
                const unsigned shift = (value_bits < it_value_bits) ? value_bits : 0;
                itv >>= shift;
            }
        }
        if(endp)
            *endp = first;
        else{
            // If the caller isn't interested in endp, then don't
            // permit non-zero values in the leftover range.
            while( first != last ){
                if( *first++ )
                    BOOST_THROW_EXCEPTION(std::invalid_argument("non-zero values in range ignored"));
            }
        }
        return ret;
    }

#if !defined(BOOST_NO_CXX11_HDR_INITIALIZER_LIST)
    template <typename V>
    static CtrType make_counter(std::initializer_list<V> il){
        return  make_counter(il.begin(), il.end());
    }
#endif

    template <unsigned HighBits>
    static bool clr_highbits(CtrType& c){
        BOOST_STATIC_CONSTANT(unsigned, incr_idx = (Nbits - HighBits)/value_bits);
        BOOST_STATIC_CONSTANT(T, incr_stride = T(1)<<((Nbits - HighBits)%value_bits));
        BOOST_STATIC_CONSTANT(T, Mask = low_bits_mask_t<(Nbits - HighBits)%value_bits>::sig_bits);
        bool bad = false;
        typename CtrType::iterator p = c.begin() + incr_idx;
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
    static CtrType incr(CtrType d){
        BOOST_STATIC_ASSERT(HighBits <= Nbits);
        BOOST_STATIC_ASSERT(HighBits > 0);
        BOOST_STATIC_CONSTANT(T, incr_stride = T(1)<<((Nbits - HighBits)%value_bits));
        BOOST_STATIC_CONSTANT(unsigned, FullCtrWords = HighBits/value_bits);
        typename CtrType::reverse_iterator p = d.rbegin();
        for(unsigned i=0; i<FullCtrWords; ++i){
            *p += 1;
            if(*p++)
                return d;
        }
        if(p == d.rend())
            BOOST_THROW_EXCEPTION(std::invalid_argument("counter_traits::incr(): ran out of counters"));
        *p += incr_stride;
        if(*p < incr_stride)
            BOOST_THROW_EXCEPTION(std::invalid_argument("counter_traits::incr(): ran out of counters"));
        return d;
    }

    template <unsigned HighBits>
    static CtrType incr(CtrType d, boost::uintmax_t n){
        BOOST_STATIC_ASSERT(HighBits <= Nbits);
        BOOST_STATIC_ASSERT(HighBits > 0);
        BOOST_STATIC_CONSTANT(unsigned, lastword_loctrbit = (Nbits - HighBits)%value_bits);
        BOOST_STATIC_CONSTANT(T, lastword_stride = high_bit_mask_t<lastword_loctrbit>::high_bit);
        BOOST_STATIC_CONSTANT(boost::uintmax_t, lastword_maxn = low_bits_mask_t<value_bits-lastword_loctrbit>::sig_bits);
        BOOST_STATIC_CONSTANT(unsigned, FullCtrWords = HighBits/value_bits);
        typename CtrType::reverse_iterator p = d.rbegin();
        for(unsigned i=0; i<FullCtrWords; ++i){
            *p += T(n);
            bool carry = (*p++ < T(n));
            n >>= value_bits-1; n>>=1;
            if(carry)
                n++;
            if(n==0)
                return d;
        }
        if( n > lastword_maxn || p==d.rend() )
            BOOST_THROW_EXCEPTION(std::invalid_argument("counter_traits::incr(n): ran out of counters"));
        n *= lastword_stride;
        *p += n;
        if(*p < n)
            BOOST_THROW_EXCEPTION(std::invalid_argument("counter_traits::incr(n): ran out of counters"));
        return d;
    }

    template <unsigned w>
    static std::size_t size(){
        if( w <= value_bits ){
            return N*( value_bits/w );
        }else{
            return N/ ((w+value_bits-1)/value_bits);
        }
    }

    template <typename result_type, unsigned w>
    static result_type at(std::size_t n, CtrType v){
        result_type r;
        if( w == value_bits ){
            return static_cast<result_type>(v[n]);
        }else if( w < value_bits ){
            const unsigned  results_per_rangeval = value_bits/w;
            const unsigned idx = n/results_per_rangeval;
            const unsigned shift = (n%results_per_rangeval)*w;
            const typename CtrType::value_type r = v[ idx ];
            const result_type wmask = low_bits_mask_t<w>::sig_bits;
            return static_cast<result_type>(r >> shift)&wmask;
        }else{
            unsigned idx = (n*w)/value_bits;
            r = v[idx++];
	    // silence a bogus warning about shift-amount-too-large.
	    // N.B.  w is always > valuebits in this branch!
	    const unsigned shift = (w>value_bits)?value_bits : 0;
            // silence another bogus warning about comparison with 0
            const unsigned imax = (w>value_bits)?w/value_bits : 1;
            for(unsigned i=1; i<imax; ++i){
                r |= static_cast<result_type>(v[idx++]) << (i*shift);
            }
            return static_cast<result_type>(r);
        }
    }

protected:
    template <typename SeedSeq>
    static CtrType _make_counter_from_seedseq(SeedSeq& s){
        CtrType ret;
        detail::seed_array_int<value_bits>(s, ret.elems);
        return ret;
    }
    template <typename, typename, unsigned, unsigned, typename, typename, typename>
    friend struct ::boost::random::counter_based_engine;
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
