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
#include <Main/globals.hxx>

#include "xmlloader.hxx"
#include "dynamicloader.hxx"
#include "runwayprefloader.hxx"

#include "dynamics.hxx"
#include "runwayprefs.hxx"

XMLLoader::XMLLoader() {}
XMLLoader::~XMLLoader() {}

void XMLLoader::load(FGAirportDynamics* d) {
  FGAirportDynamicsXMLLoader visitor(d);

  SGPath parkpath( globals->get_fg_root() );
  parkpath.append( "/AI/Airports/" );
  parkpath.append( d->getId() );
  parkpath.append( "parking.xml" );
  
  if (parkpath.exists()) {
    try {
      readXML(parkpath.str(), visitor);
      d->init();
    } catch (const sg_exception &e) {
      //cerr << "unable to read " << parkpath.str() << endl;
    }
  }

}

void XMLLoader::load(FGRunwayPreference* p) {
  FGRunwayPreferenceXMLLoader visitor(p);

  SGPath rwyPrefPath( globals->get_fg_root() );
  rwyPrefPath.append( "AI/Airports/" );
  rwyPrefPath.append( p->getId() );
  rwyPrefPath.append( "rwyuse.xml" );

  //if (ai_dirs.find(id.c_str()) != ai_dirs.end()
  //  && rwyPrefPath.exists())
  if (rwyPrefPath.exists()) {
    try {
      readXML(rwyPrefPath.str(), visitor);
    } catch (const sg_exception &e) {
      //cerr << "unable to read " << rwyPrefPath.str() << endl;
    }
  }
}
