#include <algorithms/SliceOperator.hpp>

#include <math/Complex.hpp>
#include <math/MathFunctions.hpp>
#include <util/Log.hpp>
#include <util/Exception.hpp>
#include <tcc/Tcc.hpp>
#include <Cc4s.hpp>

using namespace cc4s;

ALGORITHM_REGISTRAR_DEFINITION(SliceOperator);

Ptr<MapNode> SliceOperator::run(const Ptr<MapNode> &arguments) {
  auto op(arguments->getMap("operator"));
  auto scalarType(op->getValue<std::string>("scalarType"));
  // multiplex calls to template methods
  if (scalarType == "real64") {  
    if (Cc4s::options->dryRun) {
      return run<Real<>,DryTensorEngine>(arguments);
    } else {
      return run<Real<>,DefaultTensorEngine>(arguments);
    }
  } else if (scalarType == "complex64") {
    if (Cc4s::options->dryRun) {
      return run<Complex<>,DryTensorEngine>(arguments);
    } else {
      return run<Complex<>,DefaultTensorEngine>(arguments);
    }
  } else {
    Assert(false, "scalar type '" + scalarType + "' not supported");
  }
}

template <typename F, typename TE>
Ptr<MapNode> SliceOperator::run(
  const Ptr<MapNode> &arguments
) {
  typedef Tensor<F,TE> T;
  auto op(arguments->getMap("operator"));
  auto data(op->getValue<Ptr<T>>("data"));
  Assert(data, "expecting operator to be a tensor");

  // read dimensions from eigen energies meta data
  auto slicedEigenEnergies(arguments->getMap("slicedEigenEnergies"));
  No = slicedEigenEnergies->getValue<size_t>("holesCount");
  Nv = slicedEigenEnergies->getValue<size_t>("particlesCount");

  auto dimensions(arguments->getMap("dimensions"));
  Assert(
    dimensions,
    "Argument 'dimensions' expected, specifying the dimensions to slice."
  );
  for (auto key: dimensions->getKeys()) {
    dims.push_back(dimensions->getValue<size_t>(key));
  }

  slices = New<MapNode>();
  LOG(1,getName()) << "Slicing " << data->getName() <<
    " into holes and particles." << std::endl;
  slice(data, "");

  // create result
  auto slicedOperator(New<MapNode>());
  // copy meta data from original operator
  for (auto key: op->getKeys()) {
    if (key != "data") {
      slicedOperator->get(key) = op->get(key);
    }
  }
  // enter slices
  slicedOperator->get("slices") = slices;
  auto result(New<MapNode>());
  result->get("slicedOperator") = slicedOperator;
  return result;
}

template <typename F, typename TE>
void SliceOperator::slice(
  const Ptr<Tensor<F,TE>> &data, const std::string &parts
) {
  if (parts.length() < dims.size()) {
    slice(data, parts + "h");
    slice(data, parts + "p");
  } else {
    std::vector<size_t> begins(data->lens.size());
    std::vector<size_t> ends(data->lens);
    std::string index("");
    for (unsigned int i(0); i < data->lens.size(); ++i) {
      index += ('a' + i);
    }
    for (unsigned int i(0); i < dims.size(); ++i) {
      if (parts[i] == 'h') {
        ends[dims[i]] = No;
      } else {
        begins[dims[i]] = data->lens[dims[i]] - Nv;
      }
    }
    auto result(Tcc<TE>::template tensor<F>(data->getName() + parts ));
    slices->setValue(
      parts,
      COMPILE_RECIPE(result,
        (*result)[index] <<= (*(*data)(begins,ends))[index]
      )
    );
  }
}
