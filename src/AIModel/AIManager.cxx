// AIManager.cxx  Based on David Luff's AIMgr:
// - a global management type for AI objects
//
// Written by David Culp, started October 2003.
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
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

#include <Main/globals.hxx>

#include <Airports/simple.hxx>
#include <Traffic/TrafficMgr.hxx>

#include "AIManager.hxx"
#include "AIAircraft.hxx"
#include "AIShip.hxx"
#include "AIBallistic.hxx"
#include "AIStorm.hxx"
#include "AIThermal.hxx"
#include "AICarrier.hxx"
#include "AIStatic.hxx"
#include "AIMultiplayer.hxx"

FGAIManager::FGAIManager() {
  _dt = 0.0;
  mNumAiModels = 0;
  for (unsigned i = 0; i < FGAIBase::MAX_OBJECTS; ++i)
    mNumAiTypeModels[i] = 0;
}

FGAIManager::~FGAIManager() {
  ai_list_iterator ai_list_itr = ai_list.begin();
  while(ai_list_itr != ai_list.end()) {
      (*ai_list_itr)->unbind();
      ++ai_list_itr;
  }
}


void FGAIManager::init() {
  root = fgGetNode("sim/ai", true);

  enabled = root->getNode("enabled", true)->getBoolValue();
  if (!enabled)
      return;

  wind_from_down_node = fgGetNode("/environment/wind-from-down-fps", true);
  wind_from_east_node  = fgGetNode("/environment/wind-from-east-fps",true);
  wind_from_north_node = fgGetNode("/environment/wind-from-north-fps",true);

  user_latitude_node  = fgGetNode("/position/latitude-deg", true);
  user_longitude_node = fgGetNode("/position/longitude-deg", true);
  user_altitude_node  = fgGetNode("/position/altitude-ft", true);
  user_heading_node   = fgGetNode("/orientation/heading-deg", true);
  user_pitch_node     = fgGetNode("/orientation/pitch-deg", true);
  user_yaw_node       = fgGetNode("/orientation/side-slip-deg", true);
  user_speed_node     = fgGetNode("/velocities/uBody-fps", true);


  /// Move that into the constructor
  for(int i = 0 ; i < root->nChildren() ; i++) {
    SGPropertyNode *aiEntry = root->getChild( i );
    if( !strcmp( aiEntry->getName(), "scenario" ) ) {
      scenario_filename = aiEntry->getStringValue();
      if (!scenario_filename.empty())
        processScenario( scenario_filename );
    }
  }
}


void FGAIManager::reinit() {
   update(0.0);
   ai_list_iterator ai_list_itr = ai_list.begin();

   while(ai_list_itr != ai_list.end()) {
      (*ai_list_itr)->reinit();
      ++ai_list_itr;
   }
}


void FGAIManager::bind() {
   root = globals->get_props()->getNode("ai/models", true);
   root->tie("count", SGRawValueMethods<FGAIManager, int>(*this,
             &FGAIManager::getNumAiObjects));
}


void FGAIManager::unbind() {
    root->untie("count");
}


void FGAIManager::update(double dt) {
  // initialize these for finding nearest thermals
  range_nearest = 10000.0;
  strength = 0.0;
  if (!enabled)
    return;

  FGTrafficManager *tmgr = (FGTrafficManager*) globals->get_subsystem("Traffic Manager");
  _dt = dt;

  ai_list_iterator ai_list_itr = ai_list.begin();
  while(ai_list_itr != ai_list.end()) {
    if ((*ai_list_itr)->getDie()) {      
      tmgr->release((*ai_list_itr)->getID());
      --mNumAiModels;
      --(mNumAiTypeModels[(*ai_list_itr)->getType()]);
      (*ai_list_itr)->unbind();
      ai_list_itr = ai_list.erase(ai_list_itr);
    } else {
      fetchUserState();
      if ((*ai_list_itr)->isa(FGAIBase::otThermal)) {
        FGAIBase *base = *ai_list_itr;
        processThermal((FGAIThermal*)base); 
      } else { 
        (*ai_list_itr)->update(_dt);
      }
      ++ai_list_itr;
    }
  }
  wind_from_down_node->setDoubleValue( strength ); // for thermals
}

void
FGAIManager::attach(SGSharedPtr<FGAIBase> model)
{
  unsigned idx = mNumAiTypeModels[model->getType()];
  const char* typeString = model->getTypeString();
  SGPropertyNode* root = globals->get_props()->getNode("ai/models", true);
  SGPropertyNode* p;
  int i;
  for (i=0;i<10000;i++) //find free index in the property tree, if we have
      //more than 10000 mp-aircrafts in the property tree we should optimize the mp-server
  {
    p = root->getNode(typeString, i, false);
    if (!p) break;
    if (p->getIntValue("id",-1)==model->getID())
    {
        p->setStringValue("callsign","***invalid node***"); //debug only, should never set!
                                              
    }
  }
  p = root->getNode(typeString, i, true);
  model->setManager(this, p);
  ai_list.push_back(model);
  ++mNumAiModels;
  ++(mNumAiTypeModels[model->getType()]);
  model->init(model->getType()==FGAIBase::otAircraft
      || model->getType()==FGAIBase::otMultiplayer
      || model->getType()==FGAIBase::otStatic);
  model->bind();
}


