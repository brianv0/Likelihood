/** 
 * @file RadialDisk.cxx
 * @brief Implementation of Function object class to represent a 2D spatial disk.
 * 
 * @author M. Wood
 *
 * $Header$
 *
 */

#include <algorithm>
#include <iostream>
#include <stdexcept>

#include "facilities/Util.h"

#include "st_stream/StreamFormatter.h"

#include "st_facilities/GaussianQuadrature.h"
#include "st_facilities/Util.h"

#include "astro/SkyDir.h"

#include "optimizers/dArg.h"
#include "optimizers/Function.h"

#include "Likelihood/Event.h"
#include "Likelihood/ExposureMap.h"
#include "Likelihood/MeanPsf.h"
#include "Likelihood/ResponseFunctions.h"
#include "Likelihood/RadialDisk.h"
#include "Likelihood/SkyDirArg.h"

namespace {

  double disk(double x,double sigma) {
    if(x/sigma < 1.0) 
      return std::pow(sigma*sigma*M_PI,-1);
    else 
      return 0.0;
  }
}

namespace Likelihood {

double RadialDisk::RadialIntegrand::operator()(double xp) const {

  const double s2 = std::pow(m_sigma,2);
  double dphi = 2.0*M_PI;
  if((xp+m_x)/m_sigma>1.0)
    dphi = 2.0*std::acos((std::pow(m_x,2)+std::pow(xp,2)-s2)/(2*m_x*xp));

  return xp*m_fn(m_energy,xp)*dphi/(M_PI*s2);
}

double RadialDisk::convolve(const ResponseFunctor& fn, 
			     double energy, double x, 
			     double sigma, double err) {
  
  const double xmin = std::max(x - sigma,0.0);
  const double xmax = x + sigma;
  RadialIntegrand rIntegrand(fn,energy,x,sigma);

  int ierr;
  return st_facilities::GaussianQuadrature::dgaus8(rIntegrand, xmin,
						   xmax, err, ierr);
}


RadialDisk::RadialDisk() : SpatialFunction("RadialDisk",3) {
  m_radius = 1.0;
  addParam("Radius", 1.0, false);
  parameter("Radius").setBounds(0.0, 180.);
}

RadialDisk::RadialDisk(double ra, double dec, double radius) 
  : SpatialFunction("RadialDisk",3,ra,dec) {
  m_radius = radius;
  addParam("Radius", m_radius, false);
  parameter("Radius").setBounds(0.0, 180.);
}

RadialDisk::RadialDisk(const RadialDisk & rhs) 
  : SpatialFunction(rhs), m_radius(rhs.m_radius) {
}

RadialDisk & RadialDisk::operator=(const RadialDisk & rhs) {
   if (this != &rhs) {
      SpatialFunction::operator=(rhs);
      m_radius = rhs.m_radius;
   }
   return *this;
}

RadialDisk::~RadialDisk() {
}

double RadialDisk::value(const astro::SkyDir & dir) const {
   double delta = this->dir().difference(dir)*180./M_PI;
   return disk(delta,m_radius)*std::pow(M_PI/180.,-2);
}

double RadialDisk::value(double delta, double radius) const {
   return disk(delta,radius)*std::pow(M_PI/180.,-2);
}

double RadialDisk::spatialResponse(const astro::SkyDir & dir, double energy, const MeanPsf& psf) const {
  double delta = dir.difference(this->dir())*180./M_PI;
  return RadialDisk::convolve(BinnedResponseFunctor(psf),energy,delta,m_radius);
}

double RadialDisk::spatialResponse(double delta, double energy, const MeanPsf& psf) const {
  return RadialDisk::convolve(BinnedResponseFunctor(psf),energy,delta,m_radius);
}

double RadialDisk::diffuseResponse(const ResponseFunctor& fn, double energy,
				    double separation) const {
  return convolve(fn,energy,separation,m_radius);
}

double RadialDisk::getDiffRespLimits(const astro::SkyDir & dir, 
				      double & mumin, double & mumax,
				      double & phimin, double & phimax) const {
   mumin = std::cos(dir.difference(this->dir()) + 3.*m_radius*M_PI/180.);
   mumax = 1;
   phimin = 0;
   phimax = 2.*M_PI;
}

void RadialDisk::update() {
  SpatialFunction::update();
  double radius = getParam("Radius").getValue();  
  m_radius = radius;
}

double RadialDisk::value(const optimizers::Arg & x) const {
   const SkyDirArg & dir = dynamic_cast<const SkyDirArg &>(x);
   double offset = dir().difference(this->dir())*180./M_PI;
   return value(offset,m_radius);
}

double RadialDisk::derivByParamImp(const optimizers::Arg & x, 
                                      const std::string & parName) const {

   std::ostringstream message;
   message << "RadialDisk: cannot take derivative wrt "
           << "parameter " << parName;
   throw std::runtime_error(message.str());
}

} // namespace Likelihood
