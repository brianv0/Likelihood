#include "../Likelihood/Function.h"

namespace Likelihood {
/** 
 * @class Gaussian
 *
 * @brief A Gaussian function
 *
 * @author J. Chiang
 *    
 * $Header$
 */
    
class Gaussian : public Function {
public:

   Gaussian(){m_init(0, -2, 1);};
   Gaussian(double Prefactor, double Mean, double Sigma)
      {m_init(Prefactor, Mean, Sigma);};

   virtual double value(double) const;
   virtual double operator()(double x) const {return value(x);};
   virtual double derivByParam(double, const std::string &paramName) const;
   virtual double integral(double xmin, double xmax);

private:

   void m_init(double Prefactor, double Mean, double Sigma);
   double m_erfcc(double x);

};

} // namespace Likelihood

