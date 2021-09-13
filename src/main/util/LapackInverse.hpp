#ifndef LAPACK_INVERSE_DEFINED
#define LAPACK_INVERSE_DEFINED

#include <math/Complex.hpp>
#include <util/LapackMatrix.hpp>
#include <util/Exception.hpp>
#include <extern/Lapack.hpp>
#include <util/Log.hpp>

#include <vector>

namespace cc4s {
  // base template
  template <typename F=real>
  class LapackInverse;


  // specialization for complex
  template <>
  class LapackInverse<Complex64> {
  public:
    LapackInverse(
      const LapackMatrix<Complex64> &A_
    ): invA(A_) {
      if (A_.getRows() != A_.getColumns()) {
        throw EXCEPTION("Inverse requries a square matrix");
      }
      int rows(A_.getRows());
      std::vector<Complex64> work(rows*rows);
      int workSize(work.size());
      std::vector<int> rowPermutation(rows);
      int info;

      zgetrf_(
        &rows, &rows,
        invA.getValues(), &rows,
        rowPermutation.data(),
        &info
      );
      zgetri_(
        &rows,
        invA.getValues(), &rows,
        rowPermutation.data(),
        work.data(), &workSize,
        &info
      );
      if (info < 0) {
        std::stringstream stream;
        stream << "Argument " << -info << " of ZGETRI is illegal";
        throw EXCEPTION(stream.str());
      }
      if (info > 0) {
        throw EXCEPTION("Singular matrix cannot be inverted");
      }
    }

    ~LapackInverse() {
    }

    const LapackMatrix<Complex64> &get() const {
      return invA;
    }
  protected:
    LapackMatrix<Complex64> invA;
  };
}

#endif

