/**
 * @file likelihood.cxx
 * @brief Prototype standalone application for the Likelihood tool.
 * @author J. Chiang
 *
 * $Header$
 */

#include <cmath>
#include <cstdlib>
#include <cstring>

#include <fstream>
#include <iostream>

#include "st_app/AppParGroup.h"
#include "st_app/StApp.h"
#include "st_app/StAppFactory.h"

#include "st_facilities/Util.h"

#include "tip/IFileSvc.h"
#include "tip/Table.h"

#include "optimizers/Drmngb.h"
#include "optimizers/Exception.h"
#include "optimizers/Lbfgs.h"
#include "optimizers/Minuit.h"
#ifdef HAVE_OPT_PP
#include "optimizers/OptPP.h"
#endif // HAVE_OPT_PP

#include "Likelihood/AppHelpers.h"
#include "Likelihood/BinnedLikelihood.h"
#include "Likelihood/CountsMap.h"
#include "Likelihood/ExposureCube.h"
#include "Likelihood/LogLike.h"
#include "Likelihood/MapShape.h"
#include "Likelihood/OptEM.h"
#include "Likelihood/ResponseFunctions.h"
#include "Likelihood/Source.h"

#include "EasyPlot.h"

using namespace Likelihood;

/**
 * @class likelihood
 * @brief A class encapsulating the methods for performing an unbinned
 * Likelihood analysis in ballistic fashion.
 *
 * @author J. Chiang
 *
 * $Header$
 */

class likelihood : public st_app::StApp {
public:
   likelihood();
   virtual ~likelihood() throw() {
      try {
         delete m_logLike;
      } catch (std::exception & eObj) {
         std::cout << eObj.what() << std::endl;
      } catch (...) {
      }
   }
   virtual void run();
private:
   AppHelpers * m_helper;  //blech.
   st_app::AppParGroup & m_pars;

   LogLike * m_logLike;
   bool m_useOptEM;
   optimizers::Optimizer * m_opt;
   std::vector<std::string> m_eventFiles;
   CountsMap * m_dataMap;
   std::string m_statistic;

   void createStatistic();
   void readEventData();
   void readSourceModel();
   void selectOptimizer(std::string optimizer="");
   void writeSourceXml();
   void writeFluxXml();
   void writeCountsSpectra();
   void writeCountsMap();
   void createCountsMap();
   void printFitResults(const std::vector<double> &errors);
   bool prompt(const std::string &query);
};

st_app::StAppFactory<likelihood> myAppFactory;

likelihood::likelihood() 
   : st_app::StApp(), m_helper(0), 
     m_pars(st_app::StApp::getParGroup("likelihood")),
     m_logLike(0), m_useOptEM(false), m_opt(0), m_dataMap(0) {
   try {
      m_pars.Prompt();
      m_pars.Save();
      m_helper = new AppHelpers(m_pars);
      ResponseFunctions::setEdispFlag(m_pars["use_energy_dispersion"]);
   } catch (std::exception & eObj) {
      std::cerr << eObj.what() << std::endl;
      std::exit(1);
   } catch (...) {
      std::cerr << "Caught unknown exception in likelihood constructor." 
                << std::endl;
      std::exit(1);
   }
}

void likelihood::run() {
   m_helper->setRoi();
   m_helper->readExposureMap();
   std::string eventFile = m_pars["event_file"];
   st_facilities::Util::file_ok(eventFile);
   st_facilities::Util::resolve_fits_files(eventFile, m_eventFiles);
   createCountsMap();
   createStatistic();

// Set the verbosity level and convergence tolerance.
   long verbose = m_pars["fit_verbosity"];
   double tol = m_pars["fit_tolerance"];
   std::vector<double> errors;

// The fit loop.  If indicated, query the user at the end of each
// iteration whether the fit is to be performed again.  This allows
// the user to adjust the source model xml file by hand between
// iterations.
   bool queryLoop = m_pars["query_for_refit"];
   do {
      readSourceModel();
// Do the fit.
      if (m_useOptEM) {
         dynamic_cast<OptEM *>(m_logLike)->findMin(verbose);
      } else {
// @todo Allow the optimizer to be re-selected here by the user.    
         selectOptimizer();
         try {
            m_opt->find_min(verbose, tol);
         } catch (optimizers::Exception & eObj) {
            std::cerr << eObj.what() << std::endl;
         }
         try {
            errors = m_opt->getUncertainty();
         } catch (optimizers::Exception & eObj) {
            std::cerr << "Exception encountered while estimating errors:\n"
                      << eObj.what() << std::endl;
         }
      }
      printFitResults(errors);
      writeSourceXml();
   } while (queryLoop && prompt("Refit? [y] "));
   writeFluxXml();
   writeCountsSpectra();
//   writeCountsMap();
}

