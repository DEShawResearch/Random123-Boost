
// Copyright 2010-2014, D. E. Shaw Research.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt )

#include "concepts.hpp"
#include <boost/cstdint.hpp>
#include <string>
#include <sstream>
#include "rangeIO.hpp"
#include "printlogarray.hpp"

#define BOOST_TEST_MAIN
#include <boost/test/unit_test.hpp>

template <typename Prf>
void dokat(const std::string& s){
    std::istringstream iss(s);
    typename Prf::domain_type ctr;
    typename Prf::key_type key;
    typename Prf::range_type answer;
    iss>>std::hex;
    iss>>rangeExtractor(ctr.begin(), ctr.end());
    iss>>rangeExtractor(key.begin(), key.end());
    iss>>rangeExtractor(answer.begin(), answer.end());
    typename Prf::range_type computed = Prf(key)(ctr);
    BOOST_CHECK_EQUAL(computed, answer);
    Prf prf(key);
    BOOST_CHECK_EQUAL(prf(ctr), answer);
}

