
// Copyright 2010-2014, D. E. Shaw Research.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt )

#ifndef BOOST_PRINT_LOG_ARRAY_HPP
#define BOOST_PRINT_LOG_ARRAY_HPP

#include <boost/test/test_tools.hpp>

// I'd like to be able to do BOOST_CHECK_XXX on arrays,
// It seems that I need to jump through these hoops to do
// it...
namespace boost{namespace test_tools{
template <typename T> struct print_log_value; // forward decl
}}
namespace std{ // why??
template <typename T, std::size_t N>
std::ostream& operator<<(std::ostream& os, const ::boost::array<T,N>& a){
    //boost::io::ios_flags_saver(os); // unnecessary?
    // N.B.  this looks pretty awful... More manipulators??  spaces?
    os << std::hex;
    for(size_t i=0; i<a.size(); ++i){
        boost::test_tools::print_log_value<T>()(os, a[i]);
        if(i!=a.size()) os << ' ';
    }
    return os;
}
}

#endif
