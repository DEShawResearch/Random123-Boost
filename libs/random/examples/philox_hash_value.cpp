// A few (2) rounds of a PRNG has some appeal as a 'randomizing'
// alternative to the existing boost::hash_value.
//
// This is just a sketch... 
//
// A fully general version would require quite a bit more template
// armor to handle all the integer-promotion corner cases and aliases
// and the fact that philox and threefry have only uint32 and uint64
// variants.
//
// How many rounds is enough?  Hash_value's main use is to index into
// hash tables, so it may not have to be a certifiably "good" random
// number generator which would require 7 rounds for philox and 13 for
// threefry.  Two rounds of philox or four rounds of threefry "looks"
// random, which may be sufficient.  That may be "random" enough to
// satisfy people who are unhappy with the complete lack of 
// randomness in the current (1.55) implementation of hash_value(v).

#include <boost/random/philox.hpp>
#include <boost/cstdint.hpp>

size_t hash_value(uint32_t x){
    typedef uint32_t Uint;
    boost::array<Uint, 2> c = {x, 0};
    return size_t(boost::random::philox<2, Uint, 2>()(c)[0]);
}

size_t hash_value(uint64_t x){
    typedef uint64_t Uint;
    boost::array<Uint, 2> c = {x, 0};
    return size_t(boost::random::philox<2, Uint, 2>()(c)[0]);
}

int main(int argc, char **argv){
    for(unsigned i=0; i<10; ++i){
        size_t hv = hash_value(i);
        std::cout << "hv(" << i << ") = " << hv << std::hex << " = " << hv << std::dec << "\n";
    }
    return 0;
}
