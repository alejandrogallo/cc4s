#include <algorithms/DcsdEnergyFromSlicedIntegrals.hpp>
#include <math/Complex.hpp>
#include <math/ComplexTensor.hpp>
#include <math/MathFunctions.hpp>
#include <util/DryTensor.hpp>
#include <util/Log.hpp>
#include <util/Exception.hpp>
#include <ctf.hpp>

using namespace CTF;
using namespace cc4s;

ALGORITHM_REGISTRAR_DEFINITION(DcsdEnergyFromSlicedIntegrals);

DcsdEnergyFromSlicedIntegrals::DcsdEnergyFromSlicedIntegrals(
  std::vector<Argument> const &argumentList
): ClusterSinglesDoublesAlgorithm(argumentList) {
}

DcsdEnergyFromSlicedIntegrals::~DcsdEnergyFromSlicedIntegrals() {
}

//////////////////////////////////////////////////////////////////////
// Hirata iteration routine for the DCSD amplitudes Tabij and Tai from
// So Hirata, et. al. Chem. Phys. Letters, 345, 475 (2001)
// modified to give DCSD amplitudes according to
// D. Kats, et. al., J. Chem. Phys. 142, 064111 (2015)
//////////////////////////////////////////////////////////////////////
void DcsdEnergyFromSlicedIntegrals::iterate(int i) {
  {
    // Read the amplitudes Tai and Tabij
    Tensor<> *Tai(&TaiMixer->getNext());
    Tai->set_name("Tai");
    Tensor<> *Tabij(&TabijMixer->getNext());
    Tabij->set_name("Tabij");

    // Read the Coulomb Integrals Vabcd Vabij Vaibj Vijkl Vabci Vijka
    Tensor<> *Vabij(getTensorArgument("PPHHCoulombIntegrals"));
    Tensor<> *Vaibj(getTensorArgument("PHPHCoulombIntegrals"));
    Tensor<> *Vijkl(getTensorArgument("HHHHCoulombIntegrals"));
    Tensor<> *Vijka(getTensorArgument("HHHPCoulombIntegrals"));
  
    // Compute the No,Nv
    int No(Vabij->lens[2]);
    int Nv(Vabij->lens[0]);

    // Get abbreviation of algorithm
    std::string abbreviation(getAbbreviation());
    std::transform(abbreviation.begin(), abbreviation.end(), 
		   abbreviation.begin(), ::toupper);

    // Read the Coulomb vertex GammaGpq
    Tensor<complex> *GammaGpq( getTensorArgument<complex>("CoulombVertex"));
    int NG(GammaGpq->lens[0]);
    int Np = No + Nv;

    // Allocate and compute GammaGab,GammaGai,GammaGij from GammaGpq
    int GaiStart[] = {0 ,No, 0};
    int GaiEnd[]   = {NG,Np,No};
    int GabStart[] = {0 ,No,No};
    int GabEnd[]   = {NG,Np,Np};
    int GijStart[] = {0 , 0, 0};
    int GijEnd[]   = {NG,No,No};
    Tensor<complex> GammaGai(GammaGpq->slice(GaiStart,GaiEnd));
    Tensor<complex> GammaGab(GammaGpq->slice(GabStart,GabEnd));
    Tensor<complex> GammaGij(GammaGpq->slice(GijStart,GijEnd));

    // Split GammaGab,GammaGai,GammaGia,GammaGij into real and imaginary parts
    Tensor<> realGammaGai(3, GammaGai.lens, GammaGai.sym, 
			  *GammaGai.wrld, "RealGammaGai");
    Tensor<> imagGammaGai(3, GammaGai.lens, GammaGai.sym, 
			  *GammaGai.wrld, "ImagGammaGai");
    fromComplexTensor(GammaGai, realGammaGai, imagGammaGai);

    Tensor<> realGammaGab(3, GammaGab.lens, GammaGab.sym, 
			  *GammaGab.wrld, "RealGammaGab");
    Tensor<> imagGammaGab(3, GammaGab.lens, GammaGab.sym, 
			  *GammaGab.wrld, "ImagGammaGab");
    fromComplexTensor(GammaGab, realGammaGab, imagGammaGab);

    Tensor<> realGammaGij(3, GammaGij.lens, GammaGij.sym, 
			  *GammaGij.wrld, "RealGammaGij");
    Tensor<> imagGammaGij(3, GammaGij.lens, GammaGij.sym, 
			  *GammaGij.wrld, "ImagGammaGij");
    fromComplexTensor(GammaGij, realGammaGij, imagGammaGij);

    // Symmetries used by intermediates
    int syms[] = { NS, NS, NS, NS };

    //********************************************************************************
    //***********************  T2 amplitude equations  *******************************
    //********************************************************************************

    {
      LOG(1, abbreviation) << "Solving T2 Amplitude Equations" << std::endl;

      // Allocate Tensors for T2 amplitudes
      Tensor<> Rabij(false, *Vabij);
      Rabij.set_name("Rabij");

      if (i == 0) {
	// For first iteration compute only the MP2 amplitudes 
	// Since Tabij = 0, Vabij is the only non-zero term
	Rabij["abij"] = (*Vabij)["abij"];
      } 
      else {
	// For the rest iterations compute the DCSD amplitudes

	// Intermediate tensor Xabij=T2+T1*T1
	Tensor<> Xabij(Tabij);
	Xabij.set_name("Xabij");
	Xabij["abij"] += (*Tai)["ai"] * (*Tai)["bj"];

	{
	  // Intermediates used for T2 amplitudes
	  int vv[] = { Nv, Nv };
	  Tensor<> Lac(2, vv, syms, *Vabij->wrld, "Lac");
	  int oo[] = { No, No };
	  Tensor<> Lki(2, oo, syms, *Vabij->wrld, "Lki");

	  Tensor<> Xklij(false, *Vijkl);
	  Xklij.set_name("Xklij");
	  Tensor<> Xakci(false, *Vaibj);
	  Xakci.set_name("Xakci");
	  int voov[] = { Nv, No, No, Nv };
	  Tensor<> Xakic(4, voov, syms, *Vabij->wrld, "Xakic");

	  // Intermediate tensor Yabij=T2-2*T1*T1
	  Tensor<> Yabij(Tabij);
	  Yabij.set_name("Yabij");
	  Yabij["abij"] += ( 2.0) * (*Tai)["ai"] * (*Tai)["bj"];

	  // Build Lac
	  Lac["ac"]      = (-1.0) * (*Vabij)["cdkl"] * Yabij["adkl"]; // Use Yabij in DCSD
	  Lac["ac"]     += ( 0.5) * (*Vabij)["dckl"] * Yabij["adkl"]; // Use Yabij in DCSD
	  Lac["ac"] += ( 2.0) * realGammaGab["Gca"] * realGammaGai["Gdk"] * (*Tai)["dk"];
	  Lac["ac"] += ( 2.0) * imagGammaGab["Gca"] * imagGammaGai["Gdk"] * (*Tai)["dk"];
	  Lac["ac"] += (-1.0) * realGammaGai["Gck"] * realGammaGab["Gda"] * (*Tai)["dk"];
	  Lac["ac"] += (-1.0) * imagGammaGai["Gck"] * imagGammaGab["Gda"] * (*Tai)["dk"];

	  // Build Lki
	  Lki["ki"]  = ( 1.0) * (*Vabij)["cdkl"] * Yabij["cdil"]; // Use Yabij in DCSD
	  Lki["ki"] += (-0.5) * (*Vabij)["dckl"] * Yabij["cdil"]; // Use Yabij in DCSD
	  Lki["ki"] += ( 2.0) * (*Vijka)["klic"] * (*Tai)["cl"];
	  Lki["ki"] += (-1.0) * (*Vijka)["lkic"] * (*Tai)["cl"];
    
	  // Contract Lac with T2 Amplitudes
	  Rabij["abij"]  = ( 1.0) * Lac["ac"] * (*Tabij)["cbij"];

	  // Contract Lki with T2 Amplitudes
	  Rabij["abij"] += (-1.0) * Lki["ki"] * (*Tabij)["abkj"];

	  // Contract Coulomb integrals with T2 amplitudes
	  {
	    Tensor<> realDressedGammaGai(realGammaGai);
	    Tensor<> imagDressedGammaGai(imagGammaGai);
	    realDressedGammaGai.set_name("realDressedGammaGai");
	    imagDressedGammaGai.set_name("imagDressedGammaGai");

	    realDressedGammaGai["Gai"] += (-1.0) * realGammaGij["Gki"] * (*Tai)["ak"];
	    imagDressedGammaGai["Gai"] += (-1.0) * imagGammaGij["Gki"] * (*Tai)["ak"];

	    Rabij["abij"] += ( 1.0) * realDressedGammaGai["Gai"] * realGammaGab["Gbc"] * (*Tai)["cj"];
	    Rabij["abij"] += ( 1.0) * imagDressedGammaGai["Gai"] * imagGammaGab["Gbc"] * (*Tai)["cj"];
	  }
	  Rabij["abij"] += (-1.0) * (*Vijka)["jika"] * (*Tai)["bk"];
	  Rabij["abij"] += ( 1.0) * (*Tai)["bk"] * (*Vabij)["acik"] * (*Tai)["cj"];
	  
	  // Build Xakic
	  {
	    Tensor<> realDressedGammaGai(realGammaGai);
	    Tensor<> imagDressedGammaGai(imagGammaGai);
	    realDressedGammaGai.set_name("realDressedGammaGai");
	    imagDressedGammaGai.set_name("imagDressedGammaGai");

	    realDressedGammaGai["Gai"] += (-1.0) * realGammaGij["Gil"] * (*Tai)["al"];
	    imagDressedGammaGai["Gai"] += (-1.0) * imagGammaGij["Gil"] * (*Tai)["al"];

	    realDressedGammaGai["Gai"] += ( 1.0) * realGammaGab["Gad"] * (*Tai)["di"];
	    imagDressedGammaGai["Gai"] += ( 1.0) * imagGammaGab["Gad"] * (*Tai)["di"];

	    Xakic["akic"]  = ( 1.0) * realDressedGammaGai["Gai"] * realGammaGai["Gck"];
	    Xakic["akic"] += ( 1.0) * imagDressedGammaGai["Gai"] * imagGammaGai["Gck"];

	    Xakic["akic"] += (-0.5) * (*Vabij)["dclk"] *   Yabij ["dail"]; // Use Yabij in DCSD
	    Xakic["akic"] += ( 1.0) * (*Vabij)["dclk"] * (*Tabij)["adil"];
	    //	    Xakic["akic"] += (-0.5) * (*Vabij)["cdlk"] * (*Tabij)["adil"]; // Removed in DCSD
	  }

	  // Build Xakci
	  {
	    // Construct dressed Coulomb vertex GammaGab and GammaGij
	    Tensor<> realDressedGammaGab(realGammaGab);
	    Tensor<> imagDressedGammaGab(imagGammaGab);
	    realDressedGammaGab.set_name("realDressedGammaGab");
	    imagDressedGammaGab.set_name("imagDressedGammaGab");

	    Tensor<> realDressedGammaGij(realGammaGij);
	    Tensor<> imagDressedGammaGij(imagGammaGij);
	    realDressedGammaGij.set_name("realDressedGammaGij");
	    imagDressedGammaGij.set_name("imagDressedGammaGij");

	    realDressedGammaGab["Gac"] += (-1.0) * realGammaGai["Gcl"] * (*Tai)["al"];
	    imagDressedGammaGab["Gac"] += (-1.0) * imagGammaGai["Gcl"] * (*Tai)["al"];

	    realDressedGammaGij["Gki"] += ( 1.0) * realGammaGai["Gdk"] * (*Tai)["di"];
	    imagDressedGammaGij["Gki"] += ( 1.0) * imagGammaGai["Gdk"] * (*Tai)["di"];
	    
	    // Xakci = Vakci - Vlkci * Tal + Vakcd * Tdi - Vcdlk * Tdail
	    Xakci["akci"]  = ( 1.0) * realDressedGammaGab["Gac"] * realDressedGammaGij["Gki"];
	    Xakci["akci"] += ( 1.0) * imagDressedGammaGab["Gac"] * imagDressedGammaGij["Gki"];

	    // Xakci = 0.5 * Vcdlk * Tdail
	    //	    Xakci["akci"] += (-0.5) * (*Vabij)["cdlk"] * (*Tabij)["dail"]; // Removed in DCSD
	  }

	  // Contract Xakic and Xakci intermediates with T2 amplitudes Tabij
	  Rabij["abij"] += ( 2.0) * Xakic["akic"] * (*Tabij)["cbkj"];
	  Rabij["abij"] += (-1.0) * Xakic["akic"] * (*Tabij)["bckj"];

	  Rabij["abij"] += (-1.0) * Xakci["akci"] * (*Tabij)["cbkj"];
	  Rabij["abij"] += (-1.0) * Xakci["bkci"] * (*Tabij)["ackj"];

	  // Symmetrize Rabij by applying permutation operator
	  // to save memory we use Xakci as intermediate for the permutation operator 
	  Xakci["aibj"]  = Rabij["abij"];
	  Rabij["abij"] += Xakci["bjai"]; 

	  //////////////////////////////////////////////////////////////////////
	  // Now add all terms to Rabij that do not need to be symmetrized with
	  // the permutation operator
	  //////////////////////////////////////////////////////////////////////

	  // Add Vabij to Rabij (MP2 term)
	  Rabij["abij"] += (*Vabij)["abij"];

	  {
	    // Build Xklij intermediate
	    Xklij["klij"]  = (*Vijkl)["klij"];
	    Xklij["klij"] += (*Vijka)["klic"] * (*Tai)["cj"];
	    Xklij["klij"] += (*Vijka)["lkjc"] * (*Tai)["ci"];

	    // Contract Xklij with T2+T1*T1 Amplitudes via Xabij
	    Rabij["abij"] +=  Xklij["klij"] * Xabij["abkl"];

	    // Construct last term
	    Xklij["klij"]  =  (*Tai)["dj"] * (*Vabij)["cdkl"] * (*Tai)["ci"];

	    // Add last term contracted only with the doubles
	    // The singles term is computed in the slicing
	    Rabij["abij"] +=  Xklij["klij"] * (*Tabij)["abkl"];
	  }

	}

	{
	  // Read the sliceRank. If not provided use No
	  int sliceRank(getIntegerArgument
			("sliceRank",No));

	  // Slice loop starts here
	  for (int b(0); b < Nv; b += sliceRank) {
	    for (int a(b); a < Nv; a += sliceRank) {
	      LOG(1, abbreviation) << "Evaluting Vabcd at a=" << a << ", b=" << b << std::endl;
	      Tensor<> *Vxycd(sliceCoupledCoulombIntegrals(a, b, sliceRank));
	      Vxycd->set_name("Vxycd");
	      int lens[] = { Vxycd->lens[0], Vxycd->lens[1], No, No };
	      int syms[] = {NS, NS, NS, NS};
	      Tensor<> Rxyij(4, lens, syms, *Vxycd->wrld, "Rxyij");

	      // Contract sliced Vxycd with T2 and T1 Amplitudes using Xabij
	      Rxyij["xyij"] = (*Vxycd)["xycd"] * Xabij["cdij"];

	      sliceIntoResiduum(Rxyij, a, b, Rabij);
	      // The integrals of this slice are not needed anymore
	      delete Vxycd;
	    }
	  }
	}
      }
      // Calculate the amplitudes from the residuum
      doublesAmplitudesFromResiduum(Rabij);
      // Append amplitudes to the mixer
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

      // Intermediates used for T1 amplitudes
      int vv[] = { Nv, Nv };
      Tensor<> Kac(2, vv, syms, *Vabij->wrld, "Kac");
      int oo[] = { No, No };
      Tensor<> Kki(2, oo, syms, *Vabij->wrld, "Kki");

      // Intermediate tensor Xabij=T2+T1*T1
      Tensor<> Xabij(Tabij);
      Xabij.set_name("Xabij");
      Xabij["abij"] += (*Tai)["ai"] * (*Tai)["bj"];

      // Build Kac
      Kac["ac"]  = (-2.0) * (*Vabij)["cdkl"] * Xabij["adkl"];
      Kac["ac"] += ( 1.0) * (*Vabij)["dckl"] * Xabij["adkl"];

      // Build Kki
      Kki["ki"]  = ( 2.0) * (*Vabij)["cdkl"] * Xabij["cdil"];
      Kki["ki"] += (-1.0) * (*Vabij)["dckl"] * Xabij["cdil"];

      // Contract Kac and Kki with T1 amplitudes
      Rai["ai"]  = ( 1.0) * Kac["ac"] * (*Tai)["ci"];
      Rai["ai"] += (-1.0) * Kki["ki"] * (*Tai)["ak"];

      // Build Kck
      Kck["ck"]  = ( 2.0) * (*Vabij)["cdkl"] * (*Tai)["dl"];
      Kck["ck"] += (-1.0) * (*Vabij)["dckl"] * (*Tai)["dl"];

      // Contract all the rest terms with T1 and T2 amplitudes
      Rai["ai"] += ( 2.0) * Kck["ck"] * (*Tabij)["caki"];
      Rai["ai"] += (-1.0) * Kck["ck"] * (*Tabij)["caik"];
      Rai["ai"] += ( 1.0) * (*Tai)["ak"] * Kck["ck"] * (*Tai)["ci"];
      Rai["ai"] += ( 2.0) * (*Vabij)["acik"] * (*Tai)["ck"];
      Rai["ai"] += (-1.0) * (*Vaibj)["ciak"] * (*Tai)["ck"];
      Rai["ai"] += ( 2.0) * realGammaGab["Gca"] * realGammaGai["Gdk"] * Xabij["cdik"];
      Rai["ai"] += ( 2.0) * imagGammaGab["Gca"] * imagGammaGai["Gdk"] * Xabij["cdik"];
      Rai["ai"] += (-1.0) * realGammaGab["Gda"] * realGammaGai["Gck"] * Xabij["cdik"];
      Rai["ai"] += (-1.0) * imagGammaGab["Gda"] * imagGammaGai["Gck"] * Xabij["cdik"];
      Rai["ai"] += (-2.0) * (*Vijka)["klic"] * Xabij["ackl"];
      Rai["ai"] += ( 1.0) * (*Vijka)["lkic"] * Xabij["ackl"];

      singlesAmplitudesFromResiduum(Rai);
      TaiMixer->append(Rai);
    }
  }
}


