//// submodel.cxx - models a releasable submodel.
// Written by Dave Culp, started Aug 2004
// With major additions by Vivian Meaaza 2004 - 2007
//
// This file is in the Public Domain and comes with no warranty.

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include "submodel.hxx"

#include <algorithm>

#include <simgear/structure/exception.hxx>
#include <simgear/misc/sg_path.hxx>
#include <simgear/math/sg_geodesy.hxx>
#include <simgear/props/props_io.hxx>

#include <Main/fg_props.hxx>
#include <Main/util.hxx>

#include "AIBase.hxx"
#include "AIManager.hxx"
#include "AIBallistic.hxx"

using std::cout;
using std::endl;
using std::string;
using std::vector;

const double FGSubmodelMgr::lbs_to_slugs = 0.031080950172;

FGSubmodelMgr::FGSubmodelMgr()
{
    x_offset = y_offset = z_offset = 0.0;
    pitch_offset = 0.0;
    yaw_offset = 0.0;

    //out[0] = out[1] = out[2] = 0;
    //string contents_node;
    contrail_altitude = 30000;
    _count = 0;
    _found_sub = true;
}

FGSubmodelMgr::~FGSubmodelMgr()
{
}

FGAIManager* FGSubmodelMgr::aiManager()
{
   return globals->get_subsystem<FGAIManager>();
}

void FGSubmodelMgr::init()
{
    index = 0;

    _serviceable_node = fgGetNode("/sim/submodels/serviceable", true);
    _serviceable_node->setBoolValue(true);

    _user_lat_node = fgGetNode("/position/latitude-deg", true);
    _user_lon_node = fgGetNode("/position/longitude-deg", true);
    _user_alt_node = fgGetNode("/position/altitude-ft", true);

    _user_heading_node = fgGetNode("/orientation/heading-deg", true);
    _user_pitch_node   = fgGetNode("/orientation/pitch-deg", true);
    _user_roll_node    = fgGetNode("/orientation/roll-deg", true);
    _user_yaw_node     = fgGetNode("/orientation/yaw-deg", true);
    _user_alpha_node   = fgGetNode("/orientation/alpha-deg", true);

    _user_speed_node = fgGetNode("/velocities/uBody-fps", true);

    _user_wind_from_east_node  = fgGetNode("/environment/wind-from-east-fps", true);
    _user_wind_from_north_node = fgGetNode("/environment/wind-from-north-fps", true);

    _user_speed_down_fps_node   = fgGetNode("/velocities/speed-down-fps", true);
    _user_speed_east_fps_node   = fgGetNode("/velocities/speed-east-fps", true);
    _user_speed_north_fps_node  = fgGetNode("/velocities/speed-north-fps", true);

    _contrail_altitude_node = fgGetNode("/environment/params/contrail-altitude", true);
    contrail_altitude       = _contrail_altitude_node->getDoubleValue();
    _contrail_trigger       = fgGetNode("ai/submodels/contrails", true);
    _contrail_trigger->setBoolValue(false);

    load();
}

void FGSubmodelMgr::postinit()
{
    // postinit, so that the AI list is populated
    loadAI();

    while (_found_sub)
        loadSubmodels();

    //TODO reload submodels if an MP ac joins

    //_model_added_node = fgGetNode("ai/models/model-added", true);
    //_model_added_node->addChangeListener(this, false);
}

void FGSubmodelMgr::shutdown()
{
    std::for_each(submodels.begin(), submodels.end(), [](submodel* sm) { delete sm; });
    submodels.clear();
}

void FGSubmodelMgr::bind()
{
}

void FGSubmodelMgr::unbind()
{
    submodel_iterator = submodels.begin();
    while (submodel_iterator != submodels.end()) {
        (*submodel_iterator)->prop->untie("count");
        ++submodel_iterator;
    }
}

