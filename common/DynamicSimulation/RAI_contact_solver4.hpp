//
// Created by jhwangbo on 21/07/17.
//

#ifndef RAI_RAI_CONTACT_SOLVER4_HPP
#define RAI_RAI_CONTACT_SOLVER4_HPP

#include <vector>
#include <stdlib.h>
#include <iostream>
#include <boost/bind.hpp>
#include <math/RAI_math.hpp>
#include <math.h>
#include "Eigen/Core"
#include "UnilateralContact.hpp"
#include "math/inverseUsingCholesky.hpp"
#include "math/RandomNumberGenerator.hpp"
#include "math/GoldenSectionMethod.hpp"
#include "math/RAI_math.hpp"

namespace RAI {
namespace Dynamics {

class RAI_contact_solver4 {

  typedef Eigen::Vector3d Vector3d;
  typedef Eigen::Vector2d Vector2d;
  typedef Eigen::Matrix3d Matrix3d;
  typedef Eigen::VectorXd VectorXd;

 public:
  RAI_contact_solver4() { eye3.setIdentity(); }
  ~RAI_contact_solver4() {}

  void solve(std::vector<UnilateralContact *> &uniContacts,
             const MatrixXd &M_inv, const VectorXd &tauStar) {

    unsigned contactN = uniContacts.size();
    unsigned stateDim = M_inv.cols();
    alpha = 0.8;

    std::vector<Vector3d> c(contactN);
    std::vector<Vector3d> oldImpulseUpdate(contactN, Vector3d::Zero());
    std::vector<Vector3d> impulseUpdate(contactN);
    std::vector<Vector3d> newImpulse(contactN);

    inertia.resize(contactN);
    inertiaInv.resize(contactN);

    for (unsigned i = 0; i < contactN; i++) {
      inertiaInv[i].resize(contactN);
      c[i] = uniContacts[i]->jaco * tauStar;
      for (unsigned j = 0; j < contactN; j++)
        inertiaInv[i][j] = uniContacts[i]->jaco * (M_inv * uniContacts[j]->jaco.transpose());
      cholInv(inertiaInv[i][i], inertia[i]);
//      inertia[i] = inertiaInv[i][i].llt().solve(eye3);
    }

    int counterToBreak = 0;
    double error;

    while (true) {
      error = 0.0;

      for (unsigned i = 0; i < contactN; i++) {
        /// initially compute velcoties
        rest = c[i];
        for (unsigned j = 0; j < contactN; j++)
          if (i!=j) rest += inertiaInv[i][j] * uniContacts[j]->impulse;

        /// if it is on ground
        if (rest(2) < 0) {
          uniContacts[i]->vel = inertiaInv[i][i] * uniContacts[i]->impulse + rest;
          new_z_vel = uniContacts[i]->vel(2) * (1.0 - alpha);
          newVel(2) = new_z_vel;
          newVel.head(2).setZero();
          impulseUpdate[i] = -uniContacts[i]->impulse;
          zeroMotionImpulse = inertia[i] * (newVel - rest);
          ftan = zeroMotionImpulse.head(2).norm();

          if (zeroMotionImpulse(2) * uniContacts[i]->mu > ftan) {
            /// if no slip take a step toward the non-slip impulse
            newVel.head(2) = uniContacts[i]->vel.head(2) * (1.0 - alpha);
            uniContacts[i]->impulse = inertia[i] * (newVel - rest);
          } else {
            uniContacts[i]->vel(2) = new_z_vel;
            uniContacts[i]->impulse = inertia[i] * (uniContacts[i]->vel - rest);
            /// if it is going to slip
            ftan = uniContacts[i]->impulse.head(2).norm();
            uniContacts[i]->vel = inertiaInv[i][i] * uniContacts[i]->impulse + rest;
            if (uniContacts[i]->impulse(2) * uniContacts[i]->mu < ftan)
              projectToFeasibleSet2(uniContacts[i], rest, new_z_vel, i, ftan);
            uniContacts[i]->vel = inertiaInv[i][i] * uniContacts[i]->impulse + rest;
            direction.head(2) = uniContacts[i]->vel.head(2);
            direction(2) = (inertiaInv[i][i](2,0) * direction(0) + inertiaInv[i][i](2,1) * direction(1)) / -inertiaInv[i][i](2,2);
            direction /= -direction.norm();
            Vector3d vel_direction(inertiaInv[i][i] * direction);

            double current_angle = std::acos(uniContacts[i]->vel.head(2).dot(uniContacts[i]->impulse.head(2)) / uniContacts[i]->vel.head(2).norm() / uniContacts[i]->impulse.head(2).norm());
            if(std::fabs(current_angle-M_PI) > 1e-6) {
              double beta_1 = uniContacts[i]->impulse.head(2).norm() / std::sin(current_angle * (1.0 - alpha * 0.5))
                  * std::sin(current_angle * (alpha * 0.5));
              double angle_v_vp = std::acos(
                  uniContacts[i]->vel.head(2).dot(vel_direction.head(2)) / uniContacts[i]->vel.head(2).norm()
                      / vel_direction.head(2).norm());
              double beta_2 = uniContacts[i]->vel.head(2).norm() / std::sin(angle_v_vp - current_angle * (alpha * 0.5))
                  * std::sin(angle_v_vp) / vel_direction.norm();
//              std::cout << "current_angle " << current_angle << std::endl;
//              std::cout << "beta " << std::min(beta_1, beta_2) << std::endl;
              uniContacts[i]->impulse += std::min(beta_1, beta_2) * direction;
              projectToFeasibleSet2(uniContacts[i], rest, new_z_vel, i, uniContacts[i]->impulse.head(2).norm());
            }
          }
          uniContacts[i]->vel = inertiaInv[i][i] * uniContacts[i]->impulse + rest;
          impulseUpdate[i] += uniContacts[i]->impulse;
          velError = std::min(uniContacts[i]->vel(2), 0.0);
          error += impulseUpdate[i].squaredNorm() + velError * velError;
        } else {
          uniContacts[i]->impulse = (1.0-alpha) * uniContacts[i]->impulse;
          error += (uniContacts[i]->impulse / (alpha * (1.0-alpha))).squaredNorm();
        }
      }

      if (error < 1e-10) {
//        std::cout<<counterToBreak<<std::endl;
        break;
      }
      if (++counterToBreak > 1000) {
        if(error > 1e-5) {
          std::cout << "the Contact dynamic solver did not converge in time, the error is " << error << std::endl;
          std::cout << "the Contact force is " << uniContacts[0]->impulse.transpose() << std::endl;
        }
        break;
      }

      alpha = (alpha - alpha_low) * alpha_decay + alpha_low;
    }

  }

