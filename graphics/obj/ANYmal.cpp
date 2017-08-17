//
// Created by jhwangbo on 17. 5. 3.
//
#include <math/RAI_math.hpp>
#include "ANYmal.hpp"
#include "TypeDef.hpp"

namespace RAI {
namespace Graphics {
namespace Obj {

ANYmal::ANYmal():
    base(std::string(getenv("RAI_ROOT")) + "/RAI/taskModules/QuadrupedLocomotion/cadModel/anymal/anymal_base_1_2.dae", 0.001),
    hip_lf(std::string(getenv("RAI_ROOT")) + "/RAI/taskModules/QuadrupedLocomotion/cadModel/anymal/anymal_hip.dae", 0.001),
    hip_rf(std::string(getenv("RAI_ROOT")) + "/RAI/taskModules/QuadrupedLocomotion/cadModel/anymal/anymal_hip.dae", 0.001),
    hip_lh(std::string(getenv("RAI_ROOT")) + "/RAI/taskModules/QuadrupedLocomotion/cadModel/anymal/anymal_hip.dae", 0.001),
    hip_rh(std::string(getenv("RAI_ROOT")) + "/RAI/taskModules/QuadrupedLocomotion/cadModel/anymal/anymal_hip.dae", 0.001),
    thigh_lf(std::string(getenv("RAI_ROOT")) + "/RAI/taskModules/QuadrupedLocomotion/cadModel/anymal/anymal_thigh.dae", 0.001),
    thigh_rf(std::string(getenv("RAI_ROOT")) + "/RAI/taskModules/QuadrupedLocomotion/cadModel/anymal/anymal_thigh.dae", 0.001),
    thigh_lh(std::string(getenv("RAI_ROOT")) + "/RAI/taskModules/QuadrupedLocomotion/cadModel/anymal/anymal_thigh.dae", 0.001),
    thigh_rh(std::string(getenv("RAI_ROOT")) + "/RAI/taskModules/QuadrupedLocomotion/cadModel/anymal/anymal_thigh.dae", 0.001),
    shank_lf(std::string(getenv("RAI_ROOT")) + "/RAI/taskModules/QuadrupedLocomotion/cadModel/anymal/anymal_shank_2_LF.dae", 0.001),
    shank_rf(std::string(getenv("RAI_ROOT")) + "/RAI/taskModules/QuadrupedLocomotion/cadModel/anymal/anymal_shank_2_RF.dae", 0.001),
    shank_lh(std::string(getenv("RAI_ROOT")) + "/RAI/taskModules/QuadrupedLocomotion/cadModel/anymal/anymal_shank_2_LH.dae", 0.001),
    shank_rh(std::string(getenv("RAI_ROOT")) + "/RAI/taskModules/QuadrupedLocomotion/cadModel/anymal/anymal_shank_2_RH.dae", 0.001),
    foot_lf(std::string(getenv("RAI_ROOT")) + "/RAI/taskModules/QuadrupedLocomotion/cadModel/anymal/anymal_optoforce.dae", 0.001),
    foot_rf(std::string(getenv("RAI_ROOT")) + "/RAI/taskModules/QuadrupedLocomotion/cadModel/anymal/anymal_optoforce.dae", 0.001),
    foot_lh(std::string(getenv("RAI_ROOT")) + "/RAI/taskModules/QuadrupedLocomotion/cadModel/anymal/anymal_optoforce.dae", 0.001),
    foot_rh(std::string(getenv("RAI_ROOT")) + "/RAI/taskModules/QuadrupedLocomotion/cadModel/anymal/anymal_optoforce.dae", 0.001){
  objs.push_back(&base);

  objs.push_back(&hip_lf);
  objs.push_back(&thigh_lf);
  objs.push_back(&shank_lf);

  objs.push_back(&hip_rf);
  objs.push_back(&thigh_rf);
  objs.push_back(&shank_rf);

  objs.push_back(&hip_lh);
  objs.push_back(&thigh_lh);
  objs.push_back(&shank_lh);

  objs.push_back(&hip_rh);
  objs.push_back(&thigh_rh);
  objs.push_back(&shank_rh);

  objs.push_back(&foot_lf);
  objs.push_back(&foot_rf);
  objs.push_back(&foot_lh);
  objs.push_back(&foot_rh);

  defaultPose_.resize(17);
  for(auto& pose : defaultPose_)
    pose.setIdentity();

  /// manual adjustment
  /// hip
  RAI::Math::MathFunc::rotateHTabout_y_axis(defaultPose_[1], M_PI*0.5);
  RAI::Math::MathFunc::rotateHTabout_y_axis(defaultPose_[4], M_PI*0.5);
  RAI::Math::MathFunc::rotateHTabout_x_axis(defaultPose_[4], M_PI);
  RAI::Math::MathFunc::rotateHTabout_y_axis(defaultPose_[7], -M_PI*0.5);
  RAI::Math::MathFunc::rotateHTabout_y_axis(defaultPose_[10], -M_PI*0.5);
  RAI::Math::MathFunc::rotateHTabout_x_axis(defaultPose_[10], M_PI);

  /// thigh
  RAI::Math::MathFunc::rotateHTabout_y_axis(defaultPose_[5], 0.5*M_PI);
  RAI::Math::MathFunc::rotateHTabout_x_axis(defaultPose_[5], M_PI);
  RAI::Math::MathFunc::rotateHTabout_y_axis(defaultPose_[5], -0.5*M_PI);
  RAI::Math::MathFunc::rotateHTabout_y_axis(defaultPose_[11], -0.5*M_PI);
  RAI::Math::MathFunc::rotateHTabout_x_axis(defaultPose_[11], M_PI);
  RAI::Math::MathFunc::rotateHTabout_y_axis(defaultPose_[11], 0.5*M_PI);
}

ANYmal::~ANYmal(){}

void ANYmal::init(){
  for(auto* body: objs)
    body->init();
  shader = new Shader_basic;
}

void ANYmal::destroy(){
  for(auto* body: objs)
    body->destroy();
  delete shader;
}

void ANYmal::setPose(std::vector<HomogeneousTransform> &bodyPose) {
  for (int i = 0; i < objs.size(); i++) {
    HomogeneousTransform ht = bodyPose[i] * defaultPose_[i];
    objs[i]->setPose(ht);
  }
}

}
}
}

