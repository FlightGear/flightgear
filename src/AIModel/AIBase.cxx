// FGAIBase - abstract base class for AI objects
// Written by David Culp, started Nov 2003, based on
// David Luff's FGAIEntity class.
// - davidculp2@comcast.net
//
// With additions by Mathias Froehlich & Vivian Meazza 2004 -2007
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


#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <simgear/compiler.h>

#include <string>

#include <osg/ref_ptr>
#include <osg/Node>
#include <osgDB/FileUtils>

#include <simgear/math/SGMath.hxx>
#include <simgear/misc/sg_path.hxx>
#include <simgear/scene/model/modellib.hxx>
#include <simgear/scene/util/SGNodeMasks.hxx>
#include <simgear/debug/logstream.hxx>
#include <simgear/props/props.hxx>

#include <Main/globals.hxx>
#include <Scenery/scenery.hxx>
#include <Scripting/NasalSys.hxx>

#include "AIBase.hxx"
#include "AIManager.hxx"

const char *default_model = "Models/Geometry/glider.ac";
const double FGAIBase::e = 2.71828183;
const double FGAIBase::lbs_to_slugs = 0.031080950172;   //conversion factor

using namespace simgear;

FGAIBase::FGAIBase(object_type ot) :
    props( NULL ),
    model_removed( fgGetNode("/ai/models/model-removed", true) ),
    manager( NULL ),
    fp( NULL ),

    _impact_lat(0),
    _impact_lon(0),
    _impact_elev(0),
    _impact_hdg(0),
    _impact_pitch(0),
    _impact_roll(0),
    _impact_speed(0),

    _refID( _newAIModelID() ),
    _otype(ot),
    _initialized(false),
    _parent(""),
    _name("")

{
    tgt_heading = hdg = tgt_altitude_ft = tgt_speed = 0.0;
    tgt_roll = roll = tgt_pitch = tgt_yaw = tgt_vs = vs = pitch = 0.0;
    bearing = elevation = range = rdot = 0.0;
    x_shift = y_shift = rotation = 0.0;
    in_range = false;
    invisible = false;
    no_roll = true;
    life = 900;
    delete_me = false;
    _impact_reported = false;
    _collision_reported = false;
    _expiry_reported = false;

    _subID = 0;

    _x_offset = 0;
    _y_offset = 0;
    _z_offset = 0;

    _pitch_offset = 0;
    _roll_offset = 0;
    _yaw_offset = 0;

    userpos = SGGeod::fromDeg(0, 0);

    pos = SGGeod::fromDeg(0, 0);
    speed = 0;
    altitude_ft = 0;
    speed_north_deg_sec = 0;
    speed_east_deg_sec = 0;
    turn_radius_ft = 0;

    ft_per_deg_lon = 0;
    ft_per_deg_lat = 0;

    horiz_offset = 0;
    vert_offset = 0;
    ht_diff = 0;

    serviceable = false;

    fp = 0;

    rho = 1;
    T = 280;
    p = 1e5;
    a = 340;
    Mach = 0;
}

FGAIBase::~FGAIBase() {
    // Unregister that one at the scenery manager
    if (globals->get_scenery()) {
        globals->get_scenery()->get_scene_graph()->removeChild(aip.getSceneGraph());
    }

    if (props) {
        SGPropertyNode* parent = props->getParent();

        if (parent)
            model_removed->setStringValue(props->getPath());
    }
    delete fp;
    fp = 0;
}

void FGAIBase::readFromScenario(SGPropertyNode* scFileNode)
{
    if (!scFileNode)
        return;

    setPath(scFileNode->getStringValue("model",
            fgGetString("/sim/multiplay/default-model", default_model)));

    setHeading(scFileNode->getDoubleValue("heading", 0.0));
    setSpeed(scFileNode->getDoubleValue("speed", 0.0));
    setAltitude(scFileNode->getDoubleValue("altitude", 0.0));
    setLongitude(scFileNode->getDoubleValue("longitude", 0.0));
    setLatitude(scFileNode->getDoubleValue("latitude", 0.0));
    setBank(scFileNode->getDoubleValue("roll", 0.0));

    SGPropertyNode* submodels = scFileNode->getChild("submodels");

    if (submodels) {
        setServiceable(submodels->getBoolValue("serviceable", false));
        setSMPath(submodels->getStringValue("path", ""));
    }

}

