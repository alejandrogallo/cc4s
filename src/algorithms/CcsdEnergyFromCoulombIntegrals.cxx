#include <algorithms/CcsdEnergyFromCoulombIntegrals.hpp>
#include <math/MathFunctions.hpp>
#include <util/DryTensor.hpp>
#include <util/Log.hpp>
#include <util/Exception.hpp>
#include <ctf.hpp>

using namespace CTF;
using namespace cc4s;

ALGORITHM_REGISTRAR_DEFINITION(CcsdEnergyFromCoulombIntegrals);

CcsdEnergyFromCoulombIntegrals::CcsdEnergyFromCoulombIntegrals(
  std::vector<Argument> const &argumentList
): ClusterSinglesDoublesAlgorithm(argumentList) {
}

CcsdEnergyFromCoulombIntegrals::~CcsdEnergyFromCoulombIntegrals() {
}

//////////////////////////////////////////////////////////////////////
// Hirata iteration routine for the CCSD amplitudes Tabij and Tai from
// So Hirata, et. al. Chem. Phys. Letters, 345, 475 (2001)
//////////////////////////////////////////////////////////////////////
void CcsdEnergyFromCoulombIntegrals::iterate(int i) {
  {
    // Read the amplitudes Tai and Tabij
    Tensor<> *Tai(&TaiMixer->getNext());
    Tai->set_name("Tai");
    Tensor<> *Tabij(&TabijMixer->getNext());
    Tabij->set_name("Tabij");

    // Read the Coulomb Integrals Vabcd Vabij Vaibj Vijkl Vabci Vijka
    // the PPPPCoulombIntegrals may not be given then slicing is required
    Tensor<> *Vabcd(isArgumentGiven("PPPPCoulombIntegrals") ?
		    getTensorArgument("PPPPCoulombIntegrals") : nullptr);
    Tensor<> *Vabij(getTensorArgument("PPHHCoulombIntegrals"));
    Tensor<> *Vaibj(getTensorArgument("PHPHCoulombIntegrals"));
    Tensor<> *Vijkl(getTensorArgument("HHHHCoulombIntegrals"));
    Tensor<> *Vijka(getTensorArgument("HHHPCoulombIntegrals"));
    Tensor<> *Vabci(getTensorArgument("PPPHCoulombIntegrals"));

    // Get abbreviation of algorithm
    std::string abbreviation(getAbbreviation());
    std::transform(abbreviation.begin(), abbreviation.end(), 
		   abbreviation.begin(), ::toupper);
  
    // Compute the No,Nv
    int No(Vabij->lens[2]);
    int Nv(Vabij->lens[0]);

    // Symmetries used by intermediates
    int syms[] = { NS, NS, NS, NS };

    // Intermediates used both by T1 and T2
    int vv[] = { Nv, Nv };
    Tensor<> Kac(2, vv, syms, *Vabij->wrld, "Kac");
    int oo[] = { No, No };
    Tensor<> Kki(2, oo, syms, *Vabij->wrld, "Kki");

    //********************************************************************************
    //***********************  T2 amplitude equations  *******************************
    //********************************************************************************

    {
      LOG(1, abbreviation) << "Solving T2 Amplitude Equations" << std::endl;

      // Allocate Tensors for T2 amplitudes
      Tensor<> Rabij(false, *Vabij);
      Rabij.set_name("Rabij");

      if (i == 0) {
	// For first iteration compute only the MP2 amplitudes since Tabij = 0
	// Vabij is the only non-zero term
	Rabij["abij"] = (*Vabij)["abij"];
      } 
      else {
	// For the rest iterations compute the CCSD amplitudes
	{
	  // Intermediates used for T2 amplitudes
	  Tensor<> Lac(2, vv, syms, *Vabij->wrld, "Lac");
	  Tensor<> Lki(2, oo, syms, *Vabij->wrld, "Lki");

	  Tensor<> Xklij(false, *Vijkl);
	  Xklij.set_name("Xklij");
	  Tensor<> Xakci(false, *Vaibj);
	  Xakci.set_name("Xakci");
	  int voov[] = { Nv, No, No, Nv };
	  Tensor<> Xakic(4, voov, syms, *Vabij->wrld, "Xakic");

	  // Build Kac
	  Kac["ac"]  = -2.0 * (*Vabij)["cdkl"] * (*Tabij)["adkl"];
	  Kac["ac"] += (*Vabij)["dckl"] * (*Tabij)["adkl"];
	  Kac["ac"] -= 2.0 * (*Tai)["ak"] * (*Vabij)["cdkl"] * (*Tai)["dl"];
	  Kac["ac"] += (*Tai)["ak"] * (*Vabij)["dckl"] * (*Tai)["dl"];

	  // Build Lac
	  Lac["ac"]  = Kac["ac"];
	  Lac["ac"] += 2.0 * (*Vabci)["cdak"] * (*Tai)["dk"];
	  Lac["ac"] -= (*Vabci)["dcak"] * (*Tai)["dk"];

	  // Build Kki
	  Kki["ki"]  = 2.0 * (*Vabij)["cdkl"] * (*Tabij)["cdil"];
	  Kki["ki"] -= (*Vabij)["dckl"] * (*Tabij)["cdil"];
	  Kki["ki"] += 2.0 * (*Tai)["ci"] * (*Vabij)["cdkl"] * (*Tai)["dl"];
	  Kki["ki"] -= (*Tai)["ci"] * (*Vabij)["dckl"] * (*Tai)["dl"];

	  // Build Lki
	  Lki["ki"]  = Kki["ki"];
	  Lki["ki"] += 2.0 * (*Vijka)["klic"] * (*Tai)["cl"];
	  Lki["ki"] -= (*Vijka)["lkic"] * (*Tai)["cl"];
    
	  // Contract Lac with T2 Amplitudes
	  Rabij["abij"] = Lac["ac"] * (*Tabij)["cbij"];

	  // Contract Lki with T2 Amplitudes
	  Rabij["abij"] -= Lki["ki"] * (*Tabij)["abkj"];

	  // Contract Coulomb integrals with T2 amplitudes
	  Rabij["abij"] += (*Vabci)["baci"] * (*Tai)["cj"];
	  Rabij["abij"] -= (*Tai)["ak"] * (*Vaibj)["bkci"] * (*Tai)["cj"];
	  Rabij["abij"] -= (*Vijka)["jika"] * (*Tai)["bk"];
	  Rabij["abij"] += (*Tai)["cj"] * (*Vabij)["acik"] * (*Tai)["bk"];
	  
	  // Build Xakic
	  Xakic["akic"]  = (*Vabij)["acik"];
	  Xakic["akic"] -= (*Vijka)["lkic"] * (*Tai)["al"];
	  Xakic["akic"] += (*Vabci)["acdk"] * (*Tai)["di"];
	  Xakic["akic"] -= 0.5 * (*Vabij)["dclk"] * (*Tabij)["dail"];
	  Xakic["akic"] -= (*Tai)["al"] * (*Vabij)["dclk"] * (*Tai)["di"];
	  Xakic["akic"] += (*Vabij)["dclk"] * (*Tabij)["adil"];
	  Xakic["akic"] -= 0.5 * (*Vabij)["cdlk"] * (*Tabij)["adil"];

	  // Build Xakci
	  Xakci["akci"]  = (*Vaibj)["akci"];
	  Xakci["akci"] -= (*Vijka)["klic"] * (*Tai)["al"];
	  Xakci["akci"] += (*Vabci)["adck"] * (*Tai)["di"];
	  Xakci["akci"] -= 0.5 * (*Vabij)["cdlk"] * (*Tabij)["dail"];
	  Xakci["akci"] -= (*Tai)["al"] * (*Vabij)["cdlk"] * (*Tai)["di"];

	  // Contract Xakic and Xakci intermediates with T2 amplitudes Tabij
	  Rabij["abij"] += 2.0 * Xakic["akic"] * (*Tabij)["cbkj"];
	  Rabij["abij"] -= Xakic["akic"] * (*Tabij)["bckj"];

	  Rabij["abij"] -= Xakci["akci"] * (*Tabij)["cbkj"];
	  Rabij["abij"] -= Xakci["bkci"] * (*Tabij)["ackj"];

	  // Symmetrize Rabij by applying permutation operator
	  // to save memory we use Xakci as intermediate for the permutation operator 
	  Xakci["aibj"]  = Rabij["abij"];
	  Rabij["abij"] += Xakci["bjai"]; 

	  //////////////////////////////////////////////////////////////////////
	  // Now add all terms to Rabij that do not need to be symmetrized with
	  // the permutation operator
	  //////////////////////////////////////////////////////////////////////

	  // Rabij are the Tabij amplitudes for the next iteration and need to be build
	  Rabij["abij"] += (*Vabij)["abij"];

	  // Build Xklij intermediate
	  Xklij["klij"]  = (*Vijkl)["klij"];
	  Xklij["klij"] += (*Vijka)["klic"] * (*Tai)["cj"];
	  Xklij["klij"] += (*Vijka)["lkjc"] * (*Tai)["ci"];
	  Xklij["klij"] += (*Vabij)["cdkl"] * (*Tabij)["cdij"];
	  Xklij["klij"] += (*Tai)["ci"] * (*Vabij)["cdkl"] * (*Tai)["dj"]; 

	  // Contract Xklij with T2 Amplitudes
	  Rabij["abij"] += Xklij["klij"] * (*Tabij)["abkl"];

	  // Contract Xklij with T1 Amplitudes
	  Rabij["abij"] += (*Tai)["ak"] * Xklij["klij"] * (*Tai)["bl"];
	}

	if (Vabcd) {
	  // Build Xabcd intermediate
	  Tensor<> Xabcd(Vabcd);
	  Xabcd.set_name("Xabcd");
	  Xabcd["abcd"] -= (*Vabci)["cdak"] * (*Tai)["bk"];
	  Xabcd["abcd"] -= (*Vabci)["dcbk"] * (*Tai)["ak"];

	  // Construct intermediate tensor
	  Tensor<> Xabij(Tabij);
	  Xabij.set_name("Xabij");
	  Xabij["abij"] += (*Tai)["ai"] * (*Tai)["bj"];

	  // Contract Xabcd with T2 and T1 Amplitudes using Xabij
	  Rabij["abij"] += Xabcd["abcd"] * Xabij["cdij"];
	} 
	else {
	  // Slice if Vabcd is not specified

	  // Read the sliceRank. If not provided use No
	  int sliceRank(getIntegerArgument
			("sliceRank",No));

	  // Slice loop starts here
	  for (int b(0); b < Nv; b += sliceRank) {
	    for (int a(b); a < Nv; a += sliceRank) {
	      LOG(1, abbreviation) << "Evaluting Vabcd at a=" << a << ", b=" << b << std::endl;
	      // get the sliced integrals already coupled to the singles
	      Tensor<> *Xxycd(sliceCoupledCoulombIntegrals(a, b, sliceRank));
	      Xxycd->set_name("Xxycd");
	      int lens[] = { Xxycd->lens[0], Xxycd->lens[1], No, No };
	      int syms[] = {NS, NS, NS, NS};
	      Tensor<> Rxyij(4, lens, syms, *Xxycd->wrld, "Rxyij");

	      // Construct intermediate tensor
	      Tensor<> Xabij(Tabij);
	      Xabij.set_name("Xabij");
	      Xabij["abij"] += (*Tai)["ai"] * (*Tai)["bj"];

	      // Contract Xabcd with T2 and T1 Amplitudes using Xabij
	      Rxyij["xyij"] = (*Xxycd)["xycd"] * Xabij["cdij"];

	      //Rxyij["xyij"] =  (*Xxycd)["xycd"] * (*Tabij)["cdij"];
	      //Rxyij["xyij"] += (*Tai)["ci"] * (*Xxycd)["xycd"] * (*Tai)["dj"];

	      sliceIntoResiduum(Rxyij, a, b, Rabij);
	      // the integrals of this slice are not needed anymore
	      delete Xxycd;
	    }
	  }
	}
      }
      // calculate the amplitdues from the residuum
      doublesAmplitudesFromResiduum(Rabij);
      // and append them to the mixer
      TabijMixer->append(Rabij);
    }

    //********************************************************************************
    //***********************  T1 amplitude equations  *******************************
    //********************************************************************************
    {
      LOG(1, abbreviation) << "Solving T1 Amplitude Equations" << std::endl;
      
      // Allocate Tensors for T1 amplitudes
      Tensor<> Rai(false, *Tai);
      Rai.set_name("Rai");

      // Intermediates used for T1 amplitudes
      int vo[] = { Nv, No };
      Tensor<> Kck(2, vo, syms, *Vabij->wrld, "Kck");

      // Contract Kac and Kki with T1 amplitudes
      Rai["ai"]  = Kac["ac"] * (*Tai)["ci"];
      Rai["ai"] -= Kki["ki"] * (*Tai)["ak"];

      // Build Kck
      Kck["ck"]  = 2.0 * (*Vabij)["cdkl"] * (*Tai)["dl"];
      Kck["ck"] -= (*Vabij)["cdlk"] * (*Tai)["dl"];

      // Contract all the rest terms with T1 and T2 amplitudes
      Rai["ai"] += 2.0 * Kck["ck"] * (*Tabij)["caki"];
      Rai["ai"] -= Kck["ck"] * (*Tabij)["caik"];
      Rai["ai"] += (*Tai)["ci"] * Kck["ck"] * (*Tai)["ak"];
      Rai["ai"] += 2.0 * (*Vabij)["acik"] * (*Tai)["ck"];
      Rai["ai"] -= (*Vaibj)["akci"] * (*Tai)["ck"];
      Rai["ai"] += 2.0 * (*Vabci)["cdak"] * (*Tabij)["cdik"];
      Rai["ai"] -= (*Vabci)["dcak"] * (*Tabij)["cdik"];
      Rai["ai"] += 2.0 * (*Tai)["ci"] * (*Vabci)["cdak"] * (*Tai)["dk"];
      Rai["ai"] -= (*Tai)["ci"] * (*Vabci)["dcak"] * (*Tai)["dk"];
      Rai["ai"] -= 2.0 * (*Vijka)["klic"] * (*Tabij)["ackl"];
      Rai["ai"] += (*Vijka)["lkic"] * (*Tabij)["ackl"];
      Rai["ai"] -= 2.0 * (*Tai)["ak"] * (*Vijka)["klic"] * (*Tai)["cl"];
      Rai["ai"] += (*Tai)["ak"] * (*Vijka)["lkic"] * (*Tai)["cl"];

      singlesAmplitudesFromResiduum(Rai);
      TaiMixer->append(Rai);
    }
  }
}


