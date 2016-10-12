/*Copyright (c) 2016, Andreas Grueneis and Felix Hummel, all rights reserved.*/
#ifndef CLUSTER_SINGLES_DOUBLES_ALGORITHM_DEFINED 
#define CLUSTER_SINGLES_DOUBLES_ALGORITHM_DEFINED

#include <algorithms/ClusterDoublesAlgorithm.hpp>
#include <mixers/Mixer.hpp>
#include <string>
#include <ctf.hpp>
#include <util/DryTensor.hpp>

namespace cc4s {
  /**
   * \brief Contains all the necessary tools for an algorithm with
   * singles and doubles amplitudes. It calculates the energy from the amplitudes
   * \f$T_{a}^{i}\f$ and \f$T_{ab}^{ij}\f$ and the Coulomb integrals \f$V_{ij}^{ab}\f$. For
   * calculating the amplitudes it calls the iteration routine of the actual algorithm.
   */
  class ClusterSinglesDoublesAlgorithm: public ClusterDoublesAlgorithm {
  public:
    ClusterSinglesDoublesAlgorithm(
      std::vector<Argument> const &argumentList
    );
    virtual ~ClusterSinglesDoublesAlgorithm();
    /**
     * \brief Calculates the energy of a ClusterSinglesDoubles algorithm
     */
    virtual void run();
    /**
     * \brief Returns the abbreviation of the concrete algorithm, e.g. "CCSD",
     * "DCSD"
     */
    virtual std::string getAbbreviation() = 0;
    /**
     * \brief Performs a Dry Run
     */
    virtual void dryRun();

  protected:
    /**
     * \brief The mixer for the singles amplitudes, additionally to those
     * of the inheritided doubles amplitudes TabijMixer
     */
    Mixer<double> *TaiMixer;

    /**
     * \brief Performs one iteration of the concrete algorithm.
     */
    virtual void iterate(int i) = 0;

    /**
     * \brief Calculates the singles amplitudes from the current residuum and
     * returns them in-place.
     * Usually this is done by calculating
     * \f$T_{i}^{a} = R_{i}^{a} / (\varepsilon_i-\varepsilon_a)\f$,
     * but other methods, such as level shifting may be used.
     * \param[in] Rai Residuum Tensor.
     */
    void singlesAmplitudesFromResiduum(CTF::Tensor<> &Rai);

    /**
     * \brief Dry run for singlesAmplitudesFromResiduum.
     * \param[in] Rai Residuum Tensor.
     */
    void drySinglesAmplitudesFromResiduum(cc4s::DryTensor<> &Rai);

    /**
     * \brief Calculates and returns one slice Xxycd of the Coulomb integrals \f$V_{cd}^{ab}\f$
     * coupled to the singles amplitudes.
     * The indices x and y are restricted to the
     * range {No+a, ..., No+a+No-1} and {No+b, ..., No+b+No-1}, respectively.
     * The caller is responsible for deleting the dynamically allocated
     * result tensor. 
     * \param[in] a 1st sliced dimension (x).
     * \param[in] b 2nd sliced dimension (y).
     * \param[in] sliceRank slicing rank.
     * \param[out] Xxycd sliced coupled Coulomb integrals Xabcd
     */
    CTF::Tensor<> *sliceCoupledCoulombIntegrals(int a, int b, int sliceRank);

    /**
     * \brief Dry run for sliceCoupledCoulombIntegrals. 
     * \param[in] a 1st sliced dimension (x).
     * \param[in] b 2nd sliced dimension (y).
     * \param[in] sliceRank slicing rank.
     */
    cc4s::DryTensor<> *drySliceCoupledCoulombIntegrals(int sliceRank);
  };
}

#endif

