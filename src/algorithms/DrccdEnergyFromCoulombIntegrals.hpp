/*Copyright (c) 2015, Andreas Grueneis and Felix Hummel, all rights reserved.*/
#ifndef DRCCD_ENERGY_FROM_COULOMB_INTEGRALS_DEFINED 
#define DRCCD_ENERGY_FROM_COULOMB_INTEGRALS_DEFINED

#include <algorithms/ClusterDoublesAlgorithm.hpp>

namespace cc4s {
  // this algorithm is now based on the ClusterDoublesAlgorithm
  // inheriting its iteration and slicing functionality.
  // Only the abstract (left out) methods getAbbreviation and iterate have
  // to be implemented.
  /**
   * \brief Implements the iteration routine for the Drccd method. Calculates the
   * amplitudes \f$T_{ab}^{ij}\f$ from the Coulomb Integrals \f$V_{ij}^{ab}\f$
   * in a \f$ \mathcal{O}(N^{6}) \f$ implementation.
   */
  class DrccdEnergyFromCoulombIntegrals: public ClusterDoublesAlgorithm {
  public:
    ALGORITHM_REGISTRAR_DECLARATION(DrccdEnergyFromCoulombIntegrals);
    DrccdEnergyFromCoulombIntegrals(
      std::vector<Argument> const &argumentList
    );
    virtual ~DrccdEnergyFromCoulombIntegrals();
    /**
     * \brief Returns the abbreviation of the routine (DRCCD).
     * \return abbreviation of the routine
     */
    virtual std::string getAbbreviation() { return "Drccd"; }

  protected:
    /**
     * \brief Implements the iterate method with the DRCCD iteration.
     * \param[in] i Iteration number
     */
    virtual void iterate(
      int i, Mixer<double> *TaiMixer, Mixer<double> *TabijMixer
    );

    virtual void iterate(
      int i, Mixer<complex> *TaiMixer, Mixer<complex> *TabijMixer
    );

    template <typename F>
    void iterateTemplate(int i, Mixer<F> *TabijMixer);
  };
}

#endif

