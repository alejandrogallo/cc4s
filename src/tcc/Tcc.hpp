/*Copyright (c) 2016, Andreas Grueneis and Felix Hummel, all rights reserved.*/
#ifndef TCC_DEFINED
#define TCC_DEFINED

#include <tcc/IndexedTensor.hpp>
#include <tcc/Move.hpp>
#include <tcc/Contraction.hpp>
#include <tcc/Map.hpp>
#include <tcc/Sequence.hpp>
#include <tcc/Tensor.hpp>

#include <math/Float.hpp>
#include <util/SharedPointer.hpp>

// TODO: binary function application
// TODO: support slicing and looping over indices for memory reduction
// TODO: support hard memory limit for costs
// TODO: common subexpression optimization
// TODO: heuristics: limit number of simultaneously considered intermediates
// TODO: fix max memory assessment
// TODO: expression definitions with local index renaming
// TODO: permutation and anti-permutation operator

namespace tcc {
  template <typename TensorEngine>
  class Tcc {
  protected:
    class ProtectedToken {
    };

  public:
    template <typename F=cc4s::real>
    static PTR(ESC(Tensor<F,TensorEngine>)) tensor(
      const std::vector<size_t> &lens, const std::string &name
    ) {
      return Tensor<F,TensorEngine>::create(lens, name);
    }

    template <typename F=cc4s::real>
    static PTR(ESC(Tensor<F,TensorEngine>)) tensor(
      const PTR(ESC(Tensor<F,TensorEngine>)) &source, const std::string &name
    ) {
      return Tensor<F,TensorEngine>::create(source->getLens(), name);
    }

    template <typename F=cc4s::real>
    static PTR(ESC(Tensor<F,TensorEngine>)) tensor(const std::string &name) {
      return Tensor<F,TensorEngine>::create(name);
    }

    static PTR(Sequence<TensorEngine>) nothing() {
      return NEW(Sequence<TensorEngine>);
    }
  };
}

#endif

