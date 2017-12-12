//
// Created by joonho on 11/21/17.
//

#ifndef RAI_RECURRENTDETERMINISTICPOLICY_HPP
#define RAI_RECURRENTDETERMINISTICPOLICY_HPP

#include "rai/function/common/DeterministicPolicy.hpp"
#include "common/RecurrentParametrizedFunction_TensorFlow.hpp"
#include "RecurrentQfunction_TensorFlow.hpp"

#pragma once

namespace rai {
namespace FuncApprox {

template<typename Dtype, int stateDim, int actionDim>
class RecurrentDeterministicPolicy_TensorFlow : public virtual DeterministicPolicy<Dtype, stateDim, actionDim>,
                                                public virtual RecurrentParameterizedFunction_TensorFlow<Dtype,
                                                                                                stateDim,
                                                                                                actionDim> {

 public:
  typedef Eigen::Matrix<Dtype, -1, 1> VectorXD;
  typedef Eigen::Matrix<Dtype, -1, -1> MatrixXD;
  using PolicyBase = Policy<Dtype, stateDim, actionDim>;
  using Pfunction_tensorflow = RecurrentParameterizedFunction_TensorFlow<Dtype, stateDim, actionDim>;
  using Qfunction_tensorflow = RecurrentQfunction_TensorFlow<Dtype, stateDim, actionDim>;
  using Qfunction_ = Qfunction<Dtype, stateDim, actionDim>;

  using Pfunction_tensorflow::h;
  using Pfunction_tensorflow::hdim;

  typedef typename PolicyBase::State State;
  typedef typename PolicyBase::StateBatch StateBatch;
  typedef typename PolicyBase::Action Action;
  typedef typename PolicyBase::ActionBatch ActionBatch;
  typedef typename PolicyBase::Gradient Gradient;
  typedef typename PolicyBase::Jacobian Jacobian;
  typedef typename PolicyBase::JacobianWRTparam JacobianWRTparam;

  typedef typename PolicyBase::Tensor1D Tensor1D;
  typedef typename PolicyBase::Tensor2D Tensor2D;
  typedef typename PolicyBase::Tensor3D Tensor3D;
  typedef typename PolicyBase::Dataset Dataset;

  RecurrentDeterministicPolicy_TensorFlow(std::string pathToGraphDefProtobuf, Dtype learningRate = 1e-3) :
      Pfunction_tensorflow::RecurrentParameterizedFunction_TensorFlow(pathToGraphDefProtobuf, learningRate) {
  }

  RecurrentDeterministicPolicy_TensorFlow(std::string computeMode,
                                          std::string graphName,
                                          std::string graphParam,
                                          Dtype learningRate = 1e-3) :
      Pfunction_tensorflow::RecurrentParameterizedFunction_TensorFlow(
          "RecurrentDeterministicPolicy", computeMode, graphName, graphParam, learningRate) {
  }

  virtual void forward(State &state, Action &action) {
    std::vector<MatrixXD> vectorOfOutputs;
    MatrixXD h_, length;
    length.resize(1, 1);
    if (h.cols() != 1) {
      h_.resize(hdim, 1);
      h.setZero();
    }
    h_ = h.eMat();

    this->tf_->forward({{"state", state},
                        {"length", length},
                        {"h_init", h_}},
                       {"action"}, vectorOfOutputs);
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

  virtual Dtype performOneSolverIter(Dataset *minibatch, Tensor3D &actions) {
    std::vector<MatrixXD> vectorOfOutputs;
    actions = "targetAction";
    Tensor1D lr({1}, this->learningRate_(0), "trainUsingTargetQValue/learningRate");

    if (h.cols() != minibatch->batchNum) h.resize(hdim, minibatch->batchNum);
    h = minibatch->hiddenStates.col(0);

    this->tf_->run({minibatch->states, minibatch->lengths, actions, h, lr},
                   {"trainUsingTargetAction/loss"},
                   {"trainUsingTargetAction/solver"}, vectorOfOutputs);

    return vectorOfOutputs[0](0);
  };

  Dtype backwardUsingCritic(Qfunction_tensorflow *qFunction, Dataset *minibatch) {
    std::vector<MatrixXD> dummy;
    Tensor3D gradients("trainUsingCritic/gradientFromCritic");
    Tensor1D lr({1}, this->learningRate_(0), "trainUsingCritic/learningRate");
    Tensor2D hiddenState({hdim, minibatch->batchNum}, "h_init");
    hiddenState = minibatch->hiddenStates.col(0);

    auto pQfunction = dynamic_cast<Qfunction_tensorflow const *>(qFunction);
    LOG_IF(FATAL, pQfunction == nullptr) << "You are mixing two different library types" << std::endl;
    gradients.resize(minibatch->actions.dim());

    forward(minibatch->states, minibatch->actions, hiddenState);
    Dtype averageQ = pQfunction->getGradient_AvgOf_Q_wrt_action(minibatch, gradients);

    this->tf_->run({minibatch->states, minibatch->lengths, hiddenState, gradients, lr}, {"trainUsingCritic/gradnorm"},
                   {"trainUsingCritic/applyGradients"}, dummy);
    return averageQ;
  }
};
} // namespace FuncApprox
} // namespace rai


#endif //RAI_RECURRENTDETERMINISTICPOLICY_HPP