void likelihood::createStatistic() {
   std::string statistic = m_pars["Statistic"];
   m_statistic = statistic;  // Wow, this is annoying.
   if (statistic == "BINNED") {
      std::string expcube_file = m_pars["exposure_cube_file"];
      if (expcube_file == "none") {
         throw std::runtime_error("Please specify an exposure cube file.");
      }
      ExposureCube::readExposureCube(expcube_file);
      m_logLike = new BinnedLikelihood(*m_dataMap);
      return;
   } else if (statistic == "OPTEM") {
      m_logLike = new OptEM();
      m_useOptEM = true;
   } else if (statistic == "UNBINNED") {
      m_logLike = new LogLike();
   }
   readEventData();
}

void likelihood::readEventData() {
   long eventFileHdu = m_pars["event_file_hdu"];
   std::vector<std::string>::const_iterator evIt = m_eventFiles.begin();
   for ( ; evIt != m_eventFiles.end(); evIt++) {
      st_facilities::Util::file_ok(*evIt);
      m_logLike->getEvents(*evIt, eventFileHdu);
   }
}

void likelihood::readSourceModel() {
   std::string sourceModel = m_pars["Source_model_file"];
   if (m_logLike->getNumSrcs() == 0) {
// Read in the Source model for the first time.
      st_facilities::Util::file_ok(sourceModel);
      m_logLike->readXml(sourceModel, m_helper->funcFactory());
      if (m_statistic != "BINNED") {
         m_logLike->computeEventResponses();
      } else {
         dynamic_cast<BinnedLikelihood *>(m_logLike)
            ->saveSourceMaps("srcMaps.fits");
      }
   } else {
// Re-read the Source model from the xml file, allowing only for 
// Parameter adjustments.
      st_facilities::Util::file_ok(sourceModel);
      m_logLike->reReadXml(sourceModel);
   }
}

void likelihood::selectOptimizer(std::string optimizer) {
   delete m_opt;
   m_opt = 0;
   if (optimizer == "") {
// Type conversions for hoops do not work properly, so we are
// forced to use an intermediate variable.
      std::string opt = m_pars["optimizer"];
      optimizer = opt;
   }
   if (optimizer == "LBFGS") {
      m_opt = new optimizers::Lbfgs(*m_logLike);
   } else if (optimizer == "MINUIT") {
      m_opt = new optimizers::Minuit(*m_logLike);
   } else if (optimizer == "DRMNGB") {
      m_opt = new optimizers::Drmngb(*m_logLike);
#ifdef HAVE_OPT_PP
   } else if (optimizer == "OPTPP") {
      m_opt = new optimizers::OptPP(*m_logLike);
#endif // HAVE_OPT_PP
   }
   if (m_opt == 0) {
      throw std::invalid_argument("Invalid optimizer choice: " + optimizer);
   }
}

void likelihood::writeSourceXml() {
   std::string xmlFile = m_pars["Source_model_output_file"];
   std::string funcFileName("");
   if (xmlFile != "none") {
      std::cout << "Writing fitted model to " << xmlFile << std::endl;
      m_logLike->writeXml(xmlFile, funcFileName);
   }
}

void likelihood::writeFluxXml() {
   std::string xml_fluxFile = m_pars["flux_style_model_file"];
   if (xml_fluxFile != "none") {
      std::cout << "Writing flux-style xml model file to "
                << xml_fluxFile << std::endl;
      m_logLike->write_fluxXml(xml_fluxFile);
   }
}

void likelihood::writeCountsSpectra() {
   std::vector<double> energies;
   double emin(20);
   double emax(2e5);
   int nee(20);
   double estep = log(emax/emin)/(nee-1);
   for (int k = 0; k < nee; k++) {
      energies.push_back(emin*exp(estep*k));
   }
   std::vector<double> evals;
   std::vector<std::string> srcNames;
   m_logLike->getSrcNames(srcNames);
   std::vector< std::vector<double> > npred(srcNames.size());
   std::ofstream outputFile("counts.dat");
   for (int k = 0; k < nee - 1; k++) {
      bool writeLine(true);
      std::ostringstream line;
      line << sqrt(energies[k]*energies[k+1]) << "   ";
      for (unsigned int i = 0; i < srcNames.size(); i++) {
         Source * src = m_logLike->getSource(srcNames[i]);
         double Npred;
         try {
            Npred = src->Npred(energies[k], energies[k+1]);
            if (i==0) evals.push_back(log10(sqrt(energies[k]*energies[k+1])));
            npred[i].push_back(log10(Npred));
            line << Npred << "  ";
         } catch (std::out_of_range & eObj) {
            writeLine = false;
         }
      }
      if (writeLine) {
         outputFile << line.str() << std::endl;
      }
   }
   outputFile.close();

#ifdef HAVE_ST_GRAPH
// plot the data
   try {
      EasyPlot plot;
      for (unsigned int i = 0; i < npred.size(); i++) {
         plot.histogram(evals, npred[i]);
      }
      EasyPlot::run();
   } catch (std::exception &eObj) {
      std::string message = "RootEngine could not create";
      if (!st_facilities::Util::expectedException(eObj, message)) {
         throw;
      }
   }
#endif // HAVE_ST_GRAPH
}