void FGAIBase::update(double dt) {

    if (_otype == otStatic)
        return;

    if (_otype == otBallistic)
        CalculateMach();

    ft_per_deg_lat = 366468.96 - 3717.12 * cos(pos.getLatitudeRad());
    ft_per_deg_lon = 365228.16 * cos(pos.getLatitudeRad());
}

void FGAIBase::Transform() {

    if (!invisible) {
        aip.setVisible(true);
        aip.setPosition(pos);

        if (no_roll)
            aip.setOrientation(0.0, pitch, hdg);
        else
            aip.setOrientation(roll, pitch, hdg);

        aip.update();
    } else {
        aip.setVisible(false);
        aip.update();
    }

}

bool FGAIBase::init(bool search_in_AI_path) {
    osg::ref_ptr<osgDB::ReaderWriter::Options> opt=
        new osgDB::ReaderWriter::Options(*osgDB::Registry::instance()->getOptions());

    if(search_in_AI_path)
    {
        SGPath ai_path(globals->get_fg_root());
        ai_path.append("AI");
        opt->getDatabasePathList().push_front(ai_path.str());
    }

    string f = osgDB::findDataFile(model_path, opt.get());

    if(f.empty())
        f = fgGetString("/sim/multiplay/default-model", default_model);

    model = load3DModel(f, props);

    if (model.valid() && _initialized == false) {
        aip.init( model.get() );
        aip.setVisible(true);
        invisible = false;
        globals->get_scenery()->get_scene_graph()->addChild(aip.getSceneGraph());
        _initialized = true;

    } else if (!model_path.empty()) {
        SG_LOG(SG_INPUT, SG_WARN, "AIBase: Could not load model " << model_path);
    }

    setDie(false);
    return true;
}

void FGAIBase::initModel(osg::Node *node)
{
    if (model.valid()) { 

        if( _path != ""){
            props->setStringValue("submodels/path", _path.c_str());
            SG_LOG(SG_INPUT, SG_DEBUG, "AIBase: submodels/path " << _path);
        }

        if( _parent!= ""){
            props->setStringValue("parent-name", _parent.c_str());
        }

        fgSetString("/ai/models/model-added", props->getPath().c_str());
    } else if (!model_path.empty()) {
        SG_LOG(SG_INPUT, SG_WARN, "AIBase: Could not load model " << model_path);
    }

    setDie(false);
}


osg::Node* FGAIBase::load3DModel(const string &path, SGPropertyNode *prop_root)
{
  model = SGModelLib::loadPagedModel(path, prop_root, new FGNasalModelData(prop_root));
  initModel(model.get());
  return model.get();
}

bool FGAIBase::isa( object_type otype ) {
    return otype == _otype;
}


