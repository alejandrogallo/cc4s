#include <util/IterativePseudoInverter.hpp>

#include <util/MathFunctions.hpp>
#include <util/Log.hpp>
#include <complex>
#include <limits>
#include <random>

using namespace cc4s;
using namespace CTF;

// TODO: place in proper file
template <typename F>
void dumpMatrix(Matrix<F> &m) {
  F *values(new F[m.lens[0]*m.lens[1]]);
  m.read_all(values);
  for (int i(0); i < m.lens[0]; ++i) {
    for (int j(0); j < m.lens[1]; ++j) {
      LOG(3) << " " << values[i+j*m.lens[0]];
    }
    LOG(3) << std::endl;
  }
}

template <typename F>
IterativePseudoInverter<F>::IterativePseudoInverter(
  Matrix<F> const &matrix_
):
  matrix(matrix_),
  square(matrix_.lens[0], matrix_.lens[0], *matrix_.wrld),
  inverse(matrix_.lens[1], matrix_.lens[0], *matrix_.wrld),
  alpha()
{
  Matrix<F> conjugate(matrix.lens[1], matrix.lens[0], *matrix.wrld);
  Univar_Function<F> fConj(&conj<F>);
  conjugate.sum(1.0,matrix,"ij", 0.0,"ji",fConj);
  Matrix<F> square(matrix.lens[0], matrix.lens[0], *matrix.wrld);
  square["ij"] = matrix["ik"] * conjugate["kj"];
  Univar_Function<F> fAbs(&abs<F>);
  Vector<F> rowAbsNorms(square.lens[0], *matrix.wrld);
  rowAbsNorms.sum(1.0,square,"ij", 0.0,"i",fAbs);
  F *normValues(new F[rowAbsNorms.lens[0]]);
  rowAbsNorms.read_all(normValues);
  double max(-std::numeric_limits<double>::infinity());
  for (int i(0); i < square.lens[0]; ++i) {
    if (std::real(normValues[i]) > max) max = std::real(normValues[i]);
  }
  alpha = 1.0/max;
  LOG(4) << "alpha=" << alpha << std::endl;
  inverse["ji"] = alpha * conjugate["ji"];
}

template <typename F>
void IterativePseudoInverter<F>::iterate(double accuracy) {
  Scalar<F> s;
  Matrix<F> conjugate(matrix.lens[1], matrix.lens[0], *matrix.wrld);
  Univar_Function<F> fConj(&conj<F>);
  conjugate.sum(1.0,matrix,"ij", 0.0,"ji",fConj);
  Matrix<F> sqr(matrix.lens[1], matrix.lens[1], *matrix.wrld);  double remainder(1.0), minRemainder(std::numeric_limits<double>::infinity());
  int n(0), nMin(0);
  // TODO: use constants for limits
  while (remainder > accuracy*accuracy && n-nMin < 100 && n < 10000) {

    sqr["ij"] = -1.0 * inverse["ik"] * matrix["kj"];
    sqr["ii"] += 1.0;
    Bivar_Function<F> fRealDot(&realDot<F>);
    s.contract(1.0, sqr,"ij", sqr,"ij", 0.0,"", fRealDot);
    inverse["ij"] += alpha * sqr["ik"] * conjugate["kj"];
    remainder = std::real(s.get_val());
    if (remainder < minRemainder) {
      minRemainder = remainder;
      nMin = n;
    }
    ++n;
  }
  if (n >= 10000) {
    // failed to convege
    LOG(4) << "  failed to converge, remainder=" << remainder << std::endl;
    LOG(4) << "  minRemainder=" << minRemainder << std::endl;
    dumpMatrix(inverse);
  }
}

