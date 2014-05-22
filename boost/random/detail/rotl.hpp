
// Copyright 2010-2014, D. E. Shaw Research.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt )

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
