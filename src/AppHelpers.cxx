/**
 * @file AppHelpers.cxx
 * @brief Class of "helper" methods for Likelihood applications.
 * @author J. Chiang
 *
 * $Header$
 */

#include <map>
#include <sstream>
#include <stdexcept>
#include <vector>

#include "irfLoader/Loader.h"
#include "irfInterface/IrfsFactory.h"

#include "st_facilities/Util.h"

#include "dataSubselector/CutBase.h"
#include "dataSubselector/Cuts.h"

#include "Likelihood/AppHelpers.h"
#include "Likelihood/BandFunction.h"
#include "Likelihood/EventContainer.h"
#include "Likelihood/ExposureMap.h"
#include "Likelihood/LogParabola.h"
#include "Likelihood/MapCubeFunction.h"
#include "Likelihood/Observation.h"
#include "Likelihood/PowerLaw2.h"
#include "Likelihood/ResponseFunctions.h"
#include "Likelihood/RoiCuts.h"
#include "Likelihood/ScData.h"
#include "Likelihood/SkyDirFunction.h"
#include "Likelihood/SpatialMap.h"

using irfInterface::IrfsFactory;

namespace Likelihood {

AppHelpers::AppHelpers(st_app::AppParGroup * pars)
   : m_pars(pars), m_funcFactory(0), m_respFuncs(0) {
   prepareFunctionFactory();
   createResponseFuncs();

   m_roiCuts = new RoiCuts();
   m_scData = new ScData();
   m_expCube = new ExposureCube();
   m_expMap = new ExposureMap();
   m_eventCont = new EventContainer(*m_respFuncs, *m_roiCuts, *m_scData);
   m_observation = new Observation(m_respFuncs,
                                   m_scData,
                                   m_roiCuts,
                                   m_expCube,
                                   m_expMap,
                                   m_eventCont);
}

AppHelpers::~AppHelpers() {
   delete m_funcFactory;
   delete m_roiCuts;
   delete m_scData;
   delete m_expCube;
   delete m_expMap;
   delete m_eventCont;
   delete m_respFuncs;
   delete m_observation;
}

optimizers::FunctionFactory & AppHelpers::funcFactory() {
   return *m_funcFactory;
}

void AppHelpers::prepareFunctionFactory() {
   bool makeClone(false);
   m_funcFactory = new optimizers::FunctionFactory;
   m_funcFactory->addFunc("SkyDirFunction", new SkyDirFunction(), makeClone);
   m_funcFactory->addFunc("SpatialMap", new SpatialMap(), makeClone);
   m_funcFactory->addFunc("BandFunction", new BandFunction(), makeClone);
   m_funcFactory->addFunc("LogParabola", new LogParabola(), makeClone);
   m_funcFactory->addFunc("MapCubeFunction", new MapCubeFunction(), makeClone);
   m_funcFactory->addFunc("PowerLaw2", new PowerLaw2(), makeClone);
}

void AppHelpers::setRoi(const std::string & filename,
                        const std::string & ext, bool strict) {
   RoiCuts & roiCuts = const_cast<RoiCuts &>(m_observation->roiCuts());
   if (filename != "") {
      roiCuts.readCuts(filename, ext, strict);
      return;
   }
   st_app::AppParGroup & pars(*m_pars);
   std::string event_file = pars["evfile"];
   std::vector<std::string> eventFiles;
   st_facilities::Util::resolve_fits_files(event_file, eventFiles);
   roiCuts.readCuts(eventFiles, "EVENTS", strict);
}

void AppHelpers::readScData() {
   st_app::AppParGroup & pars(*m_pars);
   std::string scFile = pars["scfile"];
   st_facilities::Util::file_ok(scFile);
   st_facilities::Util::resolve_fits_files(scFile, m_scFiles);
   std::vector<std::string>::const_iterator scIt = m_scFiles.begin();
   for ( ; scIt != m_scFiles.end(); scIt++) {
      st_facilities::Util::file_ok(*scIt);
      m_scData->readData(*scIt);
   }
}

void AppHelpers::readExposureMap() {
   st_app::AppParGroup & pars(*m_pars);
   std::string exposureFile = pars["exposure_map_file"];
   if (exposureFile != "none") {
      st_facilities::Util::file_ok(exposureFile);
      m_expMap->readExposureFile(exposureFile);
   }
}

void AppHelpers::createResponseFuncs() {
   m_respFuncs = new ResponseFunctions();
   st_app::AppParGroup & pars(*m_pars);
   std::string responseFuncs = pars["rspfunc"];
   m_respFuncs->load(responseFuncs);
}

void AppHelpers::checkOutputFile(bool clobber, const std::string & file) {
   if (!clobber) {
      if (file != "none" && st_facilities::Util::fileExists(file)) {
         std::cout << "Output file " << file 
                   << " already exists and you have set 'clobber' to 'no'.\n"
                   << "Please provide a different output file name."
                   << std::endl;
         std::exit(1);
      }
   }
}

void AppHelpers::checkCuts(const std::string & file1,
                           const std::string & ext1,
                           const std::string & file2,
                           const std::string & ext2,
                           bool compareGtis,
                           bool relyOnStreams) {
   bool checkColumns(false);
   dataSubselector::Cuts cuts1(file1, ext1, checkColumns);
   dataSubselector::Cuts cuts2(file2, ext2, checkColumns);
   if (!checkCuts(cuts1, cuts2, compareGtis, relyOnStreams)) {
      std::ostringstream message;
      message << "AppHelpers::checkCuts:\n" 
              << "DSS keywords ";
      if (compareGtis) {
         message << "and GTIs ";
      }
      message << "in " << file1;
      if (ext1 != "") {
         message << "[" << ext1 << "] ";
      }
      message << "do not match those in " << file2;
      if (ext2 != "") {
         message << "[" << ext2 << "] ";
      }
      throw std::runtime_error(message.str());
   }
}

void AppHelpers::checkCuts(const std::vector<std::string> & files1,
                           const std::string & ext1,
                           const std::string & file2,
                           const std::string & ext2,
                           bool compareGtis, 
                           bool relyOnStreams) {
   bool checkColumns(false);
   dataSubselector::Cuts cuts1(files1, ext1, checkColumns);
   dataSubselector::Cuts cuts2(file2, ext2, checkColumns);
   if (!checkCuts(cuts1, cuts2, compareGtis, relyOnStreams)) {
      std::ostringstream message;
      message << "AppHelpers::checkCuts:\n" 
              << "DSS keywords ";
      if (compareGtis) {
         message << "and GTIs ";
      }
      message << "in \n";
      for (unsigned int i = 0; i < files1.size(); i++) {
         message << files1.at(i) << "\n";
      }
      if (ext1 != "") {
         message << "in extension " << ext1 << "\n";
      }
      message << "do not match those in " << file2;
      if (ext2 != "") {
         message << "[" << ext2 << "] ";
      }
      throw std::runtime_error(message.str());
   }
}

bool AppHelpers::checkCuts(const dataSubselector::Cuts & cuts1,
                           const dataSubselector::Cuts & cuts2,
                           bool compareGtis, bool relyOnStreams) {
   bool standardTest;
   if (relyOnStreams) {
      std::ostringstream c1, c2;
      cuts1.writeCuts(c1);
      cuts2.writeCuts(c2);
      standardTest = (c1.str() == c2.str());
   } else {
      if (compareGtis) {
         standardTest = (cuts1 == cuts2);
      } else {
         standardTest = cuts1.compareWithoutGtis(cuts2);
      }
   }
   return standardTest;
}

void AppHelpers::checkTimeCuts(const std::string & file1,
                               const std::string & ext1,
                               const std::string & file2,
                               const std::string & ext2,
                               bool compareGtis) {
   dataSubselector::Cuts cuts1(file1, ext1, false);
   dataSubselector::Cuts cuts2(file2, ext2, false);
   if (!checkTimeCuts(cuts1, cuts2, compareGtis)) {
      std::ostringstream message;
      message << "AppHelpers::checkTimeCuts:\n" 
              << "Time range cuts ";
      if (compareGtis) {
         message << "and GTI extensions ";
      }
      message << "in files " << file1;
      if (ext1 != "") {
         message << "[" << ext1 << "]";
      }
      message << " and " << file2;
      if (ext2 != "") {
         message << "[" << ext2 << "]";
      }
      message << " do not agree.";
      throw std::runtime_error(message.str());
   }
}

void AppHelpers::checkTimeCuts(const std::vector<std::string> & files1,
                               const std::string & ext1,
                               const std::string & file2,
                               const std::string & ext2,
                               bool compareGtis) {
   dataSubselector::Cuts cuts1(files1, ext1, false);
   dataSubselector::Cuts cuts2(file2, ext2, false);
   if (!checkTimeCuts(cuts1, cuts2, compareGtis)) {
      std::ostringstream message;
      message << "AppHelpers::checkTimeCuts:\n" 
              << "Time range cuts ";
      if (compareGtis) {
         message << "and GTI extensions ";
      }
      message << "in files \n";
      for (unsigned int i = 0; i < files1.size(); i++) {
         message << files1.at(i);
         if (ext1 != "") {
            message << "[" << ext1 << "]";
         }
      }
      message << "\n";
      message << " and " << file2;
      if (ext2 != "") {
         message << "[" << ext2 << "]\n";
      }
      message << " do not agree.";
      throw std::runtime_error(message.str());
   }
}

bool AppHelpers::checkTimeCuts(const dataSubselector::Cuts & cuts1,
                               const dataSubselector::Cuts & cuts2,
                               bool compareGtis) {
// This is a bit fragile as one must assume the ordering of the 
// cuts is the same for both Cuts objects.
   std::vector<const dataSubselector::CutBase *> time_cuts1;
   std::vector<const dataSubselector::CutBase *> time_cuts2;
   gatherTimeCuts(cuts1, time_cuts1, compareGtis);
   gatherTimeCuts(cuts2, time_cuts2, compareGtis);
   bool ok(true);
   if (time_cuts1.size() == time_cuts2.size()) {
      for (unsigned int i = 0; i < time_cuts1.size(); i++) {
         ok = ok && *(time_cuts1[i]) == *(time_cuts2[i]);
      }
   } else {
      ok = false;
   }
   return ok;
}

void AppHelpers::
gatherTimeCuts(const dataSubselector::Cuts & cuts,
               std::vector<const dataSubselector::CutBase *> time_cuts,
               bool compareGtis) {
   for (unsigned int i = 0; i < cuts.size(); i++) {
      if ( (compareGtis && cuts[i].type() == "GTI") || 
           (cuts[i].type() == "range" &&
            dynamic_cast<dataSubselector::RangeCut &>(
               const_cast<dataSubselector::CutBase &>(cuts[i])).colname() 
            == "TIME") ) {
         time_cuts.push_back(&cuts[i]);
      }
   }
}

} // namespace Likelihood
