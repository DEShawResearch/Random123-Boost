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
#ifndef BOOST_RANDOM_PRF_COMMON_HPP
#define BOOST_RANDOM_PRF_COMMON_HPP

#include <boost/random/detail/operators.hpp>
#include <boost/random/detail/seed.hpp>
#include <boost/random/seed_seq.hpp>
#include <boost/random/detail/seed_impl.hpp>
#include <boost/random/detail/integer_log2.hpp> // for BOOST_RANDOM_DETAIL_CONSTEXPR

#undef BOOST_RANDOM_DETAIL_SEED_SEQ_CONSTRUCTOR
#define BOOST_RANDOM_DETAIL_SEED_SEQ_CONSTRUCTOR(Self, SeedSeq, seq)    \
    template<class SeedSeq>                                             \
    explicit Self(SeedSeq& seq, typename ::boost::random::detail::disable_seed<SeedSeq>::type* = 0)

#undef BOOST_RANDOM_DETAIL_CONST_SEED_SEQ_CONSTRUCTOR
#define BOOST_RANDOM_DETAIL_CONST_SEED_SEQ_CONSTRUCTOR(Self, SeedSeq, seq)    \
    template<class SeedSeq>                                             \
    explicit Self(const SeedSeq& seq, typename ::boost::random::detail::disable_seed<SeedSeq>::type* = 0)

namespace boost{
namespace random{
namespace detail{

template <unsigned Ndomain, unsigned Nrange, unsigned Nkey, typename Uint>
struct prf_common{
    typedef array<Uint, Ndomain> domain_type;
    typedef array<Uint, Nrange> range_type;
    typedef array<Uint, Nkey> key_type ;
    BOOST_RANDOM_DETAIL_CONSTEXPR static typename domain_type::value_type domain_array_min()  { return std::numeric_limits<typename domain_type::value_type>::min(); }
    BOOST_RANDOM_DETAIL_CONSTEXPR static typename domain_type::value_type domain_array_max()  { return std::numeric_limits<typename domain_type::value_type>::max(); }

    BOOST_RANDOM_DETAIL_CONSTEXPR static typename range_type::value_type range_array_min()  { return std::numeric_limits<typename range_type::value_type>::min(); }
    BOOST_RANDOM_DETAIL_CONSTEXPR static typename range_type::value_type range_array_max()  { return std::numeric_limits<typename range_type::value_type>::max(); }


    key_type k;
    prf_common(key_type _k) : k(_k){
        //std::cerr << "prf_common(key_type)\n";
    }
    void setkey(key_type _k){ k = _k; }
    key_type getkey() const { return k; }

    prf_common() { k = key_type(); }

    prf_common(prf_common& v) : k(v.k) {
        //std::cerr << "prf_common(prf_common&)\n";
    }
    prf_common(const prf_common& v) : k(v.k) {
        //std::cerr << "prf_common(const prf_common&)\n";
    }

    BOOST_RANDOM_DETAIL_SEED_SEQ_CONSTRUCTOR(prf_common, SeedSeq, seq){
        //std::cerr << "prf_common(SeedSeq&)\n";
        seed(seq);
    }

    BOOST_RANDOM_DETAIL_CONST_SEED_SEQ_CONSTRUCTOR(prf_common, SeedSeq, seq){
        //std::cerr << "prf_common(const SeedSeq&)\n";
        seed(seq);
    }

    // Do we also need a const version?
    BOOST_RANDOM_DETAIL_SEED_SEQ_SEED(prf_common, SeedSeq, seq){
        static const size_t w = std::numeric_limits<typename key_type::value_type>::digits;
        // TODO - do this in-place, without a separate uarray.  Or will the compiler figure
        // it out without the code jumping through more hoops?
        typename key_type::value_type uarray[Nkey];
        boost::random::detail::seed_array_int<w>(seq, uarray);
        std::copy(&uarray[0], &uarray[Nkey], k.begin());
    }

    template <class It>
    prf_common(It& first, It last){
        //std::cerr << "prf_common(It&, It)\n";
        seed(first, last);
    }

    void seed(){
        k = key_type();
        //typename key_type::kvalue_type zero = typename key_type::value_type();
        //boost::random::seed_seq ss(&zero, &zero+1);
        //seed(ss);
    }

    template <class It>
    void seed(It& first, It last){
        static const size_t w = std::numeric_limits<typename key_type::value_type>::digits;
        typename key_type::value_type uarray[Nkey];
        boost::random::detail::fill_array_int<w>(first, last, uarray);
        std::copy(&uarray[0], &uarray[Nkey], k.begin());
    }

    BOOST_RANDOM_DETAIL_OSTREAM_OPERATOR(os, prf_common, f){
        for(typename key_type::const_iterator p=f.k.begin(); p!=f.k.end(); ++p)
            os << ' ' << *p;
        return os;
    }

    BOOST_RANDOM_DETAIL_ISTREAM_OPERATOR(is, prf_common, f){
        for(typename key_type::iterator p=f.k.begin(); p!=f.k.end(); ++p)
            is >> *p >> std::ws;
        return is;
    }

    BOOST_RANDOM_DETAIL_EQUALITY_OPERATOR(prf_common, lhs, rhs){ 
        return lhs.k == rhs.k;
    }

    BOOST_RANDOM_DETAIL_INEQUALITY_OPERATOR(prf_common)
};

} // namespace detail
} // namespace random
} // namespace boost

#endif
