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

namespace boost{
namespace random{
namespace detail{

template <unsigned Ndomain_, unsigned Nrange_, unsigned Nkey_, typename domain_vtype,
          typename range_vtype=domain_vtype, typename key_vtype=domain_vtype>
struct prf_common{
    BOOST_STATIC_CONSTANT(unsigned, Ndomain = Ndomain_);
    BOOST_STATIC_CONSTANT(unsigned, Nrange = Nrange_);
    BOOST_STATIC_CONSTANT(unsigned, Nkey = Nkey_);
    BOOST_STATIC_CONSTANT(unsigned, domain_bits = Ndomain*std::numeric_limits<domain_vtype>::digits);
    BOOST_STATIC_CONSTANT(unsigned, range_bits = Nrange*std::numeric_limits<range_vtype>::digits);
    BOOST_STATIC_CONSTANT(unsigned, key_bits = Nkey*std::numeric_limits<key_vtype>::digits);
    typedef array<domain_vtype, Ndomain> domain_type;
    typedef array<range_vtype, Nrange> range_type;
    typedef array<key_vtype, Nkey> key_type ;

    key_type k;
    prf_common(key_type _k) : k(_k){
        //std::cerr << "prf_common(key_type)\n";
    }

    prf_common() { k = key_type(); }

    prf_common(const prf_common& v) : k(v.k) {
        //std::cerr << "prf_common(const prf_common&)\n";
    }

    void setkey(key_type _k){ k = _k; }
    key_type getkey() const { return k; }

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
