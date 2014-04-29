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
#ifndef BOOST_RANDOM_COUNTER_BASED_ENGINE_HPP
#define BOOST_RANDOM_COUNTER_BASED_ENGINE_HPP

#include <boost/utility/enable_if.hpp>
#include <boost/type_traits/is_convertible.hpp>
#include <boost/cstdint.hpp>
#include <boost/random/seed_seq.hpp>
#include <boost/random/detail/seed_impl.hpp>
#include <boost/random/detail/operators.hpp>
#include <boost/random/detail/seed.hpp>
#include <boost/random/detail/integer_log2.hpp> // for BOOST_RANDOM_DETAIL_CONSTEXPR

#include <iosfwd>
#include <utility>
#include <stdexcept>

#undef BOOST_RANDOM_DETAIL_SEED_SEQ_CONSTRUCTOR
#define BOOST_RANDOM_DETAIL_SEED_SEQ_CONSTRUCTOR(Self, SeedSeq, seq)    \
    template<class SeedSeq>                                             \
    explicit Self(SeedSeq& seq, typename ::boost::random::detail::disable_seed<SeedSeq>::type* = 0)


namespace boost{
namespace random{

template<typename Prf>
struct counter_based_engine {
    typedef Prf prf_type;
    BOOST_STATIC_CONSTANT(bool, has_fixed_range = false);
    typedef typename Prf::range_type::value_type result_type;

protected:
    typedef typename Prf::range_type range_type;
    typedef typename Prf::domain_type domain_type;
    typedef typename Prf::key_type key_type;
    typedef typename domain_type::value_type dvalue_type;
    prf_type b;
    domain_type c;
    range_type v;
    typename range_type::const_iterator next;

    void setnext(size_t n){
        size_t nelem = v.size();
        if( n != nelem ){
            v = b(c);
        }
        next = v.begin() + n;
    }        

    size_t nth() const { return next - v.begin(); }
        
    void incr(){
        typename domain_type::iterator p = c.begin();
        do{
            if(*p != Prf::domain_array_max()){
                *p += 1;
                return;
            }
            *p = min();
        }while(++p != c.end());
        if(*--p == min())
            throw std::out_of_range("counter_based_engine::incr(): ran out of counters");
    }

    void incr(boost::uintmax_t n){
        typename domain_type::iterator p = c.begin();
        do{
            dvalue_type n0;
            // if min()/max() spans a binary dvalue_type, we can use
            // shifts and masks to figure out how much to add to *p,
            // n0, and how much to carry to the rest of the array, n.
            // N.B.  The conditional can be evaluated at compile-time,
            // so a good compiler will only generate one branch of the
            // if/else.
            if( std::numeric_limits<dvalue_type>::is_specialized &&
                std::numeric_limits<dvalue_type>::radix == 2 &&
                !std::numeric_limits<dvalue_type>::is_signed &&
                Prf::domain_array_max() == std::numeric_limits<dvalue_type>::max() && 
                Prf::domain_array_min() == 0
                ){
                n0 = n & Prf::domain_array_max();
                // Another compile-time conditional to avoid shifting by too much
                if( std::numeric_limits<dvalue_type>::digits == std::numeric_limits<boost::uintmax_t>::digits ){
                    n = 0;
                }else{
                    n >>= std::numeric_limits<dvalue_type>::digits-1;
                    n >>= 1;
                }
            }else{
                boost::uintmax_t range = static_cast<boost::uintmax_t>(1) + Prf::domain_array_max() - Prf::domain_array_min();
                n0 = n%range;
                n /= range;
            }
            if( *p <= Prf::domain_array_max() - n0 )
                *p += n0;
            else{
                // p +=  n0 - (1 +  max -  min) // add n0, wrap by modulus
                // p -= max-n0 + 1 - min
                *p -= (Prf::domain_array_max() - n0) + 1 - Prf::domain_array_min();
                n++;
            }
        }while(n && ++p != c.end());
        if(n)
            throw std::out_of_range("counter_based_engine::incr(n):  ran out of counters");
    }

    void initialize(){
        c.fill(Prf::domain_array_min());
        next = v.end();
    }

public:
    BOOST_RANDOM_DETAIL_CONSTEXPR static result_type min BOOST_PREVENT_MACRO_SUBSTITUTION () { return Prf::range_array_min(); }
    BOOST_RANDOM_DETAIL_CONSTEXPR static result_type max BOOST_PREVENT_MACRO_SUBSTITUTION () { return Prf::range_array_max(); }

    counter_based_engine()
        : b()
    {
        initialize();
        //std::cerr << "cbe()\n";
    }

    counter_based_engine(counter_based_engine& e) : b(e.b), c(e.c){
        //std::cerr << "cbe(counter_based_engine&)\n";
        setnext(e.nth());
    }

