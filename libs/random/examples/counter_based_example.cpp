// Example code demonstrating how to set up and use a
// counter_based_urng in a program.  Imagine this as a small piece of
// a much larger molecular dynamics program.
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
void thermalize(std::vector<atom>& atoms, uint32_t timestep, const Prf& prf){
    normal_distribution<float> nd;
    for(size_t i=0; i<atoms.size(); ++i){
        float rmsvelocity = sqrt(kT/atoms[i].mass);
        Prf::domain_type d = {atoms[i].id, timestep, THERMALIZE_CTXT};
        counter_based_urng<Prf> cbrng(prf, d);
        nd.reset();
        atoms[i].vx = rmsvelocity*nd(cbrng);
        atoms[i].vy = rmsvelocity*nd(cbrng);
        atoms[i].vz = rmsvelocity*nd(cbrng);
    }
}    

int main(int argc, char **argv){
    uint32_t seed = 1;
    if( argc > 1 ){
        seed = boost::lexical_cast<uint32_t>(argv[1]);
    }
    
    std::vector<atom> atoms(10);
    Prf::key_type key = {seed};
    Prf prf(key);
    std::cout << "pseudo-random function key:  " << prf << "\n";
    long timestep = 1;
    // Pick random masses uniformly between Hydrogen (1amu) and Oxygen (16amu).
    assignmasses(atoms, 1.*amu, 16.*amu, prf);
    // Now thermalize the velocities according to the Boltzmann distribution.
    thermalize(atoms, timestep, prf);

    std::cout << "id mass vx vy vz thermalized at timestep=" << timestep << "\n";
    for(size_t i=0; i<atoms.size(); ++i){
        std::cout << i << " " << atoms[i].mass << " " << atoms[i].vx << " " << atoms[i].vy << " " << atoms[i].vz << "\n";
    }

    // Now advance the timestep and rethermalize.  Show the new velocities
    timestep++;
    thermalize(atoms, timestep, prf);
    std::cout << "id mass vx vy vz thermalized at timestep=" << timestep << "\n";
    for(size_t i=0; i<atoms.size(); ++i){
        std::cout << i << " " << atoms[i].mass << " " << atoms[i].vx << " " << atoms[i].vy << " " << atoms[i].vz << "\n";
    }

    return 0;
}