void FGSubmodelMgr::update(double dt)
{
    if (!_serviceable_node->getBoolValue())
        return;

    _impact = false;
    _hit = false;
    _expiry = false;

    // Check if the submodel hit an object or terrain
    FGAIManager::ai_list_type sm_list(aiManager()->get_ai_list());
    FGAIManager::ai_list_iterator sm_list_itr = sm_list.begin();
    FGAIManager::ai_list_iterator end         = sm_list.end();

    for (; sm_list_itr != end; ++sm_list_itr) {
        FGAIBase::object_type object_type =(*sm_list_itr)->getType();

        // Continue if object is not ballistic
        if (object_type != FGAIBase::otBallistic) {
            continue;
        }

        int parent_subID = (*sm_list_itr)->_getSubID();
        int id = (*sm_list_itr)->getID();

        if (parent_subID == 0 || id == -1) // this entry in the list has no associated submodel
            continue;                       // or is invalid so we can continue

        //SG_LOG(SG_AI, SG_DEBUG, "Submodel: Impact " << _impact << " hit! "
        //        << _hit <<" parent_subID " << parent_subID);

        _hit = (*sm_list_itr)->_getCollisionData();
        _impact = (*sm_list_itr)->_getImpactData();
        _expiry = (*sm_list_itr)->_getExpiryData();

        //SG_LOG(SG_AI, SG_ALERT, "Submodel: " << (*sm_list_itr)->_getName()
        //    << " Impact " << _impact << " hit! " << _hit
        //    << " exipiry :-( " << _expiry );

        if (_impact || _hit || _expiry) {
    //        SG_LOG(SG_AI, SG_ALERT, "Submodel: Impact " << _impact << " hit! " << _hit
                //<< " exipiry :-( " << _expiry );

            submodel_iterator = submodels.begin();

            while (submodel_iterator != submodels.end()) {
                int child_ID = (*submodel_iterator)->id;
                //cout << "Impact: parent SubID " << parent_subID << " child_ID " << child_ID << endl;

                if ( parent_subID == child_ID ) {
                    _parent_lat = (*sm_list_itr)->_getImpactLat();
                    _parent_lon = (*sm_list_itr)->_getImpactLon();
                    _parent_elev = (*sm_list_itr)->_getImpactElevFt();
                    _parent_hdg = (*sm_list_itr)->_getImpactHdg();
                    _parent_pitch = (*sm_list_itr)->_getImpactPitch();
                    _parent_roll = (*sm_list_itr)->_getImpactRoll();
                    _parent_speed = (*sm_list_itr)->_getImpactSpeed();
                    (*submodel_iterator)->first_time = true;
                    //cout << "Impact: parent SubID = child_ID elev " << _parent_elev << endl;

                    if (release(*submodel_iterator, dt)) {
                        (*sm_list_itr)->setDie(true);
                        //cout << "Impact: set die" << (*sm_list_itr)->_getName() << endl;
                    }

                }

                ++submodel_iterator;
            }
        }
    }

    _contrail_trigger->setBoolValue(_user_alt_node->getDoubleValue() > contrail_altitude);

    bool trigger = false;
    int i = -1;

    submodel_iterator = submodels.begin();
    while (submodel_iterator != submodels.end())  {
        i++;

        /*SG_LOG(SG_AI, SG_DEBUG,
                "Submodels:  " << (*submodel_iterator)->id
                << " name " << (*submodel_iterator)->name
                );*/

        if ((*submodel_iterator)->trigger_node != 0) {
            _trigger_node = (*submodel_iterator)->trigger_node;
            trigger = _trigger_node->getBoolValue();
            //cout << (*submodel_iterator)->name << "trigger node found " <<  trigger << endl;
        }
        else {
            trigger = false;
            //cout << (*submodel_iterator)->name << " trigger node not found " << trigger << endl;
        }

        if (trigger && (*submodel_iterator)->count != 0) {
            //int id = (*submodel_iterator)->id;
            //const string& name = (*submodel_iterator)->name;

            SG_LOG(SG_AI, SG_DEBUG,
            "Submodels release:  " << (*submodel_iterator)->id
            << " name " << (*submodel_iterator)->name
            << " count " << (*submodel_iterator)->count
            << " slaved " << (*submodel_iterator)->slaved
            );

            release(*submodel_iterator, dt);
        } else
            (*submodel_iterator)->first_time = true;

        ++submodel_iterator;
    }
}

