// submodel.cxx - models a releasable submodel.
// Written by Dave Culp, started Aug 2004
// With major additions by Vivian Meaaza 2004 - 2007
//
// This file is in the Public Domain and comes with no warranty.

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include "submodel.hxx"

#include <simgear/structure/exception.hxx>
#include <simgear/misc/sg_path.hxx>
#include <simgear/math/sg_geodesy.hxx>

#include <Main/fg_props.hxx>
#include <Main/util.hxx>

#include "AIBase.hxx"
#include "AIManager.hxx"
#include "AIBallistic.hxx"


const double FGSubmodelMgr::lbs_to_slugs = 0.031080950172;

FGSubmodelMgr::FGSubmodelMgr()
{
    x_offset = y_offset = 0.0;
    z_offset = -4.0;
    pitch_offset = 2.0;
    yaw_offset = 0.0;

    out[0] = out[1] = out[2] = 0;
    in[3] = out[3] = 1;
    string contents_node;
    contrail_altitude = 30000;
}

FGSubmodelMgr::~FGSubmodelMgr()
{}

void FGSubmodelMgr::init()
{

    index = 0;
    load();

    _serviceable_node = fgGetNode("/sim/submodels/serviceable", true);
    _serviceable_node->setBoolValue(true);

    _user_lat_node = fgGetNode("/position/latitude-deg", true);
    _user_lon_node = fgGetNode("/position/longitude-deg", true);
    _user_alt_node = fgGetNode("/position/altitude-ft", true);

    _user_heading_node = fgGetNode("/orientation/heading-deg", true);
    _user_pitch_node =   fgGetNode("/orientation/pitch-deg", true);
    _user_roll_node =    fgGetNode("/orientation/roll-deg", true);
    _user_yaw_node =     fgGetNode("/orientation/yaw-deg", true);
    _user_alpha_node =   fgGetNode("/orientation/alpha-deg", true);

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

    ai = (FGAIManager*)globals->get_subsystem("ai_model");
}

void FGSubmodelMgr::postinit() {
    // postinit, so that the AI list is populated
    loadAI();
}

void FGSubmodelMgr::bind()
{}

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
    if (!(_serviceable_node->getBoolValue()))
        return;

    int i = -1;
    bool in_range = true;
    bool trigger = false;

    _contrail_trigger->setBoolValue(_user_alt_node->getDoubleValue() > contrail_altitude);

    submodel_iterator = submodels.begin();

    while (submodel_iterator != submodels.end()) {
        i++;

        if ((*submodel_iterator)->trigger_node != 0) {
            trigger = (*submodel_iterator)->trigger_node->getBoolValue();
            //cout << (*submodel_iterator)->name << "trigger node found" << trigger << endl;
        } else {
            trigger = true;
            //cout << (*submodel_iterator)->name << "trigger node not found" << trigger << endl;
        }

        if (trigger) {
            int id = (*submodel_iterator)->id;

            // don't release submodels from AI Objects if they are
            // too far away to be seen. id 0 is not an AI model,
            // so we can skip the whole process
            sm_list_iterator sm_list_itr = sm_list.begin();
            sm_list_iterator end = sm_list.end();

            while (sm_list_itr != end) {

                if (id == 0) {
                    SG_LOG(SG_GENERAL, SG_DEBUG,
                           "Submodels: continuing: " << id);
                    ++sm_list_itr;
                    continue;
                }

                int parent_id = (*sm_list_itr)->getID();

                if (parent_id == id) {
                    double parent_lat = (*sm_list_itr)->_getLatitude();
                    double parent_lon = (*sm_list_itr)->_getLongitude();
                    double own_lat    = _user_lat_node->getDoubleValue();
                    double own_lon    = _user_lon_node->getDoubleValue();
                    double range_nm   = getRange(parent_lat, parent_lon, own_lat, own_lon);
                    /* cout << "parent " << parent_id << ", "<< parent_lat << ", " << parent_lon << endl;
                    cout << "own " << own_lat << ", " << own_lon << " range " << range_nm << endl;*/

                    if (range_nm > 15) {
                        SG_LOG(SG_GENERAL, SG_DEBUG,
                               "Submodels: skipping release: " << id);
                        in_range = false;
                    }

                }

                ++sm_list_itr;
            } // end while

            if ((*submodel_iterator)->count != 0 && in_range)
                release((*submodel_iterator), dt);

        } else
            (*submodel_iterator)->first_time = true;

        ++submodel_iterator;
    } // end while

}