void FGAIManager::destroyObject( int ID ) {
  ai_list_iterator ai_list_itr = ai_list.begin();
  while(ai_list_itr != ai_list.end()) {
    if ((*ai_list_itr)->getID() == ID) {
      --mNumAiModels;
      --(mNumAiTypeModels[(*ai_list_itr)->getType()]);
      (*ai_list_itr)->unbind();
      ai_list_itr = ai_list.erase(ai_list_itr);
    } else
      ++ai_list_itr;
  }
}

int
FGAIManager::getNumAiObjects(void) const
{
  return mNumAiModels;
}

void FGAIManager::fetchUserState( void ) {
     user_latitude  = user_latitude_node->getDoubleValue();
     user_longitude = user_longitude_node->getDoubleValue();
     user_altitude  = user_altitude_node->getDoubleValue();
     user_heading   = user_heading_node->getDoubleValue();
     user_pitch     = user_pitch_node->getDoubleValue();
     user_yaw       = user_yaw_node->getDoubleValue();
     user_speed     = user_speed_node->getDoubleValue() * 0.592484;
     wind_from_east = wind_from_east_node->getDoubleValue();
     wind_from_north = wind_from_north_node->getDoubleValue();
}


// only keep the results from the nearest thermal
void FGAIManager::processThermal( FGAIThermal* thermal ) {
  thermal->update(_dt);
  if ( thermal->_getRange() < range_nearest ) {
     range_nearest = thermal->_getRange();
     strength = thermal->getStrength();
  }
}


void FGAIManager::processScenario( const string &filename ) {

  SGPropertyNode_ptr scenarioTop = loadScenarioFile(filename);
  if (!scenarioTop)
    return;
  SGPropertyNode* scenarios = scenarioTop->getChild("scenario");
  if (!scenarios)
    return;

  for (int i = 0; i < scenarios->nChildren(); i++) { 
    SGPropertyNode* scEntry = scenarios->getChild(i);
    std::string type = scEntry->getStringValue("type", "aircraft");

    if (type == "aircraft") {
      FGAIAircraft* aircraft = new FGAIAircraft;
      aircraft->readFromScenario(scEntry);
      attach(aircraft);

    } else if (type == "ship") {
      FGAIShip* ship = new FGAIShip;
      ship->readFromScenario(scEntry);
      attach(ship);
      
    } else if (type == "carrier") {
      FGAICarrier* carrier = new FGAICarrier;
      carrier->readFromScenario(scEntry);
      attach(carrier);
      
    } else if (type == "thunderstorm") {
      FGAIStorm* storm = new FGAIStorm;
      storm->readFromScenario(scEntry);
      attach(storm);
      
    } else if (type == "thermal") {
      FGAIThermal* thermal = new FGAIThermal;
      thermal->readFromScenario(scEntry);
      attach(thermal);
      
    } else if (type == "ballistic") {
      FGAIBallistic* ballistic = new FGAIBallistic;
      ballistic->readFromScenario(scEntry);
      attach(ballistic);
      
    } else if (type == "static") {
      FGAIStatic* aistatic = new FGAIStatic;
      aistatic->readFromScenario(scEntry);
      attach(aistatic);
      
    }
  }
}

SGPropertyNode_ptr
FGAIManager::loadScenarioFile(const std::string& filename)
{
  SGPath path(globals->get_fg_root());
  path.append("AI/" + filename + ".xml");
  try {
    SGPropertyNode_ptr root = new SGPropertyNode;
    readProperties(path.str(), root);
    return root;
  } catch (const sg_exception &e) {
    SG_LOG(SG_GENERAL, SG_ALERT, "Incorrect path specified for AI "
           "scenario: \"" << path.str() << "\"");
    return 0;
  }
}

bool FGAIManager::getStartPosition(const string& id, const string& pid,
                                   SGGeod& geodPos, double& hdng, SGVec3d& uvw)
{
  bool found = false;
  SGPropertyNode* root = fgGetNode("sim/ai", true);
  if (!root->getNode("enabled", true)->getBoolValue())
      return found;

  for(int i = 0 ; (!found) && i < root->nChildren() ; i++) {
    SGPropertyNode *aiEntry = root->getChild( i );
    if( !strcmp( aiEntry->getName(), "scenario" ) ) {
      string filename = aiEntry->getStringValue();
      SGPropertyNode_ptr scenarioTop = loadScenarioFile(filename);
      if (scenarioTop) {
        SGPropertyNode* scenarios = scenarioTop->getChild("scenario");
        if (scenarios) {
          for (int i = 0; i < scenarios->nChildren(); i++) { 
            SGPropertyNode* scEntry = scenarios->getChild(i);
            std::string type = scEntry->getStringValue("type");
            std::string pnumber = scEntry->getStringValue("pennant-number");
            std::string name = scEntry->getStringValue("name");
            if (type == "carrier" && (pnumber == id || name == id)) {
              SGSharedPtr<FGAICarrier> carrier = new FGAICarrier;
              carrier->readFromScenario(scEntry);
              
              if (carrier->getParkPosition(pid, geodPos, hdng, uvw)) {
                found = true;
                break;
              }
            }
          }
        }
      }
    }
  }
  return found;
}

//end AIManager.cxx
