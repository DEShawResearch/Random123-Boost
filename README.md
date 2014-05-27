Random123-Boost
===============

Proposed Random123 functions for Boost.Random

The purpose of this source tree is to develop a new family of
"Counter Based Uniform Random Number Engines" (CBENGs) for the
Boost.Random library.  CBENGs were introduced in the paper, "Parallel
Random Numbers -- As Easy as 1, 2, 3", by Salmon, Moraes, Dror & Shaw,
which won the Best Paper award at the SC'11  conference:

http://dl.acm.org/citation.cfm?doid=2063405
also available at
http://www.thesalmons.org/john/random123/papers/random123sc11.pdf

CBENG's are fully generic "Random Number Engines", satisfying all the
requirements of C++11 and Boost Engines.  They can be used anywhere
that other engines can be used.  When used with the provided "pseudo
random functions", threefry and philox, CBENGs are "crush resistant",
i.e., they pass the very demanding suite of 'SmallCrush' 'Crush' and
'BigCrush' tests in the TestU01 package.  CBENGs based on threefry and
philox are fast (comparable to the mersenne twister) and small
(requiring only a few words of state).

CBENGs are excellent random number engines in any context, but two
features make them especially well suited to parallel computations:
* a fast (constant time) restart() member function that allows
  the caller to manage a huge number of distinct streams.
* a fast (constant time) discard() member function.

The restart() member is unique to CBENGs, while discard() follows standard
semantics (but very few engines permit constant-time implementations).

Parallel Random Number Generation
---------------------------------

Conventional generators (such as those in Boost.Random or the C++11
<random> library) are large and difficult to initialize.  Boost's
documentation explicitly advises against frequent initialization, so
common practice is to serialize access through a single global
generator, or, in a parallel program, to instantiate one generator
per-thread.

Unlike conventional engines, CBENGs require very little storage and are
designed to be created, destroyed and restarted frequently.  If
created and destroyed in an "inner loop", a good compiler can often
keep their state entirely in registers and can generate random values
with a few dozen inlined instructions.  In addition, they have
extremely fast, constant-time implementations of 'discard()', which
allows callers to easily "leapfrog" through a logical stream of random
numbers.