void DcsdEnergyFromSlicedIntegrals::dryIterate() {
  {
    // TODO: the Mixer should provide a DryTensor in the future
    // Read the DCSD amplitudes Tai and Tabij
    DryTensor<> *Tai(getTensorArgument<double, 
    		     DryTensor<double>>("DcsdSinglesAmplitudes"));
    DryTensor<> *Tabij(getTensorArgument<double, 
		       DryTensor<double>>("DcsdDoublesAmplitudes"));

    // Read the Coulomb Integrals Vabcd Vabij Vaibj Vijkl Vijka Vabci
    DryTensor<> *Vabij(getTensorArgument<double, DryTensor<double>>("PPHHCoulombIntegrals"));
    DryTensor<> *Vaibj(getTensorArgument<double, DryTensor<double>>("PHPHCoulombIntegrals"));
    DryTensor<> *Vijkl(getTensorArgument<double, DryTensor<double>>("HHHHCoulombIntegrals"));
    getTensorArgument<double, DryTensor<double>>("HHHPCoulombIntegrals");

    // Read the Particle/Hole Eigenenergies epsi epsa
    DryTensor<> *epsi(getTensorArgument<double, DryTensor<double>>("HoleEigenEnergies"));
    DryTensor<> *epsa(getTensorArgument<double, DryTensor<double>>("ParticleEigenEnergies"));
  
    // Compute the No,Nv,Np
    int No(epsi->lens[0]);
    int Nv(epsa->lens[0]);

    // Symmetries used by intermediates
    int syms[] = { NS, NS, NS, NS };

    // Read the Coulomb vertex GammaGpq
    DryTensor<complex> *GammaGpq(getTensorArgument<complex, 
				 DryTensor<complex>>("CoulombVertex"));

    // Compute the NG,Np
    int NG(GammaGpq->lens[0]);

    // Allocate and compute GammaGab,GammaGai,GammaGij from GammaGpq
    int GaiLens[]   = {NG,Nv,No};
    int GabLens[]   = {NG,Nv,Nv};

    DryTensor<complex> GammaGai(3, GaiLens, syms);
    DryTensor<complex> GammaGab(3, GabLens, syms);

    // Split GammaGab,GammaGai into real and imaginary parts
    DryTensor<> realGammaGai(3, GaiLens, syms);
    DryTensor<> imagGammaGai(3, GaiLens, syms);

    DryTensor<> realGammaGab(3, GabLens, syms);
    DryTensor<> imagGammaGab(3, GabLens, syms);

    // Intermediates used both by T1 and T2
    int vv[] = { Nv, Nv };
    DryTensor<> Kac(2, vv, syms);
    int oo[] = { No, No };
    DryTensor<> Kki(2, oo, syms);

    // Construct intermediate tensor X=T2+T1*T1
    DryTensor<> Xabij(*Vabij);

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

    {
      // Read the sliceRank. If not provided use No
      int sliceRank(getIntegerArgument
		    ("sliceRank",No));

      DryTensor<> *Vxycd(drySliceCoupledCoulombIntegrals(sliceRank));
      int lens[] = { Vxycd->lens[0], Vxycd->lens[1], No, No };
      int syms[] = {NS, NS, NS, NS};
      DryTensor<> Rxyij(4, lens, syms);
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