bool FGSubmodelMgr::release(submodel *sm, double dt)
{
    // Only run if first time or repeat is set to true
    if (!sm->first_time && !sm->repeat) {
        return false;
    }

    sm->timer += dt;

    if (sm->timer < sm->delay) {
        //cout << "not yet: timer " << sm->timer << " delay " << sm->delay << endl;
        return false;
    }

    //cout << "released timer: " << sm->timer << " delay " << sm->delay << endl;

    sm->timer = 0.0;

    if (sm->first_time) {
        dt = 0.0;
        sm->first_time = false;
    }

    // Calculate submodel's initial conditions in world-coordinates
    transform(sm);

    FGAIBallistic* ballist = new FGAIBallistic;
    ballist->setPath(sm->model.c_str());
    ballist->setName(sm->name);
    ballist->setSlaved(sm->slaved);
    ballist->setRandom(sm->random);
    ballist->setLifeRandomness(sm->life_randomness->get_value());
    ballist->setLatitude(offsetpos.getLatitudeDeg());
    ballist->setLongitude(offsetpos.getLongitudeDeg());
    ballist->setAltitude(offsetpos.getElevationFt());
    ballist->setAzimuthRandomError(sm->azimuth_error->get_value());
    ballist->setAzimuth(IC.azimuth);
    ballist->setElevationRandomError(sm->elevation_error->get_value());
    ballist->setElevation(IC.elevation);
    ballist->setRoll(IC.roll);
    ballist->setSpeed(IC.speed / SG_KT_TO_FPS);
    ballist->setWind_from_east(IC.wind_from_east);
    ballist->setWind_from_north(IC.wind_from_north);
    ballist->setMass(IC.mass);
    ballist->setDragArea(sm->drag_area);
    ballist->setLife(sm->life);
    ballist->setBuoyancy(sm->buoyancy);
    ballist->setWind(sm->wind);
    ballist->setCdRandomness(sm->cd_randomness->get_value());
    ballist->setCd(sm->cd);
    ballist->setStabilisation(sm->aero_stabilised);
    ballist->setNoRoll(sm->no_roll);
    ballist->setCollision(sm->collision);
    ballist->setExpiry(sm->expiry);
    ballist->setImpact(sm->impact);
    ballist->setImpactReportNode(sm->impact_report);
    ballist->setFuseRange(sm->fuse_range);
    ballist->setSubmodel(sm->submodel.c_str());
    ballist->setSubID(sm->sub_id);
    ballist->setForceStabilisation(sm->force_stabilised);
    ballist->setExternalForce(sm->ext_force);
    ballist->setForcePath(sm->force_path.c_str());
    ballist->setXoffset(_x_offset);
    ballist->setYoffset(_y_offset);
    ballist->setZoffset(_z_offset);
    ballist->setPitchoffset(sm->pitch_offset->get_value());
    ballist->setYawoffset(sm->yaw_offset->get_value());
    ballist->setParentNodes(_selected_ac);
    ballist->setContentsNode(sm->contents_node);
    ballist->setWeight(sm->weight);

    aiManager()->attach(ballist);

    if (sm->count > 0)
        sm->count--;
    return true;
}

void FGSubmodelMgr::load()
{
    SGPropertyNode_ptr path_node = fgGetNode("/sim/submodels/path");

    if (path_node) {
        const int id = 0;
        const string& path = path_node->getStringValue();
        bool serviceable =_serviceable_node->getBoolValue();
        setData(id, path, serviceable, "/ai/submodels/submodel", submodels);
    }
}

