/**
 * @file ModelMap.h
 * @brief Standalone class for computing a model map using a
 * BinnedLikelihood object.
 * @author J. Chiang
 *
 * $Header$
 */

#ifndef Likelihood_ModelMap_h
#define Likelihood_ModelMap_h

#include <string>
#include <vector>

namespace Likelihood {

class BinnedLikelihood;
// EAC, add projection specific methods
class CountsMap;
class CountsMapHealpix;

/**
 * @class ModelMap
 * @brief Standalone class for computing a model map using a
 * BinnedLikelihood object.
 */

class ModelMap {

public:

   ModelMap(BinnedLikelihood & logLike,
            const std::vector<float> * model_map=0);

   void writeOutputMap(const std::string & outfile,
                       std::string outtype="CMAP");

protected:

   void writeOutputMap_wcs(const std::string & outfile,
			   const CountsMap& cmap,
			   std::string outtype="CMAP");

   void writeOutputMap_healpix(const std::string & outfile,
			       const CountsMapHealpix& cmap,
			       std::string outtype="CMAP");
  
   void trimExtensions_wcs(const std::string & outfile,
			   const std::string & outtype="CMAP");

   void trimExtensions_healpix(const std::string & outfile,
			       const std::string & outtype="CMAP");



private:

   BinnedLikelihood & m_logLike;

   std::vector<float> m_outmap;

};

} // namespace Likelihood

#endif // Likelihood_ModelMap_h
