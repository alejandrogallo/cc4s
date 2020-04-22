#include <algorithms/CoulombIntegralsFromVertex.hpp>

#include <math/Complex.hpp>
#include <math/MathFunctions.hpp>
#include <util/Log.hpp>
#include <util/Exception.hpp>
#include <tcc/Tcc.hpp>
#include <Cc4s.hpp>

using namespace cc4s;

ALGORITHM_REGISTRAR_DEFINITION(CoulombIntegralsFromVertex);

template <typename F, typename TE>
Ptr<MapNode> createTensorNode(const Ptr<Tensor<F,TE>> &tensorData) {
  auto tensor(New<MapNode>());
  auto dimensions(New<MapNode>());
  for (auto len: tensorData->lens) {
    dimensions->push_back(New<AtomicNode<size_t>>(len));
  }
  tensor->get("dimensions") = dimensions;
  tensor->setValue<std::string>("scalarType", TypeTraits<F>::getName());
  tensor->setValue<Ptr<Tensor<F,TE>>>("data", tensorData);
  return tensor;
}

Ptr<MapNode> CoulombIntegralsFromVertex::run(const Ptr<MapNode> &arguments) {
  auto slicedCoulombVertex(arguments->getMap("slicedCoulombVertex"));
  auto orbitals(slicedCoulombVertex->getValue<std::string>("orbitals"));

  // multiplex calls to template methods
  if (orbitals == "real") {
    if (Cc4s::options->dryRun) {
      return calculateRealIntegrals<DryTensorEngine>(arguments);
    } else {
      return calculateRealIntegrals<DefaultTensorEngine>(arguments);
    }
  } else if (orbitals == "complex") {
    if (Cc4s::options->dryRun) {
      return calculateComplexIntegrals<DryTensorEngine>(arguments);
    } else {
      return calculateComplexIntegrals<DefaultTensorEngine>(arguments);
    }
  } else {
    Assert(
      false, "'orbitals' must specify either 'real' or 'complex' orbitals"
    );
  }
}

