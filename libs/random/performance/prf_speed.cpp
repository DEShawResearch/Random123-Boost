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
#include <boost/random/counter_based_urng.hpp>

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
    static const unsigned N=_N;
    typedef boost::array<UINT, N> domain_type;
    typedef boost::array<UINT, N> range_type;
    typedef boost::array<UINT, 1> key_type;
    typedef UINT dvalue_type;
    typedef UINT rvalue_type;
    typedef domain_type domain_array_type;
    IdentityPrf(key_type&){}
    static dvalue_type domain_array_min()  { return std::numeric_limits<dvalue_type>::min(); }
    static dvalue_type domain_array_max()  { return std::numeric_limits<dvalue_type>::max(); }
    static domain_type make_domain(domain_array_type da){ return da; }
    
    typedef range_type range_array_type;
    static rvalue_type range_array_min()  { return std::numeric_limits<rvalue_type>::min(); }
    static rvalue_type range_array_max()  { return std::numeric_limits<rvalue_type>::max(); }
    static range_array_type make_range_array(range_type r) {return r;}

    range_type operator()(domain_type c){ return c; }
};

// simplest "random" number generator possible, to check on overhead
class counting
{
public:
  typedef int result_type;

  BOOST_STATIC_CONSTANT(bool, has_fixed_range = false);

  counting() : _x(0) { }
  result_type operator()() { return ++_x; }
  result_type min BOOST_PREVENT_MACRO_SUBSTITUTION () const { return 1; }
  result_type max BOOST_PREVENT_MACRO_SUBSTITUTION () const { return (std::numeric_limits<result_type>::max)(); }

private:
  int _x;
};


// decoration of variate_generator to make it runtime-exchangeable
// for speed comparison
template<class Ret>
class RandomGenBase
{
public:
  virtual Ret operator()() = 0;
  virtual ~RandomGenBase() { }
};

template<class URNG, class Dist, class Ret = typename Dist::result_type>
class DynamicRandomGenerator
  : public RandomGenBase<Ret>
{
public:
  DynamicRandomGenerator(URNG& urng, const Dist& d) : _rng(urng, d) { }
  Ret operator()() { return _rng(); }
private:
  boost::variate_generator<URNG&, Dist> _rng;
};

template<class Ret>
class GenericRandomGenerator
{
public:
  typedef Ret result_type;

  GenericRandomGenerator() { };
  void set(boost::shared_ptr<RandomGenBase<Ret> > p) { _p = p; }
  // takes over ownership
  void set(RandomGenBase<Ret> * p) { _p.reset(p); }
  Ret operator()() { return (*_p)(); }
private:
  boost::shared_ptr<RandomGenBase<Ret> > _p;
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
  for(int i = 0; i < iter; i++)
    tmp ^= rng();
  show_elapsed(t.elapsed(), iter, name, sizeof(tmp));
  if(tmp==0)
      std::cerr << name << ": The xor is zero.  That's surprising!\n";
}

template <typename Prf>
void run_cburng(const std::string& name, int iter){
    std::string pfx= "counter_based_urng<" + name + ">";
    // Initialize the Prf with a key not known at compile time.
    // Otherwise, the compiler might elide some of the key-related
    // code.
    typename Prf::key_type k = {iter};
    run(iter, pfx, boost::random::counter_based_urng<Prf>(Prf(k), typename Prf::domain_type()));
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

  std::cout << "\nURNGs:\n";
  run_cburng<IdentityPrf<2, uint64_t> >("Ident2x64", iter);
  run_cburng<boost::random::philox<2, uint64_t> >("philox2x64", iter);
  run_cburng<boost::random::philox<4, uint64_t> >("philox4x64", iter);
  run_cburng<boost::random::philox<4, uint64_t, 7> >("philox4x64-7", iter);
  run_cburng<boost::random::threefry<2, uint64_t> >("threefry2x64", iter);
  run_cburng<boost::random::threefry<4, uint64_t, 13> >("threefry4x64-13", iter);
  
  return 0;
}
