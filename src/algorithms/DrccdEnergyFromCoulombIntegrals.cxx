#include <algorithms/DrccdEnergyFromCoulombIntegrals.hpp>
#include <math/MathFunctions.hpp>
#include <tcc/DryTensor.hpp>
#include <util/Log.hpp>
#include <util/Exception.hpp>
#include <ctf.hpp>

using namespace CTF;
using namespace cc4s;

ALGORITHM_REGISTRAR_DEFINITION(DrccdEnergyFromCoulombIntegrals);

DrccdEnergyFromCoulombIntegrals::DrccdEnergyFromCoulombIntegrals(
  std::vector<Argument> const &argumentList
): ClusterSinglesDoublesAlgorithm(argumentList) {
}

DrccdEnergyFromCoulombIntegrals::~DrccdEnergyFromCoulombIntegrals() {
}

void DrccdEnergyFromCoulombIntegrals::iterate(int i, Mixer<double> *mixer) {
  iterate<double>(i, mixer);
}
void DrccdEnergyFromCoulombIntegrals::iterate(int i, Mixer<complex> *mixer) {
  iterate<complex>(i, mixer);
}

template <typename F>
void DrccdEnergyFromCoulombIntegrals::iterate(int i, Mixer<F> *mixer) {
  FockVector<F> *amplitudes(&mixer->getNext());
  Tensor<F> *Tai( &amplitudes->componentTensors[0] );
  Tensor<F> *Tabij( &amplitudes->componentTensors[1] );
  // Read all required integrals
  Tensor<F> *Vabij(getTensorArgument<F>("PPHHCoulombIntegrals"));
  Tensor<F> *Vaijb(getTensorArgument<F>("PHHPCoulombIntegrals"));
  Tensor<F> *Vijab(getTensorArgument<F>("HHPPCoulombIntegrals"));

  // Construct intermediate Amplitudes
  Tensor<F> Rabij(false, *Tabij);

  std::string abbreviation(getAbbreviation());
  std::transform(abbreviation.begin(), abbreviation.end(), 
                 abbreviation.begin(), ::toupper);

  int linearized(getIntegerArgument("linearized", 0));
  if (linearized) {
    LOG(1, abbreviation) << "Solving linearized T2 Amplitude Equations" << std::endl;
  } else {
    LOG(1, abbreviation) << "Solving T2 Amplitude Equations" << std::endl;
  }

  if (i == 0) {
    // For first iteration compute only the MP2 amplitudes 
    // Since Tabij = 0, Vabij is the only non-zero term

    Rabij["abij"] += (*Vabij)["abij"];
  } 
  else {
    // For the rest iterations compute the DRCCD amplitudes
    Rabij["abij"]  = (*Vabij)["abij"];
    Rabij["abij"] += 2.0 * (*Vaijb)["akic"] * (*Tabij)["cbkj"];
    Rabij["abij"] += 2.0 * (*Vaijb)["bkjc"] * (*Tabij)["acik"];
    if (!linearized) {
      // Construct intermediates
      Tensor<F> Calid(false, *Vaijb);
      Calid["alid"]  = 2.0 * (*Vijab)["klcd"] * (*Tabij)["acik"];
      Rabij["abij"] += 2.0 * Calid["alid"] * (*Tabij)["dblj"];
    }
  }

  // Calculate the amplitdues from the residuum
  amplitudesFromResiduum(Rabij, "abij");
  FockVector<F> newAmplitudes({*Tai, Rabij}, {"ai", "abij"});
  // And append them to the mixer
  mixer->append(newAmplitudes);
}

