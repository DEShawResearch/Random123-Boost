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

Conventional generators (such as those in Boost.Random or the C++11
<random> library) are large and difficult to initialize.  Boost's
documentation explicitly advises against frequent initialization, so
common practice is to serialize access through a single global
generator, or, in a parallel program, to instantiate one generator
per-thread.

Unlike conventional engines CBENGs require very little storage and are
designed to be created, destroyed and restarted frequently.  If
created and destroyed in an "inner loop", a good compiler can often
keep their state entirely in registers and can generate random values
with a few dozen inlined instructions.  In addition, they have
extremely fast, constant-time implementations of 'discard()', which
allows callers to easily "leapfrog" through a logical stream of random
numbers.  These features make them ideal for parallel computation.

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
    counter_based_engine<Prf, 32> cbeng(seed);
    for(size_t i=0; i<Natoms; ++i){
        float boltzmannfactor = sqrt(kT/atoms[i].mass);
        normal_distribution nd(0., rmsvelocity);
        Prf::domain_type base = {atoms[i].id, timestep, THERMALIZE_CTXT};
        cbeng.restart(base);
        atoms[i].vx = boltzmannfactor*nd(cbeng);
        atoms[i].vy = boltzmannfactor*nd(cbeng);
        atoms[i].vz = boltzmannfactor*nd(cbeng);
    }

Let's consider the code changes between the two fragments:

- counter_based_engine is a templated adapter class.  Its template
parameters are a Pseudo-Random Function (PRF) and the number of bits in
its 'counter'.

    typedef threefry<4, uint32_t> Prf;
    counter_based_engine<Prf, 32> cbeng(seed);

    This engine has "only" a 32-bit counter.  So it can only generate
4*2^32 random values before it needs to be restarted (typically with a
different 'base counter').  But we only use it to generate three
normally distributed values, so there is no danger of exhausting
the counter.

    We restart the generator with a new base counter every time
through the loop.  In this example, the 'domain_type' of the
threefry<4, uint32_t> pseudo-random function is (as the template
arguments suggest) 128 bits wide.  We've allocated 32 "Counter Bits"
via the seconed template argument to counter_based_engine.  That
leaves us with 96 bits to be managed as a 'base counter'.  We choose a
distinct base counter every time through the loop, and use it to
restart the engine:

         Prf::domain_type base = {atoms[i].id, timestep, THERMALIZE_CTXT};
         cbeng.restart(base);

    In C++11, with initializer-lists, this might be shortened to:

         cbeng.restart({atoms[i].id, timestep, THERMALIZE_CTXT});

    Counter_based_urngs constructed from different bases, even bases
that differ in only a single bit, will generate independent,
non-overlapping sequences of random values.  Thus, by choosing a value
in the domain that encodes some program-specific state (e.g.,
atoms[i].id and timestep), we produce a unique stream for each atom at
each timestep that is statistically independent of all other streams.
Notice that the random values generated for a particular atom at a
particular timestep are independent of the number of threads or the
assignment of atoms to threads.  The additional constant
THERMALIZE_CTXT is used to distinguish this loop from any other loop
or context in the program, eliminating the possibility that the same
sequence will be generated elsewhere in the program.



Pseudo-random functions:  Philox and Threefry
---------------------------------------------

Two Pseudo-Random functions are implemented in this source tree: threefry
and philox.  Both are templated over an unsigned width, an unsigned
integer value type, and a round-count (which takes a reasonable and safe
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

PRFs are keyed.  Two Prfs of the same type, initialized with the
same key are indistinguishable and two Prfs initialized with different
keys, even if they differ in only a single bit will give rise to
statistically independent sequences (even if they are restarted with
the same base counter).  The counter_based_engine maps 'seeds'
to Prf keys.

counter_based_engine
--------------------

The counter_based_engine class uses the pseudo-random property of PRFs
to generate random values meeting the requirements of a
UniformRandomNumberGenerator.  It reserves 'CounterBits' of
most-significant bits of the highest-index members of the domain_type
array for its own internal use as a "counter".

Whenever new random values are required, the counter bits are
incremented and the PRF is called, generating a new set of random
values that will be returned by counter_based_urng.  It is an error to
request random values after the counter bits are exhausted.  Thus,
the sequence length of a counter_based_engine is under programmer
control.  It can be as short as 2 or as long as the length of the
domain_type (up to 2^256 for the PRFs provided).

The remaining bits, i.e., the non-CounterBits are available to the
programmer to use as a 'base counter', which can be set via the
restart(Prf::domain_type) member function, via an overloaded
constructor, or via an overloaded seed() member function.  The base
counter allows the program to manage practially unlimited numbers (up
to 2^(256-CounterBits)) random sequences up to 2^CounterBits in
length.  By logically associating independent random seqences with
distinct program elements (c.f., atom.id+thread, in the example
above), it's possible to write parallel programs whose output is
independent of thread scheduling or work assignment.

'Seeding' a counter_based_engine corresponds to 'keying' its
underlying Prf.  But note that they key space is typically much larger
than a single value of the engine's output_type.  So,
counter_based_engine has additional constructors that allow it to be
constructed or seeded from a value of the Prf's key_type.  I.e.,

    counter_based_engine(Prf::key_type);

It is also possible to set the base_count at construction time:

    counter_based_engine(Prf::key_type, Prf::domain_type);
  
And of course, there are corresponding overloads of seed():

    seed(Prf::key_type);
    seed(Prf::key_type, Prf::domain_type);

Some performance-related features make counter_based_engines
particularly well-suited for parallel applications:

- they have very small memory footprint: Typically a domain_type (2-4
words), a range_type (2-4 words) a Prf (2-4 words) and an integer
index (one word).  Good compilers can often fit it all in registers,
especially if it has limited lifetime.  Thus, it can be beneficial to
create and destroy a CBENG as close as possible/convenient to the loop
that uses it.  The short lifetime may allow a good compiler to
optimize creation, use and destruction down to a handful of
instructions operating on a handful of temporary registers.

- restart() is little more than a fixed number of integer assignments.
It is very fast and highly amenable to optimization.

- discard() is also very fast and runs in constant time.  Parallel
programs can use this to "leapfrog" multiple sequences over one
another in different threads, or it can be used to initialize
generators in different threads with starting points that are
separated by enough to avoid overlap.
