/*Copyright (c) 2016, Andreas Grueneis and Felix Hummel, all rights reserved.*/
#ifndef TCC_ASSIGNMENT_DEFINED
#define TCC_ASSIGNMENT_DEFINED

#include <tcc/Expression.hpp>
#include <tcc/IndexedTensor.hpp>
#include <tcc/Operation.hpp>
#include <tcc/AssignmentOperation.hpp>
#include <util/StaticAssert.hpp>
#include <util/Exception.hpp>

#include <memory>

namespace tcc {
  template <typename F>
  class Assignment: public Expression<F> {
  public:
    Assignment(
      const std::shared_ptr<IndexedTensor<F>> &lhs_,
      const std::shared_ptr<Expression<F>> &rhs_
    ) {
      static_assert(
        cc4s::StaticAssert<F>::False,
        "Only tensors or contractions may be used as the right hand side of an assignment."
      );
    }
    Assignment(
      const std::shared_ptr<IndexedTensor<F>> &lhs_,
      const std::shared_ptr<IndexedTensor<F>> &rhs_
    ): lhs(lhs_), rhs(rhs_) {
    }
    Assignment(
      const std::shared_ptr<IndexedTensor<F>> &lhs_,
      const std::shared_ptr<Contraction<F>> &rhs_
    ): lhs(lhs_), rhs(rhs_) {
    }
    virtual ~Assignment() {
    }

    virtual std::shared_ptr<Operation<F>> compile(std::string const &) {
      return std::make_shared<AssignmentOperation<F>>(
        std::make_shared<FetchOperation<F>>(lhs),
        rhs->compile(lhs->indices)
      );
    }

    std::shared_ptr<IndexedTensor<F>> lhs;
    std::shared_ptr<Expression<F>> rhs;
  };
}

#endif