void FGSubmodelMgr::transform(submodel *sm)
{
    // Set initial conditions
    if (sm->contents_node != 0 && !sm->slaved) {
        // Get the weight of the contents (lbs) and convert to mass (slugs)
        sm->contents = sm->contents_node->getChild("level-lbs",0,1)->getDoubleValue();
        //cout << "transform: contents " << sm->contents << endl;
        IC.mass = (sm->weight + sm->contents) * lbs_to_slugs;
        //cout << "mass inc contents"  << IC.mass << endl;

        // Set contents to 0 in the parent
        sm->contents_node->getChild("level-gal_us",0,1)->setDoubleValue(0);
        /*cout << "contents " << sm->contents_node->getChild("level-gal_us")->getDoubleValue()
        << " " << sm->contents_node->getChild("level-lbs",0,1)->getDoubleValue()
        << endl;*/
    }
    else {
        IC.mass = sm->weight * lbs_to_slugs;
    }

    int id = sm->id;
    //int sub_id = sm->sub_id;
    //const string& name = sm->name;

    if (sm->speed_node != 0)
        sm->speed = sm->speed_node->getDoubleValue();

    // set the Initial Conditions for the types of submodel parent

    if (_impact || _hit || _expiry) {
        _count++;

        // Set the data for a submodel tied to a submodel
        IC.lat             = _parent_lat;
        IC.lon             = _parent_lon;
        IC.alt             = _parent_elev;
        IC.roll            = _parent_roll;    // rotation about x axis
        IC.elevation       = _parent_pitch;   // rotation about y axis
        IC.azimuth         = _parent_hdg;     // rotation about z axis
        IC.speed           = _parent_speed;
        IC.speed_down_fps  = 0;
        IC.speed_east_fps  = 0;
        IC.speed_north_fps = 0;
    }
    else if (id == 0) {
        // Set the data for a submodel tied to the main model
        IC.lat             = _user_lat_node->getDoubleValue();
        IC.lon             = _user_lon_node->getDoubleValue();
        IC.alt             = _user_alt_node->getDoubleValue();
        IC.roll            = _user_roll_node->getDoubleValue();    // rotation about x axis
        IC.elevation       = _user_pitch_node->getDoubleValue();   // rotation about y axis
        IC.azimuth         = _user_heading_node->getDoubleValue(); // rotation about z axis
        IC.speed           = _user_speed_node->getDoubleValue();
        IC.speed_down_fps  = _user_speed_down_fps_node->getDoubleValue();
        IC.speed_east_fps  = _user_speed_east_fps_node->getDoubleValue();
        IC.speed_north_fps = _user_speed_north_fps_node->getDoubleValue();
    }
    else {
        // Set the data for a submodel tied to an AI Object
        setParentNode(id);
    }

    // Set the Initial Conditions that are common to all types of parent
    IC.wind_from_east =  _user_wind_from_east_node->getDoubleValue();
    IC.wind_from_north = _user_wind_from_north_node->getDoubleValue();

    userpos.setLatitudeDeg(IC.lat);
    userpos.setLongitudeDeg(IC.lon);
    userpos.setElevationFt(IC.alt);

    if (sm->offsets_in_meter) {
        _x_offset = -sm->x_offset->get_value() * SG_METER_TO_FEET;
        _y_offset =  sm->y_offset->get_value() * SG_METER_TO_FEET;
        _z_offset =  sm->z_offset->get_value() * SG_METER_TO_FEET;
    }
    else {
        _x_offset = sm->x_offset->get_value();
        _y_offset = sm->y_offset->get_value();
        _z_offset = sm->z_offset->get_value();
    }
    
    // this updates the 'offsetpos' member of us
    setOffsetPos(sm);

    // Compute initial orientation using yaw and pitch offsets and parent's orientation
    const double yaw_offset = sm->yaw_offset->get_value();
    const double pitch_offset = sm->pitch_offset->get_value();

    SGQuatd ic_quat = SGQuatd::fromYawPitchRollDeg(IC.azimuth, IC.elevation, IC.roll);
    ic_quat *= SGQuatd::fromYawPitchRollDeg(yaw_offset, pitch_offset, 0.0);

    // Calculate total speed using speeds of submodel and parent
    SGVec3d total_speed = SGVec3d(IC.speed_north_fps, IC.speed_east_fps, IC.speed_down_fps);
    total_speed += ic_quat.rotate(SGVec3d(sm->speed, 0, 0));

    IC.speed = length(total_speed);

    // If speeds are low this calculation can become unreliable
    if (IC.speed > 1) {
        const double total_speed_north = total_speed.x();
        const double total_speed_east  = total_speed.y();
        const double total_speed_down  = total_speed.z();

        IC.azimuth = atan2(total_speed_east, total_speed_north) * SG_RADIANS_TO_DEGREES;

        // Rationalize the output
        if (IC.azimuth < 0)
            IC.azimuth += 360;
        else if (IC.azimuth >= 360)
            IC.azimuth -= 360;

        IC.elevation = -atan(total_speed_down / sqrt(total_speed_north
            * total_speed_north + total_speed_east * total_speed_east))
            * SG_RADIANS_TO_DEGREES;
    }
    else {
        double ic_roll;
        ic_quat.getEulerDeg(IC.azimuth, IC.elevation, ic_roll);
    }
}

