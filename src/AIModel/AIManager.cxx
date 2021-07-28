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

#include <simgear/debug/ErrorReportingCallback.hxx>
#include <simgear/math/sg_geodesy.hxx>
#include <simgear/props/props_io.hxx>
#include <simgear/sg_inlines.h>
#include <simgear/structure/SGBinding.hxx>
#include <simgear/structure/commands.hxx>
#include <simgear/structure/exception.hxx>

#include <Add-ons/AddonManager.hxx>
#include <Airports/airport.hxx>
#include <Main/fg_props.hxx>
#include <Main/globals.hxx>
#include <Main/sentryIntegration.hxx>
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

static bool static_haveRegisteredScenarios = false;

class FGAIManager::Scenario
{
public:
    Scenario(FGAIManager* man, const std::string& nm, SGPropertyNode* scenarios) :
        _internalName(nm)
    {
        simgear::ErrorReportContext ec("scenario-name", _internalName);
        for (auto scEntry : scenarios->getChildren("entry")) {
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
            FGNasalSys* nasalSys = globals->get_subsystem<FGNasalSys>();
            std::string moduleName = "scenario_" + _internalName;
            bool ok = nasalSys->createModule(moduleName.c_str(), moduleName.c_str(),
                                             loadScript.c_str(), loadScript.size(),
                                             nullptr);

            if (!ok) {
                // TODO: get the Nasal errors logged properly
                simgear::reportFailure(simgear::LoadFailure::BadData, simgear::ErrorCode::ScenarioLoad,
                                       "Failed to parse scenario Nasal");
            }
        }
    }

