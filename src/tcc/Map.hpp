/*Copyright (c) 2019, Andreas Grueneis and Felix Hummel, all rights reserved.*/
#ifndef TCC_MAP_DEFINED
#define TCC_MAP_DEFINED

#include <tcc/Expression.hpp>
#include <tcc/MapOperation.hpp>

#include <util/SharedPointer.hpp>
#include <util/StaticAssert.hpp>

namespace tcc {
  template <typename Target, typename Domain, typename TE>
  class Map: public TensorResultExpression<Target,TE> {
  public:
    /**
     * \brief Creates a map expression of a unary map f and one tensor
     * expressions source.
     **/
    static PTR(ESC(Map<Target,Domain,TE>)) create(
      const std::function<Target(const Domain)> &f,
      const PTR(ESC(TensorResultExpression<Domain,TE>)) &source
    ) {
      return NEW(ESC(Map<Target,Domain,TE>),
        f, source,
        typename Expression<TE>::ProtectedToken()
      );
    }

    /**
     * \brief Creates a map expression of a unary map f and one tensor
     * expressions source.
     * Not indended for direct invocation. Use Map::create instead.
     **/
    Map(
      const std::function<Target(const Domain)> &f_,
      const PTR(ESC(TensorResultExpression<Domain,TE>)) &source_,
      const typename Expression<TE>::ProtectedToken &
    ): f(f_), source(source_) {
    }

    virtual ~Map() {
    }

    virtual PTR(Operation<TE>) compile(IndexCounts &indexCounts) {
      auto sourceOperation(
        DYNAMIC_PTR_CAST(
          ESC(TensorResultOperation<Domain,TE>),
          source->compile(indexCounts)
        )
      );
      return MapOperation<Target,Domain,TE>::create(f, sourceOperation);
    }

    virtual void countIndices(IndexCounts &indexCounts) {
      source->countIndices(indexCounts);
    }

  protected:
    std::function<Target(const Domain)> f;
    PTR(ESC(TensorResultExpression<Domain,TE>)) source;
  };

  /**
   * \brief Creates a map expression of a unary map f and one tensor
   * expressions A.
   **/
  template <
    typename Target, typename RHS
  >
  inline
  PTR(ESC(Map<Target,typename RHS::FieldType,typename RHS::TensorEngine>)) map(
    const std::function<Target(const typename RHS::FieldType)> &f,
    const PTR(RHS) &A
  ) {
    return
    Map<Target,typename RHS::FieldType,typename RHS::TensorEngine>::create(
      f, A
    );
  }
}

#endif
