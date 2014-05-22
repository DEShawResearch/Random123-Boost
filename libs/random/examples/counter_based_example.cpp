
// Copyright 2014, D. E. Shaw Research.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt )

// Example code demonstrating how to set up and use a
// counter_based_engine in a threaded program.  Usage:
// 
//   counter_based_example [Nthread [Natoms]]
//
// Random numbers are generated in the functions 'assignmasses' and
// 'thermalize'.  By using a counter_based_engine, these functions can
// easily be called in multipe threads with the program's final result
// independent of the number of threads or the assignment of atoms to
// threads.

#include <boost/random/counter_based_engine.hpp>
#include <boost/random/threefry.hpp>
#include <boost/random/normal_distribution.hpp>
#include <boost/random/bernoulli_distribution.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/thread.hpp>
#include <vector>
#include <iostream>
#include <sstream>
#include <cmath>

using namespace boost::random;

// First, provide a tiny bit of illustrative "molecular dynamics" boilerplate.
struct atom{
    float vx, vy, vz; // m/s
    float mass;  // kg/mol
    unsigned id;
};
float kT = 8.314 * 300; // J/mol
float amu = 1.e-3;      // 1 amu = 1e-3 kg/mol

// Choose a pseudo-random function:
typedef threefry<2, uint64_t> Prf;
typedef counter_based_engine<uint32_t, Prf, 32> engine_t;

// An enum to distinguish the different contexts in which we generate
// randoms:
enum { THERMALIZE_CTXT, MASS_ASSIGN_CTXT };

// assignmasses - assigns masses, randomly with equal probability
// of m1 or m2.  Use a bernoulli distribution
void assignmasses(atom* atoms, size_t Natoms, float m1, float m2, const Prf::key_type& key){
    bernoulli_distribution<float> bd;
    engine_t cbeng(key);
    for(size_t i=0; i<Natoms; ++i){
#if __cplusplus>=201103L && defined(BOOST_NO_CXX11_HDR_INITIALIZER_LIST)
        cbeng.restart({atoms[i].id, 0, uint32_t(MASS_ASSIGN_CTXT)});
#else
        uint32_t base32[4] = {atoms[i].id, 0, MASS_ASSIGN_CTXT};
        Prf::domain_type start = engine_t::domain_traits::make_counter(&base32[0], &base32[4]);
        cbeng.restart(start);
#endif
        bd.reset();
        atoms[i].mass = bd(cbeng)  ? m1 : m2;
    }
}

// thermalize - assign the velocities according to the
// Maxwell-Boltzmann distribution, i.e., a normal distribution with
// zero mean and 'sigma' that depends on the temperature and the
// atomic mass.
void thermalize(atom* atoms, size_t Natoms, uint32_t timestep, const Prf::key_type& key){
    engine_t cbeng(key);
    for(size_t i=0; i<Natoms; ++i){
        float rmsvelocity = sqrt(kT/atoms[i].mass);
        normal_distribution<float> mbd(0., rmsvelocity);
#if __cplusplus>=201103L && defined(BOOST_NO_CXX11_HDR_INITIALIZER_LIST)
        cbeng.restart({atoms[i].id, timestep, uint32_t(THERMALIZE_CTXT)});
#else
        uint32_t base32[4] = {atoms[i].id, timestep, THERMALIZE_CTXT};
        Prf::domain_type start = engine_t::domain_traits::make_counter(&base32[0], &base32[4]);
        //Prf::domain_type start = {atoms[i].id, timestep, THERMALIZE_CTXT};
        cbeng.restart(start);
#endif
        atoms[i].vx = mbd(cbeng);
        atoms[i].vy = mbd(cbeng);
        atoms[i].vz = mbd(cbeng);
    }
}

// mt_assign_masses - launch Nthr threads calling assign_masses in each:
void mt_assignmasses(size_t Nthr, std::vector<atom>& atoms, float m1, float m2, const Prf::key_type& key){
    // Assign atoms to threads in blocks:
    //  thread i updates a block starting at i* (Natoms/Nthr) (rounded appopriately).
    boost::thread_group threads;
    size_t atoms_left = atoms.size();
    atom *start = &atoms[0];
    for(size_t t=0; t<Nthr; ++t){
        size_t atoms_per_thread = atoms_left/(Nthr - t);
        threads.add_thread(new boost::thread(assignmasses, start, atoms_per_thread, m1, m2, key));
        atoms_left -= atoms_per_thread;
        start += atoms_per_thread;
    }
    threads.join_all();
}

// mt_thermalize - launch Nthr threads calling thermalize in each.
void mt_thermalize(size_t Nthr, std::vector<atom>& atoms, uint32_t timestep, const Prf::key_type& key){
    // Assign atoms to threads in blocks:
    //  thread i updates a block starting at i* (Natoms/Nthr) (rounded appopriately).
    boost::thread_group threads;
    size_t atoms_left = atoms.size();
    atom *start = &atoms[0];
    for(size_t t=0; t<Nthr; ++t){
        size_t atoms_per_thread = atoms_left/(Nthr - t);
        threads.add_thread(new boost::thread(thermalize, start, atoms_per_thread, timestep, key));
        atoms_left -= atoms_per_thread;
        start += atoms_per_thread;
    }
    threads.join_all();
}

void out(const std::vector<atom>& atoms, long timestep){
    std::cout << "id mass vx vy vz thermalized at timestep=" << timestep << "\n";
    for(size_t i=0; i<atoms.size(); ++i){
        std::cout << i << " " << atoms[i].mass << " " << atoms[i].vx << " " << atoms[i].vy << " " << atoms[i].vz << "\n";
    }
}

int main(int argc, char **argv){
    size_t nthread = 4;
    if( argc > 1 ){
        nthread = boost::lexical_cast<size_t>(argv[1]);
    }
    uint32_t seed = 1;
    if( argc > 2 ){
        seed = boost::lexical_cast<uint32_t>(argv[2]);
    }
    
    std::vector<atom> atoms(10);
    for(size_t i=0; i<atoms.size(); ++i){
        atoms[i].id = i;
    }
    Prf::key_type key = {seed};
    std::cout << "running with " << nthread << " threads\n";
    long timestep = 1;
    // Pick random masses uniformly between Hydrogen (1amu) and Oxygen (16amu).
    mt_assignmasses(nthread, atoms, 1.*amu, 16.*amu, key);
    // Thermalize the velocities according to the Boltzmann distribution.
    mt_thermalize(nthread, atoms, timestep, key);
    out(atoms, timestep);

    // Advance the timestep and rethermalize.
    timestep++;
    mt_thermalize(nthread, atoms, timestep, key);
    out(atoms, timestep);

    return 0;
}
