
// Copyright 2014, D. E. Shaw Research.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt )

#include <boost/random/sha1_prf.hpp>
#include <boost/cstdint.hpp>

#define BOOST_COUNTER_BASED_ENGINE_RESULT_TYPE uint32_t
#define BOOST_PSEUDO_RANDOM_FUNCTION boost::random::sha1_prf<4, 1>
#define BOOST_COUNTER_BASED_ENGINE_CTRBITS 32

#define BOOST_RANDOM_SEED_WORDS 1
#define BOOST_RANDOM_SEED_MASK (0x1ffffff)  // 25 bits = 32-log2(Ndomain*32)

#define BOOST_RANDOM_VALIDATION_VALUE 900845231u
#define BOOST_RANDOM_SEED_SEQ_VALIDATION_VALUE 1245297593u
#define BOOST_RANDOM_ITERATOR_VALIDATION_VALUE 3701510912u

#define BOOST_RANDOM_GENERATE_VALUES { 1812412800u, 2705721983u, 3772052448u, 3271012669u }

#include "test_counter_based_engine.ipp"