void likelihood::writeCountsMap() {
// If there is no valid exposure_cube_file, do nothing and return.
   std::string expcube_file = m_pars["exposure_cube_file"];
   if (expcube_file == "none") {
      return;
   }
   ExposureCube::readExposureCube(expcube_file);
   m_dataMap->writeOutput("likelihood", "data_map.fits");
   try {
      CountsMap * modelMap;
      if (m_statistic == "BINNED") {
         modelMap = m_logLike->createCountsMap();
      } else {
         modelMap = m_logLike->createCountsMap(*m_dataMap);
      }
      modelMap->writeOutput("likelihood", "model_map.fits");
      delete modelMap;
   } catch (std::exception & eObj) {
      std::cout << eObj.what() << std::endl;
   }
}

void likelihood::createCountsMap() {
// Create a counts map from the data using the ROI as to get the map
// dimensions.
   RoiCuts * roiCuts = RoiCuts::instance();

   std::pair<double, double> elims = roiCuts->getEnergyCuts();

   const irfInterface::AcceptanceCone & roi = roiCuts->extractionRegion();
   double roi_radius = roi.radius();
   double roi_ra, roi_dec;
   RoiCuts::getRaDec(roi_ra, roi_dec);

   double pixel_size(0.5);
   unsigned long npts = static_cast<unsigned long>(2*roi_radius/pixel_size);
   unsigned long nee(21);

// CountsMap and its base class, DataProduct, want *single* event and
// scData files for extracting header keywords and gti info, so pass
// just the first from each vector.
// @todo sort out how one should handle these data when several FITS
// files are specified.
   m_dataMap = new CountsMap(m_eventFiles[0], m_helper->scFiles()[0], 
                             roi_ra, roi_dec, "CAR", npts, npts, pixel_size, 
                             0, false, "RA", "DEC", elims.first, 
                             elims.second, nee);
   for (unsigned int i = 0; i < m_eventFiles.size(); i++) {
      const tip::Table * events 
         = tip::IFileSvc::instance().readTable(m_eventFiles[i], "events");
      m_dataMap->binInput(events->begin(), events->end());
   }
}

void likelihood::printFitResults(const std::vector<double> &errors) {
   std::vector<std::string> srcNames;
   m_logLike->getSrcNames(srcNames);

// Save current set of parameters.
   std::vector<double> fitParams;
   m_logLike->getFreeParamValues(fitParams);

   std::map<std::string, double> TsValues;
// Compute TS for each source.
   int verbose(0);
   double tol(1e-4);
   double logLike_value = m_logLike->value();
   std::vector<double> null_values;
   std::cerr << "Computing TS values for each source ("
             << srcNames.size() << " total)\n";
   for (unsigned int i = 0; i < srcNames.size(); i++) {
      std::cerr << ".";
      if (srcNames[i].find("Diffuse") == std::string::npos) {
         Source * src = m_logLike->deleteSource(srcNames[i]);
         if (m_logLike->getNumFreeParams() > 0) {
            selectOptimizer();
            try {
               m_opt->find_min(verbose, tol);
            } catch (optimizers::Exception &eObj) {
               std::cout << eObj.what() << std::endl;
            }
            null_values.push_back(m_logLike->value());
            TsValues[srcNames[i]] = 2.*(logLike_value - null_values.back());
         } else {
// // Not sure this is correct in the case where the model for the null
// // hypothesis is empty.
//             TsValues[srcNames[i]] = 2.*logLike_value;
// A better default value?
            TsValues[srcNames[i]] = 0.;
         }
         m_logLike->addSource(src);
      }
   }
   std::cerr << "!" << std::endl;
// Reset parameter values.
   m_logLike->setFreeParamValues(fitParams);

   std::vector<optimizers::Parameter> parameters;
   std::vector<double>::const_iterator errIt = errors.begin();

   for (unsigned int i = 0; i < srcNames.size(); i++) {
      Source *src = m_logLike->getSource(srcNames[i]);
      Source::FuncMap srcFuncs = src->getSrcFuncs();
      srcFuncs["Spectrum"]->getParams(parameters);
      std::cout << "\n" << srcNames[i] << ":\n";
      for (unsigned int j = 0; j < parameters.size(); j++) {
         std::cout << parameters[j].getName() << ": "
                   << parameters[j].getValue();
         if (parameters[j].isFree() && errIt != errors.end()) {
            std::cout << " +/- " << *errIt;
            errIt++;
         }
         std::cout << std::endl;
      }
      std::cout << "Npred: "
                << src->Npred() << std::endl;
      if (TsValues.count(srcNames[i])) {
         std::cout << "TS value: "
                   << TsValues[srcNames[i]] << std::endl;
      }
   }
   std::cout << "\n-log(Likelihood): "
             << -m_logLike->value()
             << "\n" << std::endl;
}

bool likelihood::prompt(const std::string &query) {
   std::cout << query << std::endl;
   char answer[2];
   std::cin.getline(answer, 2);
   if (std::string(answer) == "y" || std::string(answer) == "") {
      return true;
   }
   return false;
}
