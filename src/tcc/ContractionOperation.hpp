/*Copyright (c) 2016, Andreas Grueneis and Felix Hummel, all rights reserved.*/
#ifndef TCC_CONTRACTION_OPERATION_DEFINED
#define TCC_CONTRACTION_OPERATION_DEFINED

#include <tcc/Operation.hpp>
#include <tcc/Costs.hpp>
#include <tcc/Tensor.hpp>
#include <util/Log.hpp>

#include <string>
#include <memory>

namespace tcc {
  template <typename F>
  class ContractionOperation: public Operation<F> {
  public:
    ContractionOperation(
      const std::shared_ptr<Operation<F>> &left_,
      const std::shared_ptr<Operation<F>> &right_,
      const std::shared_ptr<Tensor<F>> &result_,
      const char *resultIndices_,
      Costs &contractionCosts
    ):
      Operation<F>(left_->costs + right_->costs),
      left(left_), right(right_),
      result(result_),
      resultIndices(resultIndices_)
    {
      // so far, costs contains costs involved to get left and right factors
      // during contraction all elements of left,right and result are present
      contractionCosts.maxElementsCount =
        contractionCosts.elementsCount + this->costs.elementsCount;
      // the intermediate results are, however, no longer needed afterwards
      this->costs.elementsCount = 0;
      this->costs += contractionCosts;
    }

    virtual ~ContractionOperation() {
    }

    virtual void execute() {
      left->execute();
      right->execute();
      LOG(1, "TCC") << "executing " <<
        result->getName() << "[" << resultIndices << "] = " <<
        left->getResult()->getName() << "[" << left->getResultIndices() <<
        "] * " <<
        right->getResult()->getName() << "[" << right->getResultIndices() <<
        "]" << std::endl;
    }

    virtual std::shared_ptr<Tensor<F>> getResult() {
      return result;
    }

    virtual std::string const &getResultIndices() {
      return resultIndices;
    }

  protected:
    std::shared_ptr<Operation<F>> left;
    std::shared_ptr<Operation<F>> right;

    std::shared_ptr<Tensor<F>> result;
    std::string resultIndices;

    friend class Contraction<F>;
  };
}

#endif
