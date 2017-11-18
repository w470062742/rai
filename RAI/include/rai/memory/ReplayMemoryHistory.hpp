//
// Created by joonho on 11/17/17.
//

#ifndef RAI_REPLAYMEMORYTRAJECTORY_HPP
#define RAI_REPLAYMEMORYTRAJECTORY_HPP

// Eigen
#include <Eigen/Dense>
#include <Eigen/Core>
#include <mutex>
#include <algorithm>
#include "raiCommon/utils/RandomNumberGenerator.hpp"
#include "glog/logging.h"
#include "raiCommon/enumeration.hpp"
#include <rai/RAI_Tensor.hpp>
#include "rai/memory/Trajectory.hpp"

namespace rai {
namespace Memory {

template<typename Dtype, int stateDimension, int actionDimension>
class ReplayMemoryHistory {

  typedef rai::Tensor<Dtype, 3> Tensor3D;
  typedef rai::Tensor<Dtype, 2> Tensor2D;
  typedef rai::Tensor<Dtype, 1> Tensor1D;
  typedef Eigen::Matrix<Dtype, 1, Eigen::Dynamic> ScalarBatch;
  typedef Eigen::Matrix<Dtype, stateDimension, 1> State;
  typedef Eigen::Matrix<Dtype, actionDimension, 1> Action;
  typedef rai::Algorithm::history<Dtype, stateDimension, actionDimension> History;

 public:

  ReplayMemoryHistory(unsigned capacity, bool distInfo = false)
      : size_(0), batchIdx_(0), maxlen_(0), distInfo_(distInfo) {
    stateTensor_ = new Tensor3D;
    actionTensor_ = new Tensor3D;
    costTensor_ = new Tensor2D;
    len_ = new Tensor1D({capacity}, 0);
    if (distInfo_) {
      actionNoiseTensor_ = new Tensor3D;
      stdevs_ = new Tensor2D({actionDimension, capacity}, 0);
    }
    termtypes_ = new Tensor1D({capacity}, 0);
    capacity_ = capacity;
  }

  ~ReplayMemoryHistory() {
    delete stateTensor_;
    delete actionTensor_;
    delete costTensor_;
    delete len_;
    delete termtypes_;
    if (distInfo_) {
      delete actionNoiseTensor_;
      delete stdevs_;
    }

  }
  ///with distribution Information
  inline void SaveHistory(Tensor3D &states,
                          Tensor3D &actions,
                          Tensor2D &costs,
                          Tensor1D &lengths,
                          Tensor1D &termTypes,
                          Tensor3D &actionNoises,
                          Action &stdev) {
    LOG_IF(FATAL, !distInfo_) << "distInfo != true";

    int margin = size_ - batchIdx_;
    std::lock_guard<std::mutex> lockModel(memoryMutex_);
    if (states.cols() > maxlen_) {
      maxlen_ = states.cols();
      LOG(INFO) << maxlen_;
      stateTensor_->conservativeResize(stateDimension, maxlen_, capacity_);
      actionTensor_->conservativeResize(actionDimension, maxlen_, capacity_);
      actionNoiseTensor_->conservativeResize(actionDimension, maxlen_, capacity_);
      costTensor_->conservativeResize(maxlen_, capacity_);
    }

    int lengthCheck = 0;
    for (int i = 0; i < states.batches(); i++) {
      ///check trajectory lengths to downsize tensor
      if (lengths[i] > lengthCheck) lengthCheck = lengths[i];
      ///partially fill batch
      std::memcpy(stateTensor_->data() + batchIdx_ * stateDimension * maxlen_,
                  states.batch(i).data(), sizeof(Dtype) * stateDimension * lengths[i]);
      std::memcpy(actionTensor_->data() + batchIdx_ * actionDimension * maxlen_,
                  actions.batch(i).data(), sizeof(Dtype) * actionDimension * lengths[i]);
      std::memcpy(actionNoiseTensor_->data() + batchIdx_ * actionDimension * maxlen_,
                  actionNoises.batch(i).data(), sizeof(Dtype) * actionDimension * lengths[i]);
      costTensor_->block(0, batchIdx_, states.cols(), 1) = costs.col(i);
      stdevs_->col(batchIdx_) = stdev;
      len_->at(batchIdx_) = lengths[i];
      termtypes_->at(batchIdx_) = termTypes[i];

      batchIdx_ = (batchIdx_ + 1) % capacity_;
      size_++;
      size_ = std::min(size_, capacity_);
    }
    if (lengthCheck < maxlen_) {
      maxlen_ = lengthCheck;
      stateTensor_->conservativeResize(stateDimension, maxlen_, capacity_);
      actionTensor_->conservativeResize(actionDimension, maxlen_, capacity_);
      actionNoiseTensor_->conservativeResize(actionDimension, maxlen_, capacity_);
      costTensor_->conservativeResize(maxlen_, capacity_);
    }
//    LOG(INFO) << stateTensor_->dim(0) << " " <<stateTensor_->dim(1) << " " <<stateTensor_->dim(2);
//    std::cout << stateTensor_->eTensor() << std::endl<< std::endl;
//    std::cout << actions << std::endl;
//    std::cout << actionTensor_->eTensor() << std::endl<< std::endl;
//    std::cout << costs<<std::endl;
//    std::cout << costTensor_->eTensor() << std::endl;
  }

