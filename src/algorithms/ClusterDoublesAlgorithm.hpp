/*Copyright (c) 2016, Andreas Grueneis and Felix Hummel, all rights reserved.*/
#ifndef CLUSTER_DOUBLES_ALGORITHM_DEFINED 
#define CLUSTER_DOUBLES_ALGORITHM_DEFINED

#include <algorithms/Algorithm.hpp>
#include <mixers/Mixer.hpp>
#include <string>
#include <ctf.hpp>
#include <tcc/DryTensor.hpp>

namespace cc4s {
  /**
   * \brief Contains all the necessary tools for an algorithm with
   * only doubles amplitudes. It calculates the energy from the amplitudes
   * \f$T_{ab}^{ij}\f$ and the Coulomb integrals \f$V_{ij}^{ab}\f$. For
   * calculating the amplitudes it calls the iteration routine of the actual algorithm.
   */
  class ClusterDoublesAlgorithm: public Algorithm {
  public:
    ClusterDoublesAlgorithm(
      std::vector<Argument> const &argumentList
    );
    virtual ~ClusterDoublesAlgorithm();
    /**
     * \brief Calculates the energy of a ClusterDoubles algorithm
     */
    virtual void run();

    /**
     * \brief Performs a Dry Run
     */
    virtual void dryRun();
    /**
     * \brief Returns the abbreviation of the concrete algorithm, e.g. "Ccd",
     * "Dcd".
     */
    virtual std::string getAbbreviation() = 0;

    /**
     * \brief Defines the default number of iterations (16).
     */
    static int constexpr DEFAULT_MAX_ITERATIONS = 16;

  protected:
    /**
     * \brief The abbreviation of the algorithm in capital letters.
     **/
    std::string abbreviation;

    /**
     * \brief The mixer for the doubles amplitudes.
     */
    // TODO: is it really the best option when using complex and real mixers
    // to pass all mixers to the iterate method rather than having it as
    // field variables, as it was before?
//    Mixer<double> *TabijMixer;
//    Mixer<complex> *TabijMixer;

    /**
     * \brief Evaluates and returns the energy according to the respective
     * ClusterDoubles algorithm. An instantiation of this method
     * is called from the run method with typename F being either double
     * or complex, depending on which type of integrals are present.
     **/
    template <typename F>
    F evaluate();

    /**
     * \brief Evaluates a dry run and returns 0 according to the respective
     * ClusterDoubles algorithm. An instantiation of this method
     * is called from the dryRun method with typename F being either double
     * or complex, depending on which type of integrals are present.
     **/
    template <typename F>
    F dryEvaluate();

    /**
     * \brief Performs one iteration of the concrete algorithm
     * when operating with real valued Coulomb integrals.
     * \param[in] i Iteration number
     * \param[in] TaiMixer Mixer for the singles amplitudes, may be null
     * \param[in] TabijMixer Mixer for the doubles amplitudes.
     */
    virtual void iterate(
      int i, Mixer<double> *TaiMixer, Mixer<double> *TabijMixer
    ) = 0;
    /**
     * \brief Performs one iteration of the concrete algorithm
     * when operating with complex valued Coulomb integrals.
     * \param[in] i Iteration number
     * \param[in] TaiMixer Mixer for the singles amplitudes, may be null
     * \param[in] TabijMixer Mixer for the doubles amplitudes.
     */
    virtual void iterate(
      int i, Mixer<complex> *TaiMixer, Mixer<complex> *TabijMixer
    ) = 0;

    /**
     * \brief Performs a dry run of one iteration of the concrete algorithm
     * when operating with real valued Coulomb integrals.
     * The base class does not perform accounting and writes a warning about
     * that.
     * Note that there is no DryMixer currently.
     * \param[in] TaiMixer singles amplitudes, may be null
     * \param[in] TabijMixer doubles amplitudes.
     */
    virtual void dryIterate(
      DryTensor<double> *TaiMixer, DryTensor<double> *TabijMixer
    );
    /**
     * \brief Performs a dry run of one iteration of the concrete algorithm
     * when operating with complex valued Coulomb integrals.
     * The base class does not perform accounting and writes a warning about
     * that.
     * \param[in] TaiMixer Mixer for the singles amplitudes, may be null
     * \param[in] TabijMixer Mixer for the doubles amplitudes.
     */
    virtual void dryIterate(
      DryTensor<complex> *TaiMixer, DryTensor<complex> *TabijMixer
    );

