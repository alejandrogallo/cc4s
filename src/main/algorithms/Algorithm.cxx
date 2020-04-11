/*Copyright (c) 2015, Andreas Grueneis and Felix Hummel, all rights reserved.*/

#include <algorithms/Algorithm.hpp>
#include <Data.hpp>
#include <math/Complex.hpp>
#include <util/Exception.hpp>
#include <util/Log.hpp>

#include <iostream>
#include <sstream>

using namespace cc4s;

Algorithm::Algorithm(std::vector<Argument> const &argumentList) {
  for (auto arg(argumentList.begin()); arg != argumentList.end(); ++arg) {
    Argument argument = *arg;
    arguments[argument.getName()] = argument.getData();
  }
}

Algorithm::~Algorithm() {
}

/**
 * \brief The dryRun estimates resource consumption, especially
 * memory and processor time.
 */
void Algorithm::dryRun() {
  LOG(0, getName()) << "dry run not implemented" << std::endl;
}

bool Algorithm::isArgumentGiven(std::string const &name) {
  return arguments.find(name) != arguments.end();
}

Data *Algorithm::getArgumentData(std::string const &name) {
  auto dataIterator(arguments.find(name));
  if (dataIterator == arguments.end()) {
    std::stringstream sStream;
    sStream << "Missing argument: " << name;
//    throw new EXCEPTION(std::stringstream() << "Missing argument: " << name);
    throw new EXCEPTION(sStream.str());
  }
  Data *data = Data::get(dataIterator->second);
  if (!data) {
    std::stringstream sStream;
    sStream << "Missing data: " << dataIterator->second;
//    throw new EXCEPTION(std::stringstream() << "Missing data: " << dataIterator->second);
    throw new EXCEPTION(sStream.str());
  }
  return data;
}

std::string Algorithm::getTextArgument(std::string const &name) {
  Data *data(getArgumentData(name));
  TextData const *textData = dynamic_cast<TextData const *>(data);
  if (!textData) {
    std::stringstream sstream;
    sstream << "Incompatible type for argument: " << name << ". "
      << "Excpected Text, found " << data->getTypeName() << ".";
    throw new EXCEPTION(sstream.str());
  }
  return textData->value;
}
std::string Algorithm::getTextArgument(
  std::string const &name, std::string const &defaultValue
) {
  return isArgumentGiven(name) ? getTextArgument(name) : defaultValue;
}

bool Algorithm::getBooleanArgument(std::string const &name) {
  /*
   *TODO: Do this without the getTextArgument function, because in this
   *case the parser want to have quotes on the boolean value i.e.
   *  (myflag "true")
   */
  std::string text(getTextArgument(name));
  if (
    text.compare(".TRUE.") == 0 ||
    text.compare("true") == 0 ||
    text.compare("True") == 0 ||
    text.compare("TRUE") == 0 ||
    text.compare("1") == 0 ||
    text.compare("t") == 0 ||
    text.compare("T") == 0
  ) {
    return true;
  } else {
    return false;
  }
}
bool Algorithm::getBooleanArgument(
  std::string const &name, bool const &defaultValue
) {
  return isArgumentGiven(name) ? getBooleanArgument(name) : defaultValue;
}

int64_t Algorithm::getIntegerArgument(std::string const &name) {
  Data const *data(getArgumentData(name));
  IntegerData const *integerData = dynamic_cast<IntegerData const *>(data);
  if (!integerData) {
    std::stringstream sstream;
    sstream << "Incompatible type for argument: " << name << ". "
      << "Excpected Integer, found " << data->getTypeName() << ".";
    throw new EXCEPTION(sstream.str());
  }
  return integerData->value;
}
int64_t Algorithm::getIntegerArgument(
  std::string const &name, int64_t const defaultValue
) {
  return isArgumentGiven(name) ? getIntegerArgument(name) : defaultValue;
}

