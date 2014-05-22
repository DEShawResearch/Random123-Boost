
// Copyright 2014, D. E. Shaw Research.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt )

#ifndef BOOST_RANDOM_SHA1_PRF_HPP
#define BOOST_RANDOM_SHA1_PRF_HPP

#include <boost/static_assert.hpp>
#include <boost/array.hpp>
#include <boost/uuid/sha1.hpp>
#include <boost/cstdint.hpp>

namespace boost{
namespace random{

// sha1_prf - a wrapper around the SHA-1 implementation in
//   boost::uuids::detail::sha1 that provides all the necessary
//   methods and typedefs for the "PseudoRandomFunction" concept.
//   Thus, it can be used with counter_based_engine to provide a slow
//   but extremely high-quality random number generator.  E.g.,
//
//     counter_based_engine<sha1_prf<4,2> > 
//
//   The template parameter, Ndomain and Nkey specify how many 32-bit
//   words to assign to the Pseudo-random function's domain and key.
//   They have default values of 4 and 2.
//
//   Speed: random_speed.cpp reports that it's about 50x slower than
//   fast generators like mersenne or threefry4x64.  On the other
//   hand, it's faster than some of the 'ranlux' generators.
//
//   Quality: Assuming that boost::uuids::detail::sha1 really is sha1,
//   and that there are no bugs in counter_based_engine, then any
//   statistical failure of the engine would imply that SHA-1 itself
//   is "broken".  Not impossible, but it would come as a tremendous
//   surprise to the cryptography community and have huge implications
//   for computer security.

template <unsigned Ndomain=4, unsigned Nkey=2>
class sha1_prf {
    // SHA-1 is spec'ed in terms of 32-bit, big-endian words.
    // But uuids::detail::sha1 only has byte-oriented input
    // functions that would be endian-sensitive if we passed
    // the address of c.data() or k.data() to them.  So
    // we define a uint32 input function to eliminate the
    // endian-sensitivity.
    static void process_u32(uuids::detail::sha1& h, uint32_t v){
        h.process_byte((v>>24)&0xff);
        h.process_byte((v>>16)&0xff);
        h.process_byte((v>> 8)&0xff);
        h.process_byte((v    )&0xff);
    }
    
public:
    typedef array<uint32_t, Ndomain> domain_type;
    typedef array<uint32_t, 5> range_type;
    typedef array<uint32_t, Nkey> key_type ;

    sha1_prf() : k(){}
    sha1_prf(key_type _k) : k(_k) {}
    sha1_prf(const sha1_prf& v) : k(v.k){}

    void setkey(key_type _k){
        k = _k;
    }

    key_type getkey() const{
        return k;
    }

    bool operator==(const sha1_prf& rhs) const{
        return k == rhs.k;
    }

    bool operator!=(const sha1_prf& rhs) const{
        return k != rhs.k;
    }

    range_type operator()(domain_type c){
        boost::uuids::detail::sha1 h;
        // salt with Nkey to disambiguate sha1_prf<Ndomain1,Nkey1>
        // from sha1_prf<Nkdomain2,Nkey2> when
        //   Ndomain1+Nkey1 == Nkdomain2+Nkey2
        BOOST_STATIC_ASSERT(uint8_t(Nkey) == Nkey);
        h.process_byte(Nkey);  
        for(unsigned i=0; i<Nkey; ++i)
            process_u32(h, this->k[i]);
        for(unsigned i=0; i<Ndomain; ++i)
            process_u32(h, c[i]);
        range_type ret;
        h.get_digest(ret.elems);
        return ret;
    }

protected:
    key_type k;
};

} // namespace random
} // namespace boost

#endif // BOOST_RANDOM_SHA1_PRF_HPP
