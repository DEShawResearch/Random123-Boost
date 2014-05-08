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
#include <boost/random/detail/integer_log2.hpp>
#include <boost/integer/static_min_max.hpp>
#include <boost/integer/integer_mask.hpp>

#include <iosfwd>
#include <utility>
#include <stdexcept>
#include <algorithm>
#include <limits>

#undef BOOST_RANDOM_DETAIL_SEED_SEQ_CONSTRUCTOR
#define BOOST_RANDOM_DETAIL_SEED_SEQ_CONSTRUCTOR(Self, SeedSeq, seq)    \
    template<class SeedSeq>                                             \
    explicit Self(SeedSeq& seq, typename ::boost::random::detail::disable_seed<SeedSeq>::type* = 0)


namespace boost{
namespace random{

template<typename Prf, 
         unsigned CtrBits = static_unsigned_min<64u, Prf::range_bits/2>::value,
         typename UintType = typename Prf::range_type::value_type, 
         unsigned w = std::numeric_limits<UintType>::digits>
struct counter_based_engine {
    typedef Prf prf_type;
    typedef UintType result_type;
    BOOST_STATIC_CONSTANT(bool, has_fixed_range = false);

protected:
    typedef typename Prf::range_type range_type;
    typedef typename Prf::domain_type domain_type;
    typedef typename Prf::key_type key_type;
    typedef typename domain_type::value_type dvalue_type;
    BOOST_STATIC_CONSTANT(unsigned, dvalue_bits = std::numeric_limits<dvalue_type>::digits);
    BOOST_STATIC_ASSERT(CtrBits <= dvalue_bits*Prf::Ndomain);
    BOOST_STATIC_ASSERT(CtrBits > 0);
    BOOST_STATIC_CONSTANT(unsigned, incr_idx = (Prf::Ndomain*dvalue_bits - CtrBits)/dvalue_bits);
    BOOST_STATIC_CONSTANT(dvalue_type, incr_stride = dvalue_type(1)<<((Prf::Ndomain*dvalue_bits - CtrBits)%dvalue_bits));
    BOOST_STATIC_CONSTANT(unsigned, FullCtrWords = CtrBits/dvalue_bits);

    typedef typename key_type::value_type kvalue_type;
    BOOST_STATIC_CONSTANT(unsigned, kvalue_bits = std::numeric_limits<kvalue_type>::digits);

    BOOST_STATIC_ASSERT( std::numeric_limits<UintType>::digits >= w );
    BOOST_STATIC_ASSERT( Prf::range_bits%w == 0 );
    BOOST_STATIC_CONSTANT(unsigned, Nresult = Prf::range_bits/w);

    prf_type b;
    domain_type c;
    union{
        range_type range;
        boost::array<result_type, Nresult> endian_dependent_result;
    } vu;
    unsigned next;

    result_type nth_result(unsigned n){
#if 0
        // The results in this branch JUST AS RANDOM as the other
        // branch, BUT if w!=rvalue_bits, the numerical output values
        // will be bit-swizzled in an ENDIAN-DEPENDENT way.  If your
        // compiler doesn't do constant-propagation well, this branch
        // may be a lot faster.  Use it if you prefer to give up
        // endian-independence for the added speed.
        BOOST_STATIC_ASSERT( sizeof(vu.range) == sizeof(vu.endian_dependent_result) );
        return vu.endian_dependent_result[n];
#else
        const unsigned rvalue_bits = Prf::range_bits/Prf::Nrange;
        result_type r;
        if( w == rvalue_bits ){
            return vu.range[n];
        }else if( w < rvalue_bits ){
            const unsigned  results_per_rangeval = rvalue_bits/w;
            const unsigned idx = n/results_per_rangeval;
            const unsigned shift = (n%results_per_rangeval)*w;
            const typename range_type::value_type r = vu.range[ idx ];
            const result_type wmask = low_bits_mask_t<w>::sig_bits;
            return (r >> shift)*wmask;
        }else{
            unsigned idx = (n*w)/rvalue_bits;
            r = vu.range[idx++];
            for(int i=1; i<w/rvalue_bits; ++i){
                r = (r<<w) | vu.range[idx++];
            }
            return r;
        }
#endif
    }
            
    void setnext(unsigned n){
        if( n != Nresult ){
            vu.range = b(c);
        }
        next = n;
    }        

    void incr(){
        typename domain_type::reverse_iterator p = c.rbegin();
        for(unsigned i=0; i<FullCtrWords; ++i){
            *p += 1;
            if(*p++)
                return;
        }
        *p += incr_stride;
        if(*p >= incr_stride)
            return;
        throw std::out_of_range("counter_based_engine::incr(): ran out of counters");
    }

