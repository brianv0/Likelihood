/**
 * @file RunParams.cxx
 * @brief Implementation for the hoops wrapper class to retrieve
 * command-line parameters.
 * @author J. Chiang
 *
 * $Header$
 */

#include <fstream>

#include "facilities/Util.h"

#include "Likelihood/RunParams.h"

namespace Likelihood {

RunParams::RunParams(int iargc, char* argv[]) {

   hoops::IParFile * pf = hoops::PILParFileFactory().NewIParFile(argv[0]);
   pf->Load();

   m_prompter = hoops::PILParPromptFactory().NewIParPrompt(iargc, argv);
   m_prompter->Prompt();

   pf->Group() = m_prompter->Group();
   pf->Save();

   delete pf;
}

RunParams::~RunParams() {
   delete m_prompter;
}

void RunParams::resolve_fits_files(std::string filename, 
                                   std::vector<std::string> &files) {

   facilities::Util::expandEnvVar(&filename);
   files.clear();

// Read the first line of the file and see if the first 6 characters
// are "SIMPLE".  If so, then we assume it's a FITS file.
   std::ifstream file(filename.c_str());
   std::string firstLine;
   std::getline(file, firstLine, '\n');
   if (firstLine.find("SIMPLE") == 0) {
// This is a FITS file. Return that as the sole element in the files
// vector.
      files.push_back(filename);
      return;
   } else {
// filename contains a list of fits files.
      readLines(filename, files);
      return;
   }
}

void RunParams::readLines(std::string inputFile, 
                          std::vector<std::string> &lines) {

   facilities::Util::expandEnvVar(&inputFile);

   std::ifstream file(inputFile.c_str());
   lines.clear();
   std::string line;
   while (std::getline(file, line, '\n')) {
      lines.push_back(line);
   }
}

} // namespace Likelihood
