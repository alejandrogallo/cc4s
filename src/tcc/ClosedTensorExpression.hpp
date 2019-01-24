/*Copyright (c) 2019, Andreas Grueneis and Felix Hummel, all rights reserved.*/
#ifndef TCC_CLOSED_TENSOR_EXPRESSION_DEFINED
#define TCC_CLOSED_TENSOR_EXPRESSION_DEFINED

#include <tcc/TensorExpression.hpp>

#include <tcc/Indexing.hpp>
#include <util/SharedPointer.hpp>

namespace tcc {
  /**
   * \brief Closed tensor expressions are tensor valued expressions
   * whose dimensions are referred to by their slot number rather than
   * by a specific index label. Tensors and slices of tensors are
   * examples.
   **/
  template <typename F, typename TE>
  class ClosedTensorExpression: public TensorExpression<F,TE> {
  public:
    virtual void countIndices(Scope &scope) {
      // a closed tensor expression has no indices
    }

    /**
     * \brief Specify named indices of this tensor to be used in a
     * tensor expression. Indexed tensors are atomic types of tensor
     * expressions.
     **/
    PTR(ESC(Indexing<F,TE>)) operator[](
      const std::string &indices
    ) {
      return Indexing<F,TE>::create(
        DYNAMIC_PTR_CAST(ESC(ClosedTensorExpression<F,TE>), THIS), indices
      );
    }
  };
}

#endif

