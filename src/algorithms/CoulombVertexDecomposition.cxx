/*Copyright (c) 2016, Andreas Grueneis and Felix Hummel, all rights reserved.*/

#include <algorithms/CoulombVertexDecomposition.hpp>
#include <math/CanonicalPolyadicDecomposition.hpp>
#include <math/RandomTensor.hpp>
#include <math/MathFunctions.hpp>
#include <mixers/Mixer.hpp>
#include <util/Log.hpp>
#include <limits>

using namespace cc4s;
using namespace CTF;

ALGORITHM_REGISTRAR_DEFINITION(CoulombVertexDecomposition);

CoulombVertexDecomposition::
  CoulombVertexDecomposition
(
  std::vector<Argument> const &argumentList
):
  Algorithm(argumentList),
  composedGammaGqr(nullptr), PiqR(nullptr), regularizationEstimator(nullptr)
{
}

CoulombVertexDecomposition::
  ~CoulombVertexDecomposition()
{
  if (PiqR) delete PiqR;
  if (!isArgumentGiven("ComposedCoulombVertex") && composedGammaGqr) delete composedGammaGqr;
  if (regularizationEstimator) delete regularizationEstimator;
}

void CoulombVertexDecomposition::run() {
  GammaGqr = getTensorArgument<complex>("CoulombVertex");
  int NG(GammaGqr->lens[0]);
  int Np(GammaGqr->lens[1]);

  // calculate decomposition rank
  rank = getIntegerArgument("rank", DEFAULT_RANK_SIZE);
  // if rank is not given use rank factors (if they are not given use rankFactors=3.0)
  if (rank == -1) {
    double rankFactor(getRealArgument("rankFactor", DEFAULT_RANK_FACTOR));
    rank = NG * rankFactor;
  }

  realFactorOrbitals = getIntegerArgument(
    "realFactorOrbitals", DEFAULT_REAL_FACTOR_ORBITALS
  );
  normalizedFactorOrbitals = getIntegerArgument(
    "normalizedFactorOrbitals", DEFAULT_NORMALIZED_FACTOR_ORBITALS
  );
  LOG(0, "RALS") << "Tensor rank decomposition with rank NR=" << rank
    << ", realFactorOrbitals=" << realFactorOrbitals
    << ", normalizedFactorOrbitals=" << normalizedFactorOrbitals << std::endl;
  LOG(1, "RALS") << "Decomposing Coulomb vertex " << GammaGqr->get_name() << " with NG=" << NG
    << ", Np=" << Np << std::endl;

  writeSubIterations = getIntegerArgument(
    "writeSubIterations", DEFAULT_WRITE_SUB_ITERATIONS
  );

  // allocate factor tensors
  if (isArgumentGiven("StartingFactorOrbitals")) {
    Tensor<complex> *PirRTensor(getTensorArgument<complex>("StartingFactorOrbitals"));
    PirRTensor->set_name("StartingPirR");
    if (PirRTensor->order != 2) throw new EXCEPTION("Matrix expected as argument StartingPirR");
    LOG(1, "RALS") << "Initial PirR=" << PirRTensor->get_name() << std::endl;
    PirR = reinterpret_cast<Matrix<complex> *>(PirRTensor);
  }
  else {
    PirR = new Matrix<complex>(Np, int(rank), NS, *GammaGqr->wrld, "PirR", GammaGqr->profile);
    LOG(1, "RALS") << "Initial PirR=RandomTensor" << std::endl;
    setRandomTensor(*PirR);
    realizePi(*PirR); normalizePi(*PirR);
  }

  if (isArgumentGiven("StartingCoulombFactors")) {
    Tensor<complex> *LambdaGRTensor(getTensorArgument<complex>("StartingCoulombFactors"));
    LambdaGRTensor->set_name("StartingLambdaGR");
    if (LambdaGRTensor->order != 2) throw new EXCEPTION("Matrix expected as argument StartingLambdaGR");
    LOG(1, "RALS") << "Initial LambdaGR=" << LambdaGRTensor->get_name() << std::endl;
    LambdaGR = reinterpret_cast<Matrix<complex> *>(LambdaGRTensor);
  }
  else {
    LambdaGR = new Matrix<complex>(NG, int(rank), NS, *GammaGqr->wrld, "LambdaGR", GammaGqr->profile);
    LOG(1, "RALS") << "Initial LambdaGR=RandomTensor" << std::endl;
    setRandomTensor(*LambdaGR);
  }

  PiqR = new Matrix<complex>(Np, int(rank), NS, *GammaGqr->wrld, "PiqR", GammaGqr->profile);
  Univar_Function<complex> fConj(&cc4s::conj<complex>);
  // PiqR["qR"] = conj(PirR["qR"])
  PiqR->sum(1.0, *PirR,"qR", 0.0,"qR", fConj);

  allocatedTensorArgument<complex>("FactorOrbitals", PirR);
  allocatedTensorArgument<complex>("CoulombFactors", LambdaGR);

  composedGammaGqr = new Tensor<complex>(
    3, GammaGqr->lens, GammaGqr->sym, *GammaGqr->wrld, "composedGammaGqr",
    GammaGqr->profile
  );
  if (isArgumentGiven("ComposedCoulombVertex")) {
    allocatedTensorArgument<complex>("ComposedCoulombVertex", composedGammaGqr);
  }

  double swampingThreshold(
    getRealArgument("swampingThreshold", DEFAULT_SWAMPING_THRESHOLD)
  );
  double regularizationFriction(
    getRealArgument("regularizationFriction", DEFAULT_REGULARIZATION_FRICTION)
  );
  regularizationEstimator =
    new AlternatingLeastSquaresRegularizationEstimator(
      swampingThreshold, regularizationFriction, 1
    );
  int64_t iterationsCount(0);
  int64_t maxIterationsCount(
    getIntegerArgument("maxIterations", DEFAULT_MAX_ITERATIONS)
  );
  double delta(getRealArgument("delta", DEFAULT_DELTA));
  Delta = std::numeric_limits<double>::infinity();
  while (iterationsCount < maxIterationsCount && Delta > delta) {
    fit(iterationsCount);
    ++iterationsCount;
  }
}

