Random123-Boost
===============

Proposed Random123 functions for Boost.Random

The purpose of this source tree is to develop a new family of
"Counter Based Uniform Random Number Generators" (CBURNGs) for the
Boost.Random library.  CBURNGs were introduced in the paper, "Parallel
Random Numbers -- As Easy as 1, 2, 3", by Salmon, Moraes, Dror & Shaw,
which won the Best Paper award at the SC'11  conference:

http://dl.acm.org/citation.cfm?doid=2063405
also available at
http://www.thesalmons.org/john/random123/papers/random123sc11.pdf

Conventional generators (such as those in Boost.Random or the C++11
<random> library) are large and difficult to initialize.  Boost's
documentation explicitly advises against frequent initialization, so
common practice is to serialize access through a single global
generator, or, in a parallel program, to instantiate one generator
per-thread.

Unlike conventional generators CBURNGs require very little storage and
are designed to be created and destroyed frequently.  If created and
destroyed in an "inner loop", a good compiler can often keep their
state entirely in registers and can generate random values with a few
dozen inlined instructions.  In addition, they have extremely fast,
constant-time implementations of 'discard()', which allows callers to
easily "leapfrog" through a logical stream of random numbers.
These features make them ideal for parallel computation. 

Consider the following program fragment, using a conventional
RandomNumberEngine, mt19937:

    using namespace boost::random;
    mt_19937 rng(seed); // seed may come from the command line
    normal_distribution nd;
    for(size_t i=0; i<atoms.size(); ++i){
        float boltzmannfactor = sqrt(kT/atoms[i].mass);
        atoms[i].vx = boltzmannfactor*nd(rng);
        atoms[i].vy = boltzmannfactor*nd(rng);
        atoms[i].vz = boltzmannfactor*nd(rng);
    }

Now imagine parallelizing this loop over a number of threads or cores.
The conventional approach is to create an independent generator in
each thread, perhaps folding a thread-id into the seed, or using a
'discard' function so that threads can "leapfrog" over one another.
But both these solutions result in output that depends on the number
of threads and how atoms are assigned to threads.  Furthermore,
improperly seeding millions of generators (not an unreasonable number
on a modern supercomputer) can lead to unintended correlations among
streams, so conventional wisdom is that one must very carefully choose
the engine and the initialization method.  The problem is even harder
if the overall program structure (i.e., not just this loop) dictates a
parallelization strategy that might assign the same atom to multiple
threads.

CBRNGs overcome all these problems.  With CBRNGs, the code looks like
this (see libs/random/examples/counter_based_example.cpp for a fully
worked example):

    using namespace boost::random;
    typedef threefry<4, uint32_t> Prf;
    normal_distribution nd;
    Prf::key_type key = {seed};
    Prf prf(key);  // seed may come from command line
    for(size_t i=0; i<atoms.size(); ++i){
        float boltzmannfactor = sqrt(kT/atoms[i].mass);
        Prf::domain_type d = {atoms[i].id, timestep, THERMALIZE_CTXT};
        counter_based_urng<Prf> cbrng(prf, d);
        nd.reset();
        atoms[i].vx = boltzmannfactor*nd(cbrng);
        atoms[i].vy = boltzmannfactor*nd(cbrng);
        atoms[i].vz = boltzmannfactor*nd(cbrng);
    }

Let's consider the code changes between the two fragments:

- counter_based_urng is a templated adapter class that models a bona
fide Boost UniformRandomNumberGenerator.  Its template parameter is a
Pseudo-Random Function (PRF).  An instance of the Pseudo-Random
Function and a value from the Pseudo-Random Function's domain are
required to construct a counter_based_urng.  E.g., these lines in the
example:

        Prf::domain_type d = {atoms[i].id, timestep, THERMALIZE_CTXT};
        counter_based_urng<Prf> cbrng(prf, d);

    In C++11, with initializer-lists, this might be shortened to:

       counter_based_urng<Prf> cbrng(prf, {atoms[i].id, timestep, THERMALIZE_CTXT});

    Creation and destruction of the counter_based_urng is much faster than
    actually generating random values or processing them through a
    distribution, so it's reasonable to create and destroy the cbrng every
    time through the loop.
    
    Counter_based_urngs constructed from the same domain value and the
    same PRF are identical, i.e., they will generate exactly the same
    sequence.  On the other hand, counter_based_urngs constructed from
    domain values that differ in even a single bit generate independent,
    non-overlapping sequences of random values.  Thus, by choosing a value
    in the domain that encodes some program-specific state (e.g.,
    atoms[i].id and timestep), we are produce a unique
    stream for each atom at each timestep that is statistically
    independent of all other streams.  Notice that the random values
    generated for a particular atom at a particular timestep are
    independent of the number of threads or the assignment of atoms to
    threads.  The additional constant THERMALIZE_CTXT is used to
    distinguish this loop from any other loop or context in the program,
    eliminating the possibility that the same sequence will be generated
    elsewhere in the program.

