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

#include <cstring>
#include <algorithm>

#include <simgear/sg_inlines.h>
#include <simgear/math/sg_geodesy.hxx>
#include <simgear/props/props_io.hxx>
#include <simgear/structure/exception.hxx>
#include <simgear/structure/commands.hxx>
#include <simgear/structure/SGBinding.hxx>

#include <boost/mem_fn.hpp>
#include <boost/foreach.hpp>

#include <Main/globals.hxx>
#include <Airports/airport.hxx>
#include <Scripting/NasalSys.hxx>

#include "AIManager.hxx"
#include "AIAircraft.hxx"
#include "AIShip.hxx"
#include "AIBallistic.hxx"
#include "AIStorm.hxx"
#include "AIThermal.hxx"
#include "AICarrier.hxx"
#include "AIStatic.hxx"
#include "AIMultiplayer.hxx"
#include "AITanker.hxx"
#include "AIWingman.hxx"
#include "AIGroundVehicle.hxx"
#include "AIEscort.hxx"

class FGAIManager::Scenario
{
public:
    Scenario(FGAIManager* man, const std::string& nm, SGPropertyNode* scenarios) :
        _internalName(nm)
    {
        BOOST_FOREACH(SGPropertyNode* scEntry, scenarios->getChildren("entry")) {
            FGAIBasePtr ai = man->addObject(scEntry);
            if (ai) {
                _objects.push_back(ai);
            }
        } // of scenario entry iteration
        
        SGPropertyNode* nasalScripts = scenarios->getChild("nasal");
        if (!nasalScripts) {
            return;
        }
        
        _unloadScript = nasalScripts->getStringValue("unload");
        std::string loadScript = nasalScripts->getStringValue("load");
        if (!loadScript.empty()) {
            FGNasalSys* nasalSys = (FGNasalSys*) globals->get_subsystem("nasal");
            std::string moduleName = "scenario_" + _internalName;
            nasalSys->createModule(moduleName.c_str(), moduleName.c_str(),
                                   loadScript.c_str(), loadScript.size(),
                                   0);
        }
    }
    
    ~Scenario()
    {
        BOOST_FOREACH(FGAIBasePtr ai, _objects) {
            ai->setDie(true);
        }
        
        FGNasalSys* nasalSys = (FGNasalSys*) globals->get_subsystem("nasal");
        if (!nasalSys)
            return;
        
        std::string moduleName = "scenario_" + _internalName;
        if (!_unloadScript.empty()) {
            nasalSys->createModule(moduleName.c_str(), moduleName.c_str(),
                                   _unloadScript.c_str(), _unloadScript.size(),
                                   0);
        }
        
        nasalSys->deleteModule(moduleName.c_str());
    }
private:
    std::vector<FGAIBasePtr> _objects;
    std::string _internalName;
    std::string _unloadScript;
};

///////////////////////////////////////////////////////////////////////////////

FGAIManager::FGAIManager() :
    cb_ai_bare(SGPropertyChangeCallback<FGAIManager>(this,&FGAIManager::updateLOD,
               fgGetNode("/sim/rendering/static-lod/ai-bare", true))),
    cb_ai_detailed(SGPropertyChangeCallback<FGAIManager>(this,&FGAIManager::updateLOD,
                   fgGetNode("/sim/rendering/static-lod/ai-detailed", true)))
{

}

FGAIManager::~FGAIManager()
{
    std::for_each(ai_list.begin(), ai_list.end(), boost::mem_fn(&FGAIBase::unbind));
}

void
FGAIManager::init() {
    root = fgGetNode("sim/ai", true);

    enabled = root->getNode("enabled", true);

    thermal_lift_node = fgGetNode("/environment/thermal-lift-fps", true);
    wind_from_east_node  = fgGetNode("/environment/wind-from-east-fps",true);
    wind_from_north_node = fgGetNode("/environment/wind-from-north-fps",true);

    user_altitude_agl_node  = fgGetNode("/position/altitude-agl-ft", true);
    user_speed_node     = fgGetNode("/velocities/uBody-fps", true);
    
    globals->get_commands()->addCommand("load-scenario", this, &FGAIManager::loadScenarioCommand);
    globals->get_commands()->addCommand("unload-scenario", this, &FGAIManager::unloadScenarioCommand);
    _environmentVisiblity = fgGetNode("/environment/visibility-m");
}