 private:

  inline void projectToFeasibleSet2(UnilateralContact *contact,
                                    Vector3d &rest,
                                    double new_z_vel,
                                    int contactID,
                                    double ftan) {
    double tanTheta = -((inertiaInv[contactID][contactID](2, 0) * contact->impulse(0)
        + inertiaInv[contactID][contactID](2, 1) * contact->impulse(1)) / ftan)
        / inertiaInv[contactID][contactID](2, 2);
    double f0 = contact->impulse(2) - tanTheta * ftan;
    double newFtan = f0 / (1/contact->mu - tanTheta);
    contact->impulse(2) = f0 + tanTheta * newFtan;
    contact->impulse.head(2) *= newFtan / ftan;
  }

  inline bool signOf(const Vector3d &v1, const Vector3d &v2) {
    return signbit(v1(0)*v2(1)) - (v1(1)*v2(0));
  }

  Eigen::Matrix3d eye3;
  std::vector<std::vector<Matrix3d> > inertiaInv;
  std::vector<Matrix3d> inertia;
  Vector3d rest, newVel;
  double new_z_vel;
  double alpha;
  double alpha_low = 0.1;
  double alpha_decay = 0.99;
  double ex;
  double ey;
  double w0exPw1ey;
  double ftanProj;
  Vector3d zeroMotionImpulse;
  double velError;
  double ftan;

  /// slippage related
  Vector3d direction;
  Vector3d impulseCorrection;
  Vector3d impulseBeforeCorrection;
  double Ek;
  double quad_a;
  double quad_b;
  double quad_c;
  double quad_determinant;
  double correctionMagnitude;
  double fvMisalignment_0;
  bool sign_0;

};

}
}
#endif //RAI_RAI_CONTACT_SOLVER4_HPP
