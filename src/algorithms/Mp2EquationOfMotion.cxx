#include <algorithms/Mp2EquationOfMotion.hpp>
#include <tcc/Tcc.hpp>
#include <tcc/DryMachineTensor.hpp>
#include <math/MathFunctions.hpp>
#include <math/RandomTensor.hpp>
#include <util/Log.hpp>
#include <util/Exception.hpp>
#include <ctf.hpp>
#include <Cc4s.hpp>

using namespace cc4s;
using namespace tcc;

ALGORITHM_REGISTRAR_DEFINITION(Mp2EquationOfMotion);

// TODO: Study the requirements to treat the problem with real numbers or
// complex

Mp2EquationOfMotion::Mp2EquationOfMotion(
  std::vector<Argument> const &argumentList
): Algorithm(argumentList) {
}

Mp2EquationOfMotion::~Mp2EquationOfMotion() {
}

void Mp2EquationOfMotion::run() {
  typedef CTF::Tensor<> T;

  // Get orbital energies
  T *epsi(getTensorArgument<double, T>("HoleEigenEnergies"));
  T *epsa(getTensorArgument<double, T>("ParticleEigenEnergies"));

  int Nv(epsa->lens[0]), No(epsi->lens[0]);
  int totalDimension(1 + Nv * No + No * No * Nv * Nv);
  LOG(1, "MP2_EOM") << "Nv " << Nv << std::endl;
  LOG(1, "MP2_EOM") << "No " << No << std::endl;
  LOG(1, "MP2_EOM") << "Problem dimension " << totalDimension << std::endl;

  // Get couloumb integrals (these shoul not be antisymmetrized)
  T *Vabij(getTensorArgument<double, T>("PPHHCoulombIntegrals"));

  LOG(1, "MP2_EOM") << "Antisymmetrizing Vabij " << std::endl;
  (*Vabij)["abij"] -= (*Vabij)["abji"];

  T Tabij(false, Vabij);
  Tabij["abij"] =  (*epsi)["i"];
  Tabij["abij"] += (*epsi)["j"];
  Tabij["abij"] -= (*epsa)["a"];
  Tabij["abij"] -= (*epsa)["b"];

  T Dabij(false, Vabij);
  Dabij["abij"] = Tabij["abij"];

  CTF::Bivar_Function<> fDivide(&divide<double>);
  Tabij.contract(1.0, (*Vabij),"abij", Tabij,"abij", 0.0,"abij", fDivide);

  // Initialize 2 body L and R
  int oneBodyLens[] = {Nv, No};
  int oneBodySyms[] = {NS, NS};
  T Labij(false, Vabij);
  T *Lai( new CTF::Tensor<>(2, oneBodyLens, oneBodySyms, *Cc4s::world, "Lai") );
  T Rabij(false, Vabij);
  T *Rai( new CTF::Tensor<>(2, oneBodyLens, oneBodySyms, *Cc4s::world, "Rai") );

  //The totalDimension should be totalDimension, but the zero-particle part is
  //zero, so we restrict the hamiltonian to the space spanned by the singles
  //and doubles excitations
  int hLens[] = {totalDimension-1, totalDimension-1};
  int hSyms[] = {NS, NS};
  T *Hpq( new CTF::Tensor<>(2, hLens, hSyms, *Cc4s::world, "Lai") );

  double energy;

  for (int64_t i = 0 ; i < totalDimension-1 ; i++) {
      getCanonicalPerturbationBasis(*Lai, Labij, i);
    for (int64_t j = 0 ; j < totalDimension-1; j++) {
      //getCanonicalPerturbationBasis(*Rai, Rabij, i);

    }
  }

/*
 *  auto H20(new CTF::Tensor<>(*Vabij));
 *  auto Habij(new CTF::Tensor<>(*Vabij));
 *
 *  (*H20)["abij"] -= (*Vabij)["abji"];
 *
 *  // cdkl are row indices and abij are column indices
 *  int lens[] = {Nv,Nv,No,No, Nv,Nv,No,No};
 *  int syms[] = {NS,NS,NS,NS, NS,NS,NS,NS};
 *  auto H22( new CTF::Tensor<>(8, lens, syms, *Cc4s::world, "H22cdklabij") );
 *  // diagonal elements
 *  (*H22)["abijabij"] = Dabij["abij"];
 *  (*H22)["cbkjabij"] += (*Habij)["acik"];
 *  (*H22)["adilabij"] += (*Habij)["bdjl"];
 *
 *  allocatedTensorArgument("SimlarityTransformedHamiltonian22", H22);
 *  allocatedTensorArgument("SimlarityTransformedHamiltonian20", H20);
 *  allocatedTensorArgument("EnergyDenominators", new CTF::Tensor<>(Dabij));
 */

}

template <typename F>
void Mp2EquationOfMotion::getCanonicalPerturbationBasis(
    CTF::Tensor<F> &Tai, CTF::Tensor<F> &Tabij, int64_t i) {
  int oneBodyLength(Tai.lens[0] * Tai.lens[1]);
  int twoBodyLength(
      Tabij.lens[0] * Tabij.lens[1] * Tabij.lens[2] *  Tabij.lens[3]
  );
  int64_t indices[] = {i};
  F values[] = {1.0};

  Tabij["abij"] = 0;
  Tai["ai"] = 0;

  if (i+1 <= oneBodyLength) { // One body regime
    Tai.write(1, indices, values);
  } else { // Two body regime
    indices[0] -= oneBodyLength;
    Tabij.write(1, indices, values);
  }
  Tai.print();
  Tabij.print();

}

// instantiate
template
void Mp2EquationOfMotion::getCanonicalPerturbationBasis(
    CTF::Tensor<double> &Tai, CTF::Tensor<double> &Tabij, int64_t i);
