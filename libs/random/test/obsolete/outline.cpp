// A rough outline of what a Boost PRF library might look like:

#include <boost/random/counter_based_engine.hpp>
#include <boost/random/counter_based_urng.hpp>
#include <boost/random/threefry.hpp>
#include <boost/random/philox.hpp>
#include <boost/random/ars.hpp>
#include <boost/random/aes.hpp>

#include <iostream>
#include <boost/random/normal_distribution.hpp>
#include "rangeIO.hpp"

//using namespace boost::random;

template <typename Prf>
void doit(){
    Prf prf;
    typename Prf::ctr_type c={{}};
    typename Prf::key_type uk={{}};
    typename Prf::ctr_type r = prf(c, uk);
    std::cout << rangeInserter(r.begin(), r.end()) << "\n";

    boost::random::normal_distribution<double> nd(1.0, 2.0);
    boost::random::counter_based_engine<Prf> e;
    std::cout << e() << "\n";
    std::cout << e << "\n";
    std::cout << nd(e) << "\n";

    boost::random::counter_based_engine<Prf> e2(2);
    assert(e2 != e);
    e2.seed();
    e2();
    nd.reset();
    nd(e2);
    assert(e2 == e);

    boost::random::counter_based_urng<Prf> murng(c, uk, 5);
    std::cout << nd(murng) << "\n";
}

int main(int argc, char **argv){

    doit<boost::random::threefry<2, uint64_t> >();
    doit<boost::random::philox<2, uint64_t> >();
    doit<boost::random::philox<2, uint32_t> >();
    doit<boost::random::philox<4, uint32_t> >();
    doit<boost::random::philox<4, uint64_t> >();
    boost::random::ars<__m128i, 7> ars128i;
    
    boost::random::counter_based_engine<boost::random::ars<uint32_t, 7> > ars7;
    std::cout << ars7() << "\n";
    
    boost::random::counter_based_engine<boost::random::aes<uint64_t> > aese;

    return 0;
}
