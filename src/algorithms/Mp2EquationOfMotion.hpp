/*Copyright (c) 2017, Andreas Grueneis, Felix Hummel and Alejandro Gallo, all rights reserved.*/
#ifndef MP2_EOM_DEFINED
#define MP2_EOM_DEFINED

#include <algorithms/Algorithm.hpp>

namespace cc4s {
  /**
   * \brief Implements the iteration routine for the Mp2 method. Calculates the
   * amplitudes \f$T_{ab}^{ij}\f$ from the Coulomb Integrals \f$V_{ij}^{ab}\f$
   * in a \f$ \mathcal{O}(N^{6}) \f$ implementation.
   */
  class Mp2EquationOfMotion: public Algorithm {
  public:
    ALGORITHM_REGISTRAR_DECLARATION(Mp2EquationOfMotion);
    Mp2EquationOfMotion(
      std::vector<Argument> const &argumentList
    );
    virtual ~Mp2EquationOfMotion();

    virtual void run();

  protected:
    static constexpr int DEFAULT_MAX_ITERATIONS = 16;

    template <typename F = double>
    void getCanonicalPerturbationBasis(
        CTF::Tensor<F> &Tai, CTF::Tensor<F> &Tabij, int64_t i
    );

    template <typename F>
    void writeEOMVectors(
      CTF::Tensor<F> &Tai,
      unsigned int rank
    );

  };
}

#endif