  inline void SaveHistory(Tensor3D &states,
                          Tensor3D &actions,
                          Tensor2D &costs,
                          Tensor1D &lengths,
                          Tensor1D &termTypes) {
    int margin = size_ - batchIdx_;
    std::lock_guard<std::mutex> lockModel(memoryMutex_);
    if (states.cols() > maxlen_) {
      maxlen_ = states.cols();
      stateTensor_->conservativeResize(stateDimension, maxlen_, capacity_);
      actionTensor_->conservativeResize(actionDimension, maxlen_, capacity_);
      costTensor_->conservativeResize(maxlen_, capacity_);
    }

    int lengthCheck = 0;
    for (int i = 0; i < states.batches(); i++) {
      ///check trajectory lengths to downsize tensor
      if (lengths[i] > lengthCheck) lengthCheck = lengths[i];
      ///partially fill batch
      std::memcpy(stateTensor_->data() + batchIdx_ * stateDimension * maxlen_,
                  states.batch(i).data(), sizeof(Dtype) * stateDimension * lengths[i]);
      std::memcpy(actionTensor_->data() + batchIdx_ * actionDimension * maxlen_,
                  actions.batch(i).data(), sizeof(Dtype) * actionDimension * lengths[i]);
      costTensor_->block(0, batchIdx_, states.cols(), 1) = costs.col(i);
      len_->at(batchIdx_) = lengths[i];
      termtypes_->at(batchIdx_) = termTypes[i];

      batchIdx_ = (batchIdx_ + 1) % capacity_;
      size_++;
      size_ = std::min(size_, capacity_);
    }
    if (lengthCheck < maxlen_) {
      maxlen_ = lengthCheck;
      stateTensor_->conservativeResize(stateDimension, maxlen_, capacity_);
      actionTensor_->conservativeResize(actionDimension, maxlen_, capacity_);
      costTensor_->conservativeResize(maxlen_, capacity_);
    }
  }
  inline void sampleRandomHistory(History &historyOut, const int batchSize) {

    unsigned memoryIdx[batchSize];
    ///// randomly sampling memory indeces
    for (unsigned i = 0; i < batchSize; i++) {
      memoryIdx[i] = rn_.intRand(0, size_ - 1);
      for (unsigned j = 0; j < i; j++) {
        if (memoryIdx[i] == memoryIdx[j]) {
          i--;
          break;
        }
      }
    }

    /// resize
    historyOut.resize(maxlen_,batchSize);

    /// saving memory to Batch
    for (unsigned i = 0; i < batchSize; i++) {
      historyOut.states.batch(i) = stateTensor_->batch(memoryIdx[i]);
      historyOut.actions.batch(i) = actionTensor_->batch(memoryIdx[i]);
      historyOut.costs.col(i) = costTensor_->col(memoryIdx[i]);
      historyOut.lengths[i] = len_->at(i);
      historyOut.termtypes[i] = termtypes_->at(i);

      if(distInfo_){
        historyOut.actionNoises.batch(i) = actionNoiseTensor_->batch(memoryIdx[i]);
        historyOut.stdevs.col(i) = stdevs_->col(i);
      }
    }
  }

//
  inline void clearTheMemoryAndSetNewBatchSize(int newMemorySize) {
    std::lock_guard<std::mutex> lockModel(memoryMutex_);
    size_ = 0;
    batchIdx_ = 0;
    capacity_ = newMemorySize;

    delete stateTensor_;
    delete actionTensor_;
    delete costTensor_;
    delete len_;
    delete termtypes_;
    if (distInfo_) {
      delete actionNoiseTensor_;
      delete stdevs_;
    }

    stateTensor_ = new Tensor3D;
    actionTensor_ = new Tensor3D;
    costTensor_ = new Tensor2D;
    len_ = new Tensor1D({capacity_}, 0);
    if (distInfo_) {
      actionNoiseTensor_ = new Tensor3D;
      stdevs_ = new Tensor2D({actionDimension, capacity_}, 0);
    }
    termtypes_ = new Tensor1D({capacity_}, 0);
  }


  unsigned getCapacity() {
    return capacity_;
  }

  unsigned getSize() {
    return size_;
  }

 private:
  Tensor3D *stateTensor_;
  Tensor3D *actionTensor_;
  Tensor3D *actionNoiseTensor_;
  Tensor2D *costTensor_;
  Tensor2D *stdevs_;
  Tensor1D *len_;
  Tensor1D *termtypes_;

  bool distInfo_;
  unsigned size_;
  unsigned maxlen_;
  unsigned batchIdx_;
  unsigned capacity_;
  static std::mutex memoryMutex_;
  RandomNumberGenerator <Dtype> rn_;
};
}
}

template<typename Dtype, int stateDimension, int actionDimension>
std::mutex rai::Memory::ReplayMemoryHistory<Dtype, stateDimension, actionDimension>::memoryMutex_;

#endif //RAI_REPLAYMEMORYTRAJECTORY_HPP
