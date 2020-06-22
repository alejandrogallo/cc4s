#include <algorithms/DefineHolesAndParticles.hpp>

#include <math/Complex.hpp>
#include <math/MathFunctions.hpp>
#include <util/Log.hpp>
#include <util/Exception.hpp>
#include <tcc/Tcc.hpp>
#include <Cc4s.hpp>

using namespace cc4s;

ALGORITHM_REGISTRAR_DEFINITION(DefineHolesAndParticles);

Ptr<MapNode> DefineHolesAndParticles::run(const Ptr<MapNode> &arguments) {
  // multiplex calls to template methods
  if (Cc4s::options->dryRun) {
    return run<DryTensorEngine>(arguments);
  } else {
    return run<DefaultTensorEngine>(arguments);
  }
}

template <typename TE>
Ptr<MapNode> DefineHolesAndParticles::run(
  const Ptr<MapNode> &arguments
) {
  typedef Tensor<Real<>,TE> T;
  auto eigenEnergies(arguments->getMap("eigenEnergies"));
  auto eps(eigenEnergies->getValue<Ptr<T>>("data"));
  ASSERT_LOCATION(
    eps, "expecting eigenEnergies to be a real tensor",
    eigenEnergies->sourceLocation
  );  
  ASSERT_LOCATION(
    eps->lens.size()==1, "expecting eigenEnergies to be a rank 1 tensor",
    eigenEnergies->sourceLocation
  );  

  // read values of eps on all ranks
  auto Np(eps->lens[0]);
  std::vector<Real<>> epsilonValues(Np);
  std::vector<size_t> indices(Np);
  for (size_t i(0); i < indices.size(); ++i) { indices[i] = i; }
  eps->read(indices.size(), indices.data(), epsilonValues.data());

  // find fermi energy to determine No and Nv
  auto fermiEnergy(eigenEnergies->getValue<Real<>>("fermiEnergy"));
  size_t No(0);
  while (No < Np && epsilonValues[No] < fermiEnergy) { ++No; }
  ASSERT_LOCATION(
    0 < No, "Fermi energy below all eigen energies.",
    eigenEnergies->sourceLocation
  );
  ASSERT_LOCATION(
    No < Np, "Fermi energy above all eigen energies.",
    eigenEnergies->sourceLocation
  );

  auto Nv(Np-No);
  LOG(1,getName()) << "No=" << No << std::endl;
  LOG(1,getName()) << "Nv=" << Nv << std::endl;
  LOG(1,getName()) << "Np=" << Np << std::endl;

  auto slices(New<MapNode>(eigenEnergies->sourceLocation));
  {
    auto epsi(Tcc<TE>::template tensor<Real<>>("epsi"));
    slices->setValue(
      "h",
      COMPILE_RECIPE(epsi,
        (*epsi)["i"] <<= (*(*eps)({0},{No}))["i"]
      )
    );
  }
  {
    auto epsa(Tcc<TE>::template tensor<Real<>>("epsa"));
    slices->setValue(
      "p",
      COMPILE_RECIPE(epsa,
        (*epsa)["a"] <<= (*(*eps)({No},{Np}))["a"]
      )
    );
  }

  // create result
  auto slicedEigenEnergies(New<MapNode>(eigenEnergies->sourceLocation));
  slicedEigenEnergies->get("scalarType") = eigenEnergies->get("scalarType");
  slicedEigenEnergies->get("indices") = eigenEnergies->get("indices");
  slicedEigenEnergies->get("dimensions") = eigenEnergies->get("dimensions");
  slicedEigenEnergies->get("unit") = eigenEnergies->get("unit");
  slicedEigenEnergies->setValue<size_t>("holesCount", No);
  slicedEigenEnergies->setValue<size_t>("particlesCount", Nv);
  slicedEigenEnergies->get("slices") = slices;
  auto result(New<MapNode>(SOURCE_LOCATION));
  result->get("slicedEigenEnergies") = slicedEigenEnergies;
  return result;
}