void FGSubmodelMgr::loadAI()
{
    SG_LOG(SG_AI, SG_DEBUG, "Submodels: Loading AI submodels");

    FGAIManager::ai_list_type sm_list(aiManager()->get_ai_list());

    if (sm_list.empty()) {
        SG_LOG(SG_AI, SG_ALERT, "Submodels: Unable to read AI submodel list");
        return;
    }

    FGAIManager::ai_list_iterator sm_list_itr = sm_list.begin();
    FGAIManager::ai_list_iterator end         = sm_list.end();

    while (sm_list_itr != end) {
        string path = (*sm_list_itr)->_getSMPath();

        if (path.empty()) {
            ++sm_list_itr;
            continue;
        }

        int id = (*sm_list_itr)->getID();
        bool serviceable = (*sm_list_itr)->_getServiceable();

        //string type = (*sm_list_itr)->getTypeString();
        //cout << "loadAI: type " << type << " path "<< path << " serviceable " << serviceable << endl;

        setData(id, path, serviceable, "/ai/submodels/submodel", submodels);
        ++sm_list_itr;
    }
}

void FGSubmodelMgr::setData(int id, const string& path, bool serviceable, const string& property_path, submodel_vector_type& models)
{
    SGPropertyNode root;

    SGPath config = globals->resolve_aircraft_path(path);
    if (!config.exists()) {
        SG_LOG(SG_AI, SG_DEV_ALERT, "missing AI submodels file: " << config);
        return;
    }

    try {
        SG_LOG(SG_AI, SG_DEBUG,
                "Submodels: Trying to read AI submodels file: " << config);
        readProperties(config, &root);
    }
    catch (const sg_exception &) {
        SG_LOG(SG_AI, SG_ALERT,
                "Submodels: Unable to read AI submodels file: " << config);
        return;
    }

    vector<SGPropertyNode_ptr> children = root.getChildren("submodel");
    vector<SGPropertyNode_ptr>::iterator it = children.begin();
    vector<SGPropertyNode_ptr>::iterator end = children.end();

    for (int i = 0; it != end; ++it, i++) {
        //cout << "Reading AI submodel " << (*it)->getPath() << endl;
        submodel *sm = new submodel;
        SGPropertyNode_ptr entry_node = *it;
        sm->name             = entry_node->getStringValue("name", "none_defined");
        sm->model            = entry_node->getStringValue("model", "Models/Geometry/rocket.ac");
        sm->speed            = entry_node->getDoubleValue("speed", 2329.4);
        sm->repeat           = entry_node->getBoolValue("repeat", false);
        sm->delay            = entry_node->getDoubleValue("delay", 0.25);
        sm->count            = entry_node->getIntValue("count", 1);
        sm->slaved           = entry_node->getBoolValue("slaved", false);
        sm->drag_area        = entry_node->getDoubleValue("eda", 0.034);
        sm->life             = entry_node->getDoubleValue("life", 900.0);
        sm->buoyancy         = entry_node->getDoubleValue("buoyancy", 0);
        sm->wind             = entry_node->getBoolValue("wind", false);
        sm->cd               = entry_node->getDoubleValue("cd", 0.193);
        sm->weight           = entry_node->getDoubleValue("weight", 0.25);
        sm->aero_stabilised  = entry_node->getBoolValue("aero-stabilised", true);
        sm->no_roll          = entry_node->getBoolValue("no-roll", false);
        sm->collision        = entry_node->getBoolValue("collision", false);
        sm->expiry           = entry_node->getBoolValue("expiry", false);
        sm->impact           = entry_node->getBoolValue("impact", false);
        sm->impact_report    = entry_node->getStringValue("impact-reports");
        sm->fuse_range       = entry_node->getDoubleValue("fuse-range", 0.0);
        sm->contents_node    = fgGetNode(entry_node->getStringValue("contents", "none"), false);
        sm->speed_node       = fgGetNode(entry_node->getStringValue("speed-prop", "none"), false);
        sm->submodel         = entry_node->getStringValue("submodel-path", "");
        sm->force_stabilised = entry_node->getBoolValue("force-stabilised", false);
        sm->ext_force        = entry_node->getBoolValue("external-force", false);
        sm->force_path       = entry_node->getStringValue("force-path", "");
        sm->random           = entry_node->getBoolValue("random", false);

        SGPropertyNode_ptr prop_root = fgGetNode("/", true);
        SGPropertyNode n;
        SGPropertyNode_ptr a, b;

        // Offsets
        a = entry_node->getNode("offsets", false);
        sm->offsets_in_meter = (a != 0);

        if (a) {
            b = a->getNode("x-m");
            sm->x_offset = new FGXMLAutopilot::InputValue(*prop_root, b ? *b : n);

            b = a->getNode("y-m");
            sm->y_offset = new FGXMLAutopilot::InputValue(*prop_root, b ? *b : n);

            b = a->getNode("z-m");
            sm->z_offset = new FGXMLAutopilot::InputValue(*prop_root, b ? *b : n);

            b = a->getNode("heading-deg");
            sm->yaw_offset   = new FGXMLAutopilot::InputValue(*prop_root, b ? *b : n);

            b = a->getNode("pitch-deg");
            sm->pitch_offset = new FGXMLAutopilot::InputValue(*prop_root, b ? *b : n);
        }
        else {
            bool old = false;

            b = entry_node->getNode("x-offset");
            sm->x_offset = new FGXMLAutopilot::InputValue(*prop_root, b ? *b : n);
            if (b) old = true;

            b = entry_node->getNode("y-offset");
            sm->y_offset = new FGXMLAutopilot::InputValue(*prop_root, b ? *b : n);
            if (b) old = true;

            b = entry_node->getNode("z-offset");
            sm->z_offset = new FGXMLAutopilot::InputValue(*prop_root, b ? *b : n);
            if (b) old = true;

            b = entry_node->getNode("yaw-offset");
            sm->yaw_offset   = new FGXMLAutopilot::InputValue(*prop_root, b ? *b : n);
            if (b) old = true;

            b = entry_node->getNode("pitch-offset");
            sm->pitch_offset = new FGXMLAutopilot::InputValue(*prop_root, b ? *b : n);
            if (b) old = true;

            if (old) {
                SG_LOG(SG_AI, SG_DEV_WARN, "Submodels: <*-offset> is deprecated. Use <offsets> instead");
            }
        }

        // Randomness
        a = entry_node->getNode("randomness", true);

        // Maximum azimuth randomness error in degrees
        b = a->getNode("azimuth");
        sm->azimuth_error   = new FGXMLAutopilot::InputValue(*prop_root, b ? *b : n);

        // Maximum elevation randomness error in degrees
        b = a->getNode("elevation");
        sm->elevation_error = new FGXMLAutopilot::InputValue(*prop_root, b ? *b : n);

        // Randomness of Cd (plus or minus)
        b = a->getNode("cd");
        if (!b) {
            b = a->getNode("cd", true);
            b->setDoubleValue(0.1);
        }
        sm->cd_randomness   = new FGXMLAutopilot::InputValue(*prop_root, *b);

        // Randomness of life (plus or minus)
        b = a->getNode("life");
        if (!b) {
            b = a->getNode("life", true);
            b->setDoubleValue(0.5);
        }
        sm->life_randomness = new FGXMLAutopilot::InputValue(*prop_root, *b);

        if (sm->contents_node != 0)
            sm->contents = sm->contents_node->getDoubleValue();

        const char *trigger_path = entry_node->getStringValue("trigger", 0);
        if (trigger_path) {
            sm->trigger_node = fgGetNode(trigger_path, true);
            sm->trigger_node->setBoolValue(sm->trigger_node->getBoolValue());
        }
        else {
            sm->trigger_node = 0;
        }

        if (sm->speed_node != 0)
            sm->speed = sm->speed_node->getDoubleValue();

        sm->timer = sm->delay;
        sm->id = id;
        sm->first_time = false;
        sm->serviceable = serviceable;
        sm->sub_id = 0;

        sm->prop = fgGetNode(property_path, index, true);
        sm->prop->tie("delay", SGRawValuePointer<double>(&(sm->delay)));
        sm->prop->tie("count", SGRawValuePointer<int>(&(sm->count)));
        sm->prop->tie("repeat", SGRawValuePointer<bool>(&(sm->repeat)));
        sm->prop->tie("id", SGRawValuePointer<int>(&(sm->id)));
        sm->prop->tie("sub-id", SGRawValuePointer<int>(&(sm->sub_id)));
        sm->prop->tie("serviceable", SGRawValuePointer<bool>(&(sm->serviceable)));
        sm->prop->tie("random", SGRawValuePointer<bool>(&(sm->random)));
        sm->prop->tie("slaved", SGRawValuePointer<bool>(&(sm->slaved)));
        const string& name = sm->name;
        sm->prop->setStringValue("name", name.c_str());

        const string& submodel = sm->submodel;
        sm->prop->setStringValue("submodel", submodel.c_str());

        const string& force_path = sm->force_path;
        sm->prop->setStringValue("force_path", force_path.c_str());

        if (sm->contents_node != 0)
            sm->prop->tie("contents-lbs", SGRawValuePointer<double>(&(sm->contents)));

        index++;
        models.push_back(sm);
    }
}