void
FGAIManager::postinit()
{
    // postinit, so that it can access the Nasal subsystem

    // scenarios enabled, AI subsystem required
    if (!enabled->getBoolValue())
        enabled->setBoolValue(true);

    // process all scenarios
    BOOST_FOREACH(SGPropertyNode* n, root->getChildren("scenario")) {
        const string& name = n->getStringValue();
        if (name.empty())
            continue;

        if (_scenarios.find(name) != _scenarios.end()) {
            SG_LOG(SG_AI, SG_WARN, "won't load scenario '" << name << "' twice");
            continue;
        }

        SG_LOG(SG_AI, SG_INFO, "loading scenario '" << name << '\'');
        loadScenario(name);
    }
}

void
FGAIManager::reinit()
{
    // shutdown scenarios
    unloadAllScenarios();
    
    update(0.0);
    std::for_each(ai_list.begin(), ai_list.end(), boost::mem_fn(&FGAIBase::reinit));
    
    // (re-)load scenarios
    postinit();
}

void
FGAIManager::shutdown()
{
    unloadAllScenarios();
    
    BOOST_FOREACH(FGAIBase* ai, ai_list) {
        ai->unbind();
    }
    
    ai_list.clear();
    _environmentVisiblity.clear();
    
    globals->get_commands()->removeCommand("load-scenario");
    globals->get_commands()->removeCommand("unload-scenario");
}

void
FGAIManager::bind() {
    root = globals->get_props()->getNode("ai/models", true);
    root->tie("count", SGRawValueMethods<FGAIManager, int>(*this,
        &FGAIManager::getNumAiObjects));
}

void
FGAIManager::unbind() {
    root->untie("count");
}

void FGAIManager::removeDeadItem(FGAIBase* base)
{
    SGPropertyNode *props = base->_getProps();
    
    props->setBoolValue("valid", false);
    base->unbind();
    
    // for backward compatibility reset properties, so that aircraft,
    // which don't know the <valid> property, keep working
    // TODO: remove after a while
    props->setIntValue("id", -1);
    props->setBoolValue("radar/in-range", false);
    props->setIntValue("refuel/tanker", false);
}

void
FGAIManager::update(double dt) {
    // initialize these for finding nearest thermals
    range_nearest = 10000.0;
    strength = 0.0;

    if (!enabled->getBoolValue())
        return;

    fetchUserState();

    // partition the list into dead followed by alive
    ai_list_iterator firstAlive =
      std::stable_partition(ai_list.begin(), ai_list.end(), boost::mem_fn(&FGAIBase::getDie));
    
    // clean up each item and finally remove from the container
    for (ai_list_iterator it=ai_list.begin(); it != firstAlive; ++it) {
        removeDeadItem(*it);
    }
  
    ai_list.erase(ai_list.begin(), firstAlive);
  
    // every remaining item is alive. update them in turn, but guard for
    // exceptions, so a single misbehaving AI object doesn't bring down the
    // entire subsystem.
    BOOST_FOREACH(FGAIBase* base, ai_list) {
        try {
            if (base->isa(FGAIBase::otThermal)) {
                processThermal(dt, (FGAIThermal*)base);
            } else {
                base->update(dt);
            }
        } catch (sg_exception& e) {
            SG_LOG(SG_AI, SG_WARN, "caught exception updating AI model:" << base->_getName()<< ", which will be killed."
                   "\n\tError:" << e.getFormattedMessage());
            base->setDie(true);
        }
    } // of live AI objects iteration

    thermal_lift_node->setDoubleValue( strength );  // for thermals
}

/** update LOD settings of all AI/MP models */
void
FGAIManager::updateLOD(SGPropertyNode* node)
{
    SG_UNUSED(node);
    std::for_each(ai_list.begin(), ai_list.end(), boost::mem_fn(&FGAIBase::updateLOD));
}

void
FGAIManager::attach(FGAIBase *model)
{
    const char* typeString = model->getTypeString();
    SGPropertyNode* root = globals->get_props()->getNode("ai/models", true);
    SGPropertyNode* p;
    int i;

    // find free index in the property tree, if we have
    // more than 10000 mp-aircrafts in the property tree we should optimize the mp-server
    for (i = 0; i < 10000; i++) {
        p = root->getNode(typeString, i, false);

        if (!p || !p->getBoolValue("valid", false))
            break;

        if (p->getIntValue("id",-1)==model->getID()) {
            p->setStringValue("callsign","***invalid node***"); //debug only, should never set!
        }
    }

    p = root->getNode(typeString, i, true);
    model->setManager(this, p);
    ai_list.push_back(model);

    model->init(model->getType()==FGAIBase::otAircraft
        || model->getType()==FGAIBase::otMultiplayer
        || model->getType()==FGAIBase::otStatic);
    model->bind();
    p->setBoolValue("valid", true);
}