Real<> Algorithm::getRealArgument(std::string const &name) {
  Data *data(getArgumentData(name));
  RealData *realData(dynamic_cast<RealData *>(data));
  if (realData) return realData->value;
  IntegerData *integerData(dynamic_cast<IntegerData *>(data));
  if (integerData) return getRealArgumentFromInteger(integerData);
  TensorData<Real<>> *tensorData(dynamic_cast<TensorData<Real<>> *>(data));
  if (tensorData) return getRealArgumentFromTensor(tensorData);
  std::stringstream sstream;
  sstream << "Incompatible type for argument: " << name << ". "
    << "Excpected Real, found " << data->getTypeName() << ".";
  throw new EXCEPTION(sstream.str());
}
Real<> Algorithm::getRealArgument(
  const std::string &name, const Real<> defaultValue
) {
  return isArgumentGiven(name) ? getRealArgument(name) : defaultValue;
}
Real<> Algorithm::getRealArgumentFromInteger(IntegerData *integerData ) {
  Real<> value(integerData->value);
  if (int64_t(value) != integerData->value) {
    LOG(0, "root") << "Warning: loss of precision in conversion from integer to real."
      << std::endl;
  }
  return value;
}
Real<> Algorithm::getRealArgumentFromTensor(TensorData<Real<>> *data) {
  Assert(
    data->value->order == 0,
    "Scalar expected in conversion from tensor to real."
  );
  // retrieve the real value from the tensor
  CTF::Scalar<Real<>> scalar;
  scalar[""] = (*data->value)[""];
  return scalar.get_val();
}

template <typename F, typename T>
T *Algorithm::getTensorArgument(std::string const &name) {
  Data *data(getArgumentData(name));
  TensorData<F, T> *tensorData(dynamic_cast<TensorData<F, T> *>(data));
  if (tensorData) return tensorData->value;
  RealData *realData(dynamic_cast<RealData *>(data));
  if (realData) return getTensorArgumentFromReal<F, T>(realData);
  // TODO: provide conversion routines from real to complex tensors
  std::stringstream sStream;
  sStream << "Incompatible type for argument: " << name << ". "
    << "Excpected tensor of " << TypeTraits<F>::getName()
    << ", found " << data->getTypeName() << ".";
  throw new EXCEPTION(sStream.str());
}
// instantiate
template
CTF::Tensor<Real<64>> *Algorithm::getTensorArgument<
  Real<64>, CTF::Tensor<Real<64>>
>(std::string const &);
template
CTF::Tensor<Complex<64>> *Algorithm::getTensorArgument<
  Complex<64>, CTF::Tensor<Complex<64>>
>(std::string const &);
template
DryTensor<Real<64>> *Algorithm::getTensorArgument<
  Real<64>, DryTensor<Real<64>>
>(std::string const &);
template
DryTensor<Complex<64>> *Algorithm::getTensorArgument<
  Complex<64>, DryTensor<Complex<64>>
>(std::string const &);

#ifndef INTEL_COMPILER
template
CTF::Tensor<Real<128>> *Algorithm::getTensorArgument<
  Real<128>, CTF::Tensor<Real<128>>
>(std::string const &);
template
CTF::Tensor<Complex<128>> *Algorithm::getTensorArgument<
  Complex<128>, CTF::Tensor<Complex<128>>
>(std::string const &);
template
DryTensor<Real<128>> *Algorithm::getTensorArgument<
  Real<128>, DryTensor<Real<128>>
>(std::string const &);
template
DryTensor<Complex<128>> *Algorithm::getTensorArgument<
  Complex<128>, DryTensor<Complex<128>>
>(std::string const &);
#endif


/**
 * \brief Traits for retrieving the Scalar, Vector and Matrix tensor type.
 */
template < typename F, typename T=CTF::Tensor<F> >
class TensorTypeTraits;

