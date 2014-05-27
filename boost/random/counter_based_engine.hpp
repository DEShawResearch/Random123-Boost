
// Copyright 2010-2014, D. E. Shaw Research.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt )

#ifndef BOOST_RANDOM_COUNTER_BASED_ENGINE_HPP
#define BOOST_RANDOM_COUNTER_BASED_ENGINE_HPP

#include <boost/array.hpp>
#include <boost/cstdint.hpp>
#include <boost/limits.hpp>
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

#if !defined(BOOST_NO_CXX11_HDR_INITIALIZER_LIST)
#include <initializer_list>
#endif

namespace boost{
namespace random{

template<typename UintType,
         typename Prf, 
         unsigned CtrBits = static_unsigned_min<64u, detail::counter_traits<typename Prf::domain_type>::Nbits/2>::value,
         unsigned w = std::numeric_limits<UintType>::digits,
         typename DomainTraits = detail::counter_traits<typename Prf::domain_type>,
         typename RangeTraits = detail::counter_traits<typename Prf::range_type>,
         typename KeyTraits = detail::counter_traits<typename Prf::key_type>
>
struct counter_based_engine {
    typedef UintType result_type;
    BOOST_STATIC_CONSTANT(unsigned, word_size = w);
    BOOST_STATIC_CONSTANT(bool, has_fixed_range = false);

    // The following typedefs *extend* the standard
    // Random Number Engine concept.
    typedef Prf prf_type;
    BOOST_STATIC_CONSTANT(unsigned, counter_bits = CtrBits);
    typedef typename Prf::domain_type domain_type;
    typedef typename Prf::range_type range_type;
    typedef typename Prf::key_type key_type;
    typedef DomainTraits domain_traits;
    typedef RangeTraits range_traits;
    typedef KeyTraits key_traits;

protected:
    BOOST_STATIC_ASSERT(CtrBits <= DomainTraits::Nbits);
    BOOST_STATIC_ASSERT(CtrBits > 0);

    BOOST_STATIC_ASSERT( std::numeric_limits<UintType>::digits >= w );
    BOOST_STATIC_CONSTANT(unsigned, CtrBitsBits = static_log2<DomainTraits::Nbits>::value);

    prf_type b;
    domain_type c;
    range_type v;
    unsigned next;

    // new values for c and next, maintaining the
    // invariant that v=b(c).
    void setctr(domain_type newc, unsigned newnext){
        c = newc;
        v = b(c);
        next = newnext;
    }

    // We avoid collisions between engines with different CtrBits by
    // embedding the value CtrBits-1 in the high few bits of the last
    // element of Prf's key.  If we didn't do this, it would be too
    // easy for counter_based_engine<Prf, N> and
    // counter_based_engine<Prf, M> to "collide", producing
    // overlapping streams.
    //
    // When we accept a key from the user, e.g., seed(arithmetic) or
    // seed(key_type), an exception is thrown by chk_highkeybits if
    // the specified key has any high bits set.  On the other hand,
    // set_highkeybits, which is called by the seed_seq constructor
    // and seeder, quietly ignores high bits in its argument.
    key_type chk_highkeybits(key_type k){
        if( KeyTraits::template clr_highbits<CtrBitsBits>(k) )
            BOOST_THROW_EXCEPTION(std::invalid_argument("counter_based_engine:: high bits of key are reserved for internal use."));
        return KeyTraits::template incr<CtrBitsBits>(k, CtrBits-1);
    }

    key_type set_highkeybits(key_type k){
        KeyTraits::template clr_highbits<CtrBitsBits>(k);
        return KeyTraits::template incr<CtrBitsBits>(k, CtrBits-1);
    }

    template <typename SeedSeq>
    key_type make_key_from_seedseq(SeedSeq& seq){
        return set_highkeybits(KeyTraits::_make_counter_from_seedseq(seq));
    }

    template <typename It>
    key_type make_key_from_range(It first, It last, It *endp){
#if 1
        // This is here because test_iterator_seed in
        // test_generator.ipp demands it.  But it serves no useful
        // purpose.  KeyTraits::make_counter safely and correctly pads
        // with zeros if there aren't enough values in the range.
        // There's no reason to throw an exception.
        typedef typename std::iterator_traits<It>::value_type it_value_type;
        const unsigned it_value_bits = std::numeric_limits<it_value_type>::digits + std::numeric_limits<it_value_type>::is_signed;
        if( std::distance(first, last)*it_value_bits < KeyTraits::Nbits )
            BOOST_THROW_EXCEPTION(std::invalid_argument("counter_based_engine: not enough bits in range"));
#endif
        // Note that unlike all(?) other engines, the construct-from-range
        // and seed-from-range methods of counter_based_engine use *all*
        // the bits in the given range.  They do not gratuitously and
        // unnecessarily truncate the values to [0, 2^32).  Thankfully,
        // this does not trip over any checks in test_generator.ipp.
        return KeyTraits::make_counter(first, last, endp);
    }

public:
    BOOST_RANDOM_DETAIL_CONSTEXPR static result_type min BOOST_PREVENT_MACRO_SUBSTITUTION () { return 0; }
    BOOST_RANDOM_DETAIL_CONSTEXPR static result_type max BOOST_PREVENT_MACRO_SUBSTITUTION () { return low_bits_mask_t<w>::sig_bits; }

