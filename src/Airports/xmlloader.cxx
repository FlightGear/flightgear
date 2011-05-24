// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 2 of the
// License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//

#include <simgear/misc/sg_path.hxx>

#include <simgear/xml/easyxml.hxx>

#include <Main/globals.hxx>
#include <Main/fg_props.hxx>

#include "xmlloader.hxx"
#include "dynamicloader.hxx"
#include "runwayprefloader.hxx"

#include "dynamics.hxx"
#include "runwayprefs.hxx"

using std::string;

XMLLoader::XMLLoader() {}
XMLLoader::~XMLLoader() {}

void XMLLoader::load(FGAirportDynamics* d) {
  FGAirportDynamicsXMLLoader visitor(d);
  if (fgGetBool("/sim/paths/use-custom-scenery-data") == false) {
   SGPath parkpath( globals->get_fg_root() );
   parkpath.append( "/AI/Airports/" );
   parkpath.append( d->getId() );
   parkpath.append( "parking.xml" );
   SG_LOG(SG_GENERAL, SG_DEBUG, "running old loader:" << parkpath.c_str());
   if (parkpath.exists()) {
       try {
           readXML(parkpath.str(), visitor);
           d->init();
       } 
       catch (const sg_exception &) {
       }
   }
  } else {
    if(loadAirportXMLDataIntoVisitor(d->getId(), "groundnet", visitor)) {
        d->init();
    }
  }
}

void XMLLoader::load(FGRunwayPreference* p) {
  FGRunwayPreferenceXMLLoader visitor(p);
  if (fgGetBool("/sim/paths/use-custom-scenery-data") == false) {
    SGPath rwyPrefPath( globals->get_fg_root() );
    rwyPrefPath.append( "AI/Airports/" );
    rwyPrefPath.append( p->getId() );
    rwyPrefPath.append( "rwyuse.xml" );
    if (rwyPrefPath.exists()) {
        try {
            readXML(rwyPrefPath.str(), visitor);
        } 
        catch (const sg_exception &) {
        }
     }
  } else {
    loadAirportXMLDataIntoVisitor(p->getId(), "rwyuse", visitor);
  }
}

void XMLLoader::load(FGSidStar* p) {
  SGPath path;
  if (findAirportData(p->getId(), "SID", path)) {
    p->load(path);
  }
}

bool XMLLoader::findAirportData(const std::string& aICAO, 
    const std::string& aFileName, SGPath& aPath)
{
  string fileName(aFileName);
  int extPos = fileName.size() - 4;
  if ((int) fileName.rfind(".xml") != extPos) {
    fileName.append(".xml");
  }
  
  string_list sc = globals->get_fg_scenery();
  char buffer[128];
  ::snprintf(buffer, 128, "%c/%c/%c/%s.%s", 
    aICAO[0], aICAO[1], aICAO[2], 
    aICAO.c_str(), fileName.c_str());

  for (string_list_iterator it = sc.begin(); it != sc.end(); ++it) {
    SGPath path(*it);
    path.append("Airports");
    path.append(string(buffer));
    if (path.exists()) {
      aPath = path;
      return true;
    } // of path exists
  } // of scenery path iteration
  return false;
}

bool XMLLoader::loadAirportXMLDataIntoVisitor(const string& aICAO, 
    const string& aFileName, XMLVisitor& aVisitor)
{
  SGPath path;
  if (!findAirportData(aICAO, aFileName, path)) {
    SG_LOG(SG_GENERAL, SG_DEBUG, "loadAirportXMLDataIntoVisitor: failed to find data for " << aICAO << "/" << aFileName);
    return false;
  }

  SG_LOG(SG_GENERAL, SG_DEBUG, "loadAirportXMLDataIntoVisitor: loading from " << path.str());
  readXML(path.str(), aVisitor);
  return true;
}

