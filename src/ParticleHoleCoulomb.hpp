/*Copyright (c) 2015, Andreas Grueneis and Felix Hummel, all rights reserved.*/
#ifndef PARTICLE_HOLE_COULOMB_DEFINED
#define PARTICLE_HOLE_COULOMB_DEFINED

#include <Algorithm.hpp>

namespace cc4s {
  class ParticleHoleCoulomb: public Algorithm {
  public:
    ParticleHoleCoulomb(
      std::vector<Argument const *> const &argumentList
    );
    virtual ~ParticleHoleCoulomb();
    virtual void run();

    static Algorithm *create(std::vector<Argument const*> const &argumentList) {
      return new ParticleHoleCoulomb(argumentList);
    }
  };
}


#endif