- Since it models a URNG, cbrng can be passed as an argument to the
normal_distribution, nd.  In order to make each atom independent,

        nd.reset() 
    
    is called each time through the loop.

- PRFs are keyed.  I.e., the Prf constructor takes a key_type as an
argument.  Two Prfs of the same type, initialized with the same key
are indistinguishable.  On the other hand, two Prfs constructed from
keys that differ in any way, even by a single bit, will give rise to
statistically independent counter_based_urngs and output streams.

    In the example, we initialize the PRF outside the loop with:

        Prf::key_type key = {seed};
        Prf prf(key);  // seed may come from command line



Pseudo-random functions:  Philox and Threefry
---------------------------------------------

Two Pseudo-Random functions implemented in this source tree: threefry
and philox.  Both are templated over an unsigned width, an unsigned
integer type, and a round-count (which takes a reasonable and safe
default value).  For example:

   threefry<4, uint32_t>
   philox<2, uint64_t>

All PRFs have a key_type, a domain_type, and a range_type, all
of which are boost::arrays of the underlying unsigned type.  I.e.,

   threefry<N, U>::key_type    = boost::array<U, N>
                   domain_type = boost::array<U, N>
                   range_type  = boost::array<U, N>

     philox<N, U>::key_type    = boost::array<U, N/2>
                   domain_type = boost::array<U, N>
                   range_type  = boost::array<U, N>
   
For threefry and philox, initialization is extremely fast.  For other
PRFs, e.g., the cryptographic AES function (not implemented), there
may be non-trivial computation associated with initialization, so
initializing PRFs is discouraged in *inner* loops, but is perfectly
reasonable at thread-scope or anywhere else that a few dozen
invocations of the generator will amortize the initialization cost.

Threefry and Philox are "pseudo-random", meaning that the outputs from
any set of inputs are practically indistiguishable from random.  In
particular, one can obtain apparently random output simply by
"counting" through inputs in an entirely regular way.  Strong
empirical evidence is presented in the SC11 paper for the statistical
quality of the threefry and philox functions.  Furthermore, the
pseudo-random functions obtained with different keys are shown to be
statistically independent, even if the keys differ by only a single
bit or follow regular patterns.  Among other things, threefry and
philox pass the entire BigCrush suite of tests.

counter_based_urng
------------------

The counter_based_urng class uses the pseudo-random property of PRFs
to perform random number generation in accordance with the
requirements of a UniformRandomNumberGenerator.  It reserves some
number (by default, all) of most-significant bits of the highest-index
member of the domain_type array for its own internal use as a
"counter".  It is an error for the domain_type constructor argument to
have non-zero bits in this range.  Whenever new random values are
required, the "counter bits" are incremented and the PRF is called,
generating a new set of random values that will be returned by
counter_based_urng.  It is an error to request random values after the
counter is exhausted.  Thus, counter_based_urngs will typically have
fairly "short" sequence lengths (anything from 4 up to 2^64).  This is
usually more than sufficient to provide input to one or more
Distributions, which generally call their engine a non-deterministic,
but usually small number of times.  On the other hand, it is cheap and
efficient to create huge numbers (2^64 or more) of independent
counter_based_urngs on demand.


counter_based_engine
--------------------

The counter_based_engine class is a templated adapter class that
models a bona fide RandomNumberEngine.  In particular, it is Seedable
in the same way as other Engines, so it can be used in any program
that expects a RandomNumberEngine.  The counter_based_engine offers
two very useful properties:

1 - it has a very small size, and a very fast constructor, so it is
practical to instantiate millions of them in a large parallel
application.

2 - the discard() function is very fast and runs in constant time.
Parallel programs can use this to "leapfrog" multiple sequences over
one another in different threads, or it can be used to initialize
generators in different threads with starting points that are
separated by enough to avoid overlap.
