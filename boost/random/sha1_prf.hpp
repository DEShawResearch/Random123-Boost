/** @page LICENSE
Copyright 2014, D. E. Shaw Research.
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

#ifndef BOOST_RANDOM_SHA1_PRF_HPP
#define BOOST_RANDOM_SHA1_PRF_HPP

#include <boost/random/detail/prf_common.hpp>
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
//     counter_based_engine<sha1_prf<4,1> > 
//
//   The template parameter, Ndomain and Nkey specify how many 32-bit
//   words to assign to the Pseudo-random function's domain and key.
//   They have default values of 4 and 1.
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

template <unsigned Ndomain=4, unsigned Nkey=1>
class sha1_prf : public detail::prf_common<Ndomain, 5, Nkey, uint32_t>{
    typedef detail::prf_common<Ndomain, 5, Nkey, uint32_t> common_type;
    typedef typename common_type::domain_type _domain_type;
    typedef typename common_type::range_type _range_type;
    typedef typename common_type::key_type _key_type;

    // Calling h.process_bytes(c.data(), Ndomain*sizeof(uint32_t))
    // would be endian-dependent.  So we do this instead:
    void process_u32(uuids::detail::sha1& h, uint32_t v){
        h.process_byte((v    )&0xff);
        h.process_byte((v>> 8)&0xff);
        h.process_byte((v>>16)&0xff);
        h.process_byte((v>>24)&0xff);
    }
    
public:
    sha1_prf(_key_type _k=_key_type()) : common_type(_k){}

    _range_type operator()(_domain_type c){
        boost::uuids::detail::sha1 h;
        for(int i=0; i<Nkey; ++i)
            process_u32(h, this->k[i]);
        for(int i=0; i<Ndomain; ++i)
            process_u32(h, c[i]);
        _range_type ret;
        h.get_digest(ret.elems);
        return ret;
    }
};

} // namespace random
} // namespace boost

#endif // BOOST_RANDOM_SHA1_PRF_HPP
