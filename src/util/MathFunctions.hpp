#ifndef MATH_FUNCTIONS_DEFINED
#define MATH_FUNCTIONS_DEFINED

#include <util/Complex.hpp>
#include <cmath>

/**
 * \brief This class offers common math functions as static members.
 * As opposed to the variants in the std namesapce, these functions
 * are type closed, i.e. return the same type as the arguments,
 * which is required by Tensor::sum for univariate functions and
 * by Tensor::contract for bivariate functions.
 */
class MathFunctions {
public:
  // univariate functions
  template <typename F=double>
  static F sqrt(F const x) {
    return std::sqrt(x);
  }

  template <typename F=double>
  static F abs(F const x) {
    return std::abs(x);
  }

  template <typename F=double>
  static F conj(F const x) {
    return std::conj(x);
  }

  static double conj(double const x) {
    return x;
  }

  // bivariate functions
  template <typename F=double>
  static F dot(F const x, F const y) {
    return x * conj(y);
  }

  /**
   * \brief Calculates only the real part of x*conj(y).
   */
  template <typename F=double>
  static F realDot(F const x, F const y) {
    return std::real(x*conj(y));
  }

  template <typename F=double>
  static F divide(F const x, F const y) {
    return x / y;
  }
};

#endif
