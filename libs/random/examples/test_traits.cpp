
// Copyright 2014, D. E. Shaw Research.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt )

#include <boost/random/detail/counter_traits.hpp>
#include <boost/cstdint.hpp>
#include <x86intrin.h>

#include <iostream>

typedef boost::array<uint32_t, 4> a4x32;
typedef boost::array<uint32_t, 8> a8x32;
struct foo{ typedef int value_type; };

static const bool b1 = boost::random::detail::isBoostArray<a4x32>::value;
static const bool b2 = boost::random::detail::isBoostArray<int>::value;

typedef boost::random::detail::counter_traits<a4x32> ct4x32;
typedef boost::random::detail::counter_traits<a8x32 > ct8x32;
typedef boost::random::detail::counter_traits<foo > ctint;
typedef boost::random::detail::counter_traits<__m128i> ct1m128;

int main(int artc, char **argv){
    std::cout << boost::is_class<a4x32>::value << "\n";
    std::cout << b1 << "\n";
    std::cout << b2 << "\n";
    a4x32 a = ct4x32::key_from_value(19);
    ct4x32::insert(std::cout, a);
    a8x32 b = ct8x32::key_from_value(19);
    ct8x32::insert(std::cout, b);
    return 0;
}