    counter_based_engine()
        : b()
    {
        //std::cerr << "cbe()\n";
        setctr(DomainTraits::make_counter(), 0);
    }

    counter_based_engine(counter_based_engine& e) 
        : b(e.b)
    {
        //std::cerr << "cbe(counter_based_engine&)\n";
        setctr(e.c, e.next);
    }

    counter_based_engine(const counter_based_engine& e) 
        : b(e.b)
    {
        //std::cerr << "cbe(const counter_based_engine&)\n";
        setctr(e.c,  e.next);
    }

    counter_based_engine& operator=(const counter_based_engine& rhs){
        b = rhs.b;
        setctr(rhs.c, rhs.next);
        return *this;
    }

    // Note that the arithmetic constructor (and seed method, below)
    // takes a uintmax_t, which may be wider than a result_type.
    // Normal arithmetic promotions allow it to be called with a
    // result_type argument, but counter_based_engine does not
    // unnecessarily restrict the range of possible seeds to the width
    // of a result_type.  The constructor (and seed method) throw if
    // the value contains non-zero bits that are not allowed in the
    // seed, e.g.,
    //
    //    counter_based_engine<uint32_t, philox<2, uint32_t> >(1ull<<28);
    // or
    //    counter_based_engine<uint32_t, philox<4, uint32_t> >(1ull<<60);
    BOOST_RANDOM_DETAIL_ARITHMETIC_CONSTRUCTOR(counter_based_engine, boost::uintmax_t, value)
        : b(chk_highkeybits(KeyTraits::make_counter(value)))
    { 
        //std::cerr << "cbe(result_type)\n";
        setctr(DomainTraits::make_counter(), 0);
    }

    BOOST_RANDOM_DETAIL_SEED_SEQ_CONSTRUCTOR(counter_based_engine, SeedSeq, seq)
        : b(make_key_from_seedseq(seq))
    {
        //std::cerr << "cbe(SeedSeq)\n";
        setctr(DomainTraits::make_counter(), 0);
    }

    template<class It> counter_based_engine(It& first, It last)
        // N.B.  does *NOT* throw if there are non-zero values
        // in the 'leftover' part of the range.  CALLER BEWARE!
        : b(chk_highkeybits(make_key_from_range(first, last, &first)))
    {
        //std::cerr << "cbe(range)\n";
        setctr(DomainTraits::make_counter(), 0);
    }

    template<class It> counter_based_engine(const It& first, It last)
        // N.B.  throws an invalid_argument if there are non-zero
        // values in the 'leftover' part of the range.
        : b(chk_highkeybits(make_key_from_range(first, last, 0)))
    {
        //std::cerr << "cbe(range)\n";
        setctr(DomainTraits::make_counter(), 0);
    }

    void seed(){
        //std::cerr << "cbe::seed()\n";
        b.setkey(KeyTraits::make_counter());
        setctr(DomainTraits::make_counter(), 0);
    }

    BOOST_RANDOM_DETAIL_ARITHMETIC_SEED(counter_based_engine, boost::uintmax_t, value)
    { 
        //std::cerr << "cbe::seed(arithmetic)\n";
        b.setkey(chk_highkeybits(KeyTraits::make_counter(value)));
        setctr(DomainTraits::make_counter(), 0);
    }

    BOOST_RANDOM_DETAIL_SEED_SEQ_SEED(counter_based_engine, SeedSeq, seq){
        //std::cerr << "cbe::seed(SeedSeq)\n" << "\n";
        b.setkey(make_key_from_seedseq(seq));
        setctr(DomainTraits::make_counter(), 0);
    }

    template<class It>
    void seed(It& first, It last){
        //std::cerr << "cbe::seed(range)\n";
        //
        // N.B.  does *NOT* throw if there are non-zero values
        // in the 'leftover' part of the range.  CALLER BEWARE!
        b.setkey(chk_highkeybits(make_key_from_range(first, last, &first)));
        setctr(DomainTraits::make_counter(), 0);
    }

    template<class It>
    void seed(const It& first, It last){
        //std::cerr << "cbe::seed(range)\n";
        //
        // N.B.  throws an invalid_argument if there are non-zero
        // values in the 'leftover' part of the range.
        b.setkey(chk_highkeybits(make_key_from_range(first, last, 0)));
        setctr(DomainTraits::make_counter(), 0);
    }

