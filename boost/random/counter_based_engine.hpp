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

#include <boost/array.hpp>
#include <boost/cstdint.hpp>
#include <boost/limits.hpp>
#include <boost/random/seed_seq.hpp>
#include <boost/random/detail/seed.hpp>
#include <boost/random/detail/seed_impl.hpp>
#include <boost/random/detail/operators.hpp>
#include <boost/integer/static_min_max.hpp>
#include <boost/integer/static_log2.hpp>
#include <boost/integer/integer_mask.hpp>
#include <boost/random/detail/counter_traits.hpp>

#include <iosfwd>
#include <utility>
#include <stdexcept>
#include <algorithm>

namespace boost{
namespace random{

template<typename Prf, 
         unsigned CtrBits = static_unsigned_min<64u, detail::counter_traits<typename Prf::domain_type>::Nbits/2>::value,
         typename UintType = typename detail::counter_traits<typename Prf::range_type>::preferred_result_type,
         unsigned w = std::numeric_limits<UintType>::digits,
         typename DomainTraits = detail::counter_traits<typename Prf::domain_type>,
         typename RangeTraits = detail::counter_traits<typename Prf::range_type>,
         typename KeyTraits = detail::counter_traits<typename Prf::key_type>
>
struct counter_based_engine {
    typedef Prf prf_type;
    typedef UintType result_type;
    BOOST_STATIC_CONSTANT(unsigned, counter_bits = CtrBits);
    BOOST_STATIC_CONSTANT(unsigned, word_size = w);
    BOOST_STATIC_CONSTANT(bool, has_fixed_range = false);

protected:
    typedef typename Prf::range_type range_type;
    typedef typename Prf::domain_type domain_type;
    typedef typename Prf::key_type key_type;

    BOOST_STATIC_ASSERT(CtrBits <= DomainTraits::Nbits);
    BOOST_STATIC_ASSERT(CtrBits > 0);

    BOOST_STATIC_ASSERT( std::numeric_limits<UintType>::digits >= w );
    BOOST_STATIC_ASSERT( RangeTraits::Nbits%w == 0 );
    BOOST_STATIC_CONSTANT(unsigned, Nresult = RangeTraits::Nbits/w);

    BOOST_STATIC_CONSTANT(unsigned, CtrBitsBits = static_log2<DomainTraits::Nbits>::value);

    prf_type b;
    domain_type c;
    range_type v;
    unsigned next;

    void setnext(unsigned n){
        if( n != Nresult ){
            v = b(c);
        }
        next = n;
    }        

    void initialize(){
        c = domain_type();
        next = Nresult;
    }

    // We avoid collisions between engines with different CtrBits by
    // embedding the value CtrBits-1 in the high few bits of the last
    // element of Prf's key.  If we didn't do this, it would be too
    // easy for counter_based_engine<Prf, N> and
    // counter_based_engine<Prf, M> to "collide", producing
    // overlapping streams.
    //
    // When we accept a key from the user, e.g., seed(arithmetic) or
    // seed(CtrType), it is an error if the specified value has any
    // high bits set.  On the other hand, it's not an error if
    // seed_seq.generate() sets those bits -- we just ignore them.

    key_type chk_highkeybits(key_type k){
        if( KeyTraits::template clr_highbits<CtrBitsBits>(k) )
            BOOST_THROW_EXCEPTION(std::domain_error("counter_based_engine:: high bits of key are reserved for internal use."));
        return KeyTraits::template incr<CtrBitsBits>(k, CtrBits-1);
    }

    key_type set_highkeybits(key_type k){
        KeyTraits::template clr_highbits<CtrBitsBits>(k);
        return KeyTraits::template incr<CtrBitsBits>(k, CtrBits-1);
    }

public:
    BOOST_RANDOM_DETAIL_CONSTEXPR static result_type min BOOST_PREVENT_MACRO_SUBSTITUTION () { return 0; }
    BOOST_RANDOM_DETAIL_CONSTEXPR static result_type max BOOST_PREVENT_MACRO_SUBSTITUTION () { return low_bits_mask_t<w>::sig_bits; }