template <typename TE>
Ptr<MapNode> CoulombIntegralsFromVertex::calculateRealIntegrals(
  const Ptr<MapNode> &arguments
) {
  auto coulombVertex(arguments->getMap("slicedCoulombVertex"));
  auto slices(coulombVertex->getMap("slices"));
  // get input recipes
  auto GammaGab(slices->getValue<Ptr<TensorRecipe<Complex<>,TE>>>("Gab"));
  auto GammaGai(slices->getValue<Ptr<TensorRecipe<Complex<>,TE>>>("Gai"));
  auto GammaGia(slices->getValue<Ptr<TensorRecipe<Complex<>,TE>>>("Gia"));
  auto GammaGij(slices->getValue<Ptr<TensorRecipe<Complex<>,TE>>>("Gij"));

#define DEFINE_VERTEX_PART(PART, SLICE) \
  Ptr<TensorRecipe<Real<>,TE>> PART##Gamma##SLICE; \
  { \
    auto result(Tcc<TE>::template tensor<Real<>>( \
      std::string(#PART) + "Gamma" + #SLICE) \
    ); \
    PART##Gamma##SLICE = ( \
      (*result)[#SLICE] <<= map(PART<Complex<>>, (*Gamma##SLICE)[#SLICE]) \
    )->compileRecipe(result); \
  }
  // define intermediate recipes
  DEFINE_VERTEX_PART(real, Gab)
  DEFINE_VERTEX_PART(real, Gai)
  DEFINE_VERTEX_PART(real, Gia)
  DEFINE_VERTEX_PART(real, Gij)
  DEFINE_VERTEX_PART(imag, Gab)
  DEFINE_VERTEX_PART(imag, Gai)
  DEFINE_VERTEX_PART(imag, Gia)
  DEFINE_VERTEX_PART(imag, Gij)

#define DEFINE_INTEGRALS_SLICE(SLICE, LSLICE, LIDX, RSLICE, RIDX) \
  { \
    auto result(Tcc<TE>::template tensor<Real<>>(std::string("V") + #SLICE)); \
    integralSlices->setValue<Ptr<TensorRecipe<Real<>,TE>>>(#SLICE, \
      ( \
        (*result)[#SLICE] <<= \
          (*realGamma##LSLICE)[#LIDX] * (*realGamma##RSLICE)[#RIDX], \
        (*result)[#SLICE] += \
          (*imagGamma##LSLICE)[#LIDX] * (*imagGamma##RSLICE)[#RIDX] \
      )->compileRecipe(result) \
    ); \
  }
  // define recipes for integral slices
  auto integralSlices(New<MapNode>());
  DEFINE_INTEGRALS_SLICE(aijk, Gai, Gaj, Gij, Gik);
  DEFINE_INTEGRALS_SLICE(abij, Gai, Gai, Gai, Gbj);

  // create result
  auto coulombIntegrals(New<MapNode>());
  coulombIntegrals->get("slices") = integralSlices;
  coulombIntegrals->setValue<Real<>>(
    "unit", pow(coulombVertex->getValue<Real<>>("unit"),2.0)
  );
  coulombIntegrals->get("spins") = coulombVertex->get("spins");
  coulombIntegrals->get("orbitals") = coulombVertex->get("orbitals");
  auto result(New<MapNode>());
  result->get("coulombIntegrals") = coulombIntegrals;
  return result;
}

template <typename TE>
Ptr<MapNode> CoulombIntegralsFromVertex::calculateComplexIntegrals(
  const Ptr<MapNode> &vertex
) {
  // TODO: implement
  return New<MapNode>();
}

/*
void CoulombIntegralsFromVertex::run() {
  // Read the Coulomb vertex GammaGqr
  Tensor<complex> *GammaGqr( getTensorArgument<complex>("CoulombVertex"));

  // Read the Particle/Hole Eigenenergies
  Tensor<> *epsi(getTensorArgument<>("HoleEigenEnergies"));
  Tensor<> *epsa(getTensorArgument<>("ParticleEigenEnergies"));

  LOG(0, "Integrals") <<
    "Reading Coulomb integrals form vertex " << GammaGqr->get_name()
    << std::endl;

  // Compute the No,Nv,NG,Np
  int NG(GammaGqr->lens[0]);
  int No(epsi->lens[0]);
  int Nv(epsa->lens[0]);
  int Np(GammaGqr->lens[1]);
  EMIT()
    << YAML::Key << "NG" << YAML::Value << NG
    << YAML::Key << "No" << YAML::Value << No
    << YAML::Key << "Nv" << YAML::Value << Nv
    << YAML::Key << "Np" << YAML::Value << Np;

  std::vector<std::string> list;
  if (isArgumentGiven("PHPHCoulombIntegrals")) { list.push_back("PHPHCoulombIntegrals");}
  if (isArgumentGiven("PPHHCoulombIntegrals")) { list.push_back("PPHHCoulombIntegrals");}
  if (isArgumentGiven("HHHHCoulombIntegrals")) { list.push_back("HHHHCoulombIntegrals");}
  if (isArgumentGiven("HHHPCoulombIntegrals")) { list.push_back("HHHPCoulombIntegrals");}
  if (isArgumentGiven("PPPPCoulombIntegrals")) { list.push_back("PPPPCoulombIntegrals");}
  if (isArgumentGiven("PPPHCoulombIntegrals")) { list.push_back("PPPHCoulombIntegrals");}
  if (isArgumentGiven("PHHHCoulombIntegrals")) { list.push_back("PHHHCoulombIntegrals");}
  if (isArgumentGiven("HHPPCoulombIntegrals")) { list.push_back("HHPPCoulombIntegrals");}
  if (isArgumentGiven("PHHPCoulombIntegrals")) { list.push_back("PHHPCoulombIntegrals");}
  if (isArgumentGiven("HPHHCoulombIntegrals")) { list.push_back("HPHHCoulombIntegrals");}
  if (isArgumentGiven("HPHPCoulombIntegrals")) { list.push_back("HPHPCoulombIntegrals");}
  if (isArgumentGiven("HPPPCoulombIntegrals")) { list.push_back("HPPPCoulombIntegrals");}
  if (isArgumentGiven("PPHPCoulombIntegrals")) { list.push_back("PPHPCoulombIntegrals");}
  if (isArgumentGiven("HPPHCoulombIntegrals")) { list.push_back("HPPHCoulombIntegrals");}
  if (isArgumentGiven("HHPHCoulombIntegrals")) { list.push_back("HHPHCoulombIntegrals");}
  if (isArgumentGiven("PHPPCoulombIntegrals")) { list.push_back("PHPPCoulombIntegrals");}
  EMIT() << YAML::Key << "integrals" << YAML::Value;
  EMIT() << YAML::Flow << YAML::BeginSeq;
  for ( size_t i(0); i < list.size(); i++){
    EMIT() << list[i];
  }
  EMIT() << YAML::EndSeq;

  // Allocate coulomb integrals Vabij Vaibj Vaijb Vijkl Vabcd
  // TODO: calculate vvvv, vvvo  for COMPLEX
  syms = std::array<int,4>{{ NS, NS, NS, NS }};
  vvvv = std::array<int,4>{{ Nv, Nv, Nv, Nv }};
  vovo = std::array<int,4>{{ Nv, No, Nv, No }};
  vvoo = std::array<int,4>{{ Nv, Nv, No, No }};
  oooo = std::array<int,4>{{ No, No, No, No }};
  ooov = std::array<int,4>{{ No, No, No, Nv }};
  vvvo = std::array<int,4>{{ Nv, Nv, Nv, No }};
  vooo = std::array<int,4>{{ Nv, No, No, No }};
  voov = std::array<int,4>{{ Nv, No, No, Nv }};
  oovv = std::array<int,4>{{ No, No, Nv, Nv }};
  ovoo = std::array<int,4>{{ No, Nv, No, No }};
  ovov = std::array<int,4>{{ No, Nv, No, Nv }};
  ovvv = std::array<int,4>{{ No, Nv, Nv, Nv }};
  vvov = std::array<int,4>{{ Nv, Nv, No, Nv }};
  ovvo = std::array<int,4>{{ No, Nv, Nv, No }};
  oovo = std::array<int,4>{{ No, No, Nv, No }};
  vovv = std::array<int,4>{{ Nv, No, Nv, Nv }};

  bool realIntegrals = !getIntegerArgument("complex", 0);
  LOG(0, "CoulombIntegrals") << "Using "
    << (realIntegrals ? "real" : "complex") << " Coulomb integrals"
    << std::endl;

  int aStart(Np-Nv), aEnd(Np);
  int iStart(0), iEnd(No);
  int GijStart[] = {0, iStart,iStart};
  int GijEnd[]   = {NG,iEnd,  iEnd};
  int GiaStart[] = {0, iStart,aStart};
  int GiaEnd[]   = {NG,iEnd,  aEnd};
  int GaiStart[] = {0, aStart,iStart};
  int GaiEnd[]   = {NG,aEnd,  iEnd};
  int GabStart[] = {0, aStart,aStart};
  int GabEnd[]   = {NG,aEnd,  aEnd};
  GammaGij = new Tensor<complex>(GammaGqr->slice(GijStart, GijEnd));
  GammaGia = realIntegrals ?
    nullptr : new Tensor<complex>(GammaGqr->slice(GiaStart, GiaEnd));
  GammaGai = new Tensor<complex>(GammaGqr->slice(GaiStart, GaiEnd));
  GammaGab = new Tensor<complex>(GammaGqr->slice(GabStart, GabEnd));

  if (realIntegrals) {
    calculateRealIntegrals();
  } else {
    calculateComplexIntegrals();
  }

  delete GammaGij;
  delete GammaGai;
  delete GammaGab;
  if (GammaGia) {
    delete GammaGia;
  }

}

void CoulombIntegralsFromVertex::dryRun() {
  // Read the Coulomb vertex GammaGqr
  DryTensor<complex> *GammaGqr(
    getTensorArgument<complex,DryTensor<complex>>("CoulombVertex")
  );

  // Read the Particle/Hole Eigenenergies
  DryTensor<> *epsi(
    getTensorArgument<double,DryTensor<double>>("HoleEigenEnergies")
  );
  DryTensor<> *epsa(
    getTensorArgument<double,DryTensor<double>>("ParticleEigenEnergies")
  );

  LOG(0, "Integrals") <<
    "Reading Coulomb integrals form vertex " << GammaGqr->get_name()
    << std::endl;

  // Compute the No,Nv,NG,Np
  int NG(GammaGqr->lens[0]);
  int No(epsi->lens[0]);
  int Nv(epsa->lens[0]);

  // Allocate coulomb integrals Vabij Vaibj Vaijb Vijkl Vabcd
  syms = std::array<int,4>{{ NS, NS, NS, NS }};
  vvvv = std::array<int,4>{{ Nv, Nv, Nv, Nv }};
  vovo = std::array<int,4>{{ Nv, No, Nv, No }};
  vvoo = std::array<int,4>{{ Nv, Nv, No, No }};
  voov = std::array<int,4>{{ Nv, No, No, Nv }};
  oovv = std::array<int,4>{{ No, No, Nv, Nv }};
  oooo = std::array<int,4>{{ No, No, No, No }};
  ooov = std::array<int,4>{{ No, No, No, Nv }};
  vooo = std::array<int,4>{{ Nv, No, No, No }};
  vvvo = std::array<int,4>{{ Nv, Nv, Nv, No }};

  bool realIntegrals = !getIntegerArgument("complex", 0);
  LOG(0, "CoulombIntegrals") << "Using "
    << (realIntegrals ? "real" : "complex") << " Coulomb integrals"
    << std::endl;

  // Allocate and compute GammaGab,GammaGai,GammaGij from GammaGqr
  int GaiLens[]   = {NG,Nv,No};
  int GiaLens[]   = {NG,No,Nv};
  int GabLens[]   = {NG,Nv,Nv};
  int GijLens[]   = {NG,No,No};

  DryTensor<complex> GammaGia(3, std::vector<size_t>(GiaLens, GiaLens+3).data(), syms.data());
  DryTensor<complex> GammaGai(3, std::vector<size_t>(GaiLens, GaiLens+3).data(), syms.data());
  DryTensor<complex> GammaGab(3, std::vector<size_t>(GabLens, GabLens+3).data(), syms.data());
  DryTensor<complex> GammaGij(3, std::vector<size_t>(GijLens, GijLens+3).data(), syms.data());

  if (realIntegrals) {
    dryCalculateRealIntegrals();
  } else {
    dryCalculateComplexIntegrals();
  }
}

void CoulombIntegralsFromVertex::calculateRealIntegrals() {
  int antisymmetrize(getIntegerArgument("antisymmetrize", 0));
  if (antisymmetrize) {
    LOG(0, "CoulombIntegrals") << "Calculating antisymmetrized integrals"
      << std::endl;
  }
  Tensor<> *Vaibj(
    isArgumentGiven("PHPHCoulombIntegrals") ?
    new Tensor<>(4, vovo.data(), syms.data(), *Cc4s::world, "Vaibj") : nullptr
  );
  Tensor<> *Vabij(
    isArgumentGiven("PPHHCoulombIntegrals") ?
    new Tensor<>(4, vvoo.data(), syms.data(), *Cc4s::world, "Vabij") : nullptr
  );
  Tensor<> *Vijkl(
    isArgumentGiven("HHHHCoulombIntegrals") ?
    new Tensor<>(4, oooo.data(), syms.data(), *Cc4s::world, "Vijkl") : nullptr
  );
  Tensor<> *Vijka(
    isArgumentGiven("HHHPCoulombIntegrals") ?
    new Tensor<>(4, ooov.data(), syms.data(), *Cc4s::world, "Vijka") : nullptr
  );
  Tensor<> *Vabcd(
    isArgumentGiven("PPPPCoulombIntegrals") ?
    new Tensor<>(4, vvvv.data(), syms.data(), *Cc4s::world, "Vabcd") : nullptr
  );
  Tensor<> *Vabci(
    isArgumentGiven("PPPHCoulombIntegrals") ?
    new Tensor<>(4, vvvo.data(), syms.data(), *Cc4s::world, "Vabci") : nullptr
  );


  // Initialization of tensors created from already existing ones
  Tensor<> *Vaijk(
    isArgumentGiven("PHHHCoulombIntegrals") ?
    new Tensor<>(4, vooo.data(), syms.data(), *Cc4s::world, "Vaijk") : nullptr
  );
  Tensor<> *Vijab(
    isArgumentGiven("HHPPCoulombIntegrals") ?
    new Tensor<>(4, oovv.data(), syms.data(), *Cc4s::world, "Vijab") : nullptr
  );
  Tensor<> *Vaijb(
    isArgumentGiven("PHHPCoulombIntegrals") ?
    new Tensor<>(4, voov.data(), syms.data(), *Cc4s::world, "Vaijb") : nullptr
  );
  Tensor<> *Viajk(
    isArgumentGiven("HPHHCoulombIntegrals") ?
    new Tensor<>(4, ovoo.data(), syms.data(), *Cc4s::world, "Viajk") : nullptr
  );
  Tensor<> *Viajb(
    isArgumentGiven("HPHPCoulombIntegrals") ?
    new Tensor<>(4, ovov.data(), syms.data(), *Cc4s::world, "Viajb") : nullptr
  );
  Tensor<> *Viabc(
    isArgumentGiven("HPPPCoulombIntegrals") ?
    new Tensor<>(4, ovvv.data(), syms.data(), *Cc4s::world, "Viabc") : nullptr
  );
  Tensor<> *Vabic(
    isArgumentGiven("PPHPCoulombIntegrals") ?
    new Tensor<>(4, vvov.data(), syms.data(), *Cc4s::world, "Vabic") : nullptr
  );
  Tensor<> *Viabj(
    isArgumentGiven("HPPHCoulombIntegrals") ?
    new Tensor<>(4, ovvo.data(), syms.data(), *Cc4s::world, "Viabj") : nullptr
  );
  Tensor<> *Vijak(
    isArgumentGiven("HHPHCoulombIntegrals") ?
    new Tensor<>(4, oovo.data(), syms.data(), *Cc4s::world, "Vijak") : nullptr
  );
  Tensor<> *Vaibc(
    isArgumentGiven("PHPPCoulombIntegrals") ?
    new Tensor<>(4, vovv.data(), syms.data(), *Cc4s::world, "Vijak") : nullptr
  );


  // Split GammaGab,GammaGai,GammaGia,GammaGij into real and imaginary parts
  Tensor<> realGammaGai(3, GammaGai->lens, GammaGai->sym,
                        *GammaGai->wrld, "RealGammaGai");
  Tensor<> imagGammaGai(3, GammaGai->lens, GammaGai->sym,
                        *GammaGai->wrld, "ImagGammaGai");
  fromComplexTensor(*GammaGai, realGammaGai, imagGammaGai);

  Tensor<> realGammaGab(3, GammaGab->lens, GammaGab->sym,
                        *GammaGab->wrld, "RealGammaGab");
  Tensor<> imagGammaGab(3, GammaGab->lens, GammaGab->sym,
                        *GammaGab->wrld, "ImagGammaGab");
  fromComplexTensor(*GammaGab, realGammaGab, imagGammaGab);

  Tensor<> realGammaGij(3, GammaGij->lens, GammaGij->sym,
                        *GammaGij->wrld, "RealGammaGij");
  Tensor<> imagGammaGij(3, GammaGij->lens, GammaGij->sym,
                        *GammaGij->wrld, "ImagGammaGij");
  fromComplexTensor(*GammaGij, realGammaGij, imagGammaGij);

  // Compute the integrals Vabij Vaibj Vaijb Vijkl Vabcd
  if (Vaibj) {
    LOG(1, "CoulombIntegrals") << "Evaluating "
                               << Vaibj->get_name() << std::endl;
    (*Vaibj)["aibj"]  = realGammaGab["Gab"] * realGammaGij["Gij"];
    (*Vaibj)["aibj"] += imagGammaGab["Gab"] * imagGammaGij["Gij"];
    allocatedTensorArgument("PHPHCoulombIntegrals", Vaibj);
  }
  if (Vabij) {
    LOG(1, "CoulombIntegrals") << "Evaluating "
                               << Vabij->get_name() << std::endl;
    (*Vabij)["abij"]  = realGammaGai["Gai"] * realGammaGai["Gbj"];
    (*Vabij)["abij"] += imagGammaGai["Gai"] * imagGammaGai["Gbj"];
    allocatedTensorArgument("PPHHCoulombIntegrals", Vabij);
  }
  if (Vijkl) {
    LOG(1, "CoulombIntegrals") << "Evaluating "
                               << Vijkl->get_name() << std::endl;
    (*Vijkl)["ijkl"]  = realGammaGij["Gik"] * realGammaGij["Gjl"];
    (*Vijkl)["ijkl"] += imagGammaGij["Gik"] * imagGammaGij["Gjl"];
    allocatedTensorArgument("HHHHCoulombIntegrals", Vijkl);
  }
  if (Vijka) {
    LOG(1, "CoulombIntegrals") << "Evaluating "
                               << Vijka->get_name() << std::endl;
    (*Vijka)["ijka"]  = realGammaGij["Gik"] * realGammaGai["Gaj"];
    (*Vijka)["ijka"] += imagGammaGij["Gik"] * imagGammaGai["Gaj"];
    allocatedTensorArgument("HHHPCoulombIntegrals", Vijka);
  }
  if (Vabcd) {
    LOG(1, "CoulombIntegrals") << "Evaluating "
                               << Vabcd->get_name() << std::endl;
    (*Vabcd)["abcd"]  = realGammaGab["Gac"] * realGammaGab["Gbd"];
    (*Vabcd)["abcd"] += imagGammaGab["Gac"] * imagGammaGab["Gbd"];
    allocatedTensorArgument("PPPPCoulombIntegrals", Vabcd);
  }
  if (Vabci) {
    LOG(1, "CoulombIntegrals") << "Evaluating "
                               << Vabci->get_name() << std::endl;
    (*Vabci)["abci"]  = realGammaGab["Gac"] * realGammaGai["Gbi"];
    (*Vabci)["abci"] += imagGammaGab["Gac"] * imagGammaGai["Gbi"];
    allocatedTensorArgument("PPPHCoulombIntegrals", Vabci);
  }

  // Create the rest of integrals from the already given ones
  // --------------------------------------------------------

  if (Viabj) {
    // ovvo = hl * vvoo
    LOG(1, "CoulombIntegrals") << "Evaluating"
                               << Viabj->get_name() << " using "
                               << Vabij->get_name() << std::endl;

    (*Viabj)["iabj"] = (*Vabij)["baij"];
    if (antisymmetrize) {
      // ovov = v * vovo
      (*Viabj)["iabj"] -= (*Vaibj)["aibj"];
    }
    allocatedTensorArgument("HPPHCoulombIntegrals", Viabj);
  }
  if (Viajb) {
    // ovov = v * vovo
    LOG(1, "CoulombIntegrals") << "Evaluating "
                               << Viajb->get_name() << " using "
                               << Vaibj->get_name() << std::endl;

    (*Viajb)["iajb"] = (*Vaibj)["aibj"];
    if (antisymmetrize) {
      // ovvo = hl * vvoo
      (*Viajb)["iajb"] -= (*Vabij)["baij"];
    }
    allocatedTensorArgument("HPHPCoulombIntegrals", Viajb);
  }
  if (Viabc) {
    // ovvv = h%v * vvvo
    LOG(1, "CoulombIntegrals") << "Evaluating "
                               << Viabc->get_name() << " using "
                               << Vabci->get_name() << std::endl;

    (*Viabc)["iabc"] = (*Vabci)["cbai"];
    if (antisymmetrize) {
      // ovvv = h%v * vvvo
      (*Viabc)["iabc"] -= (*Vabci)["bcai"];
    }
    allocatedTensorArgument("HPPPCoulombIntegrals", Viabc);
  }
  if (Vijak) {
    // oovo = v * ooov
    LOG(1, "CoulombIntegrals") << "Evaluating "
                               << Vijak->get_name() << " using "
                               << Vijka->get_name() << std::endl;

    (*Vijak)["ijak"] = (*Vijka)["jika"];
    if (antisymmetrize) {
      // ooov = e * ooov
      (*Vijak)["ijak"] -= (*Vijka)["ijka"];
    }
    allocatedTensorArgument("HHPHCoulombIntegrals", Vijak);
  }
  if (Vijab) {
    // oovv = h * vvoo
    LOG(1, "CoulombIntegrals") << "Evaluating "
                               << Vijab->get_name() << " using "
                               << Vabij->get_name() << std::endl;

    (*Vijab)["ijab"] = (*Vabij)["abij"];
    if (antisymmetrize) {
      // oovv = h * vvoo
      (*Vijab)["ijab"] -= (*Vabij)["baij"];
    }
    allocatedTensorArgument("HHPPCoulombIntegrals", Vijab);
  }
  if (Vabic) {
    // vvov = v * vvvo
    LOG(1, "CoulombIntegrals") << "Evaluating "
                               << Vabic->get_name() << " using "
                               << Vabci->get_name() << std::endl;

    (*Vabic)["abic"] = (*Vabci)["baci"];
    if (antisymmetrize) {
      // vvvo = e * vvvo
      (*Vabic)["abic"] -= (*Vabci)["abci"];
    }
    allocatedTensorArgument("PPHPCoulombIntegrals", Vabic);
  }
  if (Vaijb) {
    // voov = hr * vvoo
    LOG(1, "CoulombIntegrals") << "Evaluating "
                               << Vaijb->get_name() << " using "
                               << Vabij->get_name() << std::endl;

    (*Vaijb)["aijb"] = (*Vabij)["abji"];
    if (antisymmetrize) {
      // vovo = e * vovo
      (*Vaijb)["aijb"] -= (*Vaibj)["aibj"];
    }
    allocatedTensorArgument("PHHPCoulombIntegrals", Vaijb);
  }
  if (Vaibc) {
    // vovv = h * vvvo
    LOG(1, "CoulombIntegrals") << "Evaluating "
                               << Vaibc->get_name() << " using "
                               << Vabci->get_name() << std::endl;

    (*Vaibc)["aibc"] = (*Vabci)["bcai"];
    if (antisymmetrize) {
      // vovv = h * vvvo
      (*Vaibc)["aibc"] -= (*Vabci)["cbai"];
    }
    allocatedTensorArgument("PHPPCoulombIntegrals", Vaibc);
  }
  if (Vaijk) {
    // vooo = h%v * ooov
    LOG(1, "CoulombIntegrals") << "Evaluating "
                               << Vaijk->get_name() << " using "
                               << Vijka->get_name() << std::endl;

    (*Vaijk)["aijk"] = (*Vijka)["kjia"];
    if (antisymmetrize) {
      // vooo = h%v * ooov
      (*Vaijk)["aijk"] -= (*Vijka)["jkia"];
    }
    allocatedTensorArgument("PHHHCoulombIntegrals", Vaijk);
  }
  if (Viajk) {
    // ovoo = h * ooov
    LOG(1, "CoulombIntegrals") << "Evaluating "
                               << Viajk->get_name() << " using "
                               << Vijka->get_name() << std::endl;

    (*Viajk)["iajk"] = (*Vijka)["jkia"];
    if (antisymmetrize) {
      // ovoo = h * ooov
      (*Viajk)["iajk"] -= (*Vijka)["kjia"];
    }
    allocatedTensorArgument("HPHHCoulombIntegrals", Viajk);
  }

  if (antisymmetrize) {
    // Antisymmetrize integrals calculated directly from Gamma
    // IMPORTANT: This must be written after the creation of the integrals
    //            depending on them.
    if (Vijkl) (*Vijkl)["ijkl"] -= (*Vijkl)["ijlk"];
    if (Vabcd) (*Vabcd)["abcd"] -= (*Vabcd)["abdc"];
    if (Vijka) (*Vijka)["ijka"] -= (*Vijka)["jika"];
    if (Vabci) (*Vabci)["abci"] -= (*Vabci)["baci"];
    // Vaibj depends on Vabij, do not change the order
    if (Vaibj) (*Vaibj)["aibj"] -= (*Vabij)["baij"];
    if (Vabij) (*Vabij)["abij"] -= (*Vabij)["abji"];
  }

}

void CoulombIntegralsFromVertex::dryCalculateRealIntegrals() {
  DryTensor<> *Vaibj(
    isArgumentGiven("PHPHCoulombIntegrals") ?
    new DryTensor<>(4, std::vector<size_t>(vovo.begin(), vovo.end()).data(), syms.data()) : nullptr
  );
  DryTensor<> *Vabij(
    isArgumentGiven("PPHHCoulombIntegrals") ?
    new DryTensor<>(4, std::vector<size_t>(vvoo.begin(), vvoo.end()).data(), syms.data()) : nullptr
  );
  DryTensor<> *Vijab(
    isArgumentGiven("HHPPCoulombIntegrals") ?
    new DryTensor<>(4, std::vector<size_t>(oovv.begin(), oovv.end()).data(), syms.data()) : nullptr
  );
  DryTensor<> *Vaijb(
    isArgumentGiven("PHHPCoulombIntegrals") ?
    new DryTensor<>(4, std::vector<size_t>(voov.begin(), voov.end()).data(), syms.data()) : nullptr
  );
  DryTensor<> *Vijkl(
    isArgumentGiven("HHHHCoulombIntegrals") ?
    new DryTensor<>(4, std::vector<size_t>(oooo.begin(), oooo.end()).data(), syms.data()) : nullptr
  );
  DryTensor<> *Vijka(
    isArgumentGiven("HHHPCoulombIntegrals") ?
    new DryTensor<>(4, std::vector<size_t>(ooov.begin(), ooov.end()).data(), syms.data()) : nullptr
  );
  DryTensor<> *Vaijk(
    isArgumentGiven("PHHHCoulombIntegrals") ?
    new DryTensor<>(4, std::vector<size_t>(vooo.begin(), vooo.end()).data(), syms.data()) : nullptr
  );
  DryTensor<> *Vabcd(
    isArgumentGiven("PPPPCoulombIntegrals") ?
    new DryTensor<>(4, std::vector<size_t>(vvvv.begin(), vvvv.end()).data(), syms.data()) : nullptr
  );
  DryTensor<> *Vabci(
    isArgumentGiven("PPPHCoulombIntegrals") ?
    new DryTensor<>(4, std::vector<size_t>(vvvo.begin(), vvvo.end()).data(), syms.data()) : nullptr
  );

  if (Vaibj) {
    allocatedTensorArgument("PHPHCoulombIntegrals", Vaibj);
  }
  if (Vabij) {
    allocatedTensorArgument("PPHHCoulombIntegrals", Vabij);
  }
  if (Vijab) {
    allocatedTensorArgument("HHPPCoulombIntegrals", Vijab);
  }
  if (Vaijb) {
    allocatedTensorArgument("PHHPCoulombIntegrals", Vaijb);
  }
  if (Vijkl) {
    allocatedTensorArgument("HHHHCoulombIntegrals", Vijkl);
  }
  if (Vijka) {
    allocatedTensorArgument("HHHPCoulombIntegrals", Vijka);
  }
  if (Vaijk) {
    allocatedTensorArgument("PHHHCoulombIntegrals", Vaijk);
  }
  if (Vabcd) {
    allocatedTensorArgument("PPPPCoulombIntegrals", Vabcd);
  }
  if (Vabci) {
    allocatedTensorArgument("PPPHCoulombIntegrals", Vabci);
  }
}

void CoulombIntegralsFromVertex::calculateComplexIntegrals() {
  Tensor<complex> *Vabij(
    isArgumentGiven("PPHHCoulombIntegrals") ?
      new Tensor<complex>(4, vvoo.data(), syms.data(), *Cc4s::world, "Vabij") :
      nullptr
  );

  Tensor<complex> *Vijab(
    isArgumentGiven("HHPPCoulombIntegrals") ?
      new Tensor<complex>(4, oovv.data(), syms.data(), *Cc4s::world, "Vijab") :
      nullptr
  );

  Tensor<complex> *Vaijb(
    isArgumentGiven("PHHPCoulombIntegrals") ?
      new Tensor<complex>(4, voov.data(), syms.data(), *Cc4s::world, "Vaijb") :
      nullptr
  );

  Tensor<complex> *Vaibj(
    isArgumentGiven("PHPHCoulombIntegrals") ?
      new Tensor<complex>(4, vovo.data(), syms.data(), *Cc4s::world, "Vaibj") :
      nullptr
  );

  Tensor<complex> *Vijkl(
    isArgumentGiven("HHHHCoulombIntegrals") ?
      new Tensor<complex>(4, oooo.data(), syms.data(), *Cc4s::world, "Vijkl") :
      nullptr
  );

  Tensor<complex> *Vijka(
    isArgumentGiven("HHHPCoulombIntegrals") ?
      new Tensor<complex>(4, ooov.data(), syms.data(), *Cc4s::world, "Vijka") :
      nullptr
  );

  Tensor<complex> *Vaijk(
    // TODO: PHHH is always conj(Permute(HHHP))
    isArgumentGiven("PHHHCoulombIntegrals") ?
      new Tensor<complex>(4, vooo.data(), syms.data(), *Cc4s::world, "Vaijk") :
      nullptr
  );

  Univar_Function<complex> fConj(conj<complex>);

  Tensor<complex> conjTransposeGammaGai(false, *GammaGai);
  conjTransposeGammaGai.sum(1.0,*GammaGia,"Gia", 0.0,"Gai", fConj);

  Tensor<complex> conjTransposeGammaGia(false, *GammaGia);
  conjTransposeGammaGia.sum(1.0,*GammaGai,"Gai", 0.0,"Gia", fConj);

  Tensor<complex> conjTransposeGammaGij(false, *GammaGij);
  conjTransposeGammaGij.sum(1.0,*GammaGij,"Gji", 0.0,"Gij", fConj);

  Tensor<complex> conjTransposeGammaGab(false, *GammaGab);
  conjTransposeGammaGab.sum(1.0,*GammaGab,"Gba", 0.0,"Gab", fConj);

  if (Vabij) {
    LOG(1, "CoulombIntegrals") << "Evaluating "
                               << Vabij->get_name() << std::endl;
    (*Vabij)["abij"] = conjTransposeGammaGai["Gai"] * (*GammaGai)["Gbj"];
    allocatedTensorArgument<complex>("PPHHCoulombIntegrals", Vabij);
  }

  if (Vaijb) {
    LOG(1, "CoulombIntegrals") << "Evaluating "
                               << Vaijb->get_name() << std::endl;
    (*Vaijb)["aijb"] = conjTransposeGammaGai["Gaj"] * (*GammaGia)["Gib"];
    allocatedTensorArgument<complex>("PHHPCoulombIntegrals", Vaijb);
  }

  if (Vijab) {
    LOG(1, "CoulombIntegrals") << "Evaluating "
                               << Vijab->get_name() << std::endl;
    (*Vijab)["ijab"] = conjTransposeGammaGia["Gia"] * (*GammaGia)["Gjb"];
    allocatedTensorArgument<complex>("HHPPCoulombIntegrals", Vijab);
  }

  if (Vaibj) {
    LOG(1, "CoulombIntegrals") << "Evaluating "
                               << Vaibj->get_name() << std::endl;
    (*Vaibj)["aibj"] = conjTransposeGammaGab["Gab"] * (*GammaGij)["Gij"];
    allocatedTensorArgument<complex>("PHPHCoulombIntegrals", Vaibj);
  }

  if (Vijkl) {
    LOG(1, "CoulombIntegrals") << "Evaluating "
                               << Vijkl->get_name() << std::endl;
    (*Vijkl)["ijkl"] = conjTransposeGammaGij["Gik"] * (*GammaGij)["Gjl"];
    allocatedTensorArgument<complex>("HHHHCoulombIntegrals", Vijkl);
  }

  if (Vijka) {
    LOG(1, "CoulombIntegrals") << "Evaluating "
                               << Vijka->get_name() << std::endl;
    (*Vijka)["ijka"] = conjTransposeGammaGij["Gik"] * (*GammaGia)["Gja"];
    allocatedTensorArgument<complex>("HHHPCoulombIntegrals", Vijka);
  }

  if (Vaijk) {
    LOG(1, "CoulombIntegrals") << "Evaluating "
                               << Vaijk->get_name() << std::endl;
    (*Vaijk)["aijk"] = conjTransposeGammaGai["Gaj"] * (*GammaGij)["Gik"];
    allocatedTensorArgument<complex>("PHHHCoulombIntegrals", Vaijk);
  }
}

void CoulombIntegralsFromVertex::dryCalculateComplexIntegrals() {
  DryTensor<complex> *Vaibj(
    isArgumentGiven("PHPHCoulombIntegrals") ?
    new DryTensor<complex>(4, std::vector<size_t>(vovo.begin(), vovo.end()).data(), syms.data())
    : nullptr
  );
  DryTensor<complex> *Vabij(
    isArgumentGiven("PPHHCoulombIntegrals") ?
    new DryTensor<complex>(4, std::vector<size_t>(vvoo.begin(), vvoo.end()).data(), syms.data())
    : nullptr
  );
  DryTensor<complex> *Vijab(
    isArgumentGiven("HHPPCoulombIntegrals") ?
    new DryTensor<complex>(4, std::vector<size_t>(oovv.begin(), oovv.end()).data(), syms.data())
    : nullptr
  );
  DryTensor<complex> *Vaijb(
    isArgumentGiven("PHHPCoulombIntegrals") ?
    new DryTensor<complex>(4, std::vector<size_t>(voov.begin(), voov.end()).data(), syms.data())
    : nullptr
  );
  DryTensor<complex> *Vijkl(
    isArgumentGiven("HHHHCoulombIntegrals") ?
    new DryTensor<complex>(4, std::vector<size_t>(oooo.begin(), oooo.end()).data(), syms.data())
    : nullptr
  );
  DryTensor<complex> *Vijka(
    isArgumentGiven("HHHPCoulombIntegrals") ?
    new DryTensor<complex>(4, std::vector<size_t>(ooov.begin(), ooov.end()).data(), syms.data())
    : nullptr
  );
  DryTensor<complex> *Vaijk(
    isArgumentGiven("PHHHCoulombIntegrals") ?
    new DryTensor<complex>(4, std::vector<size_t>(vooo.begin(), vooo.end()).data(), syms.data())
    : nullptr
  );

  if (Vaibj) {
    allocatedTensorArgument<complex,DryTensor<complex>>(
      "PHPHCoulombIntegrals", Vaibj
    );
  }
  if (Vabij) {
    allocatedTensorArgument<complex,DryTensor<complex>>(
      "PPHHCoulombIntegrals", Vabij
    );
  }
  if (Vijab) {
    allocatedTensorArgument<complex,DryTensor<complex>>(
      "HHPPCoulombIntegrals", Vijab
    );
  }
  if (Vaijb) {
    allocatedTensorArgument<complex,DryTensor<complex>>(
      "PHHPCoulombIntegrals", Vaijb
    );
  }
  if (Vijkl) {
    allocatedTensorArgument<complex,DryTensor<complex>>(
      "HHHHCoulombIntegrals", Vijkl
    );
  }
  if (Vijka) {
    allocatedTensorArgument<complex,DryTensor<complex>>(
      "HHHPCoulombIntegrals", Vijka
    );
  }
  if (Vaijk) {
    allocatedTensorArgument<complex,DryTensor<complex>>(
      "PHHHCoulombIntegrals", Vaijk
    );
  }
}
*/

