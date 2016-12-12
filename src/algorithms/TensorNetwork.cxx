#include <algorithms/TensorNetwork.hpp>
#include <tcc/Tensor.hpp>
#include <tcc/Contraction.hpp>
#include <tcc/Assignment.hpp>
#include <tcc/Operation.hpp>
#include <tcc/AssignmentOperation.hpp>
#include <tcc/ContractionOperation.hpp>
#include <util/CtfMachineTensor.hpp>
#include <Cc4s.hpp>

#include <initializer_list>
#include <vector>
#include <memory>
using std::shared_ptr;
using std::make_shared;

using namespace cc4s;
using namespace tcc;

ALGORITHM_REGISTRAR_DEFINITION(TensorNetwork);

TensorNetwork::TensorNetwork(
  std::vector<Argument> const &argumentList
): Algorithm(argumentList) {
}

TensorNetwork::~TensorNetwork() {
}

/**
 * \brief Testing environement
 */
void TensorNetwork::run() {
}


void TensorNetwork::dryRun() {
  int No(10);
  int Nv(90);
  int Np(No+Nv);
  int NF(200);
  int NR(300);
  shared_ptr<CtfMachineTensorFactory<>> ctfFactory(
    CtfMachineTensorFactory<>::create(Cc4s::world)
  );
  shared_ptr<Tcc<>> tcc(Tcc<>::create(ctfFactory));

/*
  shared_ptr<Tensor<complex>> Tc(
    tcc.createTensor<complex>(std::vector<int>({100,100,10,10}), "Tc")
  );
*/
  shared_ptr<Tensor<>> T(
    tcc->createTensor(std::vector<int>({100,100,10,10}), "T")
  );
  shared_ptr<Tensor<>> Pi(
    tcc->createTensor(std::vector<int>({300,100}), "Pi")
  );
  shared_ptr<Tensor<>> PiT(
    tcc->createTensor(std::vector<int>({300,100}), "PiT")
  );
  shared_ptr<Tensor<>> Lambda(
    tcc->createTensor(std::vector<int>({300,200}), "Lambda")
  );
  shared_ptr<Tensor<>> LambdaT(
    tcc->createTensor(std::vector<int>({300,200}), "LambdaT")
  );

//  CompoundDryTensorExpression<> Gamma("Fac") = PiT["Ra"] * Pi["Rc"] * Lambda["RG"]

  shared_ptr<Contraction<>> TPi(
    Contraction<>::create((*T)["cdij"], (*Pi)["Rd"])
  );
  shared_ptr<Operation<>> ladderOperation = compile(
    (*T)["abij"] <<=
      TPi * (*PiT)["Rb"] *
      (*Pi)["Sc"] * (*PiT)["Sa"] * (*LambdaT)["SF"] * (*Lambda)["RF"]
  );
  ladderOperation->execute();

// this contraction already requires heuristics
/*
  shared_ptr<Tensor<>> Pia(
    tcc.createTensor<>(std::vector<int>({NR,Nv}), "Pia")
  );
  shared_ptr<Tensor<>> Pii(
    tcc.createTensor<>(std::vector<int>({NR,No}), "Pii")
  );
  int Nn(7);
  shared_ptr<Tensor<>> w(
    tcc.createTensor<>(std::vector<int>({Nn}), "w")
  );
  shared_ptr<Tensor<>> H(
    tcc.createTensor<>(std::vector<int>({No,Nn}), "H")
  );
  shared_ptr<Tensor<>> P(
    tcc.createTensor<>(std::vector<int>({Nv,Nn}), "P")
  );
  shared_ptr<Tensor<>> e(
    tcc.createTensor<>(std::vector<int>(), "e")
  );

  shared_ptr<Operation<>> imaginaryTimeMp2Operation = compile(
    (*e)[""] <<=
      (*Pii)["Ri"]  * (*Pia)["Ra"] *
        (*LambdaT)["RF"] * (*Lambda)["SF"] *
      (*Pii)["Sj"] * (*Pia)["Sb"] *
        (*w)["n"] * (*P)["an"] * (*H)["in"] * (*P)["bn"] * (*H)["jn"] *
      (*Pii)["Ti"]  * (*Pia)["Ta"] *
        (*LambdaT)["TH"] * (*Lambda)["UH"] *
      (*Pii)["Uj"] * (*Pia)["Ub"]
  );
  imaginaryTimeMp2Operation->execute();
*/
}