void CoulombVertexDecomposition::dryRun() {
  // NOTE that in the dry run GammaGai,... are local variables
  DryTensor<complex> *GammaGqr(
    getTensorArgument<complex, DryTensor<complex>>("CoulombVertex")
  );
  int NG(GammaGqr->lens[0]);
  int Np(GammaGqr->lens[1]);


  // calculate decomposition rank
  rank = getIntegerArgument("rankSize", DEFAULT_RANK_SIZE);
  // if rank is not given use rank factors (if they are not given use rankFactors=2.0)
  if (rank == -1) {
    double rankFactor(getRealArgument("rankFactor", DEFAULT_RANK_FACTOR));
    rank = NG * rankFactor;
  }

  realFactorOrbitals = getIntegerArgument(
    "realFactorOrbitals", DEFAULT_REAL_FACTOR_ORBITALS
  );
  normalizedFactorOrbitals = getIntegerArgument(
    "normalizedFactorOrbitals", DEFAULT_NORMALIZED_FACTOR_ORBITALS
  );
  LOG(0, "RALS") << "Tensor rank decomposition with rank NR=" << rank
    << ", realFactorOrbitals=" << realFactorOrbitals
    << ", normalizedFactorOrbitals=" << normalizedFactorOrbitals << std::endl;
  LOG(1, "RALS") << "Decomposing Coulomb vertex with NG=" << NG
    << " Np=" << Np << std::endl;

  if (isArgumentGiven("StartingFactorOrbitals")) {
    LOG(1, "RALS") << "Initial PirR=StartingPirR" << std::endl;
  }
  else {
    LOG(1, "RALS") << "Initial PirR=RandomTensor" << std::endl;
  }

  if (isArgumentGiven("StartingCoulombFactors")) {
    LOG(1, "RALS") << "Initial LambdaGR=StartingLambdaGR" << std::endl;
  }
  else {
    LOG(1, "RALS") << "Initial LambdaGR=RandomTensor" << std::endl;
  }

  // allocate factor tensors
  DryTensor<complex> *PiqR = new DryMatrix<complex>(Np, int(rank), NS);
  DryTensor<complex> *PirR = new DryMatrix<complex>(Np, int(rank), NS);
  DryTensor<complex> *LambdaGR = new DryMatrix<complex>(NG, int(rank), NS);
  allocatedTensorArgument<complex, DryTensor<complex>>(
    "FactorOrbitals", PiqR
  );
  allocatedTensorArgument<complex, DryTensor<complex>>(
    "CoulombFactors", LambdaGR
  );

  DryTensor<complex> *composedGammaGqr(new DryTensor<complex>(*GammaGqr));
  if (isArgumentGiven("ComposedCoulombVertex")) {
    allocatedTensorArgument<complex, DryTensor<complex>>(
      "ComposedCoulombVertex", composedGammaGqr
    );
  }
  dryFit(GammaGqr, PiqR, PirR, LambdaGR, composedGammaGqr);
}