    BOOST_RANDOM_DETAIL_EQUALITY_OPERATOR(counter_based_engine, lhs, rhs){ 
        return DomainTraits::is_equal(lhs.c, rhs.c) && 
            lhs.next == rhs.next && 
            lhs.b == rhs.b; 
    }

    BOOST_RANDOM_DETAIL_INEQUALITY_OPERATOR(counter_based_engine)

    BOOST_RANDOM_DETAIL_OSTREAM_OPERATOR(os, counter_based_engine, f){
        os << (f.next) << ' ';
        DomainTraits::insert(os, f.c) << ' ';
        KeyTraits::insert(os,  f.b.getkey());
        return os;
    }

    BOOST_RANDOM_DETAIL_ISTREAM_OPERATOR(is, counter_based_engine, f){
        unsigned newnext;
        is >> newnext;
        domain_type newc;
        DomainTraits::extract(is, newc);
        key_type newk;
        KeyTraits::extract(is, newk);
        if( is ){
            f.b.setkey(newk);
            f.setctr(newc, newnext);
        }
        return is;
    }

    result_type operator()(){
        if( next == results_per_counter() )
            setctr(DomainTraits::template incr<CtrBits>(c), 0);
        return RangeTraits::template at<result_type, w>(next++, v);
    }

    void discard(boost::uintmax_t skip){
        const unsigned Nresult = results_per_counter();
	unsigned newnext = next + (skip % Nresult);
        skip /= Nresult;
        if (newnext > Nresult) {
            newnext -= Nresult;
	    skip++;
        }
        // next==0 is a corner case that only occurs at the beginning
        // of the sequence.  In order for operator== to work properly,
        // we must disambiguate (next=0,c+1) from (next=Nresult, c) here.
        if(newnext==0 && skip){
            newnext = results_per_counter();
            skip -= 1;
        }
        setctr(DomainTraits::template incr<CtrBits>(c, skip), newnext);
    }
         
    template <class Iter>
    void generate(Iter first, Iter last)
    { detail::generate_from_int(*this, first, last); }

    // The member functions below *extend* the standard Random Number
    // Engine concept.  
    //
    // The restart method, along with additional constructors and
    // seeders allow the caller to quickly 'restart' the
    // counter_based_engine with a new 'base counter'.  Restart is
    // very fast, and restarted counter_based_engines will produce
    // independent sequences as long as thier 'base counters' differ
    // in at least one bit.

    // restart - restart the counter with a new 'base counter'.  The
    //  Prf and key are unchanged.  The high CtrBits of the new base
    //  counter must be zero - those bits are incremented by the
    //  engine's operator()() and discard() members.
    void restart(domain_type base){ 
        if( DomainTraits::template clr_highbits<CtrBits>(base) )
            BOOST_THROW_EXCEPTION(std::invalid_argument("counter_based_engine:: high bits of key are reserved for internal use."));
            
        setctr(base, 0);
    }

#if !defined(BOOST_NO_CXX11_HDR_INITIALIZER_LIST)
    template <typename V>
    void restart(std::initializer_list<V> il){
        restart( DomainTraits::make_counter(il) );
    }
#endif

    // Constructor and seed() method to construct or re-seed a
    // counter_based_engine from a key or a Prf and an optional base
    // counter.
    explicit counter_based_engine(key_type k, domain_type base = DomainTraits::make_counter()) : 
        b(chk_highkeybits(k)), c(base), next(){
        if( DomainTraits::template clr_highbits<CtrBits>(base) )
            BOOST_THROW_EXCEPTION(std::invalid_argument("counter_based_engine base counter overlaps with counter bits"));
    }

    explicit counter_based_engine(const Prf& _b, domain_type base = DomainTraits::make_counter()) : b(_b), c(base), next(){
        chk_highkeybits(b.getkey());
        if( DomainTraits::clr_highbits<CtrBits>(base) )
            BOOST_THROW_EXCEPTION(std::invalid_argument("counter_based_engine base counter overlaps with counter bits"));
    }

    void seed(key_type k, domain_type base){
        //std::cerr << "cbe::seed(key_type, base)\n";
        *this = counter_based_engine(k, base);
    }        

    void seed(const Prf& _b, domain_type base){
        //std::cerr << "cbe::seed(Prf, base)\n";
        *this = counter_based_engine(_b, base);
    }        

    // results_per_counter - returns the number of random results made
    //  available by each invocation of the prf.  The total length of
    //  the sequence is results_per_counter() * 2^counter_bits.
    static unsigned results_per_counter(){
        return RangeTraits::template size<w>();
    }
};

} // namespace random
} // namespace boost

#endif // BOOST_RANDOM_COUNTER_BASED_ENGINE_HPP
