/** 
 * @file Event.cxx
 * @brief Event class implementation
 * @author J. Chiang
 *
 * $Header$
 */

#include <cassert>
#include <cctype>

#include <iostream>
#include <fstream>
#include <sstream>
#include <utility>

#include "latResponse/IPsf.h"
#include "latResponse/IAeff.h"
#include "latResponse/Irfs.h"
#include "latResponse/../src/Glast25.h"

#include "Likelihood/ResponseFunctions.h"
#include "Likelihood/Event.h"
#include "Likelihood/RoiCuts.h"
#include "Likelihood/ScData.h"
#include "Likelihood/DiffuseSource.h"
#include "Likelihood/TrapQuad.h"
#include "Likelihood/Exception.h"

namespace {
   double my_acos(double mu) {
      if (mu > 1) {
         return 0;
      } else if (mu < -1) {
         return M_PI;
      } else {
         return acos(mu);
      }
   }
}

namespace Likelihood {

std::vector<double> Event::s_mu;
std::vector<double> Event::s_phi;
FitsImage::EquinoxRotation Event::s_eqRot;
bool Event::s_haveSourceRegionData(false);

Event::Event(double ra, double dec, double energy, 
             double time, double sc_ra, double sc_dec, 
             double muZenith, int type) {
   m_appDir = astro::SkyDir(ra, dec);
   m_energy = energy;
   m_arrTime = time;
   m_scDir = astro::SkyDir(sc_ra, sc_dec);
   m_muZenith = muZenith;
   m_type = type;


   if (ResponseFunctions::useEdisp()) {
// For <15% energy resolution, consider true energies over the range
// (0.55, 1.45)*m_energy, i.e., nominally a >3-sigma range about the
// apparent energy.
      int npts(100);
      double emin = 0.55*m_energy;
      double emax = 1.45*m_energy;
      m_estep = (emax - emin)/(npts-1.);
      m_trueEnergies.reserve(npts);
      for (int i = 0; i < npts; i++) {
         m_trueEnergies.push_back(m_estep*i + emin);
      }
   } else {
// To mimic infinite energy resolution, we create a single element
// vector containing the apparent energy.
      m_trueEnergies.push_back(m_energy);
   }
}

double Event::diffuseResponse(double trueEnergy, 
                              std::string diffuseComponent) const 
   throw(Exception) {

   toLower(diffuseComponent);
   int indx(0);
   if (ResponseFunctions::useEdisp()) {
      indx = static_cast<int>((trueEnergy - m_trueEnergies[0])/m_estep);
      if (indx < 0 || indx >= static_cast<int>(m_trueEnergies.size())) {
         return 0;
      }
   }
   std::map<std::string, diffuse_response>::const_iterator it;
   if ((it = m_respDiffuseSrcs.find(diffuseComponent))
       != m_respDiffuseSrcs.end()) {
      if (ResponseFunctions::useEdisp()) {
         const diffuse_response & resp = it->second;
         double my_value = (trueEnergy - m_trueEnergies[indx])
            /(m_trueEnergies[indx+1] - m_trueEnergies[indx])
            *(resp[indx+1] - resp[indx]) + resp[indx];
         return my_value;
      } else {
// The response is just the single value in the diffuse_response vector.
         return it->second[0];
      }
   } else {
      std::string errorMessage 
         = "Event::diffuseResponse: \nDiffuse component " 
         + diffuseComponent 
         + " does not have an associated diffuse response.\n";
      throw Exception(errorMessage);
   }
   return 0;
}

void Event::computeResponse(std::vector<DiffuseSource *> &srcList, 
                            double sr_radius) {
   std::vector<DiffuseSource *> srcs;
   getNewDiffuseSrcs(srcList, srcs);
   if (srcs.size() == 0) return;
   
// @todo In principle, the source region should be centered on the
// event direction, making it independent of the ROI, but doing so has
// not given as good results as using the ROI center.  Need to check
// this is still true.
   FitsImage::EquinoxRotation eqRot(m_appDir.ra(), m_appDir.dec());
   if (!s_haveSourceRegionData) {
      prepareSrData(sr_radius);
   }

// Create a vector of srcDirs looping over the source region locations.
   std::vector<astro::SkyDir> srcDirs;
   for (unsigned int i = 0; i < s_mu.size(); i++) {
      for (unsigned int j = 0; j < s_phi.size(); j++) {
         astro::SkyDir srcDir;
//         getCelestialDir(s_phi[j], s_mu[i], s_eqRot, srcDir);
         getCelestialDir(s_phi[j], s_mu[i], eqRot, srcDir);
         srcDirs.push_back(srcDir);
      }
   }
   std::vector<double>::iterator trueEnergy = m_trueEnergies.begin();
   for ( ; trueEnergy != m_trueEnergies.end(); ++trueEnergy) {

// Prepare the array of integrals over phi for passing to the 
// trapezoidal integrator for integration over mu.
      std::vector< std::vector<double> > mu_integrands;
      mu_integrands.resize(srcs.size());
      for (unsigned int i = 0; i < s_mu.size(); i++) {

// Prepare phi-integrand arrays.
         std::vector< std::vector<double> > phi_integrands;
         phi_integrands.resize(srcs.size());
         for (unsigned int j = 0; j < s_phi.size(); j++) {
            int indx = i*s_phi.size() + j;
            astro::SkyDir & srcDir = srcDirs[indx];
            double inc = m_scDir.SkyDir::difference(srcDir)*180./M_PI;
            if (inc < latResponse::Glast25::incMax()) {
               double totalResp 
                  = ResponseFunctions::totalResponse(m_arrTime, 
                                                     *trueEnergy, m_energy,
                                                     srcDir, m_appDir, m_type);
               for (unsigned int k = 0; k < srcs.size(); k++) {
                  double srcDist_val = srcs[k]->spatialDist(srcDir);
                  phi_integrands[k].push_back(totalResp*srcDist_val);
               }
            } else {
               for (unsigned int k = 0; k < srcs.size(); k++)
                  phi_integrands[k].push_back(0);
            }
         }
         
// Perform the phi-integrals
         for (unsigned int k = 0; k < srcs.size(); k++) {
            TrapQuad phiQuad(s_phi, phi_integrands[k]);
            mu_integrands[k].push_back(phiQuad.integral());
         }
      }

// Perform the mu-integrals
      for (unsigned int k = 0; k < srcs.size(); k++) {
         TrapQuad muQuad(s_mu, mu_integrands[k]);
         std::string name = srcs[k]->getName();
         toLower(name);
         m_respDiffuseSrcs[name].push_back(muQuad.integral());
      }
   } // loop over trueEnergy
}

void Event::writeDiffuseResponses(const std::string & filename) {
   std::ofstream outfile(filename.c_str());
   std::map<std::string, diffuse_response>::iterator it
      = m_respDiffuseSrcs.begin();
   for ( ; it != m_respDiffuseSrcs.end(); ++it) {
      diffuse_response & resp = it->second;
      for (unsigned int ie = 0; ie < resp.size(); ie++) {
         outfile << m_trueEnergies[ie] << "  "
                 << resp[ie] << std::endl;
      }
   }
   outfile.close();
}

void Event::prepareSrData(double sr_radius, int nmu, int nphi) {
   RoiCuts *roi_cuts = RoiCuts::instance();
   astro::SkyDir roiCenter = roi_cuts->extractionRegion().center();
   s_eqRot = FitsImage::EquinoxRotation(roiCenter.ra(), roiCenter.dec());

   double mumin = cos(sr_radius*M_PI/180);
   double mustep = (1. - mumin)/(nmu - 1.);
   for (int i = 0; i < nmu; i++) {
      s_mu.push_back(mustep*i + mumin);
   }
   double phistep = 2.*M_PI/(nphi - 1.);
   for (int i = 0; i < nphi; i++) {
      s_phi.push_back(phistep*i);
   }
   s_haveSourceRegionData = true;
}

void Event::getCelestialDir(double phi, double mu, 
                            FitsImage::EquinoxRotation &eqRot,
                            astro::SkyDir &dir) {
   double sp = sin(phi);
   double arg = mu/sqrt(1 - (1 - mu*mu)*sp*sp);
   double alpha;
   if (cos(phi) < 0) {
      alpha = 2*M_PI - my_acos(arg);
   } else {
      alpha = my_acos(arg);
   }
   double delta = asin(sqrt(1 - mu*mu)*sp);

// The direction in "Equinox rotated" coordinates
   astro::SkyDir indir(alpha*180/M_PI, delta*180/M_PI);

// Convert to the unrotated coordinate system (should probably use 
// Hep3Vector methods here instead).
   eqRot.do_rotation(indir, dir);
}

void Event::getNewDiffuseSrcs(const std::vector<DiffuseSource *> & srcList,
                              std::vector<DiffuseSource *> & srcs) const {
   for (std::vector<DiffuseSource *>::const_iterator it = srcList.begin();
        it != srcList.end(); ++it) {
      std::string name = (*it)->getName();
      toLower(name);
      if (!m_respDiffuseSrcs.count(name)) {
         srcs.push_back(*it);
      }
   }
}

void Event::toLower(std::string & name) {
   for (std::string::iterator it = name.begin(); it != name.end(); ++it) {
      *it = std::tolower(*it);
   }
}

} // namespace Likelihood