bool FGSubmodelMgr::release(submodel* sm, double dt)
{
    // only run if first time or repeat is set to true
    if (!sm->first_time && !sm->repeat)
        return false;

    sm->timer += dt;

    if (sm->timer < sm->delay)
        return false;

    sm->timer = 0.0;

    if (sm->first_time) {
        dt = 0.0;
        sm->first_time = false;
    }

    transform(sm);  // calculate submodel's initial conditions in world-coordinates

    FGAIBallistic* ballist = new FGAIBallistic;
    ballist->setPath(sm->model.c_str());
    ballist->setLatitude(IC.lat);
    ballist->setLongitude(IC.lon);
    ballist->setAltitude(IC.alt);
    ballist->setAzimuth(IC.azimuth);
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
    ballist->setCd(sm->cd);
    ballist->setStabilisation(sm->aero_stabilised);
    ballist->setNoRoll(sm->no_roll);
    ballist->setName(sm->name);
    ai->attach(ballist);

    if (sm->count > 0)
        (sm->count)--;

    return true;
}

void FGSubmodelMgr::load()
{
    SGPropertyNode *path = fgGetNode("/sim/submodels/path");
    SGPropertyNode root;

    if (path) {
        SGPath config(globals->get_fg_root());
        config.append(path->getStringValue());

        try {
            readProperties(config.str(), &root);
        } catch (const sg_exception &e) {
            SG_LOG(SG_GENERAL, SG_INFO,
                   "Submodels: unable to read submodels file: " << config.str());
            return;
        }
    }

    vector<SGPropertyNode_ptr> children = root.getChildren("submodel");
    vector<SGPropertyNode_ptr>::iterator it = children.begin();
    vector<SGPropertyNode_ptr>::iterator end = children.end();

    for (int i = 0; it != end; ++it, i++) {
        // cout << "Reading submodel " << (*it)->getPath() << endl;
        submodel* sm = new submodel;
        SGPropertyNode * entry_node = *it;
        sm->name           = entry_node->getStringValue("name", "none_defined");
        sm->model          = entry_node->getStringValue("model", "Models/Geometry/rocket.ac");
        sm->speed          = entry_node->getDoubleValue("speed", 2329.4);
        sm->repeat         = entry_node->getBoolValue("repeat", false);
        sm->delay          = entry_node->getDoubleValue("delay", 0.25);
        sm->count          = entry_node->getIntValue("count", 1);
        sm->slaved         = entry_node->getBoolValue("slaved", false);
        sm->x_offset       = entry_node->getDoubleValue("x-offset", 0.0);
        sm->y_offset       = entry_node->getDoubleValue("y-offset", 0.0);
        sm->z_offset       = entry_node->getDoubleValue("z-offset", 0.0);
        sm->yaw_offset     = entry_node->getDoubleValue("yaw-offset", 0.0);
        sm->pitch_offset   = entry_node->getDoubleValue("pitch-offset", 0.0);
        sm->drag_area      = entry_node->getDoubleValue("eda", 0.034);
        sm->life           = entry_node->getDoubleValue("life", 900.0);
        sm->buoyancy       = entry_node->getDoubleValue("buoyancy", 0);
        sm->wind           = entry_node->getBoolValue("wind", false);
        sm->cd             = entry_node->getDoubleValue("cd", 0.193);
        sm->weight         = entry_node->getDoubleValue("weight", 0.25);
        sm->aero_stabilised = entry_node->getBoolValue("aero-stabilised", true);
        sm->no_roll         = entry_node->getBoolValue("no-roll", false);
        sm->contents_node   = fgGetNode(entry_node->getStringValue("contents", "none"), false);
        sm->trigger_node    = fgGetNode(entry_node->getStringValue("trigger", "none"), false);
        sm->speed_node      = fgGetNode(entry_node->getStringValue("speed-node", "none"), false);

        //cout <<  "sm->contents_node " << sm->contents_node << endl;
        if (sm->contents_node != 0)
            sm->contents = sm->contents_node->getDoubleValue();

        //cout << sm->name <<  " sm->trigger_node " << sm->trigger_node << endl;
        if (sm->trigger_node != 0)
            sm->trigger_node->setBoolValue(false);

        if (sm->speed_node != 0)
            sm->speed = sm->speed_node->getDoubleValue();

        sm->timer = sm->delay;
        sm->id = 0;
        sm->first_time = false;

        sm->prop = fgGetNode("/ai/submodels/submodel", index, true);
        sm->prop->tie("count", SGRawValuePointer<int>(&(sm->count)));
        sm->prop->tie("repeat", SGRawValuePointer<bool>(&(sm->repeat)));
        sm->prop->tie("id", SGRawValuePointer<int>(&(sm->id)));
        string name = sm->name;
        sm->prop->setStringValue("name", name.c_str());

        if (sm->contents_node != 0) {
            sm->prop->tie("contents-lbs", SGRawValuePointer<double>(&(sm->contents)));
        }

        index++;
        submodels.push_back(sm);
    }

    submodel_iterator = submodels.begin();
}

