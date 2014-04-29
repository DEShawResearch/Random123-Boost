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
o
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
#ifndef BOOST_RANDOM_DETAIL_MULHILO_HPP
#define BOOST_RANDOM_DETAIL_MULHILO_HPP

#include <limits>
#include <boost/cstdint.hpp>
#include <boost/integer.hpp>
#include <boost/utility/enable_if.hpp>
#include <boost/static_assert.hpp>

namespace boost{
namespace random{
namespace detail{

// First, we implement hilo multiplication with "half words".  This is
// the "reference implementation" which should be correct for any
// binary unsigned integral UINT with an even number of bits.  In
// practice, we try to avoid this implementation because it is so slow
// (4 multiplies plus about a dozen xor, or, +, shift, mask and
// compare operations.
template <typename Uint>
inline Uint 
mulhilo_halfword(Uint a, Uint b, Uint& hip){ 
    BOOST_STATIC_ASSERT(std::numeric_limits<Uint>::is_specialized &&
                        std::numeric_limits<Uint>::is_integer &&
                        !std::numeric_limits<Uint>::is_signed &&
                        std::numeric_limits<Uint>::radix == 2 &&
                        std::numeric_limits<Uint>::digits%2 == 0);
    const unsigned WHALF = std::numeric_limits<Uint>::digits/2;
    const Uint LOMASK = ((Uint)(~(Uint)0)) >> WHALF;
    Uint lo = a*b;
    Uint ahi = a>>WHALF;
    Uint alo = a& LOMASK;
    Uint bhi = b>>WHALF;
    Uint blo = b& LOMASK;
                                                                   
    Uint ahbl = ahi*blo;
    Uint albh = alo*bhi;
                                                                   
    Uint ahbl_albh = ((ahbl&LOMASK) + (albh&LOMASK));
    Uint hi = (ahi*bhi) + (ahbl>>WHALF) +  (albh>>WHALF);
    hi += ahbl_albh >> WHALF;
    /* carry from the sum with alo*blo */                               
    hi += ((lo >> WHALF) < (ahbl_albh&LOMASK));
    hip = hi;
    return lo;
}

// We can formulate a much faster implementation if we can use
// integers of twice the width of Uint (e.g., DblUint).  Such types
// are not always available (e.g., when Uint is uintmax_t), but when
// they are, we find that modern compilers (gcc, MSVC, Intel) pattern
// match the structure of the mulhilo below and turn it into an
// optimized instruction sequence, e.g., mulw or mull.
//
// However, the alternative implementation, which we want to use
// when there *is* a DblUint would be ambiguoous without some enable_if
// hackery.  To support that, we need a has_double_width type trait.
//
// N.B.  It should be possible to do this with some SFINAE wrapped
// around an instantiation of uint_t<2*W>::least.  My attempts to do
// so ran into the problem described in
// https://svn.boost.org/trac/boost/ticket/6169 from Nov 23, 2011
// (still open in April 2014).  So instead just check that twice the
// number of digits in Uint is less than or equal to the number of
// digits in uintmax_t.  FWIW, there is currently (as of 1.53)
// a BOOST_STATIC_ASSERT in integer.hpp that insists on a very
// similar condition:  Bits <= sizeof(boost::uintmax_t)*CHAR_BIT.

template <typename Uint>
class has_double_width{
public:
    static const bool value = std::numeric_limits<Uint>::is_specialized &&
        std::numeric_limits<Uint>::is_integer &&
        !std::numeric_limits<Uint>::is_signed &&
        std::numeric_limits<Uint>::radix == 2 &&
        2*std::numeric_limits<Uint>::digits <= std::numeric_limits< boost::uintmax_t >::digits;
};

// mulhilo using double-width DblUint
template <typename Uint>
inline typename boost::enable_if_c<has_double_width<Uint>::value, Uint>::type
mulhilo(Uint a, Uint b, Uint& hip){
    typedef typename uint_t<2*std::numeric_limits<Uint>::digits>::least DblUint;
    DblUint product = ((DblUint)a)*((DblUint)b);
    hip = product>>std::numeric_limits<Uint>::digits;
    return (Uint)product;
}

// When there is no DblUint and there are no specializations that use
// machine-specific intrinsics (below), fall back to mulhilo_halfword.
template <typename Uint>
inline typename boost::enable_if_c<!has_double_width<Uint>::value, Uint>::type 
mulhilo(Uint a, Uint b, Uint& hip){
    return mulhilo_halfword(a, b, hip);
}

// Every ISA I know (x86, ppc, arm, CUDA) has an instruction that
// gives the hi word of the product of two uintmax_t's FAR more
// quickly and succinctly than a call to mulhilo_halfword.  Without
// them, philox<N, uintmax_t> would be impractically slow.
// Unfortunately, they require compiler-and-hardware-specific
// intrinsics or asm statements.
//
// FIXME - add more special cases here, e.g., MSVC intrinsics and
// asm for PowerPC and ARM.
#if defined(__GNUC__) && defined(__x86_64__)
template <>
inline uint64_t 
mulhilo(uint64_t ax, uint64_t b, uint64_t& hip){
    uint64_t dx;
    __asm__("\n\t"
        "mulq %2\n\t"
        : "=a"(ax), "=d"(dx)
        : "r"(b), "0"(ax)
        );
    hip = dx;
    return ax;
}
#endif

} // namespace detail
} // namespace random
} // namespace boost

#endif // BOOST_RANDOM_DETAIL_MULHILO_HPP
