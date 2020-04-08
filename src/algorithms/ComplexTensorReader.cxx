/*Copyright (c) 2016, Andreas Grueneis and Felix Hummel, all rights reserved.*/
#include <algorithms/ComplexTensorReader.hpp>
#include <math/Complex.hpp>
#include <util/TensorIo.hpp>
#include <util/Log.hpp>
#include <Cc4s.hpp>
#include <fstream>
#include <ctf.hpp>
#include <util/Emitter.hpp>

using namespace CTF;
using namespace cc4s;

ALGORITHM_REGISTRAR_DEFINITION(ComplexTensorReader);

ComplexTensorReader::ComplexTensorReader(
  std::vector<Argument> const &argumentList
): Algorithm(argumentList) {
}

ComplexTensorReader::~ComplexTensorReader() {
}

void ComplexTensorReader::run() {
  std::string dataName(getArgumentData("Data")->getName());

  // make sure all processes start reading the file at the same time in case
  // it has been modified before
  Cc4s::world->barrier();

  std::string mode(getTextArgument("mode", "text"));
  Tensor<complex> *A;
  if (mode == "binary") {
    std::string fileName(getTextArgument("file", dataName + ".bin"));
    A = TensorIo::readBinary<complex>(fileName);
    EMIT() << YAML::Key << "file" << YAML::Value << fileName;
  } else {
    std::string fileName(getTextArgument("file", dataName + ".dat"));
    std::string delimiter(getTextArgument("delimiter", " "));
    int64_t bufferSize(getIntegerArgument("bufferSize", 128l*1024*1024));
    A = TensorIo::readText<complex>(fileName, delimiter, bufferSize);
    EMIT() << YAML::Key << "file" << YAML::Value << fileName;
  }
  A->set_name(dataName.c_str());
  allocatedTensorArgument<complex>("Data", A);
  EMIT() << YAML::Key << "Data" << YAML::Value << dataName;

  int64_t indexCount(1);
  for (int dim(0); dim < A->order; ++dim) {
    indexCount *= A->lens[dim];
  }
  EMIT() << YAML::Key << "elements" << YAML::Value << indexCount;


}