template <typename F>
class TensorTypeTraits< F, CTF::Tensor<F> > {
public:
  typedef CTF::Tensor<F> BaseType;
  typedef CTF::Scalar<F> ScalarType;
  typedef CTF::Vector<F> VectorType;
  typedef CTF::Matrix<F> MatrixType;
};
template <typename F>
class TensorTypeTraits< F, CTF::Matrix<F> > {
public:
  typedef CTF::Tensor<F> BaseType;
};
template <typename F>
class TensorTypeTraits< F, CTF::Vector<F> > {
public:
  typedef CTF::Tensor<F> BaseType;
};
template <typename F>
class TensorTypeTraits< F, CTF::Scalar<F> > {
public:
  typedef CTF::Tensor<F> BaseType;
};
template <typename F>
class TensorTypeTraits< F, DryTensor<F> > {
public:
  typedef DryTensor<F> BaseType;
  typedef DryScalar<F> ScalarType;
  typedef DryVector<F> VectorType;
  typedef DryMatrix<F> MatrixType;
};
template <typename F>
class TensorTypeTraits< F, DryMatrix<F> > {
public:
  typedef DryTensor<F> BaseType;
};
template <typename F>
class TensorTypeTraits< F, DryVector<F> > {
public:
  typedef DryTensor<F> BaseType;
};
template <typename F>
class TensorTypeTraits< F, DryScalar<F> > {
public:
  typedef DryTensor<F> BaseType;
};


/**
 * \brief Converts the given real data into a scalar tensor.
 */
template <typename F, typename T>
T *Algorithm::getTensorArgumentFromReal(RealData *realData) {
  // FIXME: left to leak memory...
  // a better solution would be to replace the RealData with the allocated
  // TensorData and support down-cast for Scalars to Real
  return new typename TensorTypeTraits<F,T>::ScalarType(realData->value);
}
// instantiate
template
CTF::Tensor<Real<64>> *Algorithm::getTensorArgumentFromReal<
  Real<64>, CTF::Tensor<Real<64>>
>(RealData *);
template
CTF::Tensor<Complex<64>> *Algorithm::getTensorArgumentFromReal<
  Complex<64>, CTF::Tensor<Complex<64>>
>(RealData *);
template
DryTensor<Real<64>> *Algorithm::getTensorArgumentFromReal<
  Real<64>, DryTensor<Real<64>>
>(RealData *);
template
DryTensor<Complex<64>> *Algorithm::getTensorArgumentFromReal<
  Complex<64>, DryTensor<Complex<64>>
>(RealData *);

#ifndef INTEL_COMPILER
template
CTF::Tensor<Real<128>> *Algorithm::getTensorArgumentFromReal<
  Real<128>, CTF::Tensor<Real<128>>
>(RealData *);
template
CTF::Tensor<Complex<128>> *Algorithm::getTensorArgumentFromReal<
  Complex<128>, CTF::Tensor<Complex<128>>
>(RealData *);
template
DryTensor<Real<128>> *Algorithm::getTensorArgumentFromReal<
  Real<128>, DryTensor<Real<128>>
>(RealData *);
template
DryTensor<Complex<128>> *Algorithm::getTensorArgumentFromReal<
  Complex<128>, DryTensor<Complex<128>>
>(RealData *);
#endif

template <typename F, typename T>
void Algorithm::allocatedTensorArgument(
  std::string const &name, T *tensor
) {
  Data *mentionedData(getArgumentData(name));
  new TensorData<F, typename TensorTypeTraits<F, T>::BaseType>(
    mentionedData->getName(), tensor
  );
  // NOTE: the constructor of TensorData enteres its location in the
  // data map and destroys the previous content, i.e. mentionedData.
}
// instantiate
template
void Algorithm::allocatedTensorArgument<
  Real<64>, CTF::Tensor<Real<64>>
>(std::string const &name, CTF::Tensor<Real<64>> *tensor);
// TODO: remove specialized tensors (matrix, vector, scalar)
template
void Algorithm::allocatedTensorArgument<
  Real<>, CTF::Matrix<Real<>>
