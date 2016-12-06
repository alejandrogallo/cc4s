/*Copyright (c) 2016, Andreas Grueneis and Felix Hummel, all rights reserved.*/
#ifndef TCC_FETCH_OPERATION_DEFINED
#define TCC_FETCH_OPERATION_DEFINED

#include <tcc/Tensor.hpp>
#include <tcc/IndexedTensor.hpp>
#include <util/Log.hpp>

#include <memory>

namespace tcc {
  template <typename F>
  class FetchOperation: public Operation<F> {
  public:
    /**
     * \brief Creates a fetch operation of a tensor making it accessible
     * for subsequent move or contraction operations.
     * Not intended for direct invocation. Use compile(expression) to
     * generate operations.
     **/
    FetchOperation(
      const std::shared_ptr<IndexedTensor<F>> &t_,
      const typename Operation<F>::ProtectedToken &
    ):
      Operation<F>(Costs(t_->tensor->getElementsCount())),
      tensor(t_->tensor),
      indices(t_->indices)
    {
    }
    virtual ~FetchOperation() {
    }

    virtual void execute() {
      // nothing to be done in a fetch
    }

    virtual std::shared_ptr<Tensor<F>> getResult() {
      return tensor;
    }

    virtual std::string const &getResultIndices() {
      return indices;
    }

  protected:
    std::shared_ptr<Tensor<F>> tensor;
    std::string indices;
  };
}

#endif

