/*Copyright (c) 2016, Andreas Grueneis and Felix Hummel, all rights reserved.*/
#ifndef CCD_ENERGY_FROM_SLICED_INTEGRALS_DEFINED 
#define CCD_ENERGY_FROM_SLICED_INTEGRALS_DEFINED

#include <algorithms/ClusterDoublesAlgorithm.hpp>

namespace cc4s {
  // this algorithm is now based on the ClusterDoublesAlgorithm
  // inheriting its iteration and slicing functionality.
  // Only the abstract (left out) methods getAbbreviation and iterate have
  // to be implemented.
  /**
   * \brief Implements the iteration routine for the Ccd method. Calculates the
   * amplitudes \f$T_{ab}^{ij}\f$ from the Coulomb Integrals \f$V_{ij}^{ab}, V_{bj}^{ai},
   * V_{kl}^{ij}\f$, and the Coulomb
   * Vertex \f$\Gamma_{pG}^q\f$.
   */
  class CcdEnergyFromSlicedIntegrals: public ClusterDoublesAlgorithm {
  public:
    ALGORITHM_REGISTRAR_DECLARATION(CcdEnergyFromSlicedIntegrals);
    CcdEnergyFromSlicedIntegrals(
      std::vector<Argument> const &argumentList
    );
    virtual ~CcdEnergyFromSlicedIntegrals();
    /**
     * \brief Returns the abbreviation of the routine (CCD).
     * \return abbreviation of the routine
     */
    virtual std::string getAbbreviation() { return "Ccd"; }

  protected:
    /**
     * \brief Implements the iterate method with the CCD iteration. Iteration
     * routine taken from So Hirata, et. al. Chem. Phys. Letters, 345, 475 (2001).
     * \param[in] i Iteration number
     */
    virtual void iterate(int i);
    /**
     * \brief Implements the iterate method with the CCD iteration. Iteration
     * routine taken from Rev. Mod. Phys. 79, 291  Page 305, Figure 8.
     * \param[in] i Iteration number
     */
    virtual void iterateBartlett(int i);
    /**
     * \brief Implements the dry iterate method with the CCD iteration.
     */
    virtual void dryIterate();
  };
}

#endif
