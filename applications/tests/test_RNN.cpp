//
// Created by joonho on 11/12/17.
//


#include <rai/RAI_core>

#include <iostream>
#include "rai/function/tensorflow/common/ParameterizedFunction_TensorFlow.hpp"
#include "rai/function/tensorflow/Qfunction_TensorFlow.hpp"
#include "rai/function/tensorflow/DeterministicPolicy_TensorFlow.hpp"
#include "rai/function/tensorflow/StochasticPolicy_TensorFlow.hpp"
#include "rai/function/tensorflow/RecurrentQfunction_TensorFlow.hpp"

#include "rai/function/tensorflow/ValueFunction_TensorFlow.hpp"
#include "rai/function/common/Policy.hpp"
#include <math.h>
#include <boost/random.hpp>
#include <boost/random/normal_distribution.hpp>
#include <functional>
#include <rai/function/tensorflow/RecurrentStochasticPolicy_TensorFlow.hpp>
#include "rai/noiseModel/NormalDistributionNoise.hpp"

#include "rai/RAI_core"
#include "rai/RAI_Tensor.hpp"

#include "rai/experienceAcquisitor/TrajectoryAcquisitor_MultiThreadBatch.hpp"
#include "rai/tasks/poleBalancing/PoleBalancing.hpp"
#include <rai/algorithm/common/LearningData.hpp>
#include <rai/algorithm/common/dataStruct.hpp>

#include <vector>
#include "rai/memory/ReplayMemoryHistory.hpp"

using std::cout;
using std::endl;
using std::cin;
using rai::Task::ActionDim;
using rai::Task::StateDim;

using Dtype = float;

using PolicyBase = rai::FuncApprox::Policy<Dtype, StateDim, ActionDim>;
using RnnQfunc = rai::FuncApprox::RecurrentQfunction_TensorFlow<Dtype, StateDim, ActionDim>;

using RnnPolicy = rai::FuncApprox::RecurrentStochasticPolicy_TensorFlow<Dtype, StateDim, ActionDim>;
using Acquisitor_ = rai::ExpAcq::TrajectoryAcquisitor_MultiThreadBatch<Dtype, StateDim, ActionDim>;
using Task_ = rai::Task::PoleBalancing<Dtype>;

using MatrixXD = Eigen::Matrix<Dtype, -1, -1>;
using VectorXD = Eigen::Matrix<Dtype, -1, 1>;
using Advantages = Eigen::Matrix<Dtype, 1, Eigen::Dynamic>;
typedef typename PolicyBase::State State;
typedef typename PolicyBase::StateBatch StateBatch;
typedef typename PolicyBase::Action Action;
typedef typename PolicyBase::ActionBatch ActionBatch;
typedef typename PolicyBase::JacobianWRTparam JacobianWRTparam;
using NormNoise = rai::Noise::NormalDistributionNoise<Dtype, ActionDim>;
using NoiseCov = Eigen::Matrix<Dtype, ActionDim, ActionDim>;
using Noise = rai::Noise::Noise<Dtype, ActionDim>;
using TensorBatchBase = rai::Algorithm::TensorBatch<Dtype>;
using TensorBatch = rai::Algorithm::history<Dtype, StateDim, ActionDim>;
using namespace rai;

int main() {
  RAI_init();
  bool teststdev = false;
  bool testgradient = false;
  const int sampleN = 5;
  int Batsize = 100;
  int len = 100;
  Acquisitor_ acquisitor;
  rai::Algorithm::LearningData<Dtype, StateDim, ActionDim> ld_(&acquisitor);
  Task_ task;
  task.setTimeLimitPerEpisode(0.2);

  NoiseCov covariance = NoiseCov::Identity();
  NormNoise noise(covariance);

  RnnPolicy policy("cpu", "GRUMLP", "tanh 3 5 / 8 1", 0.001);

  rai::Tensor<Dtype, 3> states;
  states.resize(StateDim, len, Batsize);
  states.setZero();

  std::vector<rai::Task::Task<Dtype, StateDim, ActionDim, 0> *> taskVector = {&task};
  std::vector<rai::Noise::Noise<Dtype, ActionDim> *> noiseVector = {&noise};
  ////
  ld_.acquireTrajForNTimeSteps(taskVector, noiseVector, &policy, 50);

  LOG(INFO) << ld_.Data.states.cols() << ", " << ld_.Data.states.batches();
  std::cout << ld_.Data.states << std::endl << std::endl;
  rai::Memory::ReplayMemoryHistory<Dtype, StateDim, ActionDim> memory(20);

  for (int i = 0; i < 3; i++)
    memory.SaveHistory(ld_.Data.states, ld_.Data.actions, ld_.Data.costs, ld_.Data.lengths, ld_.Data.termtypes);

  TensorBatch test_bat(3, 4);
  TensorBatch test_minibat;
  TensorBatchBase *test_base;

    RnnQfunc Qfunction("cpu", "GRUMLP2", "tanh 3 1 5 / 8 1", 0.001);

};