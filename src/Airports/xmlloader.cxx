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
#include <Main/fg_props.hxx>

#include "xmlloader.hxx"
#include "dynamicloader.hxx"
#include "runwayprefloader.hxx"

#include "dynamics.hxx"
#include "runwayprefs.hxx"

XMLLoader::XMLLoader() {}
XMLLoader::~XMLLoader() {}

string XMLLoader::expandICAODirs(const string in){
     //cerr << "Expanding " << in << endl;
     if (in.size() == 4) {
          char buffer[11];
          snprintf(buffer, 11, "%c/%c/%c", in[0], in[1], in[2]);
          //cerr << "result: " << buffer << endl;
          return string(buffer);
     } else {
           return in;
     }
     //exit(1);
}

void XMLLoader::load(FGAirportDynamics* d) {
    FGAirportDynamicsXMLLoader visitor(d);
    if (fgGetBool("/sim/traffic-manager/use-custom-scenery-data") == false) {
       SGPath parkpath( globals->get_fg_root() );
       parkpath.append( "/AI/Airports/" );
       parkpath.append( d->getId() );
       parkpath.append( "parking.xml" );
       if (parkpath.exists()) {
           try {
               readXML(parkpath.str(), visitor);
               d->init();
           } 
           catch (const sg_exception &e) {
           }
       } else {
            string_list sc = globals->get_fg_scenery();
            char buffer[32];
            snprintf(buffer, 32, "%s.groundnet.xml", d->getId().c_str() );
            string airportDir = XMLLoader::expandICAODirs(d->getId());
            for (string_list_iterator i = sc.begin(); i != sc.end(); i++) {
                SGPath parkpath( *i );
                parkpath.append( "Airports" );
                parkpath.append ( airportDir );
                parkpath.append( string (buffer) );
                if (parkpath.exists()) {
                    try {
                        readXML(parkpath.str(), visitor);
                        d->init();
                    } 
                    catch (const sg_exception &e) {
                    }
                    return;
                }
            }
        }
    }
}

void XMLLoader::load(FGRunwayPreference* p) {
    FGRunwayPreferenceXMLLoader visitor(p);
    if (fgGetBool("/sim/traffic-manager/use-custom-scenery-data") == false) {
        SGPath rwyPrefPath( globals->get_fg_root() );
        rwyPrefPath.append( "AI/Airports/" );
        rwyPrefPath.append( p->getId() );
        rwyPrefPath.append( "rwyuse.xml" );
        if (rwyPrefPath.exists()) {
            try {
                readXML(rwyPrefPath.str(), visitor);
            } 
            catch (const sg_exception &e) {
            }
         }
      }  else {
       string_list sc = globals->get_fg_scenery();
       char buffer[32];
       snprintf(buffer, 32, "%s.rwyuse.xml", p->getId().c_str() );
       string airportDir = expandICAODirs(p->getId());
       for (string_list_iterator i = sc.begin(); i != sc.end(); i++) {
           SGPath rwypath( *i );
           rwypath.append( "Airports" );
           rwypath.append ( airportDir );
           rwypath.append( string(buffer) );
           if (rwypath.exists()) {
               try {
                   readXML(rwypath.str(), visitor);
                } 
                catch (const sg_exception &e) {
                }
                return;
            }
        }
    }
}

