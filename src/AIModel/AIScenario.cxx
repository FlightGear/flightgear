// FGAIScenario.cxx - class for loading an AI scenario
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

#include <cstdio>

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

#include "AIScenario.hxx"
#include "AIFlightPlan.hxx"

static list<string>
getAllStringNodeVals(const char* name, SGPropertyNode * entry_node);
static list<ParkPosition>
getAllOffsetNodeVals(const char* name, SGPropertyNode * entry_node);

FGAIScenario::FGAIScenario(const string &filename)
{
  int i;
  SGPath path( globals->get_fg_root() );
  
//   cout << "/Data/AI/" << filename << endl;
  
  path.append( ("/Data/AI/" + filename + ".xml").c_str() );
  SGPropertyNode root;
  readProperties(path.str(), &root);
  
//   cout <<"path " << path.str() << endl;
  
  try {
      readProperties(path.str(), &root);
  } catch (const sg_exception &e) {
      SG_LOG(SG_GENERAL, SG_ALERT,
       "Incorrect path specified for AI scenario: ");
       
       cout << path.str() << endl;
      
      return;
  }

  entries.clear();
  SGPropertyNode * node = root.getNode("scenario");
  for (i = 0; i < node->nChildren(); i++) { 
     
//      cout << "Reading entity data entry " << i << endl;        
     
     SGPropertyNode * entry_node = node->getChild(i);

     FGAIModelEntity* en = new FGAIModelEntity;
     en->callsign       = entry_node->getStringValue("callsign", "none");
     en->m_type         = entry_node->getStringValue("type", "aircraft");
     en->m_class        = entry_node->getStringValue("class", "jet_transport");
     en->path           = entry_node->getStringValue("model", "Models/Geometry/glider.ac");
     en->flightplan     = entry_node->getStringValue("flightplan", "");
     en->repeat         = entry_node->getDoubleValue("repeat", 0.0); 
     en->latitude       = entry_node->getDoubleValue("latitude", 0.0); 
     en->longitude      = entry_node->getDoubleValue("longitude", 0.0); 
     en->altitude       = entry_node->getDoubleValue("altitude", 0.0); 
     en->speed          = entry_node->getDoubleValue("speed", 0.0); 
     en->heading        = entry_node->getDoubleValue("heading", 0.0); 
     en->roll           = entry_node->getDoubleValue("roll", 0.0); 
     en->azimuth        = entry_node->getDoubleValue("azimuth", 0.0); 
     en->elevation      = entry_node->getDoubleValue("elevation", 0.0); 
     en->rudder         = entry_node->getFloatValue("rudder", 0.0);
     en->strength       = entry_node->getDoubleValue("strength-fps", 8.0);
     en->turb_strength  = entry_node->getDoubleValue("strength-norm", 1.0);  
     en->diameter       = entry_node->getDoubleValue("diameter-ft", 0.0);
     en->height_msl     = entry_node->getDoubleValue("height-msl", 5000.0);
     en->eda            = entry_node->getDoubleValue("eda", 0.007);
     en->life           = entry_node->getDoubleValue("life", 900.0);
     en->buoyancy       = entry_node->getDoubleValue("buoyancy", 0);
     en->wind_from_east = entry_node->getDoubleValue("wind_from_east", 0);
     en->wind_from_north = entry_node->getDoubleValue("wind_from_north", 0);
     en->wind            = entry_node->getBoolValue  ("wind", false);
     en->cd              = entry_node->getDoubleValue("cd", 0.029); 
     en->mass            = entry_node->getDoubleValue("mass", 0.007); 
     en->radius          = entry_node->getDoubleValue("turn-radius-ft", 2000);
     en->TACAN_channel_ID= entry_node->getStringValue("TACAN-channel-ID", "029Y");
     en->name            = entry_node->getStringValue("name", "Nimitz");
     en->pennant_number  = entry_node->getStringValue("pennant-number", "");
     en->wire_objects     = getAllStringNodeVals("wire", entry_node);
     en->catapult_objects = getAllStringNodeVals("catapult", entry_node);
     en->solid_objects    = getAllStringNodeVals("solid", entry_node);
     en->ppositions       = getAllOffsetNodeVals("parking-pos", entry_node);
     en->max_lat          = entry_node->getDoubleValue("max-lat", 0);
     en->min_lat          = entry_node->getDoubleValue("min-lat",0);
     en->max_long          = entry_node->getDoubleValue("max-long", 0);
     en->min_long          = entry_node->getDoubleValue("min-long", 0);
     list<ParkPosition> flolspos = getAllOffsetNodeVals("flols-pos", entry_node);
     en->flols_offset     = flolspos.front().offset;

     en->fp             = NULL;
     if (en->flightplan != ""){
        en->fp = new FGAIFlightPlan( en->flightplan );
     }
     entries.push_back( en );
   }

  entry_iterator = entries.begin();
  //cout << entries.size() << " entries read." << endl;
}


FGAIScenario::~FGAIScenario()
{
  entries.clear();
}


FGAIModelEntity* const
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

static list<string>
getAllStringNodeVals(const char* name, SGPropertyNode * entry_node)
{
  list<string> retval;
  int i=0;
  do {
    char nodename[100];
    snprintf(nodename, sizeof(nodename), "%s[%d]", name, i);
    const char* objname = entry_node->getStringValue(nodename, 0);
    if (objname == 0)
      return retval;

    retval.push_back(string(objname));
    ++i;
  } while (1);

  return retval;
}

static list<ParkPosition>
getAllOffsetNodeVals(const char* name, SGPropertyNode * entry_node)
{
  list<ParkPosition> retval;

  vector<SGPropertyNode_ptr>::const_iterator it;
  vector<SGPropertyNode_ptr> children = entry_node->getChildren(name);
  for (it = children.begin(); it != children.end(); ++it) {
    string name = (*it)->getStringValue("name", "unnamed");
    double offset_x = (*it)->getDoubleValue("x-offset-m", 0);
    double offset_y = (*it)->getDoubleValue("y-offset-m", 0);
    double offset_z = (*it)->getDoubleValue("z-offset-m", 0);
    double hd = (*it)->getDoubleValue("heading-offset-deg", 0);
    ParkPosition pp(name, Point3D(offset_x, offset_y, offset_z), hd);
    retval.push_back(pp);
  }

  return retval;
}

// end scenario.cxx

