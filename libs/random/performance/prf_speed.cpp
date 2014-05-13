/* boost random_speed.cpp performance measurements
 *
 * Copyright Jens Maurer 2000
 * Distributed under the Boost Software License, Version 1.0. (See
 * accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * $Id: //depot/desrad/main/sw/libs/Random123/boost/libs/random/performance/prf_speed.cpp#1 $
 */

#include <iostream>
#include <cstdlib>
#include <string>
#include <boost/config.hpp>
#include <boost/random.hpp>
#include <boost/progress.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/random/philox.hpp>
#include <boost/random/threefry.hpp>
#include <boost/random/counter_based_engine.hpp>

/*
 * Configuration Section
 */

// define if your C library supports the non-standard drand48 family
//#define HAVE_DRAND48

// define if you have the original mt19937int.c (with commented out main())
#undef HAVE_MT19937INT_C

// define if you have the original mt19937ar.c (with commented out main())
// #define HAVE_MT19937AR_C

// set to your CPU frequency
static const double cpu_frequency = 3.07 * 1e9;

using namespace boost::random;

/*
 * End of Configuration Section
 */

/*
 * General portability note:
 * MSVC mis-compiles explicit function template instantiations.
 * For example, f<A>() and f<B>() are both compiled to call f<A>().
 * BCC is unable to implicitly convert a "const char *" to a std::string
 * when using explicit function template instantiations.
 *
 * Therefore, avoid explicit function template instantiations.
 */

// provides an "identity" prf, just for comparison
template <unsigned _N, typename UINT>
struct IdentityPrf{
    static const unsigned Ndomain=_N;
    static const unsigned Nrange=_N;
    static const unsigned Nkey=1;
    static const unsigned range_bits=std::numeric_limits<UINT>::digits*Nrange;
    typedef boost::array<UINT, Ndomain> domain_type;
    typedef boost::array<UINT, Nrange> range_type;
    typedef boost::array<UINT, 1> key_type;
    typedef UINT dvalue_type;
    typedef UINT rvalue_type;
    IdentityPrf(key_type){}
    IdentityPrf(UINT){}

    range_type operator()(domain_type c){ return c; }
};

// start implementation of measuring timing

void show_elapsed(double end, int iter, const std::string & name, size_t bytes_per_iter)
{
  double usec = end/iter*1e6;
  double cycles = usec * cpu_frequency/1e6;
  std::cout << name << ": " 
            << usec*1e3 << " nsec/loop = "
            << cycles << " CPU cycles = "
            << cycles/bytes_per_iter << " CPB"
            << std::endl;
}

template<class RNG>
void run(int iter, const std::string & name, RNG rng)
{
  std::cout << (RNG::has_fixed_range ? "fixed-range " : "");
  // BCC has trouble with string autoconversion for explicit specializations
  
  // make sure we're not optimizing too much
  std::cout << "Engine: ";
  typename RNG::result_type tmp = 0;
  boost::timer t;
  for(int i = 0; i < iter; i++){
      tmp ^= rng();
  }
  show_elapsed(t.elapsed(), iter, name, sizeof(tmp));
  if(tmp==0)
      std::cerr << name << ": The xor is zero.  That's surprising!\n";
}

template <typename Otype, typename Prf>
void  __attribute__((noinline)) run_cbeng(const std::string& name, int iter){
    std::string pfx= "counter_based_engine<" + name + ">";
    // N.B.  Use 24 bits of iter as the key.  Not really necessary, but
    // it prevents the compiler from possibly performing some of the key
    // arithmetic at compile-time.
    run(iter, pfx, counter_based_engine<Otype, Prf>(iter&0xffffff));
}

void do_threefry(int iter){
  // WARNING - if we include the 2x32 tests, then gcc-4.8 reports
  // *much lower* (3x) performance for some of the other functions.
  // Wild guess - we've hit some limit meant to prevent too much
  std::cout << "Threefry: with recommended safety margin\n";
  run_cbeng<uint64_t, threefry<4, uint64_t> >("threefry4x64", iter);
  run_cbeng<uint32_t, threefry<4, uint64_t> >("threfry4x64/32", iter);
  run_cbeng<uint32_t, threefry<2, uint32_t> >("threefry2x32", iter);
  //run_cbeng<uint32_t, threefry<4, uint32_t> >("threefry4x32", iter);
  //run_cbeng<uint64_t, threefry<2, uint64_t> >("threefry2x64", iter);

  std::cout << "Threefry:  Crush-resistant, with no safety margin\n";
  // Note - on a 3.07GHz Xeon 5667 (Westmere) threefry4x64-12 should
  // be the winner at around 1.7 CPB.
  run_cbeng<uint64_t, threefry<4, uint64_t, 12> >("threefry4x64-12", iter);
  run_cbeng<uint32_t, threefry<4, uint64_t, 12> >("threefry4x64-12/32", iter);
  //run_cbeng<uint64_t, threefry<4, uint32_t, 13> >("threefry4x32-13/64", iter);
  //run_cbeng<threefry<4, uint32_t, 12> >("threefry4x32-12", iter);
  //run_cbeng<threefry<2, uint64_t, 13> >("threefry2x64-13", iter);
  //run_cbeng<threefry<2, uint32_t, 13> >("threefry2x32-13", iter);
}

void do_philox(int iter){
  std::cout << "Philox: with recommended safety margin\n";
  run_cbeng<uint64_t, philox<4, uint64_t> >("philox4x64", iter);
  run_cbeng<uint32_t, philox<4, uint32_t> >("philox4x32", iter);
  run_cbeng<uint64_t, philox<2, uint64_t> >("philox2x64", iter);
  run_cbeng<uint32_t, philox<2, uint32_t> >("philox2x32", iter);

  std::cout << "Philox:  Crush-resistant, with no safety margin\n";
  run_cbeng<uint64_t, philox<4, uint64_t, 7> >("philox4x64-7", iter);
  run_cbeng<uint32_t, philox<4, uint32_t, 7> >("philox4x32-7", iter);
  run_cbeng<uint64_t, philox<2, uint64_t, 6> >("philox2x64-6", iter);
  run_cbeng<uint32_t, philox<2, uint32_t, 7> >("philox2x32-7", iter);
}

int main(int argc, char*argv[])
{
  if(argc != 2) {
    std::cerr << "usage: " << argv[0] << " iterations" << std::endl;
    return 1;
  }

  // okay, it's ugly, but it's only used here
  int iter =
#ifndef BOOST_NO_STDC_NAMESPACE
    std::
#endif
    atoi(argv[1]);

  std::cout << "\nPseudo-random functions:\n";
  //run_cbeng<uint64_t, IdentityPrf<2, uint64_t> >("Ident2x64", iter);

  do_threefry(iter);
  do_philox(iter);
  
  return 0;
}