>(std::string const &name, CTF::Matrix<Real<>> *tensor);
template
void Algorithm::allocatedTensorArgument<
  Real<>, CTF::Vector<Real<>>
>(std::string const &name, CTF::Vector<Real<>> *tensor);
template
void Algorithm::allocatedTensorArgument<
  Real<>, CTF::Scalar<Real<>>
>(std::string const &name, CTF::Scalar<Real<>> *tensor);

template
void Algorithm::allocatedTensorArgument<
  Complex<64>, CTF::Tensor<Complex<64>>
>(std::string const &name, CTF::Tensor<Complex<64>> *tensor);
// TODO: remove specialized tensors (matrix, vector, scalar)
template
void Algorithm::allocatedTensorArgument<
  Complex<64>, CTF::Matrix<Complex<64>>
>(std::string const &name, CTF::Matrix<Complex<64>> *tensor);
template
void Algorithm::allocatedTensorArgument<
  Complex<64>, CTF::Vector<Complex<64>>
>(std::string const &name, CTF::Vector<Complex<64>> *tensor);
template
void Algorithm::allocatedTensorArgument<
  Complex<64>, CTF::Scalar<Complex<64>>
>(std::string const &name, CTF::Scalar<Complex<64>> *tensor);

template
void Algorithm::allocatedTensorArgument<
  Real<64>, DryTensor<Real<64>>
>(std::string const &name, DryTensor<Real<64>> *tensor);
// TODO: remove specialized tensors (matrix, vector, scalar)
template
void Algorithm::allocatedTensorArgument<
  Real<64>, DryMatrix<Real<64>>
>(std::string const &name, DryMatrix<Real<64>> *tensor);
template
void Algorithm::allocatedTensorArgument<
  Real<64>, DryVector<Real<64>>
>(std::string const &name, DryVector<Real<64>> *tensor);
template
void Algorithm::allocatedTensorArgument<
  Real<64>, DryScalar<Real<64>>
>(std::string const &name, DryScalar<Real<64>> *tensor);

template
void Algorithm::allocatedTensorArgument<
  Complex<64>, DryTensor<Complex<64>>
>(std::string const &name, DryTensor<Complex<64>> *tensor);
// TODO: remove specialized tensors (matrix, vector, scalar)
template
void Algorithm::allocatedTensorArgument<
  Complex<64>, DryMatrix<Complex<64>>
>(std::string const &name, DryMatrix<Complex<64>> *tensor);
template
void Algorithm::allocatedTensorArgument<
  Complex<64>, DryVector<Complex<64>>
>(std::string const &name, DryVector<Complex<64>> *tensor);
template
void Algorithm::allocatedTensorArgument<
  Complex<64>, DryScalar<Complex<64>>
>(std::string const &name, DryScalar<Complex<64>> *tensor);

#ifndef INTEL_COMPILER
template
void Algorithm::allocatedTensorArgument<
  Real<128>, CTF::Tensor<Real<128>>
>(std::string const &name, CTF::Tensor<Real<128>> *tensor);
template
void Algorithm::allocatedTensorArgument<
  Complex<128>, CTF::Tensor<Complex<128>>
>(std::string const &name, CTF::Tensor<Complex<128>> *tensor);
template
void Algorithm::allocatedTensorArgument<
  Real<128>, DryTensor<Real<128>>
>(std::string const &name, DryTensor<Real<128>> *tensor);
template
void Algorithm::allocatedTensorArgument<
  Complex<128>, DryTensor<Complex<128>>
>(std::string const &name, DryTensor<Complex<128>> *tensor);
#endif


void Algorithm::setRealArgument(std::string const &name, const Real<> value) {
  Data *mentionedData(getArgumentData(name));
  new RealData(mentionedData->getName(), value);
}

void Algorithm::setIntegerArgument(std::string const &name, int const value) {
  Data *mentionedData(getArgumentData(name));
  new IntegerData(mentionedData->getName(), value);
}

AlgorithmFactory::AlgorithmMap *AlgorithmFactory::algorithmMap;

