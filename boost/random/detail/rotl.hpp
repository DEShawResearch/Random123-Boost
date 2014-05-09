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
#ifndef BOOST_RANDOM_DETAIL_ROTL_HPP
#define BOOST_RANDOM_DETAIL_ROTL_HPP

#include <boost/static_assert.hpp>
#include <limits>

namespace boost{
namespace random{
namespace detail{
template <typename Uint>
static Uint rotl(Uint x, unsigned s){
    // Non-binary types would be a huge headache.  Sign-extension of
    // right-shift would not give the intended result.  These should
    // never come up, so it's not worth the trouble to try to handle
    // them "correctly".  But just in case they do, let's blow an
    // assertion rather than generating incorrect code.
    BOOST_STATIC_ASSERT(std::numeric_limits<Uint>::is_specialized);
    BOOST_STATIC_ASSERT(std::numeric_limits<Uint>::radix == 2);
    BOOST_STATIC_ASSERT(!std::numeric_limits<Uint>::is_signed);
    return (x<<s) | (x>>(std::numeric_limits<Uint>::digits-s));
}
}
}
}

#endif