void FGAIBase::bind() {
    props->tie("id", SGRawValueMethods<FGAIBase,int>(*this,
        &FGAIBase::getID));
    props->tie("velocities/true-airspeed-kt",  SGRawValuePointer<double>(&speed));
    props->tie("velocities/vertical-speed-fps",
        SGRawValueMethods<FGAIBase,double>(*this,
        &FGAIBase::_getVS_fps,
        &FGAIBase::_setVS_fps));

    props->tie("position/altitude-ft",
        SGRawValueMethods<FGAIBase,double>(*this,
        &FGAIBase::_getAltitude,
        &FGAIBase::_setAltitude));
    props->tie("position/latitude-deg",
        SGRawValueMethods<FGAIBase,double>(*this,
        &FGAIBase::_getLatitude,
        &FGAIBase::_setLatitude));
    props->tie("position/longitude-deg",
        SGRawValueMethods<FGAIBase,double>(*this,
        &FGAIBase::_getLongitude,
        &FGAIBase::_setLongitude));

    props->tie("position/global-x",
        SGRawValueMethods<FGAIBase,double>(*this,
        &FGAIBase::_getCartPosX,
        0));
    props->tie("position/global-y",
        SGRawValueMethods<FGAIBase,double>(*this,
        &FGAIBase::_getCartPosY,
        0));
    props->tie("position/global-z",
        SGRawValueMethods<FGAIBase,double>(*this,
        &FGAIBase::_getCartPosZ,
        0));
    props->tie("callsign",
        SGRawValueMethods<FGAIBase,const char*>(*this,
        &FGAIBase::_getCallsign,
        0));

    props->tie("orientation/pitch-deg",   SGRawValuePointer<double>(&pitch));
    props->tie("orientation/roll-deg",    SGRawValuePointer<double>(&roll));
    props->tie("orientation/true-heading-deg", SGRawValuePointer<double>(&hdg));

    props->tie("radar/in-range", SGRawValuePointer<bool>(&in_range));
    props->tie("radar/bearing-deg",   SGRawValuePointer<double>(&bearing));
    props->tie("radar/elevation-deg", SGRawValuePointer<double>(&elevation));
    props->tie("radar/range-nm", SGRawValuePointer<double>(&range));
    props->tie("radar/h-offset", SGRawValuePointer<double>(&horiz_offset));
    props->tie("radar/v-offset", SGRawValuePointer<double>(&vert_offset));
    props->tie("radar/x-shift", SGRawValuePointer<double>(&x_shift));
    props->tie("radar/y-shift", SGRawValuePointer<double>(&y_shift));
    props->tie("radar/rotation", SGRawValuePointer<double>(&rotation));
    props->tie("radar/ht-diff-ft", SGRawValuePointer<double>(&ht_diff));
    props->tie("subID", SGRawValuePointer<int>(&_subID));
    props->tie("controls/lighting/nav-lights",
        SGRawValueFunctions<bool>(_isNight));
    props->setBoolValue("controls/lighting/beacon", true);
    props->setBoolValue("controls/lighting/strobe", true);
    props->setBoolValue("controls/glide-path", true);

    props->setStringValue("controls/flight/lateral-mode", "roll");
    props->setDoubleValue("controls/flight/target-hdg", hdg);
    props->setDoubleValue("controls/flight/target-roll", roll);

    props->setStringValue("controls/flight/longitude-mode", "alt");
    props->setDoubleValue("controls/flight/target-alt", altitude_ft);
    props->setDoubleValue("controls/flight/target-pitch", pitch);

   props->setDoubleValue("controls/flight/target-spd", speed);

}

void FGAIBase::unbind() {
    props->untie("id");
    props->untie("velocities/true-airspeed-kt");
    props->untie("velocities/vertical-speed-fps");

    props->untie("position/altitude-ft");
    props->untie("position/latitude-deg");
    props->untie("position/longitude-deg");
    props->untie("position/global-x");
    props->untie("position/global-y");
    props->untie("position/global-z");
    props->untie("callsign");

    props->untie("orientation/pitch-deg");
    props->untie("orientation/roll-deg");
    props->untie("orientation/true-heading-deg");

    props->untie("radar/in-range");
    props->untie("radar/bearing-deg");
    props->untie("radar/elevation-deg");
    props->untie("radar/range-nm");
    props->untie("radar/h-offset");
    props->untie("radar/v-offset");
    props->untie("radar/x-shift");
    props->untie("radar/y-shift");
    props->untie("radar/rotation");
    props->untie("radar/ht-diff-ft");

    props->untie("controls/lighting/nav-lights");

    props->setBoolValue("/sim/controls/radar/", true);

}

