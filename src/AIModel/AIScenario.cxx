// FGAIScenario - class for loading an AI scenario
// Written by David Culp, started May 2004
// - davidculp2@comcast.net
//
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
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.


#include "AIScenario.hxx"
#include <simgear/misc/sg_path.hxx>
#include <simgear/debug/logstream.hxx>
#include <simgear/structure/exception.hxx>
#include <simgear/constants.h>
#ifdef __BORLANDC__
#  define exception c_exception
#endif
#include <simgear/props/props.hxx>
#include <Main/globals.hxx>
#include <Main/fg_props.hxx>


FGAIScenario::FGAIScenario(string filename)
{
  int i;
  SGPath path( globals->get_fg_root() );
  path.append( ("/Data/AI/" + filename + ".xml").c_str() );
  SGPropertyNode root;

  try {
      readProperties(path.str(), &root);
  } catch (const sg_exception &e) {
      SG_LOG(SG_GENERAL, SG_ALERT,
       "Incorrect path specified for AI scenario: ");
       cout << path.str() << endl;
      return;
  }

  SGPropertyNode * node = root.getNode("scenario");
  for (i = 0; i < node->nChildren(); i++) { 
     //cout << "Reading entry " << i << endl;        
     entry* en = new entry;
     entries.push_back( en );
     SGPropertyNode * entry_node = node->getChild(i);
     en->callsign       = entry_node->getStringValue("callsign", "none");
     en->aitype         = entry_node->getStringValue("type", "aircraft");
     en->aircraft_class = entry_node->getStringValue("class", "jet_transport");
     en->model_path     = entry_node->getStringValue("model", "Models/Geometry/glider.ac");
     en->flightplan     = entry_node->getStringValue("flightplan", "");
     en->repeat         = entry_node->getDoubleValue("repeat", 0.0); 
   }

  entry_iterator = entries.begin();
  //cout << entries.size() << " entries read." << endl;
}


FGAIScenario::~FGAIScenario()
{
  entries.clear();
}


FGAIScenario::entry*
FGAIScenario::getNextEntry( void )
{
  if (entries.size() == 0) return 0;
  if (entry_iterator != entries.end()) {
    return *entry_iterator++;
  } else {
    return 0;
  }
}

int FGAIScenario::nEntries( void )
{
  return entries.size();
}