void FGSubmodelMgr::transform(submodel* sm)
{
    // get initial conditions
    if (sm->contents_node != 0) {
        // get the weight of the contents (lbs) and convert to mass (slugs)
        sm->contents = sm->contents_node->getDoubleValue();
        IC.mass = (sm->weight + sm->contents) * lbs_to_slugs;

        // set contents to 0 in the parent
        sm->contents_node->setDoubleValue(0);
    } else
        IC.mass = sm->weight * lbs_to_slugs;

    //cout << "mass "  << IC.mass << endl;

    if (sm->speed_node != 0)
        sm->speed = sm->speed_node->getDoubleValue();

    int ind = sm->id;

    if (ind == 0) {
        // set the data for a submodel tied to the main model
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

    } else {
        // set the data for a submodel tied to an AI Object
        sm_list_iterator sm_list_itr = sm_list.begin();
        sm_list_iterator end = sm_list.end();

        while (sm_list_itr != end) {
            int id = (*sm_list_itr)->getID();

            if (ind != id) {
                ++sm_list_itr;
                continue;
            }

            //cout << "found id " << id << endl;
            IC.lat             = (*sm_list_itr)->_getLatitude();
            IC.lon             = (*sm_list_itr)->_getLongitude();
            IC.alt             = (*sm_list_itr)->_getAltitude();
            IC.roll            = (*sm_list_itr)->_getRoll();
            IC.elevation       = (*sm_list_itr)->_getPitch();
            IC.azimuth         = (*sm_list_itr)->_getHeading();
            IC.alt             = (*sm_list_itr)->_getAltitude();
            IC.speed           = (*sm_list_itr)->_getSpeed() * SG_KT_TO_FPS;
            IC.speed_down_fps  = -(*sm_list_itr)->_getVS_fps();
            IC.speed_east_fps  = (*sm_list_itr)->_get_speed_east_fps();
            IC.speed_north_fps = (*sm_list_itr)->_get_speed_north_fps();

            ++sm_list_itr;
        }

    }

    /*cout << "heading " << IC.azimuth << endl ;
    cout << "speed down " << IC.speed_down_fps << endl ;
    cout << "speed east " << IC.speed_east_fps << endl ;
    cout << "speed north " << IC.speed_north_fps << endl ;
    cout << "parent speed fps in" << IC.speed << "sm speed in " << sm->speed << endl ;*/

    IC.wind_from_east =  _user_wind_from_east_node->getDoubleValue();
    IC.wind_from_north = _user_wind_from_north_node->getDoubleValue();

    in[0] = sm->x_offset;
    in[1] = sm->y_offset;
    in[2] = sm->z_offset;

    // pre-process the trig functions
    cosRx = cos(-IC.roll * SG_DEGREES_TO_RADIANS);
    sinRx = sin(-IC.roll * SG_DEGREES_TO_RADIANS);
    cosRy = cos(-IC.elevation * SG_DEGREES_TO_RADIANS);
    sinRy = sin(-IC.elevation * SG_DEGREES_TO_RADIANS);
    cosRz = cos(IC.azimuth * SG_DEGREES_TO_RADIANS);
    sinRz = sin(IC.azimuth * SG_DEGREES_TO_RADIANS);

    // set up the transform matrix
    trans[0][0] =  cosRy * cosRz;
    trans[0][1] =  -1 * cosRx * sinRz + sinRx * sinRy * cosRz ;
    trans[0][2] =  sinRx * sinRz + cosRx * sinRy * cosRz;

    trans[1][0] =  cosRy * sinRz;
    trans[1][1] =  cosRx * cosRz + sinRx * sinRy * sinRz;
    trans[1][2] =  -1 * sinRx * cosRx + cosRx * sinRy * sinRz;

    trans[2][0] =  -1 * sinRy;
    trans[2][1] =  sinRx * cosRy;
    trans[2][2] =  cosRx * cosRy;


    // multiply the input and transform matrices
    out[0] = in[0] * trans[0][0] + in[1] * trans[0][1] + in[2] * trans[0][2];
    out[1] = in[0] * trans[1][0] + in[1] * trans[1][1] + in[2] * trans[1][2];
    out[2] = in[0] * trans[2][0] + in[1] * trans[2][1] + in[2] * trans[2][2];

    // convert ft to degrees of latitude
    out[0] = out[0] / (366468.96 - 3717.12 * cos(IC.lat * SG_DEGREES_TO_RADIANS));

    // convert ft to degrees of longitude
    out[1] = out[1] / (365228.16 * cos(IC.lat * SG_DEGREES_TO_RADIANS));

    // set submodel initial position
    IC.lat += out[0];
    IC.lon += out[1];
    IC.alt += out[2];

    // get aircraft velocity vector angles in XZ and XY planes
    //double alpha = _user_alpha_node->getDoubleValue();
    //double velXZ = IC.elevation - alpha * cosRx;
    //double velXY = IC.azimuth - (IC.elevation - alpha * sinRx);

    // Get submodel initial velocity vector angles in XZ and XY planes.
    // This needs to be fixed. This vector should be added to aircraft's vector.
    IC.elevation += (sm->yaw_offset * sinRx) + (sm->pitch_offset * cosRx);
    IC.azimuth   += (sm->yaw_offset * cosRx) - (sm->pitch_offset * sinRx);

    // calcuate the total speed north
    IC.total_speed_north = sm->speed * cos(IC.elevation * SG_DEGREES_TO_RADIANS)
            * cos(IC.azimuth * SG_DEGREES_TO_RADIANS) + IC.speed_north_fps;

    // calculate the total speed east
    IC.total_speed_east = sm->speed * cos(IC.elevation * SG_DEGREES_TO_RADIANS)
            * sin(IC.azimuth * SG_DEGREES_TO_RADIANS) + IC.speed_east_fps;

    // calculate the total speed down
    IC.total_speed_down = sm->speed * -sin(IC.elevation * SG_DEGREES_TO_RADIANS)
            + IC.speed_down_fps;

    // re-calculate speed, elevation and azimuth
    IC.speed = sqrt(IC.total_speed_north * IC.total_speed_north
            + IC.total_speed_east * IC.total_speed_east
            + IC.total_speed_down * IC.total_speed_down);

    //cout << " speed fps out" << IC.speed << endl ;
    IC.azimuth = atan(IC.total_speed_east / IC.total_speed_north) * SG_RADIANS_TO_DEGREES;

    // rationalise the output
    if (IC.total_speed_north <= 0) {
        IC.azimuth = 180 + IC.azimuth;
    } else {

        if (IC.total_speed_east <= 0)
            IC.azimuth = 360 + IC.azimuth;

    }

    IC.elevation = -atan(IC.total_speed_down / sqrt(IC.total_speed_north
            * IC.total_speed_north + IC.total_speed_east * IC.total_speed_east))
            * SG_RADIANS_TO_DEGREES;

}

