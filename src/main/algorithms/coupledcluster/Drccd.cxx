/* Copyright 2021 cc4s.org
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <algorithms/coupledcluster/Drccd.hpp>
#include <math/MathFunctions.hpp>
#include <util/Log.hpp>
#include <util/SharedPointer.hpp>
#include <util/Exception.hpp>

using namespace cc4s;

template <typename F, typename TE>
CoupledClusterMethodRegistrar<
  F,TE,Drccd<F,TE>
> Drccd<F,TE>::registrar_("Drccd");


template <typename F, typename TE>
Ptr<TensorUnion<F,TE>> Drccd<F,TE>::getResiduum(
  const int iteration, const bool restart,
  const Ptr<TensorUnion<F,TE>> &amplitudes
) {
  // read all required integrals
  auto coulombIntegrals(this->arguments->getMap("coulombIntegrals"));
  auto coulombSlices(coulombIntegrals->getMap("slices"));
  auto Vpphh(coulombSlices->template getPtr<TensorExpression<F,TE>>("pphh"));
  auto Vphhp(coulombSlices->template getPtr<TensorExpression<F,TE>>("phhp"));
  auto Vhhpp(coulombSlices->template getPtr<TensorExpression<F,TE>>("hhpp"));

  // get spins
  auto orbitalType(
    coulombIntegrals->getMap(
      "indices"
    )->getMap("orbital")->template getValue<std::string>("type")
  );
  Real<> spins;
  if (orbitalType == "spatial") {
    spins = 2;
  } else if (orbitalType == "spin") {
    spins = 1;
  } else {
    ASSERT_LOCATION(
      false, "unsupported orbital type '" + orbitalType + "'",
      coulombIntegrals->getMap(
        "indices"
      )->getMap("orbital")->get("type")->sourceLocation
    );
  }

  // get amplitude parts
  auto Tpphh( amplitudes->get(1) );

  // construct residuum
  auto residuum( New<TensorUnion<F,TE>>(*amplitudes) );
  *residuum *= F(0);
  auto Rpphh( residuum->get(1) );

  auto methodArguments(this->arguments->getMap("method"));
  bool linearized(methodArguments->template getValue<bool>("linearized", false));
  bool adjacentPairsExchange(
    methodArguments->template getValue<bool>("adjacentPairsExchange", false)
  );
  if (linearized) {
//    OUT() << "Solving linearized T2 Amplitude Equations" << std::endl;
  } else {
//    OUT() << "Solving T2 Amplitude Equations" << std::endl;
  }

  // TODO: deal with starting amplitudes
  if (iteration > 0 || restart) { // || isArgumentGiven("startingDoublesAmplitudes")) {
    auto Whhpp( Tcc<TE>::template tensor<F>("Whhpp") );
    // for the remaining iterations compute the drCCD residuum
    COMPILE(
      (*Rpphh)["abij"] += (*Vpphh)["abij"],
      (*Rpphh)["abij"] += spins * (*Vphhp)["akic"] * (*Tpphh)["cbkj"],
      (*Rpphh)["abij"] += spins * (*Vphhp)["bkjc"] * (*Tpphh)["acik"],
      (linearized) ? (
        // linearized: nothing more to do
        Tcc<TE>::sequence()
      ) : (
        // otherwise: do quadratic contribution
        (*Whhpp)["ijab"] <<= spins * (*Vhhpp)["ijab"],
        (adjacentPairsExchange) ? (
          // adjacent pairs correction: also exchange holes in Whhpp
          (*Whhpp)["ijab"] -= (*Vhhpp)["jiab"],
          Tcc<TE>::sequence()
        ) : (
          // otherwise: do nothing else with Whhpp
          Tcc<TE>::sequence()
        ),
        // compute quadratic contribution
        (*Rpphh)["abij"] +=
          spins * (*Whhpp)["klcd"] * (*Tpphh)["acik"] * (*Tpphh)["dblj"],
        Tcc<TE>::sequence()
      )
    )->execute();
// TODO: adjacent pairs exchange
/*
      Tensor<F> Wijab(false, *Vijab);
      Wijab["ijab"] = ;
      if (getIntegerArgument("adjacentPairsExchange", 0)) {
        Wijab["ijab"] -= (*Vijab)["jiab"];
      }
*/
  } else {
    // no amplitudes given: start with MP2 amplitudes
    COMPILE(
      (*Rpphh)["abij"] += (*Vpphh)["abij"]
    )->execute();
  }

  return residuum;
}

// instantiate
template class cc4s::Drccd<Real<64>, DefaultDryTensorEngine>;
template class cc4s::Drccd<Complex<64>, DefaultDryTensorEngine>;
template class cc4s::Drccd<Real<64>, DefaultTensorEngine>;
template class cc4s::Drccd<Complex<64>, DefaultTensorEngine>;
