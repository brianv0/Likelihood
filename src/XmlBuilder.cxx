/**
 * @file XmlBuilder.cxx
 * @brief Concrete implementation that is shareable by subclasses.
 * @author J. Chiang
 *
 * $Header$
 */

#include "xmlBase/XmlParser.h"

#include "optimizers/Dom.h"

#include "Likelihood/XmlBuilder.h"

#include "Likelihood/XmlParser.h"

namespace Likelihood {

XmlBuilder::XmlBuilder()  {
//   m_parser = XmlParser::instance();
   m_parser = XmlParser_instance();
   m_doc = optimizers::Dom::createDocument();
}


XmlBuilder::~XmlBuilder() {
   m_doc->release();
}

} //namespace Likelihood