template <typename F>
void IterativePseudoInverter<F>::iterateQuadratically(double accuracy) {
  Scalar<F> s;
  double remainder(1.0), minRemainder(std::numeric_limits<double>::infinity());
  int n(0), nMin(0);
  // TODO: use constants for limits
  while (remainder > accuracy*accuracy && n-nMin < 20 && n < 10000) {
    square["ij"] = -1.0 * matrix["ik"] * inverse["kj"];
    square["ii"] += 2.0;
    inverse["ij"] = inverse["ik"] * square["kj"];
    square["ii"] += -1.0;
    Bivar_Function<F> fRealDot(&realDot<F>);
    s.contract(1.0, square,"ij", square,"ij", 0.0,"", fRealDot);
    remainder = std::real(s.get_val());
    if (remainder < minRemainder) {
      minRemainder = remainder;
      nMin = n;
    }
    ++n;
  }
  if (n >= 10000) {
    // failed to convege
    LOG(4) << " failed to converge, remainder=" << remainder << std::endl;
    LOG(4) << " minRemainder=" << minRemainder << std::endl;
    dumpMatrix(inverse);
  }
}

template <typename F>
Matrix<F> &IterativePseudoInverter<F>::invert(double accuracy) {
  iterateQuadratically(accuracy);
//  iterate(accuracy);
  return inverse;
}

// instantiate
template
IterativePseudoInverter<double>::IterativePseudoInverter(
  Matrix<double> const &matrix
);
template
Matrix<double> &IterativePseudoInverter<double>::invert(double accuracy);

template
IterativePseudoInverter<complex>::IterativePseudoInverter(
  Matrix<complex> const &matrix
);
template
Matrix<complex> &IterativePseudoInverter<complex>::invert(double accuracy);


template <typename F>
void IterativePseudoInverter<F>::generateHilbertMatrix(Matrix<F> &m) {
  int64_t indicesCount, *indices;
  F *values;
  m.read_local(&indicesCount, &indices, &values);
  for (int64_t l(0); l < indicesCount; ++l) {
    int i = int(l % m.lens[0]);
    int j = int(l / m.lens[0]);
    values[l] = 1.0 / (i+j+1);
  }
  m.write(indicesCount, indices, values);
  free(indices); free(values);
}

template <>
void IterativePseudoInverter<double>::setRandom(
  double &value,
  std::mt19937 &random, std::normal_distribution<double> &normalDistribution
) {
  value = normalDistribution(random);
}

template <>
void IterativePseudoInverter<complex>::setRandom(
  complex &value,
  std::mt19937 &random, std::normal_distribution<double> &normalDistribution
) {
  value.real(normalDistribution(random));
  value.imag(normalDistribution(random));
}

// TODO: place in proper file
template <typename F>
void IterativePseudoInverter<F>::generateRandomMatrix(Matrix<F> &m) {
  int64_t indicesCount, *indices;
  F *values;
  std::mt19937 random;
  random.seed(m.wrld->rank);
  std::normal_distribution<double> normalDistribution(0.0, 1.0);
  m.read_local(&indicesCount, &indices, &values);
  for (int64_t i(0); i < indicesCount; ++i) {
    setRandom(values[i], random, normalDistribution);
  }
  m.write(indicesCount, indices, values);
  free(indices); free(values);
}


template <typename F>
void IterativePseudoInverter<F>::test(World *world) {
  Matrix<F> m(10, 10, NS, *world);
  {
    generateHilbertMatrix(m);
    IterativePseudoInverter pseudoInverter(m);
    Matrix<F> im(pseudoInverter.invert());
    dumpMatrix(im);
    im["ij"] = m["ik"] * im["kj"];
    im["ii"] += -1.0;
    Scalar<F> s(*world);
    s[""] = im["ij"] * im["ij"];
    double n(std::real(s.get_val()));
    LOG(3) << n << std::endl;
  }
  {
    generateRandomMatrix(m);
    IterativePseudoInverter pseudoInverter(m);
    Matrix<F> im(pseudoInverter.invert());
    dumpMatrix(im);
    im["ij"] = m["ik"] * im["kj"];
    im["ii"] += -1.0;
    Scalar<F> s(*world);
    s[""] = im["ij"] * im["ij"];
    double n(std::real(s.get_val()));
    LOG(3) << n << std::endl;
  }
}

// instantiate
template
void IterativePseudoInverter<double>::test(World *world);
template
void IterativePseudoInverter<complex>::test(World *world);
