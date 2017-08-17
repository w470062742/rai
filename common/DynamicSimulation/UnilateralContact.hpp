//
// Created by jhwangbo on 03.03.17.
//

#ifndef RAI_UNILATERALCONTACT_HPP
#define RAI_UNILATERALCONTACT_HPP

#include "Contact.hpp"

namespace RAI {
namespace Dynamics {

class UnilateralContact : public Contact {
 public:
  UnilateralContact(double muIn) {
    mu = muIn;
    mu2 = muIn * muIn;
  }

  double mu;
  double mu2;
};

}
}

#endif //RAI_UNILATERALCONTACT_HPP