void CcsdEnergyFromCoulombIntegrals::dryIterate() {
  {
    // TODO: the Mixer should provide a DryTensor in the future
    // Read the CCSD amplitudes Tai and Tabij
    DryTensor<> *Tai(getTensorArgument<double, 
    		     DryTensor<double>>("CcsdSinglesAmplitudes"));
    DryTensor<> *Tabij(getTensorArgument<double, 
		       DryTensor<double>>("CcsdDoublesAmplitudes"));

    // Read the Coulomb Integrals Vabcd Vabij Vaibj Vijkl
    // the PPPPCoulombIntegrals may not be given then slicing is required
    DryTensor<> *Vabcd(isArgumentGiven("PPPPCoulombIntegrals") ? getTensorArgument<double, 
		       DryTensor<double>>("PPPPCoulombIntegrals") : nullptr);
    DryTensor<> *Vabij(getTensorArgument<double, DryTensor<double>>("PPHHCoulombIntegrals"));
    DryTensor<> *Vaibj(getTensorArgument<double, DryTensor<double>>("PHPHCoulombIntegrals"));
    DryTensor<> *Vijkl(getTensorArgument<double, DryTensor<double>>("HHHHCoulombIntegrals"));
    getTensorArgument<double, DryTensor<double>>("PPPHCoulombIntegrals");
    getTensorArgument<double, DryTensor<double>>("HHHPCoulombIntegrals");

    // Read the Particle/Hole Eigenenergies epsi epsa
    DryTensor<> *epsi(getTensorArgument<double, DryTensor<double>>("HoleEigenEnergies"));
    DryTensor<> *epsa(getTensorArgument<double, DryTensor<double>>("ParticleEigenEnergies"));
  
    // Compute the no,nv,np
    int No(epsi->lens[0]);
    int Nv(epsa->lens[0]);

    // Symmetries used by intermediates
    int syms[] = { NS, NS, NS, NS };

    // Intermediates used both by T1 and T2
    int vv[] = { Nv, Nv };
    DryTensor<> Kac(2, vv, syms);
    int oo[] = { No, No };
    DryTensor<> Kki(2, oo, syms);

    {
      // Allocate Tensors for T2 amplitudes
      DryTensor<> Rabij(*Tabij);

      // Intermediates used for T2 amplitudes
      DryTensor<> Lac(2, vv, syms);
      DryTensor<> Lki(2, oo, syms);

      DryTensor<> Xklij(*Vijkl);
      DryTensor<> Xakci(*Vaibj);
      int voov[] = { Nv, No, No, Nv };
      DryTensor<> Xakic(4, voov, syms);
    }

    if (Vabcd) {
      // Build Xabcd intermediate
      DryTensor<> Xabcd(*Vabcd);

      // Construct intermediate tensor
      DryTensor<> Xabij(*Vabij);
    } 
    else {
      // Slice if Vabcd is not specified

      // Read the sliceRank. If not provided use No
      int sliceRank(getIntegerArgument
		    ("sliceRank",No));

      int lens[] = { sliceRank, sliceRank, Nv, Nv };
      int syms[] = {NS, NS, NS, NS};
      // TODO: implement drySliceCoulombIntegrals
      DryTensor<> Vxycd(4, lens, syms);
      DryTensor<> Rxyij(*Vijkl);
      // Construct intermediate tensor
      DryTensor<> Xabij(*Vabij);
    }
    // TODO: implment dryDoublesAmplitudesFromResiduum
    // at the moment, assume usage of Dabij
    DryTensor<> Dabij(*Vabij);

    {
      // Allocate Tensors for T1 amplitudes
      DryTensor<> Rai(*Tai);
    }
    // TODO: implment dryDoublesAmplitudesFromResiduum
    // at the moment, assume usage of Dabij
    DryTensor<> Dai(*Tai);
  }
}
