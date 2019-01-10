/*Copyright (c) 2016, Andreas Grueneis and Felix Hummel, all rights reserved.*/
#ifndef TCC_MACHINE_TENSOR_DEFINED
#define TCC_MACHINE_TENSOR_DEFINED

#include <util/SharedPointer.hpp>
#include <util/Exception.hpp>

#include <vector>
#include <string>
#include <functional>

namespace tcc {
  /**
   * \brief A MachineTensor object represents the underlying
   * implementation of the actual numeric information of a tensor.
   * The implementation of this class needs to provide a set of
   * basic operations, such as move and contraction operations.
   **/
  template <typename F>
  class MachineTensor {
  public:
    virtual ~MachineTensor() {
    }

    /**
     * \brief Perorms a move operation of the form
     * this[bIndices] <<= alpha * A[aIndices] + beta * this[bIndices]
     **/
    virtual void move(
      F alpha,
      const PTR(MachineTensor<F>) &A, const std::string &aIndices,
      F beta,
      const std::string &bIndices
    ) = 0;

    /**
     * \brief Performs a generic move operation of the form 
     * this[bIndices] <<= f(alpha * A[aIndices]) + beta * this[bIndices]
     **/
    virtual void move(
      F alpha,
      const PTR(MachineTensor<F>) &A, const std::string &aIndices,
      F beta,
      const std::string &bIndices,
      const std::function<F(const F)> &f
    ) {
      throw new EXCEPTION("Function application not implemented by machine tensor.");
    }

    /**
     * \brief Perform a contraction operation of the form
     * this[cIndices] <<=
     *   alpha * A[aIndices] * B[bIndices] + beta * this[cIndices]
     **/
    virtual void contract(
      F alpha,
      const PTR(MachineTensor<F>) &A, const std::string &aIndices,
      const PTR(MachineTensor<F>) &B, const std::string &bIndices,
      F beta,
      const std::string &cIndices
    ) = 0;

    /**
     * \brief Perform a generic contraction operation of the form
     * this[cIndices] <<=
     *   alpha * g(A[aIndices], B[bIndices]) + beta * this[cIndices]
     **/
    virtual void contract(
      F alpha,
      const PTR(MachineTensor<F>) &A, const std::string &aIndices,
      const PTR(MachineTensor<F>) &B, const std::string &bIndices,
      F beta,
      const std::string &cIndices,
      const std::function<F(const F, const F)> &g
    ) {
      throw new EXCEPTION("Contraction with custom function not implemented by machine tensor.");
    }

    /**
     * \brief Slices a part of the given tensor into this tensor.
     * this[begins,ends) = alpha*A[aBegins,aEnds] + beta*this[begins,ends)
     * ends-begins must match aEnds-aBegins.
     **/
    virtual void slice(
      F alpha,
      const PTR(MachineTensor<F>) &A,
      const std::vector<int> aBegins,
      const std::vector<int> aEnds,
      F beta,
      const std::vector<int> begins,
      const std::vector<int> ends
    ) {
      throw new EXCEPTION("Slice not implemented by machine tensor.");
    }

    // TODO: interfaces to be defined: permute, transform

    /**
     * \brief Returns the shape of the underlying implementation tensor,
     * which is assumed to be immutable.
     **/
    virtual std::vector<int> getLens() const = 0;

    /**
     * \brief Returns the given name of the underlying implementation tensor,
     * which is assumed to be immutable. Tcc assumes tensor having common
     * names to carry identical data. If this routine returns the empty
     * string a random string will be generated by tcc.
     **/
    virtual std::string getName() const = 0;
  };


  /**
   * \brief A MachineTensorFactory object provides an interface to create
   * instances of the underlying implementation of the MachineTensor class.
   **/
  template <typename F=double>
  class MachineTensorFactory {
  public:
    /**
     * \brief Create a concrete MachineTensor object according the underlying
     * implementation having dimensions lens_[0] x lens_[1] x ... with
     * a specified name.
     */
    virtual PTR(MachineTensor<F>) createTensor(
      const std::vector<int> &lens,
      const std::string &name
    ) = 0;
  };
}

#endif

