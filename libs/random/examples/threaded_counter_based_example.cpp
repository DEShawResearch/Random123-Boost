// Example code demonstrating how to set up and use a
// counter_based_urng in a threaded program.  Usage:
// 
//   threaded_counter_based_example [Nthread [Natoms]]
//
// By using a counter_based_urng, the loops (assignmasses and
// thermalize) can be parallelized so that the program's results are
// independent of the number of threads or the assignment of atoms to
// threads.

#include <boost/random/counter_based_urng.hpp>
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
enum { THERMALIZE_CTXT, MASS_ASSIGN_CTXT };
float kT = 8.314 * 300; // J/mol
float amu = 1.e-3;      // 1 amu = 1e-3 kg/mol

// Choose a pseudo-random function:
typedef threefry<4, uint32_t> Prf;

// assignmasses - assigns masses, randomly with equal probability
// of m1 or m2.
void assignmasses(std::vector<atom>& atoms, float m1, float m2, const Prf& prf){
    bernoulli_distribution<float> bd;
    for(size_t i=0; i<atoms.size(); ++i){
        atoms[i].id = i;
        Prf::domain_type d = {atoms[i].id, 0, MASS_ASSIGN_CTXT};
        counter_based_urng<Prf> cbrng(prf, d);
        bd.reset();
        atoms[i].mass = bd(cbrng)  ? m1 : m2;
    }
}

// thermalize - assign the velocities, randomly chosen from a Boltzmann distribution.
//  start, stride and Natoms tell us which elements of the atoms[] array
//  this thread should update.
void thermalize(atom* atoms, uint32_t timestep, const Prf& prf, size_t start, size_t stride, size_t Natoms){
    normal_distribution<float> nd;
    for(size_t i=start; i<Natoms; i+=stride){
        float rmsvelocity = sqrt(kT/atoms[i].mass);
        Prf::domain_type d = {atoms[i].id, timestep, THERMALIZE_CTXT};
        counter_based_urng<Prf> cbrng(prf, d);
        nd.reset();
        atoms[i].vx = rmsvelocity*nd(cbrng);
        atoms[i].vy = rmsvelocity*nd(cbrng);
        atoms[i].vz = rmsvelocity*nd(cbrng);
    }
}

// mt_thermalize - call thermalize in Nthr threads.
void thermalize_mt(size_t Nthr, std::vector<atom>& atoms, uint32_t timestep, const Prf& prf){
    // Our threading strategy isn't particularly smart, but it is illustrative:
    //  thread i updates atoms[i], atoms[i+Nthr], atoms[i+2*Nthr], ...
    boost::thread_group threads;
    for(size_t t=0; t<Nthr; ++t){
        threads.add_thread(new boost::thread(thermalize, &atoms[0], timestep, prf, t, Nthr, atoms.size()));
    }
    threads.join_all();
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
    Prf::key_type key = {seed};
    Prf prf(key);
    std::cout << "pseudo-random function key:  " << prf << "\n";
    std::cout << "running with " << nthread << " threads\n";
    long timestep = 1;
    // Pick random masses uniformly between Hydrogen (1amu) and Oxygen (16amu).
    assignmasses(atoms, 1.*amu, 16.*amu, prf);
    // Now thermalize the velocities according to the Boltzmann distribution.
    thermalize_mt(nthread, atoms, timestep, prf);

    std::cout << "id mass vx vy vz thermalized at timestep=" << timestep << "\n";
    for(size_t i=0; i<atoms.size(); ++i){
        std::cout << i << " " << atoms[i].mass << " " << atoms[i].vx << " " << atoms[i].vy << " " << atoms[i].vz << "\n";
    }

    // Now advance the timestep and rethermalize.  Show the new velocities
    timestep++;
    thermalize_mt(nthread, atoms, timestep, prf);
    std::cout << "id mass vx vy vz thermalized at timestep=" << timestep << "\n";
    for(size_t i=0; i<atoms.size(); ++i){
        std::cout << i << " " << atoms[i].mass << " " << atoms[i].vx << " " << atoms[i].vy << " " << atoms[i].vz << "\n";
    }

    return 0;
}