    ~Scenario()
    {
        std::for_each(_objects.begin(), _objects.end(),
                      [](FGAIBasePtr ai) { ai->setDie(true); });


        FGNasalSys* nasalSys = globals->get_subsystem<FGNasalSys>();
        if (!nasalSys) // happens during shutdown / reset
            return;

        std::string moduleName = "scenario_" + _internalName;
        if (!_unloadScript.empty()) {
            nasalSys->createModule(moduleName.c_str(), moduleName.c_str(),
                                   _unloadScript.c_str(), _unloadScript.size(),
                                   nullptr);
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
               fgGetNode("/sim/rendering/static-lod/aimp-bare", true))),
    cb_ai_detailed(SGPropertyChangeCallback<FGAIManager>(this, &FGAIManager::updateLOD,
        fgGetNode("/sim/rendering/static-lod/aimp-detailed", true))),
    cb_interior(SGPropertyChangeCallback<FGAIManager>(this, &FGAIManager::updateLOD,
        fgGetNode("/sim/rendering/static-lod/aimp-interior", true)))
{

}

FGAIManager::~FGAIManager()
{
    std::for_each(ai_list.begin(), ai_list.end(), std::mem_fn(&FGAIBase::unbind));
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
    globals->get_commands()->addCommand("add-aiobject", this, &FGAIManager::addObjectCommand);
    globals->get_commands()->addCommand("remove-aiobject", this, &FGAIManager::removeObjectCommand);
    _environmentVisiblity = fgGetNode("/environment/visibility-m");
    _groundSpeedKts_node = fgGetNode("/velocities/groundspeed-kt", true);
    
    // Create an (invisible) AIAircraft representation of the current
    // users's aircraft, that mimicks the user aircraft's behavior.

    _userAircraft = new FGAIAircraft;
    _userAircraft->setCallSign ( fgGetString("/sim/multiplay/callsign")  );
    _userAircraft->setGeodPos(globals->get_aircraft_position());
    _userAircraft->setPerformance("", "jet_transport");
    _userAircraft->setHeading(fgGetDouble("/orientation/heading-deg"));
    _userAircraft->setSpeed(_groundSpeedKts_node->getDoubleValue());
    
    // radar properties
    _simRadarControl = fgGetNode("/sim/controls/radar", true);
    if (!_simRadarControl->hasValue()) {
        // default to true, but only if not already set
        _simRadarControl->setBoolValue(true);
    }
    _radarRangeNode = fgGetNode("/instrumentation/radar/range", true);
    _radarDebugNode = fgGetNode("/instrumentation/radar/debug-mode", true);

    // register scenarios if we didn't do it already
    registerScenarios();
}

void FGAIManager::registerScenarios(SGPropertyNode_ptr root)
{
    if (!root) {
        // depending on if we're using a carrier startup, this function may get
        // called early or during normal FGAIManager init, so guard against double
        // invocation.
        // we clear this flag on shudtdown so reset works as expected
        if (static_haveRegisteredScenarios)
            return;

        static_haveRegisteredScenarios = true;
        root = globals->get_props();
    }
    
    // find all scenarios at standard locations (for driving the GUI)
    std::vector<SGPath> scenarioSearchPaths;
    scenarioSearchPaths.push_back(globals->get_fg_root() / "AI");
    scenarioSearchPaths.push_back(globals->get_fg_home() / "Scenarios");
    scenarioSearchPaths.push_back(SGPath(fgGetString("/sim/aircraft-dir")) / "Scenarios");
    
    // add-on scenario directories
    const auto& addonsManager = flightgear::addons::AddonManager::instance();
    if (addonsManager) {
        for (auto a : addonsManager->registeredAddons()) {
            scenarioSearchPaths.push_back(a->getBasePath() / "Scenarios");
        }
    }

    SGPropertyNode_ptr scenariosNode = root->getNode("/sim/ai/scenarios", true);
    for (auto p : scenarioSearchPaths) {
        if (!p.exists())
            continue;
        
        simgear::Dir dir(p);
        for (auto xmlPath : dir.children(simgear::Dir::TYPE_FILE, ".xml")) {
            registerScenarioFile(root, xmlPath);
        } // of xml files in the scenario dir iteration
    } // of scenario dirs iteration
}

SGPropertyNode_ptr FGAIManager::registerScenarioFile(SGPropertyNode_ptr root, const SGPath& xmlPath)
{
    if (!xmlPath.exists()) return {};
    
    auto scenariosNode = root->getNode("/sim/ai/scenarios", true);
    SGPropertyNode_ptr sNode;

    simgear::ErrorReportContext ectx("scenario-path", xmlPath.utf8Str());

    try {
        SGPropertyNode_ptr scenarioProps(new SGPropertyNode);
        readProperties(xmlPath, scenarioProps);
        
        for (auto xs : scenarioProps->getChildren("scenario")) {
            if (!xs->hasChild("name") || !xs->hasChild("description")) {
                SG_LOG(SG_AI, SG_DEV_WARN, "Scenario is missing name/description:" << xmlPath);
            }
            
            sNode = scenariosNode->addChild("scenario");
            
            const auto bareName = xmlPath.file_base();
            sNode->setStringValue("id", bareName);
            sNode->setStringValue("path", xmlPath.utf8Str());
            
            if (xs->hasChild("name")) {
                sNode->setStringValue("name", xs->getStringValue("name"));
            } else {
                auto cleanedName = bareName;
               // replace _ and - in bareName with spaces
                // auto s = simgear::strutils::srep
                sNode->setStringValue("name", cleanedName);
            }
            
            if (xs->hasChild("description")) {
                sNode->setStringValue("description", xs->getStringValue("description"));
            }
            
            FGAICarrier::extractCarriersFromScenario(xs, sNode);
        } // of scenarios in the XML file
    } catch (sg_exception& e) {
        SG_LOG(SG_AI, SG_WARN, "Skipping malformed scenario file:" << xmlPath);
        simgear::reportFailure(simgear::LoadFailure::BadData, simgear::ErrorCode::ScenarioLoad,
                               string{"The scenario couldn't be loaded:"} + e.getFormattedMessage(),
                               e.getLocation());
        sNode.reset();
    }

    return sNode;
}

void
FGAIManager::postinit()
{
    // postinit, so that it can access the Nasal subsystem

    // scenarios enabled, AI subsystem required
    if (!enabled->getBoolValue())
        enabled->setBoolValue(true);

    // process all scenarios
    for (auto n : root->getChildren("scenario")) {
        const string& name = n->getStringValue();
        if (name.empty())
            continue;

        if (_scenarios.find(name) != _scenarios.end()) {
            SG_LOG(SG_AI, SG_DEV_WARN, "won't load scenario '" << name << "' twice");
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
    std::for_each(ai_list.begin(), ai_list.end(), std::mem_fn(&FGAIBase::reinit));

    // (re-)load scenarios
    postinit();
}

void
FGAIManager::shutdown()
{
    unloadAllScenarios();

    for (FGAIBase* ai : ai_list) {
        // other subsystems, especially ATC, may have references. This
        // lets them detect if the AI object should be skipped
        ai->setDie(true);
        ai->unbind();
    }

    ai_list.clear();
    _environmentVisiblity.clear();
    _userAircraft.clear();
    static_haveRegisteredScenarios = false;
    
    globals->get_commands()->removeCommand("load-scenario");
    globals->get_commands()->removeCommand("unload-scenario");
    globals->get_commands()->removeCommand("add-aiobject");
    globals->get_commands()->removeCommand("remove-aiobject");
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
FGAIManager::update(double dt)
{
    // initialize these for finding nearest thermals
    range_nearest = 10000.0;
    strength = 0.0;

    if (!enabled->getBoolValue())
        return;

    fetchUserState(dt);
    
    // fetch radar state. Ensure we only do this once per frame.
    _radarEnabled = _simRadarControl->getBoolValue();
    _radarDebugMode = _radarDebugNode->getBoolValue();
    _radarRangeM = _radarRangeNode->getDoubleValue() * SG_NM_TO_METER;
    
    // partition the list into dead followed by alive
    auto firstAlive =
      std::stable_partition(ai_list.begin(), ai_list.end(), std::mem_fn(&FGAIBase::getDie));

    // clean up each item and finally remove from the container
    for (auto it=ai_list.begin(); it != firstAlive; ++it) {
        removeDeadItem(*it);
    }

    ai_list.erase(ai_list.begin(), firstAlive);

    // every remaining item is alive. update them in turn, but guard for
    // exceptions, so a single misbehaving AI object doesn't bring down the
    // entire subsystem.
    for (FGAIBase* base : ai_list) {
        try {
            if (base->isa(FGAIBase::otThermal)) {
                processThermal(dt, static_cast<FGAIThermal*>(base));
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
    std::for_each(ai_list.begin(), ai_list.end(), std::mem_fn(&FGAIBase::updateLOD));
}

void
FGAIManager::attach(const SGSharedPtr<FGAIBase> &model)
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

    model->init(model->getSearchOrder());
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
    return static_cast<int>(ai_list.size());
}

void
FGAIManager::fetchUserState( double dt )
{

    globals->get_aircraft_orientation(user_heading, user_pitch, user_roll);
    user_speed     = user_speed_node->getDoubleValue() * 0.592484;
    wind_from_east = wind_from_east_node->getDoubleValue();
    wind_from_north   = wind_from_north_node->getDoubleValue();
    user_altitude_agl = user_altitude_agl_node->getDoubleValue();

    _userAircraft->setGeodPos(globals->get_aircraft_position());
    _userAircraft->setHeading(user_heading);
    _userAircraft->setSpeed(_groundSpeedKts_node->getDoubleValue());
    _userAircraft->update(dt);
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

bool FGAIManager::loadScenarioCommand(const SGPropertyNode* args, SGPropertyNode *)
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
        SGPropertyNode* scenarioNode = root->addChild("scenario");
        scenarioNode->setStringValue(name);
    }

    return ok;
}

bool FGAIManager::unloadScenarioCommand(const SGPropertyNode * arg, SGPropertyNode * root)
{
    SG_UNUSED(root);
    std::string name = arg->getStringValue("name");
    return unloadScenario(name);
}

bool FGAIManager::addObjectCommand(const SGPropertyNode* arg, const SGPropertyNode* root)
{
    SG_UNUSED(root);
    if (!arg){
        return false;
    }
    addObject(arg);
    return true;
}

FGAIBasePtr FGAIManager::addObject(const SGPropertyNode* definition)
{
    const std::string& type = definition->getStringValue("type", "aircraft");
    
    FGAIBase* ai = nullptr;
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
	if((ai->isValid())){
		attach(ai);
        SG_LOG(SG_AI, SG_DEBUG, "attached scenario " << ai->_getName());
	}
	else{
		ai->setDie(true);
		SG_LOG(SG_AI, SG_ALERT, "killed invalid scenario "  << ai->_getName());
	}
    return ai;
}

bool FGAIManager::removeObjectCommand(const SGPropertyNode* arg, const SGPropertyNode* root)
{
    SG_UNUSED(root);
    if (!arg) {
        return false;
    }
    return removeObject(arg);
}

bool FGAIManager::removeObject(const SGPropertyNode* args)
{
    int id = args->getIntValue("id");
    for (FGAIBase* ai : get_ai_list()) {
        if (ai->getID() == id) {
            ai->setDie(true);
            break;
        }
    }

    return false;
}

FGAIBasePtr FGAIManager::getObjectFromProperty(const SGPropertyNode* aProp) const
{
    auto it = std::find_if(ai_list.begin(), ai_list.end(),
                           [aProp](FGAIBasePtr ai) { return ai->_getProps() == aProp; });
    if (it == ai_list.end()) {
        return nullptr;
    }
    return *it;
}

bool
FGAIManager::loadScenario( const string &id )
{
    SGPath path;
    SGPropertyNode_ptr file = loadScenarioFile(id, path);
    if (!file) {
        return false;
    }

    simgear::ErrorReportContext ec("scenario-path", path.utf8Str());
    SGPropertyNode_ptr scNode = file->getChild("scenario");
    if (!scNode) {
        simgear::reportFailure(simgear::LoadFailure::Misconfigured,
                               simgear::ErrorCode::ScenarioLoad,
                               "No <scenario> element in file", path);
        return false;
    }

    assert(_scenarios.find(id) == _scenarios.end());
    _scenarios[id] = new Scenario(this, id, scNode);
    return true;
}


bool
FGAIManager::unloadScenario( const string &filename)
{
    auto it = _scenarios.find(filename);
    if (it == _scenarios.end()) {
        SG_LOG(SG_AI, SG_WARN, "unload scenario: not found:" << filename);
        return false;
    }

// remove /sim/ai node
    for (auto n : root->getChildren("scenario")) {
        if (n->getStringValue() == filename) {
            root->removeChild(n);
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
    std::for_each(_scenarios.begin(), _scenarios.end(),
                  [](const ScenarioDict::value_type& v) { delete v.second; });
    // remove /sim/ai node
    if(root) {
        root->removeChildren("scenario");
    }
    _scenarios.clear();
}


SGPropertyNode_ptr
FGAIManager::loadScenarioFile(const std::string& scenarioName, SGPath& outPath)
{
    auto s = fgGetNode("/sim/ai/scenarios");
    if (!s) return {};

    for (auto n : s->getChildren("scenario")) {
        if (n->getStringValue("id") == scenarioName) {
            SGPath path{n->getStringValue("path")};
            outPath = path;
            simgear::ErrorReportContext ec("scenario-path", path.utf8Str());
            try {
                SGPropertyNode_ptr root = new SGPropertyNode;
                readProperties(path, root);
                return root;
            } catch (const sg_exception &t) {
                SG_LOG(SG_AI, SG_ALERT, "Failed to load scenario '"
                       << path << "': " << t.getFormattedMessage());
                simgear::reportFailure(simgear::LoadFailure::BadData, simgear::ErrorCode::ScenarioLoad,
                                       "Failed to laod scenario XML:" + t.getFormattedMessage(),
                                       t.getLocation());
            }
        }
    }
    
    return {};
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
    return nullptr;
}

double
FGAIManager::calcRangeFt(const SGVec3d& aCartPos, const FGAIBase* aObject) const
{
    double distM = dist(aCartPos, aObject->getCartPos());
    return distM * SG_METER_TO_FEET;
}

FGAIAircraft* FGAIManager::getUserAircraft() const
{
    return _userAircraft.get();
}

// Register the subsystem.
SGSubsystemMgr::Registrant<FGAIManager> registrantFGAIManager(
    SGSubsystemMgr::POST_FDM,
    {{"nasal", SGSubsystemMgr::Dependency::HARD}});

//end AIManager.cxx