void FGSubmodelMgr::loadSubmodels()
{
    SG_LOG(SG_AI, SG_DEBUG, "Submodels: Loading sub submodels");

    _found_sub = false;

    submodel_iterator = submodels.begin();

    while (submodel_iterator != submodels.end()) {
        const string& submodel  = (*submodel_iterator)->submodel;
        if (!submodel.empty()) {
            //int id = (*submodel_iterator)->id;
            bool serviceable = true;
            SG_LOG(SG_AI, SG_DEBUG, "found path sub sub "
                    << submodel
                    << " index " << index
                    << " name " << (*submodel_iterator)->name);

            if ((*submodel_iterator)->sub_id == 0) {
                (*submodel_iterator)->sub_id = index;
                _found_sub = true;
                setData(index, submodel, serviceable, "/ai/submodels/subsubmodel", subsubmodels);
            }
        }

        ++submodel_iterator;
    }

    submodels.reserve(submodels.size() + subsubmodels.size());

    // Add all elements from subsubmodels to submodels
    subsubmodel_iterator = subsubmodels.begin();
    while (subsubmodel_iterator != subsubmodels.end()) {
        submodels.push_back(*subsubmodel_iterator);
        ++subsubmodel_iterator;
    }

    subsubmodels.clear();
}

SGVec3d FGSubmodelMgr::getCartOffsetPos(submodel* sm) const
{
    // Transform to the right coordinate frame, configuration is done in
    // the x-forward, y-right, z-up coordinates (feet), computation
    // in the simulation usual body x-forward, y-right, z-down coordinates
    // (meters) )
    SGVec3d offset;
    if (sm->offsets_in_meter) {
        offset = SGVec3d(-sm->x_offset->get_value(),
                         sm->y_offset->get_value(),
                         -sm->z_offset->get_value());
    } else {
        offset = SGVec3d(sm->x_offset->get_value() * SG_FEET_TO_METER,
                         sm->y_offset->get_value() * SG_FEET_TO_METER,
                         -sm->z_offset->get_value() * SG_FEET_TO_METER);
    }

    // Transform the user position to the horizontal local coordinate system.
    SGQuatd hlTrans = SGQuatd::fromLonLat(userpos);

    // And postrotate the orientation of the user model wrt the horizontal
    // local frame
    hlTrans *= SGQuatd::fromYawPitchRollDeg(
       IC.azimuth,
       IC.elevation,
       IC.roll);

    // postrotate by any pitch/heasing/roll offset
    hlTrans *= SGQuatd::fromYawPitchRollDeg(sm->yaw_offset->get_value(),
                                            sm->pitch_offset->get_value(),
                                            0.0);
    
    // The offset converted to the usual body fixed coordinate system
    // rotated to the earth-fixed coordinates axis
    SGVec3d off = hlTrans.backTransform(offset);

    // Add the position offset of the user model to get the geocentered position
    return SGVec3d::fromGeod(userpos) + off;
}