void CoulombVertexDecomposition::fit(
  int64_t const iterationsCount
) {

  int fitFactorOrbitals(getIntegerArgument
                        ("fitFactorOrbitals", 1));

  if (fitFactorOrbitals) {
    iterateQuadraticFactor(iterationsCount);
  }

  int fitCoulombFactors(getIntegerArgument
                        ("fitCoulombFactors", 1));

  if (fitCoulombFactors) {
    fitRegularizedAlternatingLeastSquaresFactor(*GammaGqr,"Gqr", *PirR,'r', *PiqR,'q',
                                                *LambdaGR,'G', regularizationEstimator);
  }

  Delta = getDelta();
  LOG(0, "RALS") << "iteration=" << (iterationsCount+1)
    << " Delta=" << Delta << std::endl;
}

void CoulombVertexDecomposition::dryFit(
  DryTensor<complex> *GammaGqr,
  DryTensor<complex> *PiqR, DryTensor<complex> *PirR,
  DryTensor<complex> *LambdaGR,
  DryTensor<complex> *composedGammaGqr
) {
  dryFitRegularizedAlternatingLeastSquaresFactor(
    *GammaGqr,"Gqr", *PiqR,'q', *LambdaGR,'G',
    *PirR,'r'
  );
  dryFitRegularizedAlternatingLeastSquaresFactor(
    *GammaGqr,"Gqr", *LambdaGR,'G', *PirR,'r',
    *PiqR,'q'
  );
  dryFitRegularizedAlternatingLeastSquaresFactor(
    *GammaGqr,"Gqr", *PirR,'r', *PiqR,'q',
    *LambdaGR,'G'
  );
  dryComposeCanonicalPolyadicDecompositionTensors(
    *LambdaGR, *PiqR, *PirR, *composedGammaGqr
  );
}

void CoulombVertexDecomposition::normalizePi(
  Matrix<complex> &Pi
) {
  Bivar_Function<complex> fDot(&cc4s::dot<complex>);
  Vector<complex> norm(Pi.lens[0], *Pi.wrld);
  // norm["q"] = Pi["qR"] * conj(Pi["qR"])
  norm.contract(1.0, Pi,"qR", Pi,"qR", 0.0,"q", fDot);
  Matrix<complex> quotient(Pi);
  Univar_Function<complex> fSqrt(&cc4s::sqrt<complex>);
  // quotient["qR"] = sqrt(norm["q"])
  quotient.sum(1.0, norm,"q", 0.0,"qR", fSqrt);
  Bivar_Function<complex> fDivide(&cc4s::divide<complex>);
  // Pi["qR"] = Pi["qR"] / quotient["qR"]
  Pi.contract(1.0, Pi,"qR", quotient,"qR", 0.0,"qR", fDivide);
}