double FGAIBase::UpdateRadar(FGAIManager* manager) {
    bool control = fgGetBool("/sim/controls/radar", true);

    if(!control) return 0;

    double radar_range_ft2 = fgGetDouble("/instrumentation/radar/range");
    bool force_on = fgGetBool("/instrumentation/radar/debug-mode", false);
    radar_range_ft2 *= SG_NM_TO_METER * SG_METER_TO_FEET * 1.1; // + 10%
    radar_range_ft2 *= radar_range_ft2;

    double user_latitude  = manager->get_user_latitude();
    double user_longitude = manager->get_user_longitude();
    double lat_range = fabs(pos.getLatitudeDeg() - user_latitude) * ft_per_deg_lat;
    double lon_range = fabs(pos.getLongitudeDeg() - user_longitude) * ft_per_deg_lon;
    double range_ft2 = lat_range*lat_range + lon_range*lon_range;

    //
    // Test whether the target is within radar range.
    //
    in_range = (range_ft2 && (range_ft2 <= radar_range_ft2));

    if ( in_range || force_on ) {
        props->setBoolValue("radar/in-range", true);

        // copy values from the AIManager
        double user_altitude  = manager->get_user_altitude();
        double user_heading   = manager->get_user_heading();
        double user_pitch     = manager->get_user_pitch();
        //double user_yaw       = manager->get_user_yaw();
        //double user_speed     = manager->get_user_speed();

        // calculate range to target in feet and nautical miles
        double range_ft = sqrt( range_ft2 );
        range = range_ft / 6076.11549;

        // calculate bearing to target
        if (pos.getLatitudeDeg() >= user_latitude) {
            bearing = atan2(lat_range, lon_range) * SG_RADIANS_TO_DEGREES;
            if (pos.getLongitudeDeg() >= user_longitude) {
                bearing = 90.0 - bearing;
            } else {
                bearing = 270.0 + bearing;
            }
        } else {
            bearing = atan2(lon_range, lat_range) * SG_RADIANS_TO_DEGREES;
            if (pos.getLongitudeDeg() >= user_longitude) {
                bearing = 180.0 - bearing;
            } else {
                bearing = 180.0 + bearing;
            }
        }

        // This is an alternate way to compute bearing and distance which
        // agrees with the original scheme within about 0.1 degrees.
        //
        // Point3D start( user_longitude * SGD_DEGREES_TO_RADIANS,
        //                user_latitude * SGD_DEGREES_TO_RADIANS, 0 );
        // Point3D dest( pos.getLongitudeRad(), pos.getLatitudeRad(), 0 );
        // double gc_bearing, gc_range;
        // calc_gc_course_dist( start, dest, &gc_bearing, &gc_range );
        // gc_range *= SG_METER_TO_NM;
        // gc_bearing *= SGD_RADIANS_TO_DEGREES;
        // printf("orig b = %.3f %.2f  gc b= %.3f, %.2f\n",
        //        bearing, range, gc_bearing, gc_range);

        // calculate look left/right to target, without yaw correction
        horiz_offset = bearing - user_heading;
        if (horiz_offset > 180.0) horiz_offset -= 360.0;
        if (horiz_offset < -180.0) horiz_offset += 360.0;

        // calculate elevation to target
        elevation = atan2( altitude_ft - user_altitude, range_ft ) * SG_RADIANS_TO_DEGREES;

        // calculate look up/down to target
        vert_offset = elevation - user_pitch;

        /* this calculation needs to be fixed, but it isn't important anyway
        // calculate range rate
        double recip_bearing = bearing + 180.0;
        if (recip_bearing > 360.0) recip_bearing -= 360.0;
        double my_horiz_offset = recip_bearing - hdg;
        if (my_horiz_offset > 180.0) my_horiz_offset -= 360.0;
        if (my_horiz_offset < -180.0) my_horiz_offset += 360.0;
        rdot = (-user_speed * cos( horiz_offset * SG_DEGREES_TO_RADIANS ))
        +(-speed * 1.686 * cos( my_horiz_offset * SG_DEGREES_TO_RADIANS ));
        */

        // now correct look left/right for yaw
        // horiz_offset += user_yaw; // FIXME: WHY WOULD WE WANT TO ADD IN SIDE-SLIP HERE?

        // calculate values for radar display
        y_shift = range * cos( horiz_offset * SG_DEGREES_TO_RADIANS);
        x_shift = range * sin( horiz_offset * SG_DEGREES_TO_RADIANS);
        rotation = hdg - user_heading;
        if (rotation < 0.0) rotation += 360.0;
        ht_diff = altitude_ft - user_altitude;

    }

    return range_ft2;
}