    void incr(boost::uintmax_t n){
        typename domain_type::reverse_iterator p = c.rbegin();
        for(unsigned i=0; i<FullCtrWords; ++i){
            *p += dvalue_type(n);
            n >>= dvalue_bits-1; n>>=1;
            if(*p++ == 0)
                n++;
            if(n==0)
                return;
        }
        // FIXME - these don't test overflow correctly!!
        *p += n*incr_stride;
        if(*p >= incr_stride)
            return;
        throw std::out_of_range("counter_based_engine::incr(): ran out of counters");
    }

    void initialize(){
        c.fill(0);
        next = Nresult;
    }

    void chk_highbasebits(domain_type c){
        bool bad = false;
        typename domain_type::const_iterator p = c.begin() + incr_idx;
        if( *p++ >= incr_stride )
            bad = true;
        for( ; p != c.end(); ++p)
            if(*p) bad = true;
        if(bad)
            throw std::out_of_range("Initial value of counter_based_engine's counter is too large");
    }

    // key_from_{value,range,seedseq} - construct a key from the
    //   argument.  These functions make no effort to check or set the
    //   high bits of the key.  They should be passed through either
    //   chk_highkeybits or set_highkeybits before being passed on to
    //   a Prf constructor or Prf::setkey
    key_type key_from_value(kvalue_type v){
        key_type ret = {{v}};
        return ret;
    }

    template <typename SeedSeq>
    key_type key_from_seedseq(SeedSeq& seq){
        key_type ret;
        detail::seed_array_int<kvalue_bits>(seq, ret.elems);
        return ret;
    }

    template <typename It>
    key_type key_from_range(It& first, It last){
        key_type ret;
        detail::fill_array_int<kvalue_bits>(first, last, ret.elems);
        return ret;
    }

    // We avoid collisions between engines with different CtrBits by
    // embedding the value CtrBits-1 in the high few bits of the last
    // element of Prf's key.  If we didn't do this, it would be too
    // easy for counter_based_engine<Prf, N> and
    // counter_based_engine<Prf, M> to "collide", producing
    // overlapping streams.
    //
    // When we accept a key from the user, e.g., seed(arithmetic) or
    // seed(key_type), it is an error if the specified value has any
    // high bits set.  On the other hand, it's not an error if
    // seed_seq.generate() sets those bits -- we just ignore them.

    // chk_highkeybits - first check that the high CtrBitsBits of k
    //  are 0.  If they're not, throw an out_of_range exception.  If
    //  they are, then call set_highkeybits.
    key_type chk_highkeybits(key_type k){
        const unsigned CtrBitsBits = detail::integer_log2(Prf::Ndomain*dvalue_bits);
        //BOOST_STATIC_ASSERT(CtrBitsBits <= kvalue_bits);
        BOOST_STATIC_CONSTANT(kvalue_type, CtrBitsMask = (~kvalue_type(0))>> CtrBitsBits );
        if( k[Prf::Nkey-1] & ~CtrBitsMask )
            throw std::out_of_range("high bits of key are reserved for internal use by counter_based_engine");
        return set_highkeybits(k);
    }

    // set_highkeybits - set the high CtrBitsBits of k to CtrBits-1,
    //  regardless of their original contents.
    key_type set_highkeybits(key_type k){
        const unsigned CtrBitsBits = detail::integer_log2(Prf::Ndomain*dvalue_bits);
        //BOOST_STATIC_ASSERT(CtrBitsBits <= kvalue_bits);
        BOOST_STATIC_CONSTANT(kvalue_type, CtrBitsMask = (~kvalue_type(0))>> CtrBitsBits );
        k[Prf::Nkey-1] &= CtrBitsMask;
        k[Prf::Nkey-1] |= kvalue_type(CtrBits-1)<<(kvalue_bits - CtrBitsBits);
        return k;
    }

public:
    BOOST_RANDOM_DETAIL_CONSTEXPR static result_type min BOOST_PREVENT_MACRO_SUBSTITUTION () { return 0; }
    BOOST_RANDOM_DETAIL_CONSTEXPR static result_type max BOOST_PREVENT_MACRO_SUBSTITUTION () { return std::numeric_limits<dvalue_type>::max(); }

    counter_based_engine()
        : b(), c(), next(Nresult)
    {
        vu.range.fill(0); // NOT logically necessary.  Silences a spurious gcc warning.
        initialize();
        //std::cerr << "cbe()\n";
    }

    counter_based_engine(counter_based_engine& e) : b(e.b), c(e.c), next(Nresult){
        //std::cerr << "cbe(counter_based_engine&)\n";
        setnext(e.next);
    }

