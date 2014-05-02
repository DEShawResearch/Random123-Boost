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
#ifndef BOOST_RANDOM_COUNTER_BASED_URNG_HPP
#define BOOST_RANDOM_COUNTER_BASED_URNG_HPP

#include <stdexcept>
#include <boost/cstdint.hpp>
#include <boost/static_assert.hpp>
#include <boost/type_traits/make_unsigned.hpp>
#include <boost/random/detail/operators.hpp>
#include <boost/random/detail/seed_impl.hpp>
#include <boost/random/detail/signed_unsigned_tools.hpp>
#include <boost/random/detail/integer_log2.hpp>

namespace boost{
namespace random{

template<typename Prf>
class counter_based_urng{
    // According to C++0x, a URNG requires only a result_type,
    // operator()(), min() and max() methods.  counter_based_urng
    // also has prf_type, domain_array_type and a constructor
    // that takes a prf_type and a domain_array_type as arguments.
public:
    typedef Prf prf_type;
    BOOST_STATIC_CONSTANT(bool, has_fixed_range = false);
    typedef typename prf_type::domain_type domain_type;
    typedef typename prf_type::range_type::value_type result_type;

    counter_based_urng(const counter_based_urng& e) : b(e.b), c(e.c){
        //std::cerr << "cbe(const counter_based_urng&)\n";
        setnext(e.nth());
    }

    counter_based_urng& operator=(const counter_based_urng& rhs){
        b = rhs.b;
        c = rhs.c;
        setnext(rhs.nth());
        return *this;
    }

    result_type operator()(){
        if(next == rdata.end()){
            c.back() += delta();
            rdata = b(c);
            next = rdata.begin();
        }
        return *next++;
    }
    counter_based_urng(prf_type _b, domain_type _c0)
        : b(_b), c(_c0), next(rdata.end())
    {
        chkhighbits();
    }

    BOOST_RANDOM_DETAIL_CONSTEXPR static result_type min BOOST_PREVENT_MACRO_SUBSTITUTION () { return Prf::range_array_min(); }
    BOOST_RANDOM_DETAIL_CONSTEXPR static result_type max BOOST_PREVENT_MACRO_SUBSTITUTION () { return Prf::range_array_max(); }

    template <class Iter>
    void generate(Iter first, Iter last)
    { detail::generate_from_int(*this, first, last); }

    // N.B.  URNGs aren't *required* to have ==, !=, << or >>
    // operators, but they're trivial to implement, so we might
    // as well make them available to users.
    BOOST_RANDOM_DETAIL_EQUALITY_OPERATOR(counter_based_urng, lhs, rhs){ 
        return lhs.c==rhs.c && 
            lhs.nth() == rhs.nth() && 
            lhs.b == rhs.b; 
    }

    BOOST_RANDOM_DETAIL_INEQUALITY_OPERATOR(counter_based_urng)

    BOOST_RANDOM_DETAIL_OSTREAM_OPERATOR(os, counter_based_urng, f){
        os << (f.nth()) << ' ';
        for(typename domain_type::const_iterator p=f.c.begin(); p!=f.c.end(); ++p)
            os << ' ' << *p;
        os << f.b;
        return os;
    }

    BOOST_RANDOM_DETAIL_ISTREAM_OPERATOR(is, counter_based_urng, f){
        size_t n;
        is >> n;
        for(typename domain_type::iterator p=f.c.begin(); p!=f.c.end(); ++p)
            is >> *p >> std::ws;
        is >> f.b;
        f.setnext(n);
        return is;
    }

    void discard(boost::uintmax_t skip){
        size_t nelem = rdata.size();
	size_t newnth = nth() + (skip % nelem);
        skip /= nelem;
        if (newnth > nelem) {
            newnth -= nelem;
	    skip++;
        }
        c.back() += skip*delta();
        setnext(newnth);
    }

protected:
    typedef typename domain_type::value_type dvalue_type;
    prf_type b;
    domain_type c;
    typename prf_type::range_type rdata;
    typename prf_type::range_type::iterator next;

    size_t nth() const { return next - rdata.begin(); }

    void setnext(size_t n){
        size_t nelem = rdata.size();
        if( n != nelem ){
            rdata = b(c);
        }
        next = rdata.begin() + n;
    }        

    // N.B. in C++03 or C++14 dmaxmin, domain_bits and ctr_bits would be
    // temporaries inside delta().  But in C++11 we're not allowed to
    // declare temporaries in a constexpr function.
    typedef typename boost::make_unsigned<dvalue_type>::type unsigned_type;
    BOOST_RANDOM_DETAIL_CONSTEXPR static unsigned_type dmaxmin(){
        return boost::random::detail::subtract<dvalue_type>()(prf_type::domain_array_max(), prf_type::domain_array_min());
    }

    BOOST_RANDOM_DETAIL_CONSTEXPR static int domain_bits(){
        return (dmaxmin() == (std::numeric_limits<unsigned_type>::max)()) ?
            std::numeric_limits<unsigned_type>::digits :
            detail::integer_log2(dmaxmin() + 1);
    }

    BOOST_RANDOM_DETAIL_CONSTEXPR static int ctr_bits(){
        return (std::min)(32, domain_bits());
    }

    BOOST_RANDOM_DETAIL_CONSTEXPR static dvalue_type delta(){
        return static_cast<dvalue_type>(1)<<(domain_bits() - std::min(32, domain_bits()));
    }

    void chkhighbits(){
        if( c.back() >= delta()+Prf::domain_array_min() )
            throw std::out_of_range("Initial value of counter_based_urng's counter is too large");
    }
};

} // namespace random
} // namespace boost

#endif // BOOST_RANDOM_COUNTER_BASED_URNG_HPP