/*
* Getters and Setters
*/

SGVec3d FGAIBase::getCartPosAt(const SGVec3d& _off) const {
    // Transform that one to the horizontal local coordinate system.
    SGQuatd hlTrans = SGQuatd::fromLonLat(pos);

    // and postrotate the orientation of the AIModel wrt the horizontal
    // local frame
    hlTrans *= SGQuatd::fromYawPitchRollDeg(hdg, pitch, roll);

    // The offset converted to the usual body fixed coordinate system
    // rotated to the earth fixed coordinates axis
    SGVec3d off = hlTrans.backTransform(_off);

    // Add the position offset of the AIModel to gain the earth centered position
    SGVec3d cartPos = SGVec3d::fromGeod(pos);

    return cartPos + off;
}

SGVec3d FGAIBase::getCartPos() const {
    SGVec3d cartPos = SGVec3d::fromGeod(pos);
    return cartPos;
}

bool FGAIBase::getGroundElevationM(const SGGeod& pos, double& elev,
                                   const SGMaterial** material) const {
    return globals->get_scenery()->get_elevation_m(pos, elev, material,
                                                   model.get());
}

double FGAIBase::_getCartPosX() const {
    SGVec3d cartPos = getCartPos();
    return cartPos.x();
}

double FGAIBase::_getCartPosY() const {
    SGVec3d cartPos = getCartPos();
    return cartPos.y();
}

double FGAIBase::_getCartPosZ() const {
    SGVec3d cartPos = getCartPos();
    return cartPos.z();
}

void FGAIBase::_setLongitude( double longitude ) {
    pos.setLongitudeDeg(longitude);
}

void FGAIBase::_setLatitude ( double latitude )  {
    pos.setLatitudeDeg(latitude);
}

void FGAIBase::_setUserPos(){
    userpos.setLatitudeDeg(manager->get_user_latitude());
    userpos.setLongitudeDeg(manager->get_user_longitude());
    userpos.setElevationM(manager->get_user_altitude() * SG_FEET_TO_METER);
}

void FGAIBase::_setSubID( int s ) {
    _subID = s;
}

bool FGAIBase::setParentNode() {

    if (_parent == ""){
       SG_LOG(SG_GENERAL, SG_ALERT, "AIBase: " << _name
            << " parent not set ");
       return false;
    }

    const SGPropertyNode_ptr ai = fgGetNode("/ai/models", true);

    for (int i = ai->nChildren() - 1; i >= -1; i--) {
        SGPropertyNode_ptr model;

        if (i < 0) { // last iteration: selected model
            model = _selected_ac;
        } else {
            model = ai->getChild(i);
            string path = ai->getPath();
            const string name = model->getStringValue("name");

            if (!model->nChildren()){
                continue;
            }
            if (name == _parent) {
                _selected_ac = model;  // save selected model for last iteration
                break;
            }

        }
        if (!model)
            continue;

    }// end for loop

    if (_selected_ac != 0){
        const string name = _selected_ac->getStringValue("name");
        return true;
    } else {
        SG_LOG(SG_GENERAL, SG_ALERT, "AIBase: " << _name
            << " parent not found: dying ");
        setDie(true);
        return false;
    }

}

double FGAIBase::_getLongitude() const {
    return pos.getLongitudeDeg();
}

double FGAIBase::_getLatitude() const {
    return pos.getLatitudeDeg();
}

double FGAIBase::_getElevationFt() const {
    return pos.getElevationFt();
}

double FGAIBase::_getRdot() const {
    return rdot;
}

double FGAIBase::_getVS_fps() const {
    return vs*60.0;
}

double FGAIBase::_get_speed_east_fps() const {
    return speed_east_deg_sec * ft_per_deg_lon;
}

double FGAIBase::_get_speed_north_fps() const {
    return speed_north_deg_sec * ft_per_deg_lat;
}

void FGAIBase::_setVS_fps( double _vs ) {
    vs = _vs/60.0;
}