bool FGAIManager::isVisible(const SGGeod& pos) const
{
  double visibility_meters = _environmentVisiblity->getDoubleValue();
  return ( dist(globals->get_view_position_cart(), SGVec3d::fromGeod(pos)) ) <= visibility_meters;
}

int
FGAIManager::getNumAiObjects() const
{
    return ai_list.size();
}

void
FGAIManager::fetchUserState( void ) {

    globals->get_aircraft_orientation(user_heading, user_pitch, user_roll);
    user_speed     = user_speed_node->getDoubleValue() * 0.592484;
    wind_from_east = wind_from_east_node->getDoubleValue();
    wind_from_north   = wind_from_north_node->getDoubleValue();
    user_altitude_agl = user_altitude_agl_node->getDoubleValue();

}

// only keep the results from the nearest thermal
void
FGAIManager::processThermal( double dt, FGAIThermal* thermal ) {
    thermal->update(dt);

    if ( thermal->_getRange() < range_nearest ) {
        range_nearest = thermal->_getRange();
        strength = thermal->getStrength();
    }

}

bool FGAIManager::loadScenarioCommand(const SGPropertyNode* args)
{
    std::string name = args->getStringValue("name");
    if (args->hasChild("load-property")) {
        // slightly ugly, to simplify life in the dialogs, make load allow
        // loading or unloading based on a bool property.
        bool loadIt = fgGetBool(args->getStringValue("load-property"));
        if (!loadIt) {
            // user actually wants to unload, fine.
            return unloadScenario(name);
        }
    }
    
    if (_scenarios.find(name) != _scenarios.end()) {
        SG_LOG(SG_AI, SG_WARN, "scenario '" << name << "' already loaded");
        return false;
    }
    
    bool ok = loadScenario(name);
    if (ok) {
        // create /sim/ai node for consistency
        int index = 0;
        for (; root->hasChild("scenario", index); ++index) {}
        
        SGPropertyNode* scenarioNode = root->getChild("scenario", index, true);
        scenarioNode->setStringValue(name);
    }
    
    return ok;
}

bool FGAIManager::unloadScenarioCommand(const SGPropertyNode* args)
{
    std::string name = args->getStringValue("name");
    return unloadScenario(name);
}

bool FGAIManager::addObjectCommand(const SGPropertyNode* definition)
{
    addObject(definition);
    return true;
}

FGAIBasePtr FGAIManager::addObject(const SGPropertyNode* definition)
{
    const std::string& type = definition->getStringValue("type", "aircraft");
    
    FGAIBase* ai = NULL;
    if (type == "tanker") { // refueling scenarios
        ai = new FGAITanker; 
    } else if (type == "wingman") {
        ai = new FGAIWingman;
    } else if (type == "aircraft") {
        ai = new FGAIAircraft;
    } else if (type == "ship") {
        ai = new FGAIShip;
    } else if (type == "carrier") {
        ai = new FGAICarrier;
    } else if (type == "groundvehicle") {
        ai = new FGAIGroundVehicle;
    } else if (type == "escort") {
        ai = new FGAIEscort;
    } else if (type == "thunderstorm") {
        ai = new FGAIStorm;
    } else if (type == "thermal") {
        ai = new FGAIThermal;
    } else if (type == "ballistic") {
        ai = new FGAIBallistic;
    } else if (type == "static") {
        ai = new FGAIStatic;
    }

    ai->readFromScenario(const_cast<SGPropertyNode*>(definition));
    attach(ai);
    return ai;
}

bool FGAIManager::removeObject(const SGPropertyNode* args)
{
    int id = args->getIntValue("id");
    BOOST_FOREACH(FGAIBase* ai, get_ai_list()) {
        if (ai->getID() == id) {
            ai->setDie(true);
            break;
        }
    }
    
    return false;
}

FGAIBasePtr FGAIManager::getObjectFromProperty(const SGPropertyNode* aProp) const
{
    BOOST_FOREACH(FGAIBase* ai, get_ai_list()) {
        if (ai->_getProps() == aProp) {
            return ai;
        }
    } // of AI objects iteration
    
    return NULL;
}

bool
FGAIManager::loadScenario( const string &filename )
{
    SGPropertyNode_ptr file = loadScenarioFile(filename);
    if (!file) {
        return false;
    }
    
    SGPropertyNode_ptr scNode = file->getChild("scenario");
    if (!scNode) {
        return false;
    }
    
    _scenarios[filename] = new Scenario(this, filename, scNode);
    return true;
}


