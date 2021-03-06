/** 
 * @file LogGaussian.cxx
 * @brief Implementation for the LogGaussian Function class
 * @author J. Chiang
 *
 * $Header$
 */

#include <cmath>

#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#include "optimizers/dArg.h"
#include "optimizers/ParameterNotFound.h"

#include "Likelihood/LogGaussian.h"

namespace Likelihood {

LogGaussian::LogGaussian(double norm, double mean, double sigma) 
   : optimizers::Function("LogGaussian", 3, "Norm"),
     m_log_term(std::log(std::sqrt(2.*M_PI)*sigma)) {
   addParam("Norm", norm, true);
   addParam("Mean", mean, true);
   addParam("Sigma", sigma, true);
}

double LogGaussian::value(const optimizers::Arg & xarg) const {
   double x = dynamic_cast<const optimizers::dArg &>(xarg).getValue();
   enum ParamTypes {Norm, Mean, Sigma};
   double norm = m_parameter[Norm].getTrueValue();
   double mean = m_parameter[Mean].getTrueValue();
   double sigma = m_parameter[Sigma].getTrueValue();
   return norm*(-(x - mean)*(x - mean)/2./sigma/sigma - m_log_term);
}

double LogGaussian::
derivByParamImp(const optimizers::Arg & xarg,
                const std::string & paramName) const {
   double x = dynamic_cast<const optimizers::dArg &>(xarg).getValue();

   int iparam(-1);
   for (unsigned int i = 0; i < m_parameter.size(); i++) {
      if (paramName == m_parameter[i].getName()) {
         iparam = i;
	 break;
      }
   }

   if (iparam == -1) {
      throw optimizers::ParameterNotFound(paramName, getName(),
                                          "PowerLaw2::derivByParam");
   }
   
   enum ParamTypes {Norm, Mean, Sigma};

   double norm = m_parameter[Norm].getTrueValue();
   double mean = m_parameter[Mean].getTrueValue();
   double sigma = m_parameter[Sigma].getTrueValue();
   double s3 = sigma*sigma*sigma;

   switch (iparam) {
   case Norm:
      return value(xarg)/norm;
      break;
   case Mean:
      return -norm*mean/sigma/sigma;
      break;
   case Sigma:
      return norm*((x - mean)*(x - mean)/6./s3 - 1./sigma);
      break;
   default:
      break;
   }
   return 0;
}

double LogGaussian::derivative(const optimizers::Arg & xarg) const {
   double x = dynamic_cast<const optimizers::dArg &>(xarg).getValue();
   enum ParamTypes {Norm, Mean, Sigma};
   double norm = m_parameter[Norm].getTrueValue();
   double mean = m_parameter[Mean].getTrueValue();
   double sigma = m_parameter[Sigma].getTrueValue();
   return norm*(-(x - mean)/sigma/sigma);
}

} // namespace Likelihood
