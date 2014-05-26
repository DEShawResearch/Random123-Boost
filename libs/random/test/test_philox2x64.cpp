
// Copyright 2010-2014, D. E. Shaw Research.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt )

#include <boost/random/philox.hpp>
#include <boost/cstdint.hpp>

#define BOOST_COUNTER_BASED_ENGINE_RESULT_TYPE uint64_t
#define BOOST_PSEUDO_RANDOM_FUNCTION boost::random::philox<2, uint64_t>
#define BOOST_COUNTER_BASED_ENGINE_CTRBITS 32

#define BOOST_RANDOM_SEED_WORDS 2

#define BOOST_RANDOM_VALIDATION_VALUE UINT64_C(12548399570197440638)
#define BOOST_RANDOM_SEED_SEQ_VALIDATION_VALUE UINT64_C(1950189153452696268)
#define BOOST_RANDOM_ITERATOR_VALIDATION_VALUE UINT64_C(16632851287075411822)

#define BOOST_RANDOM_GENERATE_VALUES { 2554582833,  3389038661, 3383248309, 1724006946 }
#include "test_counter_based_engine.ipp"