void FGSubmodelMgr::updatelat(double lat)
{
    ft_per_deg_latitude = 366468.96 - 3717.12 * cos(lat / SG_RADIANS_TO_DEGREES);
    ft_per_deg_longitude = 365228.16 * cos(lat / SG_RADIANS_TO_DEGREES);
}

void FGSubmodelMgr::loadAI()
{
    SG_LOG(SG_GENERAL, SG_DEBUG, "Submodels: Loading AI submodels ");
    SGPropertyNode root;
    sm_list = ai->get_ai_list();

    if (sm_list.empty()) {
        SG_LOG(SG_GENERAL, SG_DEBUG, "Submodels: Unable to read AI submodel list");
        return;
    }

    sm_list_iterator sm_list_itr = sm_list.begin();
    sm_list_iterator end = sm_list.end();

    while (sm_list_itr != end) {
        string path = (*sm_list_itr)->_getPath();
        bool serviceable = (*sm_list_itr)->_getServiceable();

        if (path.empty()) {
            ++sm_list_itr;
            continue;
        }

        //cout << " path " << path << " serviceable " << serviceable << endl;

        SGPath config(globals->get_fg_root());
        config.append(path);
        int id = (*sm_list_itr)->getID();

        //cout << "id: " << id << endl;

        try {
            SG_LOG(SG_GENERAL, SG_DEBUG,
                   "Submodels: Trying to read AI submodels file: " << config.str());
            readProperties(config.str(), &root);
        } catch (const sg_exception &e) {
            SG_LOG(SG_GENERAL, SG_DEBUG,
                   "Submodels: Unable to read AI submodels file: " << config.str());
            return;
        }

        vector<SGPropertyNode_ptr> children = root.getChildren("submodel");
        vector<SGPropertyNode_ptr>::iterator it = children.begin();
        vector<SGPropertyNode_ptr>::iterator end = children.end();

        for (int i = 0; it != end; ++it, i++) {
            //cout << "Reading AI submodel " << (*it)->getPath() << endl;
            submodel* sm = new submodel;
            SGPropertyNode * entry_node = *it;
            sm->name            = entry_node->getStringValue("name", "none_defined");
            sm->model           = entry_node->getStringValue("model", "Models/Geometry/rocket.ac");
            sm->speed           = entry_node->getDoubleValue("speed", 2329.4);
            sm->repeat          = entry_node->getBoolValue("repeat", false);
            sm->delay           = entry_node->getDoubleValue("delay", 0.25);
            sm->count           = entry_node->getIntValue("count", 1);
            sm->slaved          = entry_node->getBoolValue("slaved", false);
            sm->x_offset        = entry_node->getDoubleValue("x-offset", 0.0);
            sm->y_offset        = entry_node->getDoubleValue("y-offset", 0.0);
            sm->z_offset        = entry_node->getDoubleValue("z-offset", 0.0);
            sm->yaw_offset      = entry_node->getDoubleValue("yaw-offset", 0.0);
            sm->pitch_offset    = entry_node->getDoubleValue("pitch-offset", 0.0);
            sm->drag_area       = entry_node->getDoubleValue("eda", 0.034);
            sm->life            = entry_node->getDoubleValue("life", 900.0);
            sm->buoyancy        = entry_node->getDoubleValue("buoyancy", 0);
            sm->wind            = entry_node->getBoolValue("wind", false);
            sm->cd              = entry_node->getDoubleValue("cd", 0.193);
            sm->weight          = entry_node->getDoubleValue("weight", 0.25);
            sm->aero_stabilised = entry_node->getBoolValue("aero-stabilised", true);
            sm->no_roll         = entry_node->getBoolValue("no-roll", false);
            sm->contents_node   = fgGetNode(entry_node->getStringValue("contents", "none"), false);
            sm->trigger_node    = fgGetNode(entry_node->getStringValue("trigger", "none"), false);
            sm->speed_node      = fgGetNode(entry_node->getStringValue("speed-node", "none"), false);

            //cout <<  "sm->contents_node " << sm->contents_node << endl;
            if (sm->contents_node != 0)
                sm->contents = sm->contents_node->getDoubleValue();
            //cout <<  "sm->trigger_node " << sm->trigger_node << endl;
            if (sm->trigger_node != 0)
                sm->trigger_node->setBoolValue(false);

            if (sm->speed_node != 0)
                sm->speed = sm->speed_node->getDoubleValue();

            sm->timer = sm->delay;
            sm->id = id;
            sm->first_time = false;

            sm->serviceable = (*sm_list_itr)->_getServiceable();

            sm->prop = fgGetNode("/ai/submodels/submodel", index, true);
            sm->prop->tie("count", SGRawValuePointer<int>(&(sm->count)));
            sm->prop->tie("repeat", SGRawValuePointer<bool>(&(sm->repeat)));
            sm->prop->tie("id", SGRawValuePointer<int>(&(sm->id)));
            sm->prop->tie("serviceable", SGRawValuePointer<bool>(&(sm->serviceable)));
            string name = sm->name;
            sm->prop->setStringValue("name", name.c_str());

            if (sm->contents_node != 0)
                sm->prop->tie("contents-lbs", SGRawValuePointer<double>(&(sm->contents)));

            index++;
            submodels.push_back(sm);
        }

        submodel_iterator = submodels.begin();
        ++sm_list_itr;
    }

}

double FGSubmodelMgr::getRange(double lat, double lon, double lat2, double lon2) const
{

    double course, distance, az2;

    //calculate the bearing and range of the second pos from the first
    geo_inverse_wgs_84(lat, lon, lat2, lon2, &course, &az2, &distance);
    distance *= SG_METER_TO_NM;
    return distance;
}
// end of submodel.cxx