void FGSubmodelMgr::setOffsetPos(submodel* sm)
{
    // Convert the offset geocentered position to geodetic
    SGVec3d cartoffsetPos = getCartOffsetPos(sm);
    SGGeodesy::SGCartToGeod(cartoffsetPos, offsetpos);
}

void FGSubmodelMgr::valueChanged(SGPropertyNode *prop)
{
    return; // this isn't working atm

    const char* _model_added = _model_added_node->getStringValue();

    std::basic_string <char>::size_type indexCh2b;

    string str2 = _model_added;
    const char *cstr2b = "multiplayer";
    indexCh2b = str2.find(cstr2b, 0);

    // Ignoring Ballistic Objects; there are potentially too many
    if (indexCh2b != string::npos ) {
        //cout << "Submodels: model added - " << str2 <<" read path "<< endl;
        //return;
        SGPropertyNode *a_node = fgGetNode(_model_added, true);
        SGPropertyNode *sub_node = a_node->getChild("sim", 0, true);
        SGPropertyNode_ptr path_node = sub_node->getChild("path", 0, true);
        SGPropertyNode_ptr callsign_node = a_node->getChild("callsign", 0, true);

        //const string& callsign = callsign_node->getStringValue();
        //cout << "Submodels: model added - " << callsign <<" read callsign "<< endl;
    }
    else {
        cout << "model added - " << str2 <<" returning " << endl;
    }
}

