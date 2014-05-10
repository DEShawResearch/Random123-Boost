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

#include <boost/random/sha1_prf.hpp>
#include <boost/cstdint.hpp>

#define BOOST_PSEUDO_RANDOM_FUNCTION boost::random::sha1_prf<4, 1>
#define BOOST_COUNTER_BASED_ENGINE_CTRBITS 32

#define BOOST_RANDOM_SEED_WORDS 1
#define BOOST_RANDOM_SEED_MASK (0x1ffffff)  // 25 bits = 32-log2(Ndomain*32)

#define BOOST_RANDOM_VALIDATION_VALUE 900845231u
#define BOOST_RANDOM_SEED_SEQ_VALIDATION_VALUE 1245297593u
#define BOOST_RANDOM_ITERATOR_VALIDATION_VALUE 3701510912u

#define BOOST_RANDOM_GENERATE_VALUES { 1812412800u, 2705721983u, 3772052448u, 3271012669u }

#include "test_counter_based_engine.ipp"