bool
FGAIManager::unloadScenario( const string &filename)
{
    ScenarioDict::iterator it = _scenarios.find(filename);
    if (it == _scenarios.end()) {
        SG_LOG(SG_AI, SG_WARN, "unload scenario: not found:" << filename);
        return false;
    }
    
// remove /sim/ai node
    unsigned int index = 0;
    for (SGPropertyNode* n = NULL; (n = root->getChild("scenario", index)) != NULL; ++index) {
        if (n->getStringValue() == filename) {
            root->removeChild("scenario", index);
            break;
        }
    }
    
    delete it->second;
    _scenarios.erase(it);
    return true;
}

void
FGAIManager::unloadAllScenarios()
{
    ScenarioDict::iterator it = _scenarios.begin();
    for (; it != _scenarios.end(); ++it) {
        delete it->second;
    } // of scenarios iteration
    
    
    // remove /sim/ai node
    root->removeChildren("scenario");
    _scenarios.clear();
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
    } catch (const sg_exception &t) {
        SG_LOG(SG_AI, SG_ALERT, "Failed to load scenario '"
            << path.str() << "': " << t.getFormattedMessage());
    }
    return 0;
}

bool
FGAIManager::getStartPosition(const string& id, const string& pid,
                              SGGeod& geodPos, double& hdng, SGVec3d& uvw)
{
    bool found = false;
    SGPropertyNode* root = fgGetNode("sim/ai", true);
    if (!root->getNode("enabled", true)->getBoolValue())
        return found;

    for (int i = 0 ; (!found) && i < root->nChildren() ; i++) {
        SGPropertyNode *aiEntry = root->getChild( i );
        if ( !strcmp( aiEntry->getName(), "scenario" ) ) {
            const string& filename = aiEntry->getStringValue();
            SGPropertyNode_ptr scenarioTop = loadScenarioFile(filename);
            if (scenarioTop) {
                SGPropertyNode* scenarios = scenarioTop->getChild("scenario");
                if (scenarios) {
                    for (int i = 0; i < scenarios->nChildren(); i++) {
                        SGPropertyNode* scEntry = scenarios->getChild(i);
                        const std::string& type = scEntry->getStringValue("type");
                        const std::string& pnumber = scEntry->getStringValue("pennant-number");
                        const std::string& name = scEntry->getStringValue("name");
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

const FGAIBase *
FGAIManager::calcCollision(double alt, double lat, double lon, double fuse_range)
{
    // we specify tgt extent (ft) according to the AIObject type
    double tgt_ht[]     = {0,  50, 100, 250, 0, 100, 0, 0,  50,  50, 20, 100,  50};
    double tgt_length[] = {0, 100, 200, 750, 0,  50, 0, 0, 200, 100, 40, 200, 100};
    ai_list_iterator ai_list_itr = ai_list.begin();
    ai_list_iterator end = ai_list.end();

    SGGeod pos(SGGeod::fromDegFt(lon, lat, alt));
    SGVec3d cartPos(SGVec3d::fromGeod(pos));
    
    while (ai_list_itr != end) {
        double tgt_alt = (*ai_list_itr)->_getAltitude();
        int type       = (*ai_list_itr)->getType();
        tgt_ht[type] += fuse_range;

        if (fabs(tgt_alt - alt) > tgt_ht[type] || type == FGAIBase::otBallistic
            || type == FGAIBase::otStorm || type == FGAIBase::otThermal ) {
                //SG_LOG(SG_AI, SG_DEBUG, "AIManager: skipping "
                //    << fabs(tgt_alt - alt)
                //    << " "
                //    << type
                //    );
                ++ai_list_itr;
                continue;
        }

        int id         = (*ai_list_itr)->getID();

        double range = calcRangeFt(cartPos, (*ai_list_itr));

        //SG_LOG(SG_AI, SG_DEBUG, "AIManager:  AI list size "
        //    << ai_list.size()
        //    << " type " << type
        //    << " ID " << id
        //    << " range " << range
        //    //<< " bearing " << bearing
        //    << " alt " << tgt_alt
        //    );

        tgt_length[type] += fuse_range;

        if (range < tgt_length[type]){
            SG_LOG(SG_AI, SG_DEBUG, "AIManager: HIT! "
                << " type " << type
                << " ID " << id
                << " range " << range
                << " alt " << tgt_alt
                );
            return (*ai_list_itr).get();
        }
        ++ai_list_itr;
    }
    return 0;
}

double
FGAIManager::calcRangeFt(const SGVec3d& aCartPos, FGAIBase* aObject) const
{
    double distM = dist(aCartPos, aObject->getCartPos());
    return distM * SG_METER_TO_FEET;
}

//end AIManager.cxx