Consider the following program fragment, using a conventional
RandomNumberEngine, mt19937:

    mt_19937 rng(seed); // seed may come from the command line
    normal_distribution nd;
    for(size_t i=0; i<Natoms; ++i){
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

CBENGs overcome all these problems.  With CBENGs, the code looks like
this (see libs/random/examples/counter_based_example.cpp for a fully
worked example):

    typedef threefry<4, uint32_t> Prf;
    counter_based_engine<uint32_t, Prf, 32> cbeng(seed);
    for(size_t i=0; i<Natoms; ++i){
        float boltzmannfactor = sqrt(kT/atoms[i].mass);
        normal_distribution nd(0., rmsvelocity);
        cbeng.restart({atoms[i].id, timestep, THERMALIZE_CTXT}); // C++11 initializer list
        atoms[i].vx = boltzmannfactor*nd(cbeng);
        atoms[i].vy = boltzmannfactor*nd(cbeng);
        atoms[i].vz = boltzmannfactor*nd(cbeng);
    }

Let's consider the code changes between the two fragments:

- threefry is a family of "pseudo-random functions" (PRF).  Template
parameters give the caller some control over the internals.
Reasonable values for the template parameters are discussed below.

        typedef threefry<4, uint32_t> Prf;

This particular prf has a 128 bit "domain", a 128 bit "range"
and a 128 bit "key".

- counter_based_engine is a templated adapter class.  Its template
parameters are a result_type, a PRF type and the number of bits
reserved for its 'counter'.

        counter_based_engine<uint32_t, Prf, 32> cbeng(seed);

    The sequence length of the engine is determined by the third
template parameter (in this case 32).  The PRF in this example has a
128-bit domain so each "turn of the crank" generates 128 bits, which
counter_based_engine converts into 4 successive outputs of the
result_type (uint32_t).  Thus, with a 32-bit counter this engine can
generate "only" 4*2^32 random values before it needs to be
restart()-ed with a new 'base counter' (see below).  Since we only use
it to generate three normally distributed values, there is no danger
of exhausting the counter.  In fact, we could have gotten away with
many fewer counter bits (3 would probably suffice).

- The restart() function is a public member of counter_based_engine
(but not of other C++11 or Boost Engines).  We use it to restart the
counter_based_engine with a new base counter every time through the
loop.  In this example, the engine's 'domain_type' is 128 bits wide
and we've reserved 32 counter bits for the engine, leaving us with 96
bits to be managed as a 'base counter' -- more than enough to
comfortably create a new, unique base counter every time through the
loop:

    In C++11, with initializer-lists, this looks like:

         cbeng.restart({atoms[i].id, timestep, THERMALIZE_CTXT}

Without initializer lists, it's slightly more verbose to create an
instance of the domain type:

         uint32_t base[3] = {atoms[i].id, 0, MASS_ASSIGN_CTXT};
         Prf::domain_type d = engine_t::domain_traits::make_counter(&base[0], &base[3]);
         cbeng.restart(d);

    Counter_based_urngs restart()-ed or constructed from different
bases, even bases that differ in only a single bit, will generate
independent, non-overlapping sequences of random values.  Thus, by
choosing a value in the domain that encodes some program-specific
state (e.g., atoms[i].id and timestep), we produce a unique stream for
each atom at each timestep that is statistically independent of all
other streams.  The additional constant THERMALIZE_CTXT distinguishes
this loop from any other loop or context in the program, eliminating
the possibility that the same sequence will be generated elsewhere.

    Notice that the random values generated for a particular atom at a
particular timestep are independent of the number of threads, how they
are scheduled and the assignment of atoms to threads.




Pseudo-random functions:  Philox and Threefry
---------------------------------------------

Two Pseudo-Random functions are implemented in this source tree:
threefry and philox.  They are both described in the SC11 paper.  Both
are templated over an unsigned element-count, an unsigned integer
value type, and a round-count (which takes a reasonable and safe
default value).  For example:

     threefry<4, uint32_t>
     philox<2, uint64_t>

All PRFs have a key_type, a domain_type, and a range_type, all
of which are boost::arrays of the underlying value type.  I.e.,

     threefry<N, U>::key_type    = boost::array<U, N>
                   domain_type = boost::array<U, N>
                   range_type  = boost::array<U, N>

     philox<N, U>::key_type    = boost::array<U, N/2>
                   domain_type = boost::array<U, N>
                   range_type  = boost::array<U, N>
   
PRFs are keyed pseudo-random functions.  Two Prfs of the same type,
initialized with the same key are indistinguishable and two Prfs
initialized with different keys, even keys that differ in only a single
bit appear to be unrelated, statistically independent pseudo-random
functions.  Pseudo-random functions produce apparently random output simply by
"counting" through inputs in an perfectly regular way.  

The counter_based_engine conceptually maps 'seeds' to Prf keys and
then "counts" through the Prf's domain, generating random output in
the Prf's range and then slicing the range into individual
'result_types' which are returned by operator()().  Strong empirical
evidence is presented in the SC11 paper for the statistical quality of
the threefry and philox functions. Among other things, threefry and
philox pass the entire BigCrush suite of tests and will not repeat
over their entire domain, of length 2^(N*sizeof(U)).  They are also
very fast - 1.5-4 cycles per output byte on modern CPUs.

For threefry and philox, initialization, i.e., re-keying or re-seeding
is also fast.  They can safely be initialized inside an inner loop.
For other PRFs, e.g., the cryptographic AES function (not
implemented), there may be non-trivial computation associated with
initialization, so initializing PRFs isn't always appropriate in
*inner* loops, but is perfectly reasonable at thread-scope or anywhere
else that a few dozen invocations of the generator will amortize the
initialization cost.

Finally, note that although they borrow some design elements from
cryptography, threefry and philox are emphatically *NOT* cryptographic
pseudo-random functions.  TestU01 confirms that their output contains
no discernable statistical anomalies, but nevertheless, a determined
attacker could almost certainly determine their internal state from a
sample of outputs.

counter_based_engine
--------------------

The counter_based_engine class uses the pseudo-random property of PRFs
to generate random values meeting the requirements of a
UniformRandomNumberGenerator.  It reserves 'CounterBits' of
most-significant bits of the highest-index members of the domain_type
array for its own internal use as a "counter".

Whenever new random values are required, the counter bits are
incremented and the PRF is called, generating a new set of random
values that will be returned by operator()().  It is an error to
request random values after the counter bits are exhausted.  Thus,
the sequence length of a counter_based_engine is under programmer
control.  It can be as short as 2 or as long as the length of the
domain_type (up to 2^256 for the PRFs provided).

The remaining bits, i.e., the non-CounterBits are available to the
programmer to use as a 'base counter', which can be set via the
restart(Prf::domain_type) member function, via an overloaded
constructor, or via an overloaded seed() member function.  The base
counter allows the program to manage practially unlimited numbers (up
to 2^(256-CounterBits)) of random sequences, each of length
2^CounterBits.  By logically associating independent random seqences
with distinct program elements (c.f., atom.id+thread, in the example
above), it's possible to write parallel programs whose output is
independent of thread scheduling or work assignment.

'Seeding' a counter_based_engine corresponds to 'keying' its
underlying Prf.  But note that the key space is typically much larger
than a single value of the engine's output_type.  So,
counter_based_engine has additional constructors that allow it to be
constructed or seeded from a value of the Prf's key_type or from
an existing Prf.  I.e.,

    counter_based_engine(uintmax_t);
    counter_based_engine(Prf::key_type);
    counter_based_engine(const Prf&)

It is also possible to set the base_count at construction time:

    counter_based_engine(Prf::key_type, Prf::domain_type);
    counter_based_engine(const Prf&, Prf::domain_type);
  
And of course, there are corresponding overloads of seed():

    seed(uintmax_t);
    seed(Prf::key_type);
    seed(const Prf&);
    seed(Prf::key_type, Prf::domain_type);
    seed(const Prf&, Prf::domain_type);

To prevent collisions between counter_based_engines instantiated with
different values of CtrBits, the counter_based_engine reserves the top
few bits of the full key space to itself.  Attempting to set a key
(either with the constructor or with seed(...) that has non-zero
bits in the highest log_2(DomainBits) of its key will throw an
invalid_argument exception.

Some performance-related features make counter_based_engines
particularly well-suited for parallel applications:

- they have very small memory footprint: Typically a domain_type (2-4
words), a range_type (2-4 words) a Prf (1-4 words) and an integer
index (one word).  Good compilers can often fit it all in registers,
especially if the CBENG has limited lifetime.  Thus, it can be
beneficial to create and destroy a CBENG as close as
possible/convenient to the loop that uses it.  The short lifetime may
allow a good compiler to optimize creation, use and destruction down
to a handful of instructions operating on a handful of temporary
registers.

- restart() is little more than a fixed number of integer assignments.
It is very fast and highly amenable to optimization.  It is intended
for use in inner loops, near where the engine is used.

- discard() is also very fast and runs in constant time.  Parallel
programs can use this to "leapfrog" multiple sequences over one
another in different threads, or it can be used to initialize
generators in different threads with starting points that are
separated by enough to avoid overlap.  Parallelization strategies
based on discard() will generally prefer large values of CtrBits, (128
or more), allowing large jumps and long sequences without fear of
exhaustion.