    counter_based_engine(const counter_based_engine& e) : b(e.b), c(e.c){
        //std::cerr << "cbe(const counter_based_engine&)\n";
        setnext(e.nth());
    }

    counter_based_engine& operator=(const counter_based_engine& rhs){
        b = rhs.b;
        c = rhs.c;
        setnext(rhs.nth());
        return *this;
    }

    BOOST_RANDOM_DETAIL_ARITHMETIC_CONSTRUCTOR(counter_based_engine, result_type, value)
        : b(boost::random::seed_seq(&value, &value+1))
    { 
        //std::cerr << "cbe(result_type)\n";
        initialize();
    }

    BOOST_RANDOM_DETAIL_SEED_SEQ_CONSTRUCTOR(counter_based_engine, SeedSeq, seq)
        : b(seq)
    {
        //std::cerr << "cbe(SeedSeq)\n";
        initialize();
    }

    template<class It> counter_based_engine(It& first, It last)
        : b(first, last)
    {
        //std::cerr << "cbe(range)\n";
        initialize();
    }

    void seed(){
        //std::cerr << "cbe::seed()\n";
        b.seed();
        initialize();
    }

    BOOST_RANDOM_DETAIL_ARITHMETIC_SEED(counter_based_engine, result_type, value)
    { 
        //std::cerr << "cbe::seed(arithmetic)\n";
        boost::random::seed_seq ss(&value, &value+1);
        seed(ss);
    }

    BOOST_RANDOM_DETAIL_SEED_SEQ_SEED(counter_based_engine, SeedSeq, seq){
        //std::cerr << "cbe::seed(SeedSeq)\n" << "\n";
        b.seed(seq);
        initialize();
    }

    template<class It>
    void seed(It& first, It last){
        //std::cerr << "cbe::seed(range)\n";
        b.seed(first, last);
        initialize();
    }

    BOOST_RANDOM_DETAIL_EQUALITY_OPERATOR(counter_based_engine, lhs, rhs){ 
        return lhs.c==rhs.c && 
            lhs.nth() == rhs.nth() && 
            lhs.b == rhs.b; 
    }

    BOOST_RANDOM_DETAIL_INEQUALITY_OPERATOR(counter_based_engine)

    BOOST_RANDOM_DETAIL_OSTREAM_OPERATOR(os, counter_based_engine, f){
        os << (f.nth()) << ' ';
        for(typename domain_type::const_iterator p=f.c.begin(); p!=f.c.end(); ++p)
            os << ' ' << *p;
        os << f.b;
        return os;
    }

    BOOST_RANDOM_DETAIL_ISTREAM_OPERATOR(is, counter_based_engine, f){
        size_t n;
        is >> n;
        for(typename domain_type::iterator p=f.c.begin(); p!=f.c.end(); ++p)
            is >> *p >> std::ws;
        is >> f.b;
        f.setnext(n);
        return is;
    }

    result_type operator()(){
        if( next == v.end() ){
            incr();
            v = b(c);
            next = v.begin();
        }
        return *next++;
    }

    void discard(boost::uintmax_t skip){
        size_t nelem = v.size();
	size_t newnth = nth() + (skip % nelem);
        skip /= nelem;
        if (newnth > nelem) {
            newnth -= nelem;
	    skip++;
        }
        incr(skip);
        setnext(newnth);
    }
         
    template <class Iter>
    void generate(Iter first, Iter last)
    { detail::generate_from_int(*this, first, last); }

    //--------------------------
    // Some bonus methods, not required for a Random Number
    // Engine

    // A pos_type and tell() and seek(pos_type) methods that give the
    // caller some visibility into and control over where we are in
    // the counter sequence.
    typedef std::pair<domain_type, size_t> pos_type;
    pos_type tell() const{ 
        return make_pair(c, nth());
    }
    void seek(pos_type pos){ 
        c = pos.first; setnext(pos.second);
    }

    // Constructors and seed() method to create an counter_based_engine with
    // a given key as its seed.
    // We need const and non-const to supersede the SeedSeq template.
    // FIXME - these might benefite from something like BOOST_DETAIL_???_CONSTRUCTOR treatment.
    explicit counter_based_engine(const key_type &uk) : b(uk), c(), next(v.end()){}
    explicit counter_based_engine(key_type &uk) : b(uk), c(), next(v.end()){}
    void seed(const key_type& uk){
        *this = counter_based_engine(uk);
    }        
    void seed(key_type& uk){
        *this = counter_based_engine(uk);
    }        

    // Since you can seed *this with key_type, it seems reasonable
    // to allow the caller to know what seed/key *this is using.
    key_type getseed() const{
        return b.getkey();
    }
};

} // namespace random
} // namespace boost

#endif // BOOST_RANDOM_COUNTER_BASED_ENGINE_HPP
