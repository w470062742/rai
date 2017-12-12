//
// Created by joonho on 12/1/17.
//

#ifndef RAI_RECURRENTSTOCHASTICPOLICYVALUE_TENSORFLOW_HPP
#define RAI_RECURRENTSTOCHASTICPOLICYVALUE_TENSORFLOW_HPP

#include <rai/function/common/StochasticPolicy.hpp>
#include <rai/function/common/ValueFunction.hpp>

#include "common/RecurrentParametrizedFunction_TensorFlow.hpp"
#include "rai/common/VectorHelper.hpp"

namespace rai {
namespace FuncApprox {

template<typename Dtype, int stateDim, int actionDim>
class RecurrentStochasticPolicyValue_Tensorflow : public virtual StochasticPolicy<Dtype, stateDim, actionDim>,
                                                  public virtual ValueFunction<Dtype, stateDim>,
                                                  public virtual RecurrentParameterizedFunction_TensorFlow<Dtype,
                                                                                             stateDim,
                                                                                             actionDim> {
 public:
  typedef Eigen::Matrix<Dtype, -1, 1> VectorXD;
  typedef Eigen::Matrix<Dtype, -1, -1> MatrixXD;
  using PolicyBase = StochasticPolicy<Dtype, stateDim, actionDim>;
  using Pfunction_tensorflow = RecurrentParameterizedFunction_TensorFlow<Dtype, stateDim, actionDim>;

  using Pfunction_tensorflow::h;
  using Pfunction_tensorflow::hdim;
/// To avoid ambiguity
  using Pfunction_tensorflow::getLearningRate;
  using Pfunction_tensorflow::setLearningRate;
  using Pfunction_tensorflow::dumpParam;
  using Pfunction_tensorflow::loadParam;
  using Pfunction_tensorflow::copyStructureFrom;
  using Pfunction_tensorflow::copyLPFrom;
  using Pfunction_tensorflow::copyAPFrom;
  using Pfunction_tensorflow::interpolateLPWith;
  using Pfunction_tensorflow::interpolateAPWith;
  using Pfunction_tensorflow::getLPSize;
  using Pfunction_tensorflow::getAPSize;
  using Pfunction_tensorflow::getLP;
  using Pfunction_tensorflow::getAP;
  using Pfunction_tensorflow::setLP;
  using Pfunction_tensorflow::setAP;

  typedef typename PolicyBase::State State;
  typedef typename PolicyBase::StateBatch StateBatch;
  typedef typename PolicyBase::Action Action;
  typedef typename PolicyBase::ActionBatch ActionBatch;
  typedef typename PolicyBase::Gradient Gradient;
  typedef typename PolicyBase::Jacobian Jacobian;
  typedef typename PolicyBase::Tensor1D Tensor1D;
  typedef typename PolicyBase::Tensor2D Tensor2D;
  typedef typename PolicyBase::Tensor3D Tensor3D;
  typedef typename PolicyBase::Dataset Dataset;

  RecurrentStochasticPolicyValue_Tensorflow(std::string pathToGraphDefProtobuf, Dtype learningRate = 1e-3) :
      Pfunction_tensorflow::RecurrentParameterizedFunction_TensorFlow(pathToGraphDefProtobuf, learningRate) {
  }

  RecurrentStochasticPolicyValue_Tensorflow(std::string computeMode,
                                       std::string graphName,
                                       std::string graphParam,
                                       Dtype learningRate = 1e-3) :
      Pfunction_tensorflow::RecurrentParameterizedFunction_TensorFlow("RecurrentStochasticPolicyValue", computeMode, graphName, graphParam, learningRate) {

  }

  void getdistribution(StateBatch &states, ActionBatch &means, Action &stdev) {
    std::vector<MatrixXD> vectorOfOutputs;
    this->tf_->run({{"state", states}}, {"action", "stdev"}, {}, vectorOfOutputs);
    means = vectorOfOutputs[0];
    stdev = vectorOfOutputs[1].col(0);
  }

  ///PPO
  virtual void PPOpg(Dataset *minibatch,
                     Tensor2D & predictedvalue,
                     Action &Stdev,
                     VectorXD &grad) {
    std::vector<tensorflow::Tensor> vectorOfOutputs;
    Tensor1D StdevT(Stdev, {Stdev.rows()}, "stdv_o");
    Tensor2D hiddenState({hdim, minibatch->states.batches()}, "h_init");
    hiddenState = minibatch->hiddenStates.col(0);

    this->tf_->run({minibatch->states,
                    minibatch->actions,
                    minibatch->actionNoises,
                    minibatch->advantages,
                    minibatch->lengths,
                    minibatch->values,
                    predictedvalue,
                    hiddenState, StdevT},
                   {"Algo/RPPO/Pg"},
                   {},
                   vectorOfOutputs);
    std::memcpy(grad.data(), vectorOfOutputs[0].flat<Dtype>().data(), sizeof(Dtype) * grad.size());
  }

  virtual Dtype PPOgetkl(Dataset *minibatch,
                         Action &Stdev) {
    std::vector<tensorflow::Tensor> vectorOfOutputs;
    Tensor1D StdevT(Stdev, {Stdev.rows()}, "stdv_o");
    Tensor2D hiddenState({hdim,  minibatch->states.batches()}, "h_init");
    hiddenState = minibatch->hiddenStates.col(0);

    this->tf_->run({minibatch->states,
                    minibatch->actions,
                    minibatch->actionNoises,
                    minibatch->lengths,
                    hiddenState, StdevT},
                   {"Algo/RPPO/kl_mean"},
                   {},
                   vectorOfOutputs);
    return vectorOfOutputs[0].flat<Dtype>().data()[0];
  }
  virtual void test(Dataset *minibatch,
                    Action &Stdev) {
    std::vector<tensorflow::Tensor> dummy;

    Tensor1D StdevT(Stdev, {Stdev.rows()}, "stdv_o");
    Tensor2D hiddenState({hdim,  minibatch->states.batches()}, "h_init");
    hiddenState = minibatch->hiddenStates.col(0);
    MatrixXD test;
    this->tf_->run({minibatch->states,
                    minibatch->actions,
                    minibatch->actionNoises,
                    minibatch->lengths,
                    minibatch->advantages,
                    hiddenState, StdevT},
                   {"Algo/RPPO/test"},
                   {},
                   dummy);
    test.resize(1, dummy[0].dim_size(0) + dummy[0].dim_size(1));

    std::memcpy(test.data(), dummy[0].template flat<Dtype>().data(), sizeof(Dtype) * test.size());
    std::cout << "test" <<std::endl<< test << std::endl;
  }

  virtual void setStdev(const Action &Stdev) {
    std::vector<MatrixXD> dummy;
    this->tf_->run({{"Stdev_placeholder", Stdev}}, {}, {"assignStdev"}, dummy);
  }

  virtual void getStdev(Action &Stdev) {
    std::vector<MatrixXD> vectorOfOutputs;
    this->tf_->run({{"Stdev_placeholder", Stdev}}, {"getStdev"}, {}, vectorOfOutputs);
    Stdev = vectorOfOutputs[0];
  }

  virtual void setParams(const VectorXD params) {
    std::vector<MatrixXD> dummy;
    this->tf_->run({{"PPO_params_placeholder", params}}, {}, {"PPO_param_assign_ops"}, dummy);
  }

  virtual void forward(State &state, Action &action) {
    std::vector<MatrixXD> vectorOfOutputs;
    MatrixXD h_, length;
    length.resize(1,1);
    if (h.cols() != 1) {
      h_.resize(hdim, 1);
      h.setZero();
    }
    h_ = h.eMat();

    this->tf_->forward({{"state", state},
                        {"length", length},
                        {"h_init", h_}},
                       {"action",  "h_state"}, vectorOfOutputs);
    action = vectorOfOutputs[0];
    std::memcpy(action.data(), vectorOfOutputs[0].data(), sizeof(Dtype) * action.size());
    h.copyDataFrom(vectorOfOutputs[1]);
  }

  virtual void forward(StateBatch &states, ActionBatch &actions) {
    std::vector<tensorflow::Tensor> vectorOfOutputs;
    Tensor3D stateT({stateDim, 1, states.cols()}, "state");
    Tensor1D len({states.cols()}, 1, "length");
    stateT.copyDataFrom(states);

    if (h.cols() != states.cols()) {
      h.resize(hdim, states.cols());
      h.setZero();
    }

    this->tf_->run({stateT, h, len}, {"action", "h_state"}, {}, vectorOfOutputs);
    std::memcpy(actions.data(), vectorOfOutputs[0].flat<Dtype>().data(), sizeof(Dtype) * actions.size());
    h.copyDataFrom(vectorOfOutputs[1]);
  }

  virtual void trainUsingGrad(const VectorXD &grad) {
    std::vector<MatrixXD> dummy;
    this->tf_->run({{"trainUsingGrad/Inputgradient", grad},
                    {"trainUsingGrad/learningRate", this->learningRate_}}, {},
                   {"trainUsingGrad/applyGradients"}, dummy);
  }

  virtual Dtype performOneSolverIter(StateBatch &states, ActionBatch &actions) {
    LOG(FATAL) << "Not implemented";
    return 0;
  }
};
}//namespace FuncApprox
}//namespace rai
#endif //RAI_RECURRENTSTOCHASTICPOLICYVALUE_TENSORFLOW_HPP