    counter_based_engine()
        : b(), c(), next(Nresult)
    {
        //std::cerr << "cbe()\n";
        initialize();
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
        : b(chk_highkeybits(KeyTraits::key_from_value(value)))
    { 
        //std::cerr << "cbe(result_type)\n";
        initialize();
    }

    BOOST_RANDOM_DETAIL_SEED_SEQ_CONSTRUCTOR(counter_based_engine, SeedSeq, seq)
        : b(set_highkeybits(KeyTraits::key_from_seedseq(seq)))
    {
        //std::cerr << "cbe(SeedSeq)\n";
        initialize();
    }

    template<class It> counter_based_engine(It& first, It last)
        : b(chk_highkeybits(KeyTraits::key_from_range(first, last)))
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
        b.setkey(chk_highkeybits(KeyTraits::key_from_value(value)));
        initialize();
    }

    BOOST_RANDOM_DETAIL_SEED_SEQ_SEED(counter_based_engine, SeedSeq, seq){
        //std::cerr << "cbe::seed(SeedSeq)\n" << "\n";
        b.setkey(set_highkeybits(KeyTraits::key_from_seedseq(seq)));
        initialize();
    }

    template<class It>
    void seed(It& first, It last){
        //std::cerr << "cbe::seed(range)\n";
        b.setkey(chk_highkeybits(KeyTraits::key_from_range(first, last)));
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
        DomainTraits::insert(os, f.c);
        KeyTraits::insert(os,  f.b.getkey());
        return os;
    }

    BOOST_RANDOM_DETAIL_ISTREAM_OPERATOR(is, counter_based_engine, f){
        size_t n;
        is >> n;
        DomainTraits::extract(is, f.c);
        key_type k;
        KeyTraits::extract(is, k);
        f.b.setkey(k);
        f.setnext(n);
        return is;
    }

    result_type operator()(){
        if( next == Nresult ){
            c = DomainTraits::template incr<CtrBits>(c);
            v = b(c);
            next = 0;
        }
        return RangeTraits::template nth_result<result_type, w>(next++, v);
    }

    void discard(boost::uintmax_t skip){
	size_t newnext = next + (skip % Nresult);
        skip /= Nresult;
        if (newnext > Nresult) {
            newnext -= Nresult;
	    skip++;
        }
        c = DomainTraits::template incr<CtrBits>(c, skip);
        setnext(newnext);
    }
         
    template <class Iter>
    void generate(Iter first, Iter last)
    { detail::generate_from_int(*this, first, last); }

    // The member functions below *extend* the standard Random Number
    // Engine concept.  They provide the ability to quickly 'restart'
    // the engine with a new 'base counter'.  Restart is very fast,
    // and restarted engines will produce independent sequences as
    // long as thier 'base counters' differ in at least one bit.

    // restart - restart the counter with a new 'base counter' without
    //  touching the Prf or its key.  The counter is reset so there
    //  are again 2^CtrBits counters available.
    void restart(domain_type start){ 
        DomainTraits::template chk_highbits<CtrBits>(start);
        c = start;
        next = Nresult;
    }

    // Constructor and seed() method to construct or re-seed a
    // counter_based_engine from a key or a Prf and an optional base
    // counter.  It's an error to specify a key with high bits set
    // because the high bits are reserved for use by the engine to
    // disambiguate engines created with different CounterBits.
    explicit counter_based_engine(key_type k, domain_type base = domain_type()) : 
        b(chk_highbits(k)), c(base), next(Nresult){
        DomainTraits::template clr_bits<CtrBits>(base);
    }

    explicit counter_based_engine(const Prf& _b, domain_type base = domain_type()) : b(_b), c(base), next(Nresult){
        chk_highkeybits(b.getkey());
        DomainTraits::chk_highbasebits<CtrBits>(base);
    }

    void seed(key_type k, domain_type base){
        //std::cerr << "cbe::seed(key_type, base)\n";
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