double FGAIBase::_getAltitude() const {
    return altitude_ft;
}

double FGAIBase::_getAltitudeAGL(SGGeod inpos, double start){
    getGroundElevationM(SGGeod::fromGeodM(inpos, start),
        _elevation_m, &_material);
    return inpos.getElevationFt() - _elevation_m * SG_METER_TO_FEET;
}

bool FGAIBase::_getServiceable() const {
    return serviceable;
}

SGPropertyNode* FGAIBase::_getProps() const {
    return props;
}

void FGAIBase::_setAltitude( double _alt ) {
    setAltitude( _alt );
}

bool FGAIBase::_isNight() {
    return (fgGetFloat("/sim/time/sun-angle-rad") > 1.57);
}

bool FGAIBase::_getCollisionData() {
    return _collision_reported;
}

bool FGAIBase::_getExpiryData() {
    return _expiry_reported;
}

bool FGAIBase::_getImpactData() {
    return _impact_reported;
}

double FGAIBase::_getImpactLat() const {
    return _impact_lat;
}

double FGAIBase::_getImpactLon() const {
    return _impact_lon;
}

double FGAIBase::_getImpactElevFt() const {
    return _impact_elev * SG_METER_TO_FEET;
}

double FGAIBase::_getImpactPitch() const {
    return _impact_pitch;
}

double FGAIBase::_getImpactRoll() const {
    return _impact_roll;
}

double FGAIBase::_getImpactHdg() const {
    return _impact_hdg;
}

double FGAIBase::_getImpactSpeed() const {
    return _impact_speed;
}

int FGAIBase::getID() const {
    return  _refID;
}

int FGAIBase::_getSubID() const {
    return  _subID;
}

double FGAIBase::_getSpeed() const {
    return speed;
}

double FGAIBase::_getRoll() const {
    return roll;
}

double FGAIBase::_getPitch() const {
    return pitch;
}

double FGAIBase::_getHeading() const {
    return hdg;
}

double  FGAIBase::_getXOffset() const {
    return _x_offset;
}

double  FGAIBase::_getYOffset() const {
    return _y_offset;
}

double  FGAIBase::_getZOffset() const {
    return _z_offset;
}

const char* FGAIBase::_getPath() const {
    return model_path.c_str();
}

const char* FGAIBase::_getSMPath() const {
    return _path.c_str();
}

const char* FGAIBase::_getName() const {
    return _name.c_str();
}

const char* FGAIBase::_getCallsign() const {
    return _callsign.c_str();
}

const char* FGAIBase::_getSubmodel() const {
    return _submodel.c_str();
}

void FGAIBase::CalculateMach() {
    // Calculate rho at altitude, using standard atmosphere
    // For the temperature T and the pressure p,
    double altitude = altitude_ft;

    if (altitude < 36152) {		// curve fits for the troposphere
        T = 59 - 0.00356 * altitude;
        p = 2116 * pow( ((T + 459.7) / 518.6) , 5.256);
    } else if ( 36152 < altitude && altitude < 82345 ) {    // lower stratosphere
        T = -70;
        p = 473.1 * pow( e , 1.73 - (0.000048 * altitude) );
    } else {                                    //  upper stratosphere
        T = -205.05 + (0.00164 * altitude);
        p = 51.97 * pow( ((T + 459.7) / 389.98) , -11.388);
    }

    rho = p / (1718 * (T + 459.7));

    // calculate the speed of sound at altitude
    // a = sqrt ( g * R * (T + 459.7))
    // where:
    // a = speed of sound [ft/s]
    // g = specific heat ratio, which is usually equal to 1.4
    // R = specific gas constant, which equals 1716 ft-lb/slug/R
    a = sqrt ( 1.4 * 1716 * (T + 459.7));

    // calculate Mach number
    Mach = speed/a;

    // cout  << "Speed(ft/s) "<< speed <<" Altitude(ft) "<< altitude << " Mach " << Mach << endl;
}

int FGAIBase::_newAIModelID() {
    static int id = 0;

    if (!++id)
        id++;	// id = 0 is not allowed.

    return id;
}

