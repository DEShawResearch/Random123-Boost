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

template<typename Prf, unsigned CtrBits=std::numeric_limits<typename Prf::domain_type::value_type>::digits*Prf::Ndomain>
struct counter_based_engine {
    typedef Prf prf_type;
    BOOST_STATIC_CONSTANT(bool, has_fixed_range = false);
    typedef typename Prf::range_type::value_type result_type;

protected:
    typedef typename Prf::range_type range_type;
    typedef typename Prf::domain_type domain_type;
    typedef typename Prf::key_type key_type;
    typedef typename domain_type::value_type dvalue_type;
    BOOST_STATIC_CONSTANT(unsigned, dvalue_bits = std::numeric_limits<dvalue_type>::digits);
    BOOST_STATIC_CONSTANT(unsigned, incr_idx = (Prf::Ndomain*dvalue_bits - CtrBits)/dvalue_bits);
    BOOST_STATIC_ASSERT(incr_idx < Prf::Ndomain);
    BOOST_STATIC_CONSTANT(dvalue_type, incr_stride = dvalue_type(1)<<((Prf::Ndomain*dvalue_bits - CtrBits)%dvalue_bits));
    BOOST_STATIC_CONSTANT(unsigned, FullCtrWords = CtrBits/dvalue_bits);
    BOOST_STATIC_CONSTANT(dvalue_type, dvalue_mask = ~dvalue_type(0));

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
        typename domain_type::reverse_iterator p = c.rbegin();
        for(unsigned w=0; w<FullCtrWords; ++w){
            *p += 1;
            if(*p++)
                return;
        }
        *p += incr_stride;
        if(*p < incr_stride)
            return;
        throw std::out_of_range("counter_based_engine::incr(): ran out of counters");
    }

    void incr(boost::uintmax_t n){
        typename domain_type::reverse_iterator p = c.rbegin();
        for(unsigned w=0; w<FullCtrWords; ++w){
            *p += n&dvalue_mask;
            n >>= dvalue_bits-1; n>>=1;
            if(*p++ == 0)
                n++;
            if(n==0)
                return;
        }
        // FIXME - these don't test overflow correctly!!
        *p += n*incr_stride;
        if(*p < incr_stride)
            return;
        throw std::out_of_range("counter_based_engine::incr(): ran out of counters");
    }

    void initialize(){
        c.fill(0);
        next = v.end();
    }

    void chkhighbits(domain_type c){
        bool bad = false;
        typename domain_type::const_iterator p = c.begin() + incr_idx;
        if( *p++ >= incr_stride )
            bad = true;
        for( ; p != c.end(); ++p)
            if(*p) bad = true;
        if(bad)
            throw std::out_of_range("Initial value of counter_based_engine's counter is too large");
    }

public:
    BOOST_RANDOM_DETAIL_CONSTEXPR static result_type min BOOST_PREVENT_MACRO_SUBSTITUTION () { return 0; }
    BOOST_RANDOM_DETAIL_CONSTEXPR static result_type max BOOST_PREVENT_MACRO_SUBSTITUTION () { return std::numeric_limits<dvalue_type>::max(); }

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
        : b(value)
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
        b.seed(value);
        initialize();
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

    // Constructors and seed() method to create a counter_based_engine with
    // a given Prf and a user-managed starting point.
    //
    // Note that Prfs can be implicitly constructed from a key_type, so
    // these also work as:
    //    Prf::key_type k;
    //    Prf::domain_type start;
    //    counter_based_engine<Prf> cbe1(k);
    //    counter_based_engine<Prf, CtrBits> cbe2(k, start);
    //    cbe2.seed(k, start);
    //
    // With C++11 initializer lists you can have a philox4x32 engine
    // with 64 user-managed key-bits, 96 user-managed "start" bits and
    // 32 bits of counter with: typedef philox<4, uint32_t> Prf;
    // counter_based_engine<Prf, 32> cbe3({1,2}, {3,4,5});
    explicit counter_based_engine(Prf _b, domain_type start = domain_type()) : b(_b), c(start), next(v.end()){
        chkhighbits(start);
    }

    void seed(Prf b, domain_type start){
        //std::cerr << "cbe::seed(Prf, start)\n";
        *this = counter_based_engine(b, start);
    }        

    // restart - resets the user-managed 'start' bits without
    //  touching the Prf or its key.  The counter is reset
    //  so there are again 2^CtrBits counters available.
    void restart(domain_type start){ 
        chkhighbits(start);
        c = start;
        next = v.end();
    }

};

} // namespace random
} // namespace boost

#endif // BOOST_RANDOM_COUNTER_BASED_ENGINE_HPP