void CoulombVertexDecomposition::realizePi(
  Matrix<complex> &Pi
) {
  Univar_Function<complex> fConj(&cc4s::conj<complex>);
  Matrix<complex> conjX(Pi);
  // conjX["qR"] = conj(Pi["qR"])
  conjX.sum(1.0, Pi,"qR", 0.0,"qR", fConj);
  Pi["qR"] += conjX["qR"];
  Pi["qR"] *= 0.5;
}

void CoulombVertexDecomposition::iterateQuadraticFactor(int i) {
  // create a mixer
  std::string mixerName(getTextArgument("mixer", "LinearMixer"));
  Mixer<complex> *mixer(MixerFactory<complex>::create(mixerName, this));
  if (!mixer) {
    std::stringstream stringStream;
    stringStream << "Mixer not implemented: " << mixerName;
    throw new EXCEPTION(stringStream.str());
  }

  Univar_Function<complex> fConj(&cc4s::conj<complex>);
  // initial guess
  double quadraticDelta(getDelta());
  fitAlternatingLeastSquaresFactor(
    *GammaGqr,"Gqr", *PiqR,'q', *LambdaGR,'G', *PirR,'r'
  );
  if (realFactorOrbitals) realizePi(*PirR);
  if (normalizedFactorOrbitals) normalizePi(*PirR);
  mixer->append(*PirR);
  // (*PiqR)["qR"] = (*PirR)["qR"];
  PiqR->sum(1.0, *PirR,"qR", 0.0,"qR",fConj);
  if (writeSubIterations) {
    LOG(1, "Babylonian") << "|Pi^(" << (i+1) << "," << 0 << ")"
      << "Pi^(" << (i+1) << "," << 0 << ")"
      << "Lambda^(n) - Gamma|=" << quadraticDelta << std::endl;
  }

  // Babylonian algorithm to solve quadratic form
  int maxSubIterationsCount(getIntegerArgument("maxSubIterations", 8));
  int minSubIterationsCount(getIntegerArgument("minSubIterations", 1));
  int j(0);
  Delta = quadraticDelta;
  while (
    j < minSubIterationsCount ||
    (Delta < quadraticDelta && j < maxSubIterationsCount)
  ) {
    fitAlternatingLeastSquaresFactor(
      *GammaGqr,"Gqr", *PiqR,'q', *LambdaGR,'G', *PirR,'r'
    );
    if (realFactorOrbitals) realizePi(*PirR);
    if (normalizedFactorOrbitals) normalizePi(*PirR);
    mixer->append(*PirR);
    if (writeSubIterations) {
      quadraticDelta = getDelta();
      LOG(1, "Babylonian") << "|Pi^(" << (i+1) << "," << (j+1) << ")"
        << "Pi^(" << (i+1) << "," << j << ")"
        << "Lambda^(n) - Gamma|=" << quadraticDelta << std::endl;
    }
    (*PirR)["qR"] = mixer->getNext()["qR"];
    // (*PiqR)["qR"] = (*PirR)["qR"];
    PiqR->sum(1.0, *PirR,"qR", 0.0,"qR",fConj);
    quadraticDelta = getDelta();
    if (writeSubIterations) {
      LOG(1, "Babylonian") << "|Pi^(" << (i+1) << "," << (j+1) << ")"
        << "Pi^(" << (i+1) << "," << (j+1) << ")"
        << "Lambda^(n) - Gamma|=" << quadraticDelta << std::endl;
    }
    ++j;
  }
  delete mixer;
}

double CoulombVertexDecomposition::getDelta() {
  composeCanonicalPolyadicDecompositionTensors(
    *LambdaGR, *PiqR, *PirR, *composedGammaGqr
  );
  (*composedGammaGqr)["Gqr"] -= (*GammaGqr)["Gqr"];
  double Delta(frobeniusNorm(*composedGammaGqr));
  (*composedGammaGqr)["Gqr"] += (*GammaGqr)["Gqr"];
  return Delta;
}