    /**
     * \brief Calculates the energy from the amplitudes currently contained
     * in the mixer.
     **/
    template <typename F>
    F calculateEnergy(Mixer<F> *TabijMixer);

    /**
     * \brief Calculates the amplitudes from the current residuum and
     * returns them in-place.
     * Usually this is done by calculating
     * \f$T_{ij}^{ab} = R_{ij}^{ab} / (\varepsilon_i+\varepsilon_j-\varepsilon_a-\varepsilon_b)\f$,
     * but other methods, such as level shifting may be used.
     * \param[in] Rabij Residuum Tensor.
     */
    template <typename F>
    void doublesAmplitudesFromResiduum(CTF::Tensor<F> &Rabij);

    /**
     * \brief Dry run for doublesAmplitudesFromResiduum.
     * \param[in] Rabij Residuum Tensor.
     */
    void dryDoublesAmplitudesFromResiduum(cc4s::DryTensor<> &Rabij);

    /**
     * \brief Calculates and returns one slice Vxycd of the Coulomb integrals
     * from the Coulomb vertex. The indices x and y are restricted to the
     * range {No+a, ..., No+a+No-1} and {No+b, ..., No+b+No-1}, respectively.
     * The caller is responsible for deleting the dynamically allocated
     * result tensor. 
     * \param[in] a 1st sliced dimension (x).
     * \param[in] b 2nd sliced dimension (y).
     * \param[in] integralsSliceSize slicing size.
     * \param[out] Vxycd sliced Coulomb integrals Vabcd
     */
    CTF::Tensor<> *sliceCoulombIntegrals(int a, int b, int integralsSliceSize);

    /**
     * \brief Dry run for sliceCoulombIntegrals.
     * \param[in] a 1st sliced dimension (x).
     * \param[in] b 2nd sliced dimension (y).
     * \param[in] integralsSliceSize slicing size.
     * \param[out] Vxycd sliced Coulomb integrals Vabcd
     */
    cc4s::DryTensor<> *drySliceCoulombIntegrals(int integralsSliceSize);

    /**
     * \brief Adds the given slice of the residuum tensor Rxyij to the
     * entire residuum tensor Rabij at the respective index range.
     * \param[in] a0 1st sliced dimension (x).
     * \param[in] b0 2nd sliced dimension (y).
     * \param[in] Rxyij sliced residuum
     * \param[in] Rabij entire residuum.
     */
    void sliceIntoResiduum(
      CTF::Tensor<> &Rxyij, int a0, int b0, CTF::Tensor<> &Rabij
    );

    /**
     * \brief Calculates and returns one slice Xabij of the residuum
     * from the Coulomb factors. The slice is computed from
     * Rx and Ry and are restricted to the
     * range {a, ..., factorsSliceSize+a-1} and {b, ..., factorsSliceSize+b-1}, respectively.
     * The caller is responsible for deleting the dynamically allocated
     * result tensor. 
     * \param[in] a 1st sliced dimension (Rx).
     * \param[in] b 2nd sliced dimension (Ry).
     * \param[in] factorsSliceSize slicing size of NR.
     * \param[out] Fabij sliced Residuum
     */
// TODO: translate to template methods
//    CTF::Tensor<> *sliceAmplitudesFromCoulombFactors(int a, int b, int factorsSliceSize);

// TODO: translate to template methods
//    CTF::Tensor<> *sliceAmplitudesFromCoulombFactorsTcc(int a, int factorsSliceSize);

    /**
     * \brief Dry run for sliceAmplitudesFromCoulombFactors.
     * \param[in] factorsSliceSize slicing size of NR.
     * \param[out] Fabij dry tensor of sliced residuum.
     */
// TODO: translate to template methods
//    cc4s::DryTensor<> *drySliceAmplitudesFromCoulombFactors(int factorsSliceSize);


    /**
     * \brief Prints the energy from the residuum Rabij.
     * \param[in] Rabij the residuum.
     */
    void printEnergyFromResiduum(CTF::Tensor<> &Rabij);

  };
}

#endif

