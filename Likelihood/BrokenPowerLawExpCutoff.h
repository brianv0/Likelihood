/**
 * @file BrokenPowerLawExpCutoff.h
 * @brief Declaration for the BrokenPowerLawExpCutoff Function class
 * @author Jennifer Carson
 *
 * $Header$
 */

#ifndef Likelihood_BrokenPowerLawExpCutoff_h
#define Likelihood_BrokenPowerLawExpCutoff_h

#include "optimizers/Function.h"
#include "optimizers/Arg.h"

namespace Likelihood {

/**
 * @class BrokenPowerLawExpCutoff
 *
 * @brief Broken Power Law with Exponential Cutoff function
 *
 */

class BrokenPowerLawExpCutoff : public optimizers::Function {

public:

   BrokenPowerLawExpCutoff(double Prefactor=10 , double Index1=-2.1, 
			   double Index2=-2.1, double BreakValue=1000.,
                           double Eabs=10., double P1=150.);

   virtual optimizers::Function * clone() const {
      return new BrokenPowerLawExpCutoff(*this);
   }

protected:

   double value(optimizers::Arg&) const;

   double derivByParamImp(optimizers::Arg & x,
                          const std::string & paramName) const;
};

} // namespace Likelihood

#endif // Likelihood_BrokenPowerLawExpCutoff_h