    counter_based_engine(const counter_based_engine& e) : b(e.b), c(e.c), next(Nresult){
        //std::cerr << "cbe(const counter_based_engine&)\n";
        setnext(e.next);
    }

    counter_based_engine& operator=(const counter_based_engine& rhs){
        b = rhs.b;
        c = rhs.c;
        setnext(rhs.next);
        return *this;
    }

    BOOST_RANDOM_DETAIL_ARITHMETIC_CONSTRUCTOR(counter_based_engine, result_type, value)
        : b(chk_highkeybits(key_from_value(value)))
    { 
        //std::cerr << "cbe(result_type)\n";
        initialize();
    }

    BOOST_RANDOM_DETAIL_SEED_SEQ_CONSTRUCTOR(counter_based_engine, SeedSeq, seq)
        : b(set_highkeybits(key_from_seedseq(seq)))
    {
        //std::cerr << "cbe(SeedSeq)\n";
        initialize();
    }

    template<class It> counter_based_engine(It& first, It last)
        : b(chk_highkeybits(key_from_range(first, last)))
    {
        //std::cerr << "cbe(range)\n";
        initialize();
    }

    void seed(){
        //std::cerr << "cbe::seed()\n";
        b.setkey(key_type());
        initialize();
    }

    BOOST_RANDOM_DETAIL_ARITHMETIC_SEED(counter_based_engine, result_type, value)
    { 
        //std::cerr << "cbe::seed(arithmetic)\n";
        b.setkey(chk_highkeybits(key_from_value(value)));
        initialize();
    }

    BOOST_RANDOM_DETAIL_SEED_SEQ_SEED(counter_based_engine, SeedSeq, seq){
        //std::cerr << "cbe::seed(SeedSeq)\n" << "\n";
        b.setkey(set_highkeybits(key_from_seedseq(seq)));
        initialize();
    }

    template<class It>
    void seed(It& first, It last){
        //std::cerr << "cbe::seed(range)\n";
        b.setkey(chk_highkeybits(key_from_range(first, last)));
        initialize();
    }

    BOOST_RANDOM_DETAIL_EQUALITY_OPERATOR(counter_based_engine, lhs, rhs){ 
        return lhs.c==rhs.c && 
            lhs.next == rhs.next && 
            lhs.b == rhs.b; 
    }

    BOOST_RANDOM_DETAIL_INEQUALITY_OPERATOR(counter_based_engine)

    BOOST_RANDOM_DETAIL_OSTREAM_OPERATOR(os, counter_based_engine, f){
        os << (f.next) << ' ';
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
        if( next == Nresult ){
            incr();
            vu.range = b(c);
            next = 0;
        }
        //return vu.result[next++];
        return nth_result(next++);
    }

    void discard(boost::uintmax_t skip){
	size_t newnext = next + (skip % Nresult);
        skip /= Nresult;
        if (newnext > Nresult) {
            newnext -= Nresult;
	    skip++;
        }
        incr(skip);
        setnext(newnext);
    }
         
    template <class Iter>
    void generate(Iter first, Iter last)
    { detail::generate_from_int(*this, first, last); }

    // The member functions functions below *extend* the standard
    // Random Number Engine concept.  They provide the ability to
    // quickly 'restart' the engine with a new 'base counter'.
    // Restart is very fast, and restarted engines will produce
    // independent sequences as long as thier 'base counters'
    // differ in at least one bit.

    // restart - restart the counter with a new 'base counter' without
    //  touching the Prf or its key.  The counter is reset so there
    //  are again 2^CtrBits counters available.
    void restart(domain_type start){ 
        chk_highbasebits(start);
        c = start;
        next = Nresult;
    }

    // Constructor and seed() method to construct or re-seed a
    // counter_based_engine from a key or a Prf and an optional base
    // counter.  It's an error to specify a key with high bits set
    // because the high bits are reserved for use by the engine to
    // disambiguate engines created with different CounterBits.
    explicit counter_based_engine(key_type k, domain_type base = domain_type()) : b(chk_highkeybits(k)), c(base), next(Nresult){
        chk_highbasebits(base);
    }

    explicit counter_based_engine(const Prf& _b, domain_type base = domain_type()) : b(_b), c(base), next(Nresult){
        chk_highkeybits(b.getkey());
        chk_highbasebits(base);
    }

    void seed(key_type k, domain_type base){
        //std::cerr << "cbe::seed(Prf, base)\n";
        *this = counter_based_engine(k, base);
    }        

    void seed(const Prf& _b, domain_type base){
        //std::cerr << "cbe::seed(Prf, base)\n";
        *this = counter_based_engine(_b, base);
    }        


};

} // namespace random
} // namespace boost

#endif // BOOST_RANDOM_COUNTER_BASED_ENGINE_HPP
