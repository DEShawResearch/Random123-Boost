#include <boost/random/philox.hpp>
#include <boost/random/ars.hpp>
#include <boost/random/aes.hpp>
#include <cassert>
#include "rangeIO.hpp"

//using boost::detail::rangeInserter;

// A real KAT test would read from a data file.  To get a
// minimal sanity check, we hard-code a few of the lines
// from the Random123 kat_vectors.
int main(int argc, char **argv){
    {
    typedef boost::random::philox<2, uint32_t> G;
    G gen;
    G::ctr_type c = {{0x243f6a88, 0x85a308d3}};
    G::ctr_type kat = {{0x249cdc9c, 0x4f745808}};
    G::key_type k = {{1}};
    G::ctr_type r = gen(c, k);
    std::cout << std::hex << rangeInserter(r.begin(), r.end()) << std::endl;
    assert( r == kat );
    }

    {
    typedef boost::random::philox<4, uint64_t> G;
    G gen;
    G::ctr_type c = {{0x243f6a8885a308d3, 0x13198a2e03707344, 0xa4093822299f31d0, 0x082efa98ec4e6c89}};
    G::ctr_type kat = {{0xc306b6f69047b4d3, 0x50a141b88a52f51f, 0xc1255f575c5efa40, 0xe9ec1af0c5fd12e7}};
    G::key_type k = {{1, 0}};
    G::ctr_type r = gen(c, k);
    std::cout << std::hex << rangeInserter(r.begin(), r.end()) << std::endl;
    assert( r == kat );
    }

    {
    typedef boost::random::ars<uint64_t, 10> G;
    G gen;
#if 1
    G::ctr_type c = {{0x85a308d3243f6a88, 0x0370734413198a2e }};
    G::key_type k = {{0x299f31d0a4093822, 0xec4e6c89082efa98 }};
    G::ctr_type kat = {{0x8357ad74a516e7d6, 0x8763fff35b59b3ec }};
#else
    G::ctr_type c = {{0x0, 0x0}};
    G::ctr_type kat = {{0x506401ef8d73ee19, 0x0cbe9c0d13c2dbe4}};
    G::key_type k = {{0x0, 0x0}};
#endif
    G::ctr_type r = gen(c, k);
    std::cout << std::hex << rangeInserter(r.begin(), r.end()) << std::endl;
    assert( r == kat );
    }

    {
    typedef boost::random::ars<uint32_t, 10> G;
    G gen;
#if 1
    G::ctr_type c = {{0x243f6a88, 0x85a308d3, 0x13198a2e, 0x03707344}};
    G::key_type k = {{0xa4093822, 0x299f31d0, 0x082efa98, 0xec4e6c89}};
    G::ctr_type kat = {{0xa516e7d6, 0x8357ad74, 0x5b59b3ec, 0x8763fff3}};
#else
    G::ctr_type c = {{0x0, 0x0}};
    G::ctr_type kat = {{0x8d73ee19, 0x506401ef, 0x13c2dbe4, 0x0cbe9c0d }};
    G::key_type k = {{0x0, 0x0}};
#endif
    G::ctr_type r = gen(c, k);
    std::cout << std::hex << rangeInserter(r.begin(), r.end()) << std::endl;
    assert( r == kat );
    }

    {
    typedef boost::random::ars<uint32_t, 10> G;
    G gen(false);
#if 0
    G::ctr_type c = {{0x243f6a88, 0x85a308d3, 0x13198a2e, 0x03707344}};
    G::key_type k = {{0xa4093822, 0x299f31d0, 0x082efa98, 0xec4e6c89}};
    G::ctr_type kat = {{0xa516e7d6, 0x8357ad74, 0x5b59b3ec, 0x8763fff3}};
#else
    G::ctr_type c = {{0x0, 0x0}};
    G::ctr_type kat = {{0x8d73ee19, 0x506401ef, 0x13c2dbe4, 0x0cbe9c0d }};
    G::key_type k = {{0x0, 0x0}};
#endif
    G::ctr_type r = gen(c, k);
    std::cout << std::hex << rangeInserter(r.begin(), r.end()) << std::endl;
    assert( r == kat );
    }

    {
    typedef boost::random::aes<uint32_t> G;
    G gen;
#if 1
    G::ctr_type c = {{0x243f6a88, 0x85a308d3, 0x13198a2e, 0x03707344}};
    G::key_type k = {{0xa4093822, 0x299f31d0, 0x082efa98, 0xec4e6c89}};
    G::ctr_type kat = {{0xca693cbf, 0x134a4f64, 0x965e0cfd, 0x5217a28f}};
#else
    G::ctr_type c = {{0x0, 0x0}};
    G::ctr_type kat = {{0xd44be966, 0x3b2c8aef, 0x59fa4c88, 0x2e2b34ca}};
    G::key_type k = {{0x0, 0x0}};
#endif
    G::ctr_type r = gen(c, k);
    std::cout << std::hex << rangeInserter(r.begin(), r.end()) << std::endl;
    assert( r == kat );
    }

    {
    typedef boost::random::aes<uint32_t> G;
    G gen(false);
#if 1
    G::ctr_type c = {{0x243f6a88, 0x85a308d3, 0x13198a2e, 0x03707344}};
    G::key_type k = {{0xa4093822, 0x299f31d0, 0x082efa98, 0xec4e6c89}};
    G::ctr_type kat = {{0xca693cbf, 0x134a4f64, 0x965e0cfd, 0x5217a28f}};
#else
    G::ctr_type c = {{0x0, 0x0}};
    G::ctr_type kat = {{0xd44be966, 0x3b2c8aef, 0x59fa4c88, 0x2e2b34ca}};
    G::key_type k = {{0x0, 0x0}};
#endif
    G::ctr_type r = gen(c, k);
    std::cout << std::hex << rangeInserter(r.begin(), r.end()) << std::endl;
    assert( r == kat );
    }

    return 0;
}