void FGSubmodelMgr::setParentNode(int id)
{
    const SGPropertyNode_ptr ai = fgGetNode("/ai/models", true);

    for (int i = ai->nChildren() - 1; i >= -1; i--) {
        SGPropertyNode_ptr model;

        if (i < 0) {
            // Last iteration: selected model
            model = _selected_ac;
        }
        else {
            model = ai->getChild(i);
            int parent_id = model->getIntValue("id");
            if (!model->nChildren()) {
                continue;
            }
            if (parent_id == id) {
                // Save selected model for last iteration
                _selected_ac = model;
                break;
            }
        }
        if (!model)
            continue;
    }

    if (_selected_ac != 0) {
        const string name  = _selected_ac->getStringValue("name");
        IC.lat             = _selected_ac->getDoubleValue("position/latitude-deg");
        IC.lon             = _selected_ac->getDoubleValue("position/longitude-deg");
        IC.alt             = _selected_ac->getDoubleValue("position/altitude-ft");
        IC.roll            = _selected_ac->getDoubleValue("orientation/roll-deg");
        IC.elevation       = _selected_ac->getDoubleValue("orientation/pitch-deg");
        IC.azimuth         = _selected_ac->getDoubleValue("orientation/true-heading-deg");
        IC.speed           = _selected_ac->getDoubleValue("velocities/true-airspeed-kt") * SG_KT_TO_FPS;
        IC.speed_down_fps  = -_selected_ac->getDoubleValue("velocities/vertical-speed-fps");
        IC.speed_east_fps  = _selected_ac->getDoubleValue("velocities/speed-east-fps");
        IC.speed_north_fps = _selected_ac->getDoubleValue("velocities/speed-north-fps");
    }
    else {
        SG_LOG(SG_AI, SG_ALERT, "AISubmodel: parent node not found ");
    }
}


// Register the subsystem.
SGSubsystemMgr::Registrant<FGSubmodelMgr> registrantFGSubmodelMgr(
    SGSubsystemMgr::POST_FDM);
