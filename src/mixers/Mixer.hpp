/*Copyright (c) 2017, Andreas Grueneis and Felix Hummel, all rights reserved.*/
#ifndef MIXER_DEFINED
#define MIXER_DEFINED

#include <algorithms/Algorithm.hpp>
#include <math/Complex.hpp>
#include <math/FockVector.hpp>
#include <util/SharedPointer.hpp>

#include <string>

namespace cc4s {
  template <typename F, typename TE>
  class Mixer {
  public:
    Mixer(Algorithm *algorithm);
    virtual ~Mixer();

    /**
     * \brief Returns the name of the implenting mixer.
     **/
    virtual std::string getName() = 0;

    /**
     * \brief Appends the given pair (A,R) of FockVectors to the mixer,
     * where R is the residuum when using the amplitudes A.
     * The mixer may use the given amplitudes and residua to provide
     * an estimated amplitude with a lower expected residuum.
     * A and R are not expected to change upon return.
     **/
    virtual void append(
      const PTR(ESC(FockVector<F,TE>)) &A,
      const PTR(ESC(FockVector<F,TE>)) &R
    ) = 0;

    /**
     * \brief Returns the current best estimate of the amplitudes
     * according to previously given pairs of amplitudes and residua.
     * Requires one or more previous calls to append.
     * The returned FockVectors must not be changed.
     **/
    virtual PTR(ESC(const FockVector<F,TE>)) get() = 0;

    /**
     * \brief Returns the estimated residuum of the current best estimate
     * of the amplitdues according to previously given pairs of amplitudes
     * and residua.
     * Requires one or more previous calls to append.
     * The returned FockVectors must not be changed.
     **/
    virtual PTR(ESC(const FockVector<F,TE>)) getResiduum() = 0;

    Algorithm *algorithm;
  };

  template <typename F, typename TE>
  class MixerFactory {
  public:
// FIXME: find out why typedef doesn't work in this case
/*
    typedef std::map<
      std::string,
      std::function<Mixer<F> *(Algorithm *)>
    > MixerMap;
*/
    /**
     * \brief Creates a mixer object of the mixer type specified
     * by the given name.
     * The instantiated mixer must be registered using the
     * MixerRegistrar class.
     */
    static PTR(ESC(Mixer<F,TE>)) create(
      std::string const &name, Algorithm *algorithm
    ) {
      auto iterator(getMixerMap()->find(name));
      return iterator != getMixerMap()->end() ?
        iterator->second(algorithm) : PTR(ESC(Mixer<F,TE>))();
    }
  protected:
    static std::map<
      std::string,
      std::function<PTR(ESC(Mixer<F,TE>)) (Algorithm *algorithm)>
    > *getMixerMap() {
      return mixerMap ? mixerMap : (
        mixerMap = new std::map<
          std::string,
          std::function<PTR(ESC(Mixer<F,TE)>) (Algorithm *)>
        >
      );
    }
    static std::map<
      std::string,
      std::function<PTR(ESC(Mixer<F,TE>)) (Algorithm *)>
    > *mixerMap;
/*
    static MixerMap *getMixerMap() {
      return mixerMap ? mixerMap : (mixerMap = new MixerMap);
    }
    static MixerMap *mixerMap;
*/
  };

  /**
   * \brief template function creating an instance of the given class.
   */
  template <typename F, typename TE, typename MixerType>
  PTR(ESC(Mixer<F,TE>)) createMixer(Algorithm *algorithm) {
    return NEW(MixerType, algorithm);
  }

  /**
   * \brief Class to be statically instantiated by a mixer to register
   * it in the MixerFactory. Registered mixers can be instantiated
   * from the cc4s control language.
   */
  template <typename F, typename TE, typename MixerType>
  class MixerRegistrar: protected MixerFactory<F,TE> {
  public:
    /**
     * \brief Constructs the registrating instance. The mixer type
     * must be given as template argument, the mixer name as
     * method argument.
     */
    MixerRegistrar(std::string const &name) {
      (*MixerFactory<F,TE>::getMixerMap())[name] = &createMixer<F,TE,MixerType>;
    }
  };

  /**
   * \brief Auxiliary macro declaring the mixer registrar for
   * the mixer type of the given name. This macro is to be
   * used in the mixer declaration within the .hpp file.
   * Note that name is a symbol name not a string.
   */
  #define MIXER_REGISTRAR_DECLARATION(NAME) \
    virtual std::string getName() { return #NAME; } \
    static MixerRegistrar<F,TE,NAME<F,TE>> registrar_;

  /**
   * \brief Auxiliary macro defining the mixer registrar for
   * the mixer type of the given name. This macro is to be
   * used in the mixer definition within the .cxx file.
   * Note that name is a symbol name not a string.
   */
  #define MIXER_REGISTRAR_DEFINITION(NAME) \
    template <typename F, typename TE> \
    MixerRegistrar<F,TE,NAME<F,TE>> NAME<F,TE>::registrar_(#NAME);
}

#endif

