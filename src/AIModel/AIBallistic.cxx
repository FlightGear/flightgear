// FGAIBallistic - FGAIBase-derived class creates a ballistic object
//
// Written by David Culp, started November 2003.
// - davidculp2@comcast.net
//
// With major additions by Mathias Froehlich & Vivian Meazza 2004-2008
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

#include <simgear/math/sg_random.h>
#include <simgear/math/sg_geodesy.hxx>
#include <simgear/scene/model/modellib.hxx>

#include <Scenery/scenery.hxx>

#include "AIBallistic.hxx"

#include <Main/util.hxx>

using namespace simgear;

const double FGAIBallistic::slugs_to_kgs = 14.5939029372;
const double FGAIBallistic::slugs_to_lbs = 32.1740485564;

FGAIBallistic::FGAIBallistic(object_type ot) :
    FGAIBase(ot),
    _height(0.0),
    _ht_agl_ft(0.0),
    _azimuth(0.0),
    _elevation(0.0),
    _rotation(0.0),
    _formate_to_ac(false),
    _aero_stabilised(false),
    _drag_area(0.007),
    _life_timer(0.0),
    _gravity(32.1740485564),
    _buoyancy(0),
    _wind(true),
    _mass(0),
    _random(false),
    _load_resistance(0),
    _solid(false),
    _force_stabilised(false),
    _slave_to_ac(false),
    _slave_load_to_ac(false),
    _contents_lb(0),
    _report_collision(false),
	_report_expiry(false),
    _report_impact(false),
    _external_force(false),
    _impact_report_node(fgGetNode("/ai/models/model-impact", true)),
    _old_height(0)

{
    no_roll = false;
}

FGAIBallistic::~FGAIBallistic() {
}

void FGAIBallistic::readFromScenario(SGPropertyNode* scFileNode) {
    if (!scFileNode){
        return;
    }

    FGAIBase::readFromScenario(scFileNode);

    //setPath(scFileNode->getStringValue("model", "Models/Geometry/rocket.ac"));
    setAzimuth(scFileNode->getDoubleValue("azimuth", 0.0));
    setElevation(scFileNode->getDoubleValue("elevation", 0));
    setDragArea(scFileNode->getDoubleValue("eda", 0.007));
    setLife(scFileNode->getDoubleValue("life", 900.0));
    setBuoyancy(scFileNode->getDoubleValue("buoyancy", 0));
    setWind_from_east(scFileNode->getDoubleValue("wind_from_east", 0));
    setWind_from_north(scFileNode->getDoubleValue("wind_from_north", 0));
    setWind(scFileNode->getBoolValue("wind", false));
    setRoll(scFileNode->getDoubleValue("roll", 0.0));
    setCd(scFileNode->getDoubleValue("cd", 0.029));
    //setMass(scFileNode->getDoubleValue("mass", 0.007));
    setWeight(scFileNode->getDoubleValue("weight", 0.25));
    setStabilisation(scFileNode->getBoolValue("aero-stabilized", false));
    setNoRoll(scFileNode->getBoolValue("no-roll", false));
    setRandom(scFileNode->getBoolValue("random", false));
    setImpact(scFileNode->getBoolValue("impact", false));
    setImpactReportNode(scFileNode->getStringValue("impact-reports"));
    setName(scFileNode->getStringValue("name", "Rocket"));
    setFuseRange(scFileNode->getDoubleValue("fuse-range", 0.0));
    setSMPath(scFileNode->getStringValue("submodel-path", ""));
    setSubID(scFileNode->getIntValue("SubID", 0));
    setExternalForce(scFileNode->getBoolValue("external-force", false));
    setForcePath(scFileNode->getStringValue("force-path", ""));
    setForceStabilisation(scFileNode->getBoolValue("force-stabilized", false));
    setXoffset(scFileNode->getDoubleValue("x-offset", 0.0));
    setYoffset(scFileNode->getDoubleValue("y-offset", 0.0));
    setZoffset(scFileNode->getDoubleValue("z-offset", 0.0));
    setPitchoffset(scFileNode->getDoubleValue("pitch-offset", 0.0));
    setRolloffset(scFileNode->getDoubleValue("roll-offset", 0.0));
    setYawoffset(scFileNode->getDoubleValue("yaw-offset", 0.0));
    setGroundOffset(scFileNode->getDoubleValue("ground-offset", 0.0));
    setLoadOffset(scFileNode->getDoubleValue("load-offset", 0.0));
    setSlaved(scFileNode->getBoolValue("slaved", false));
    setSlavedLoad(scFileNode->getBoolValue("slaved-load", false));
    setContentsNode(scFileNode->getStringValue("contents"));
    setRandom(scFileNode->getBoolValue("random", false));
}

bool FGAIBallistic::init(bool search_in_AI_path) {
    FGAIBase::init(search_in_AI_path);

    _impact_reported = false;
    _collision_reported = false;
    invisible = false;

    _elapsed_time += (sg_random() * 100);

    props->setStringValue("material/name", "");
    props->setStringValue("name", _name.c_str());
    props->setStringValue("submodels/path", _submodel.c_str());

    // start with high value so that animations don't trigger yet
    _ht_agl_ft = 1e10;
    hdg = _azimuth;
    pitch = _elevation;
    roll = _rotation;

    Transform();

    return true;
}

void FGAIBallistic::bind() {
    //    FGAIBase::bind();

    props->tie("sim/time/elapsed-sec",
        SGRawValueMethods<FGAIBallistic,double>(*this,
        &FGAIBallistic::_getTime));
    props->tie("mass-slug",
        SGRawValueMethods<FGAIBallistic,double>(*this,
        &FGAIBallistic::getMass));
    props->tie("material/load-resistance",
                SGRawValuePointer<double>(&_load_resistance));
    props->tie("material/solid",
                SGRawValuePointer<bool>(&_solid));
    props->tie("altitude-agl-ft",
                SGRawValuePointer<double>(&_ht_agl_ft));
    props->tie("controls/slave-to-ac",
        SGRawValueMethods<FGAIBallistic,bool>
        (*this, &FGAIBallistic::getSlaved, &FGAIBallistic::setSlaved));
    props->tie("controls/invisible",
        SGRawValuePointer<bool>(&invisible));

    if(_external_force){
        props->tie("controls/force_stabilized",
            SGRawValuePointer<bool>(&_force_stabilised));
        props->tie("position/global-x", 
            SGRawValueMethods<FGAIBase,double>(*this, &FGAIBase::_getCartPosX, 0));
        props->tie("position/global-y",
            SGRawValueMethods<FGAIBase,double>(*this, &FGAIBase::_getCartPosY, 0));
        props->tie("position/global-z",
            SGRawValueMethods<FGAIBase,double>(*this, &FGAIBase::_getCartPosZ, 0));
        props->tie("velocities/vertical-speed-fps",
            SGRawValuePointer<double>(&vs));
        props->tie("velocities/true-airspeed-kt",
            SGRawValuePointer<double>(&speed));
        props->tie("velocities/horizontal-speed-fps",
            SGRawValuePointer<double>(&hs));
        props->tie("position/altitude-ft",
            SGRawValueMethods<FGAIBase,double>(*this, &FGAIBase::_getAltitude, &FGAIBase::_setAltitude));
        props->tie("position/latitude-deg", 
            SGRawValueMethods<FGAIBase,double>(*this, &FGAIBase::_getLatitude, &FGAIBase::_setLatitude));
        props->tie("position/longitude-deg",
            SGRawValueMethods<FGAIBase,double>(*this, &FGAIBase::_getLongitude, &FGAIBase::_setLongitude));
        props->tie("orientation/hdg-deg",
            SGRawValuePointer<double>(&hdg));
        props->tie("orientation/pitch-deg",
            SGRawValuePointer<double>(&pitch));
        props->tie("orientation/roll-deg",
            SGRawValuePointer<double>(&roll));
        props->tie("controls/slave-load-to-ac",
            SGRawValueMethods<FGAIBallistic,bool>
            (*this, &FGAIBallistic::getSlavedLoad, &FGAIBallistic::setSlavedLoad));
        props->tie("position/load-offset",
            SGRawValueMethods<FGAIBallistic,double>
            (*this, &FGAIBallistic::getLoadOffset, &FGAIBallistic::setLoadOffset));
        props->tie("load/distance-to-hitch-ft",
            SGRawValueMethods<FGAIBallistic,double>
            (*this, &FGAIBallistic::getDistanceLoadToHitch));
        props->tie("load/elevation-to-hitch-deg",
            SGRawValueMethods<FGAIBallistic,double>
            (*this, &FGAIBallistic::getElevLoadToHitch));
        props->tie("load/bearing-to-hitch-deg",
            SGRawValueMethods<FGAIBallistic,double>
            (*this, &FGAIBallistic::getBearingLoadToHitch));
    }

}

void FGAIBallistic::unbind() {
    //    FGAIBase::unbind();

    props->untie("sim/time/elapsed-sec");
    props->untie("mass-slug");
    props->untie("material/load-resistance");
    props->untie("material/solid");
    props->untie("altitude-agl-ft");
    props->untie("controls/slave-to-ac");
    props->untie("controls/invisible");

    if(_external_force){
        props->untie("position/global-y");
        props->untie("position/global-x");
        props->untie("position/global-z");
        props->untie("velocities/vertical-speed-fps");
        props->untie("velocities/true-airspeed-kt");
        props->untie("velocities/horizontal-speed-fps");
        props->untie("position/altitude-ft");
        props->untie("position/latitude-deg");
        props->untie("position/longitude-deg");
        props->untie("position/ht-agl-ft");
        props->untie("orientation/hdg-deg");
        props->untie("orientation/pitch-deg");
        props->untie("orientation/roll-deg");
        props->untie("controls/force_stabilized");
        props->untie("position/load-offset");
        props->untie("load/distance-to-hitch-ft");
        props->untie("load/elevation-to-hitch-deg");
        props->untie("load/bearing-to-hitch-deg");
    }
}

void FGAIBallistic::update(double dt) {
    FGAIBase::update(dt);
    _setUserPos();

    if (_slave_to_ac){
        slaveToAC(dt);
        Transform();
        setHitchVelocity(dt);
    } else if (_formate_to_ac){
        formateToAC(dt);
        Transform();
        setHitchVelocity(dt);
    } else if (!invisible){
    Run(dt);
    Transform();
}

}

void FGAIBallistic::setAzimuth(double az) {
    hdg = _azimuth = az;
}

void FGAIBallistic::setElevation(double el) {
    pitch = _elevation = el;
}

void FGAIBallistic::setRoll(double rl) {
    roll = _rotation = rl;
}

void FGAIBallistic::setStabilisation(bool val) {
    _aero_stabilised = val;
}

void FGAIBallistic::setForceStabilisation(bool val) {
    _force_stabilised = val;
}

void FGAIBallistic::setNoRoll(bool nr) {
    no_roll = nr;
}

void FGAIBallistic::setDragArea(double a) {
    _drag_area = a;
}

void FGAIBallistic::setLife(double seconds) {
    life = seconds;
}

void FGAIBallistic::setBuoyancy(double fpss) {
    _buoyancy = fpss;
}

void FGAIBallistic::setWind_from_east(double fps) {
    _wind_from_east = fps;
}

void FGAIBallistic::setWind_from_north(double fps) {
    _wind_from_north = fps;
}

void FGAIBallistic::setWind(bool val) {
    _wind = val;
}

void FGAIBallistic::setCd(double c) {
    _Cd = c;
}

void FGAIBallistic::setMass(double m) {
    _mass = m;
}

void FGAIBallistic::setWeight(double w) {
    _weight_lb = w;
}

void FGAIBallistic::setRandom(bool r) {
    _random = r;
}

void FGAIBallistic::setImpact(bool i) {
    _report_impact = i;
}

void FGAIBallistic::setCollision(bool c) {
    _report_collision = c;
}

void FGAIBallistic::setExpiry(bool e) {
    _report_expiry = e;
//	cout <<  "_report_expiry " << _report_expiry << endl;
}

void FGAIBallistic::setExternalForce(bool f) {
    _external_force = f;
}

void FGAIBallistic::setImpactReportNode(const string& path) {

    if (!path.empty())
        _impact_report_node = fgGetNode(path.c_str(), true);
}

void FGAIBallistic::setName(const string& n) {
    _name = n;
}

void FGAIBallistic::setSMPath(const string& s) {
    _submodel = s;
}

void FGAIBallistic::setFuseRange(double f) {
    _fuse_range = f;
}

void FGAIBallistic::setSubID(int i) {
    _subID = i;
}

void FGAIBallistic::setSubmodel(const string& s) {
    _submodel = s;
}

void FGAIBallistic::setGroundOffset(double g) {
    _ground_offset = g;
}

void FGAIBallistic::setLoadOffset(double l) {
    _load_offset = l;
}

double FGAIBallistic::getLoadOffset() const {
    return _load_offset;
}

void FGAIBallistic::setSlaved(bool s) {
    _slave_to_ac = s;
}

void FGAIBallistic::setFormate(bool f) {
    _formate_to_ac = f;
}

void FGAIBallistic::setContentsNode(const string& path) {
    if (!path.empty()) {
        _contents_node = fgGetNode(path.c_str(), true);
    }
}

bool FGAIBallistic::getSlaved() const {
    return _slave_to_ac;
}  

double FGAIBallistic::getMass() const {
    return _mass;
}

double FGAIBallistic::getContents() {
    if(_contents_node) 
        _contents_lb = _contents_node->getChild("level-lbs",0,1)->getDoubleValue();
    return _contents_lb;
}

void FGAIBallistic::setContents(double c) {
    if(_contents_node) 
        _contents_lb = _contents_node->getChild("level-gal_us",0,1)->setDoubleValue(c);
}

void FGAIBallistic::setSlavedLoad(bool l) {
    _slave_load_to_ac = l;
}

bool FGAIBallistic::getSlavedLoad() const {
    return _slave_load_to_ac;
}

void FGAIBallistic::setForcePath(const string& p) {
    _force_path = p;
    if (!_force_path.empty()) {
        SGPropertyNode *fnode = fgGetNode(_force_path.c_str(), 0, true );
        _force_node = fnode->getChild("force-lb", 0, true);
        _force_azimuth_node = fnode->getChild("force-azimuth-deg", 0, true);
        _force_elevation_node = fnode->getChild("force-elevation-deg", 0, true);
    }
}

bool FGAIBallistic::getHtAGL(){

    if (getGroundElevationM(SGGeod::fromGeodM(pos, 10000),
                            _elevation_m, &_material)) {
            _ht_agl_ft = pos.getElevationFt() - _elevation_m * SG_METER_TO_FEET;
            if (_material) {
                const vector<string>& names = _material->get_names();

                _solid = _material->get_solid();
                _load_resistance = _material->get_load_resistance();
                _frictionFactor =_material->get_friction_factor();
                if (!names.empty())
                    props->setStringValue("material/name", names[0].c_str());
                else
                    props->setStringValue("material/name", "");
                /*cout << "material " << mat_name 
                << " solid " << _solid 
                << " load " << _load_resistance
                << " frictionFactor " << frictionFactor
                << endl;*/
            }
            return true;
    } else {
        return false;
    }

}

double FGAIBallistic::getRecip(double az){
    // calculate the reciprocal of the input azimuth 
    if(az - 180 < 0){
        return az + 180;
    } else {
        return az - 180; 
    }
}

void FGAIBallistic::setPch(double e, double dt, double coeff){
    double c = dt / (coeff + dt);
    pitch = (e * c) + (pitch * (1 - c));
}

void FGAIBallistic::setBnk(double r, double dt, double coeff){
    double c = dt / (coeff + dt);
    roll = (r * c) + (roll * (1 - c));
}

void FGAIBallistic::setHt(double h, double dt, double coeff){
    double c = dt / (coeff + dt);
    _height = (h * c) + (_height * (1 - c));
}

void FGAIBallistic::setHdg(double az, double dt, double coeff){
    double recip = getRecip(hdg);
    double c = dt / (coeff + dt);
    //we need to ensure that we turn the short way to the new hdg
    if (az < recip && az < hdg && hdg > 180) {
        hdg = ((az + 360) * c) + (hdg * (1 - c));
    } else if (az > recip && az > hdg && hdg <= 180){
        hdg = ((az - 360) * c) + (hdg * (1 - c));
    } else {
        hdg = (az * c) + (hdg * (1 - c));
    }
    }

double  FGAIBallistic::getTgtXOffset() const {
    return _tgt_x_offset;
}

double  FGAIBallistic::getTgtYOffset() const {
    return _tgt_y_offset;
} 

double  FGAIBallistic::getTgtZOffset() const {
    return _tgt_z_offset;
}

void FGAIBallistic::setTgtXOffset(double x){
    _tgt_x_offset = x;
}

void FGAIBallistic::setTgtYOffset(double y){
    _tgt_y_offset = y;
}

void FGAIBallistic::setTgtZOffset(double z){
    _tgt_z_offset = z;
}

void FGAIBallistic::slaveToAC(double dt){

    setHitchPos();
    pos.setLatitudeDeg(hitchpos.getLatitudeDeg());
    pos.setLongitudeDeg(hitchpos.getLongitudeDeg());
    pos.setElevationFt(hitchpos.getElevationFt());
    setHeading(manager->get_user_heading());
    setPitch(manager->get_user_pitch() + _pitch_offset);
    setBank(manager->get_user_roll() + _roll_offset);
    setSpeed(manager->get_user_speed());
    //update the mass (slugs)
    _mass = (_weight_lb + getContents()) / slugs_to_lbs;

    /*cout <<"_mass "<<_mass <<" " << getContents() 
    <<" " << getContents() / slugs_to_lbs << endl;*/
}

void FGAIBallistic::Run(double dt) {
    _life_timer += dt;

    // if life = -1 the object does not die
	if (_life_timer > life && life != -1){

		if (_report_expiry && !_expiry_reported){
			handle_expiry();
		} else
			setDie(true);

	}

    //set the contents in the appropriate tank or other property in the parent to zero
    setContents(0);

    //randomise Cd by +- 5%
    if (_random)
        _Cd = _Cd * 0.95 + (0.05 * sg_random());

    // Adjust Cd by Mach number. The equations are based on curves
    // for a conventional shell/bullet (no boat-tail).
    double Cdm;

    if (Mach < 0.7)
        Cdm = 0.0125 * Mach + _Cd;
    else if (Mach < 1.2 )
        Cdm = 0.3742 * pow(Mach, 2) - 0.252 * Mach + 0.0021 + _Cd;
    else
        Cdm = 0.2965 * pow(Mach, -1.1506) + _Cd;

    //cout << "Mach " << Mach << " Cdm " << Cdm << "// ballistic speed kts "<< speed <<  endl;

    // drag = Cd * 0.5 * rho * speed * speed * drag_area;
    // rho is adjusted for altitude in void FGAIBase::update,
    // using Standard Atmosphere (sealevel temperature 15C)
    // acceleration = drag/mass;
    // adjust speed by drag
    speed -= (Cdm * 0.5 * rho * speed * speed * _drag_area/_mass) * dt;

    // don't let speed become negative
    if ( speed < 0.0 )
        speed = 0.0;

    double speed_fps = speed * SG_KT_TO_FPS;
    //double hs;

    // calculate vertical and horizontal speed components
    if (speed == 0.0) {
        hs = vs = 0.0;
    } else {
        vs = sin( _elevation * SG_DEGREES_TO_RADIANS ) * speed_fps;
        hs = cos( _elevation * SG_DEGREES_TO_RADIANS ) * speed_fps;
    }

    //resolve horizontal speed into north and east components:
    double speed_north_fps = cos(_azimuth / SG_RADIANS_TO_DEGREES) * hs;
    double speed_east_fps = sin(_azimuth / SG_RADIANS_TO_DEGREES) * hs;

    // convert horizontal speed (fps) to degrees per second
    double speed_north_deg_sec = speed_north_fps / ft_per_deg_lat;
    double speed_east_deg_sec  = speed_east_fps / ft_per_deg_lon;

    // if wind not required, set to zero
    if (!_wind) {
        _wind_from_north = 0;
        _wind_from_east = 0;
    } else {
        _wind_from_north = manager->get_wind_from_north();
        _wind_from_east = manager->get_wind_from_east();
    }

    //calculate velocity due to external force
    double force_speed_north_deg_sec = 0;
    double force_speed_east_deg_sec = 0;
//    double vs_force_fps = 0;
    double hs_force_fps = 0;
    double v_force_acc_fpss = 0;
    double force_speed_north_fps = 0;
    double force_speed_east_fps = 0;
    double h_force_lbs = 0;
    double normal_force_lbs = 0;
    double normal_force_fpss = 0;
    double static_friction_force_lbs = 0;
    double dynamic_friction_force_lbs = 0;
    double friction_force_speed_north_fps = 0;
    double friction_force_speed_east_fps = 0;
    double friction_force_speed_north_deg_sec = 0;
    double friction_force_speed_east_deg_sec = 0;
    double force_elevation_deg = 0;

    if (_external_force) {
        SGPropertyNode *n = fgGetNode(_force_path.c_str(), true);
        double force_lbs            = n->getChild("force-lb", 0, true)->getDoubleValue();
        force_elevation_deg         = n->getChild("force-elevation-deg", 0, true)->getDoubleValue();
        double force_azimuth_deg    = n->getChild("force-azimuth-deg", 0, true)->getDoubleValue();

        //resolve force into vertical and horizontal components:
        double v_force_lbs = force_lbs * sin( force_elevation_deg * SG_DEGREES_TO_RADIANS );
        h_force_lbs = force_lbs * cos( force_elevation_deg * SG_DEGREES_TO_RADIANS );

        //ground interaction 

        if (getHtAGL()){
            double deadzone = 0.1;

            if (_ht_agl_ft <= (0 + _ground_offset + deadzone) && _solid){
                normal_force_lbs = (_mass * slugs_to_lbs) - v_force_lbs;

                if ( normal_force_lbs < 0 )
                    normal_force_lbs = 0;

                pos.setElevationFt(0 + _ground_offset);
                if (vs < 0) 
                    vs = -vs * 0.5;

                // calculate friction
                // we assume a static Coefficient of Friction (mu) of 0.62 (wood on concrete)
                double mu = 0.62;

                static_friction_force_lbs = mu * normal_force_lbs * _frictionFactor;

                //adjust horizontal force. We assume that a speed of <= 5 fps is static 
                if (h_force_lbs <= static_friction_force_lbs && hs <= 5){
                    h_force_lbs = hs = 0;
                    speed_north_fps = speed_east_fps = 0;
                } else
                    dynamic_friction_force_lbs = (static_friction_force_lbs * 0.95);

                //ignore wind when on the ground for now
                //TODO fix this
                _wind_from_north = 0;
                _wind_from_east = 0;

            }

        }

        //acceleration = (force(lbsf)/mass(slugs))
        v_force_acc_fpss = v_force_lbs/_mass;
        normal_force_fpss = normal_force_lbs/_mass;
        double h_force_acc_fpss = h_force_lbs/_mass;
        double dynamic_friction_acc_fpss = dynamic_friction_force_lbs/_mass;

        // velocity = acceleration * dt
        hs_force_fps = h_force_acc_fpss * dt;
        double friction_force_fps = dynamic_friction_acc_fpss * dt;

        //resolve horizontal speeds into north and east components:
        force_speed_north_fps   = cos(force_azimuth_deg * SG_DEGREES_TO_RADIANS) * hs_force_fps;
        force_speed_east_fps    = sin(force_azimuth_deg * SG_DEGREES_TO_RADIANS) * hs_force_fps;

        friction_force_speed_north_fps = cos(getRecip(hdg) * SG_DEGREES_TO_RADIANS) * friction_force_fps;
        friction_force_speed_east_fps  = sin(getRecip(hdg) * SG_DEGREES_TO_RADIANS) * friction_force_fps;

        // convert horizontal speed (fps) to degrees per second
        force_speed_north_deg_sec = force_speed_north_fps / ft_per_deg_lat;
        force_speed_east_deg_sec  = force_speed_east_fps / ft_per_deg_lon;

        friction_force_speed_north_deg_sec = friction_force_speed_north_fps / ft_per_deg_lat;
        friction_force_speed_east_deg_sec  = friction_force_speed_east_fps / ft_per_deg_lon;
    }

    // convert wind speed (fps) to degrees lat/lon per second
    double wind_speed_from_north_deg_sec = _wind_from_north / ft_per_deg_lat;
    double wind_speed_from_east_deg_sec  = _wind_from_east / ft_per_deg_lon;

    //recombine the horizontal velocity components
    hs = sqrt(((speed_north_fps + force_speed_north_fps + friction_force_speed_north_fps) 
        * (speed_north_fps + force_speed_north_fps + friction_force_speed_north_fps))
        + ((speed_east_fps + force_speed_east_fps + friction_force_speed_east_fps) 
        * (speed_east_fps + force_speed_east_fps + friction_force_speed_east_fps)));

    if (hs <= 0.00001)
        hs = 0;

    // adjust vertical speed for acceleration of gravity, buoyancy, and vertical force
    vs -= (_gravity - _buoyancy - v_force_acc_fpss - normal_force_fpss) * dt;

    if (vs <= 0.00001 && vs >= -0.00001)
        vs = 0;

    // set new position
    if(_slave_load_to_ac) {
        setHitchPos();
        pos.setLatitudeDeg(hitchpos.getLatitudeDeg());
        pos.setLongitudeDeg(hitchpos.getLongitudeDeg());
        pos.setElevationFt(hitchpos.getElevationFt());

        if (getHtAGL()){
            double deadzone = 0.1;

            if (_ht_agl_ft <= (0 + _ground_offset + deadzone) && _solid){
                pos.setElevationFt(0 + _ground_offset);
            } else {
                pos.setElevationFt(hitchpos.getElevationFt() + _load_offset);
            }

        }
    } else {
        pos.setLatitudeDeg( pos.getLatitudeDeg()
            + (speed_north_deg_sec - wind_speed_from_north_deg_sec 
            + force_speed_north_deg_sec + friction_force_speed_north_deg_sec) * dt );
        pos.setLongitudeDeg( pos.getLongitudeDeg()
            + (speed_east_deg_sec - wind_speed_from_east_deg_sec 
            + force_speed_east_deg_sec + friction_force_speed_east_deg_sec) * dt );
        pos.setElevationFt(pos.getElevationFt() + vs * dt);
    }

    // recalculate total speed
    if ( vs == 0 && hs == 0)
        speed = 0;
    else
        speed = sqrt( vs * vs + hs * hs) / SG_KT_TO_FPS;

    // recalculate elevation and azimuth (velocity vectors)
    _elevation = atan2( vs, hs ) * SG_RADIANS_TO_DEGREES;
    _azimuth =  atan2((speed_east_fps + force_speed_east_fps + friction_force_speed_east_fps), 
        (speed_north_fps + force_speed_north_fps + friction_force_speed_north_fps))
        * SG_RADIANS_TO_DEGREES;

    // rationalise azimuth
    if (_azimuth < 0)
        _azimuth += 360;

    if (_aero_stabilised) { // we simulate rotational moment of inertia by using a filter
        const double coeff = 0.9;

        // we assume a symetrical MI about the pitch and yaw axis
        setPch(_elevation, dt, coeff);
        setHdg(_azimuth, dt, coeff);
    } else if (_force_stabilised) { // we simulate rotational moment of inertia by using a filter
        const double coeff = 0.9;
        double ratio = h_force_lbs/(_mass * slugs_to_lbs);

        if (ratio >  1) ratio =  1;
        if (ratio < -1) ratio = -1;

        double force_pitch = acos(ratio) * SG_RADIANS_TO_DEGREES;

        if (force_pitch <= force_elevation_deg)
            force_pitch = force_elevation_deg;

        // we assume a symetrical MI about the pitch and yaw axis
        setPch(force_pitch,dt, coeff);
        setHdg(_azimuth, dt, coeff);
    }

    //do impacts and collisions
    if (_report_impact && !_impact_reported)
        handle_impact();

    if (_report_collision && !_collision_reported)
        handle_collision();

    // set destruction flag if altitude less than sea level -1000
    if (altitude_ft < -1000.0 && life != -1)
        setDie(true);

}  // end Run

double FGAIBallistic::_getTime() const {
    return _life_timer;
}

void FGAIBallistic::handle_impact() {

    // try terrain intersection
    if(!getHtAGL()) 
        return;

    if (_ht_agl_ft <= 0) {
        SG_LOG(SG_GENERAL, SG_DEBUG, "AIBallistic: terrain impact");
        report_impact(_elevation_m);
        _impact_reported = true;

        if (life == -1){
            invisible = true;
        } else if (_subID == 0)  // kill the AIObject if there is no subsubmodel
            setDie(true);
    }
}

void FGAIBallistic::handle_expiry() {

        report_impact(pos.getElevationM());
        _expiry_reported = true;

		SG_LOG(SG_GENERAL, SG_ALERT, "AIBallistic: expiry");
        //if (life == -1){
        //    invisible = true;
        //} else if (_subID == 0)  // kill the AIObject if there is no subsubmodel
        //    setDie(true);
   
}

void FGAIBallistic::handle_collision()
{
    const FGAIBase *object = manager->calcCollision(pos.getElevationFt(),
            pos.getLatitudeDeg(),pos.getLongitudeDeg(), _fuse_range);

    if (object) {
        SG_LOG(SG_GENERAL, SG_DEBUG, "AIBallistic: object hit");
        report_impact(pos.getElevationM(), object);
        _collision_reported = true;
    }
}

void FGAIBallistic::report_impact(double elevation, const FGAIBase *object)
{
    _impact_lat    = pos.getLatitudeDeg();
    _impact_lon    = pos.getLongitudeDeg();
    _impact_elev   = elevation;
    _impact_speed  = speed * SG_KT_TO_MPS;
    _impact_hdg    = hdg;
    _impact_pitch  = pitch;
    _impact_roll   = roll;

    SGPropertyNode *n = props->getNode("impact", true);
    if (object)
        n->setStringValue("type", object->getTypeString());
    else
        n->setStringValue("type", "terrain");

    n->setDoubleValue("longitude-deg", _impact_lon);
    n->setDoubleValue("latitude-deg", _impact_lat);
    n->setDoubleValue("elevation-m", _impact_elev);
    n->setDoubleValue("heading-deg", _impact_hdg);
    n->setDoubleValue("pitch-deg", _impact_pitch);
    n->setDoubleValue("roll-deg", _impact_roll);
    n->setDoubleValue("speed-mps", _impact_speed);

    _impact_report_node->setStringValue(props->getPath());
}

SGVec3d FGAIBallistic::getCartUserPos() const {
    SGVec3d cartUserPos = SGVec3d::fromGeod(userpos);
    return cartUserPos;
}

SGVec3d FGAIBallistic::getCartHitchPos() const{

    // convert geodetic positions to geocentered
    SGVec3d cartuserPos = getCartUserPos();
    SGVec3d cartPos = getCartPos();

    // Transform to the right coordinate frame, configuration is done in
    // the x-forward, y-right, z-up coordinates (feet), computation
    // in the simulation usual body x-forward, y-right, z-down coordinates
    // (meters) )
    SGVec3d _off(_x_offset * SG_FEET_TO_METER,
        _y_offset * SG_FEET_TO_METER,
        -_z_offset * SG_FEET_TO_METER);

    // Transform the user position to the horizontal local coordinate system.
    SGQuatd hlTrans = SGQuatd::fromLonLat(userpos);

    // and postrotate the orientation of the user model wrt the horizontal
    // local frame
    hlTrans *= SGQuatd::fromYawPitchRollDeg(
        manager->get_user_heading(),
        manager->get_user_pitch(),
        manager->get_user_roll());

    // The offset converted to the usual body fixed coordinate system
    // rotated to the earth-fixed coordinates axis
    SGVec3d off = hlTrans.backTransform(_off);

    // Add the position offset of the user model to get the geocentered position
    SGVec3d offsetPos = cartuserPos + off;

    return offsetPos;
}

void FGAIBallistic::setHitchPos(){
    // convert the hitch geocentered position to geodetic
    SGVec3d carthitchPos = getCartHitchPos();

    SGGeodesy::SGCartToGeod(carthitchPos, hitchpos);
}

double FGAIBallistic::getDistanceLoadToHitch() const {
    //calculate the distance load to hitch 
    SGVec3d carthitchPos = getCartHitchPos();
    SGVec3d cartPos = getCartPos();

    SGVec3d diff = carthitchPos - cartPos;
    double distance = norm(diff);
    return distance * SG_METER_TO_FEET;
}

void FGAIBallistic::setHitchVelocity(double dt) {
    //calculate the distance from the previous hitch position
    SGVec3d carthitchPos = getCartHitchPos();
    SGVec3d diff = carthitchPos - _oldcarthitchPos;

    double distance = norm(diff);

    //calculate speed knots
    speed = (distance/dt) * SG_MPS_TO_KT;

    //now calulate the angle between the old and current hitch positions (degrees)
    double angle = 0;
    double daltM = hitchpos.getElevationM() - oldhitchpos.getElevationM();

    if (fabs(distance) < SGLimits<float>::min()) {
        angle = 0;
    } else {
        double sAngle = daltM/distance;
        sAngle = SGMiscd::min(1, SGMiscd::max(-1, sAngle));
        angle = SGMiscd::rad2deg(asin(sAngle));
    }

    _elevation = angle;

    //calculate the bearing of the new hitch position from the old
    double az1, az2, dist;

    geo_inverse_wgs_84(oldhitchpos, hitchpos, &az1, &az2, &dist);

    _azimuth = az1;

    // and finally store the new values
    _oldcarthitchPos = carthitchPos;
    oldhitchpos = hitchpos;
}

double FGAIBallistic::getElevLoadToHitch() const {
    // now the angle, positive angles are upwards
    double distance = getDistanceLoadToHitch() * SG_FEET_TO_METER;
    double angle = 0;
    double daltM = hitchpos.getElevationM() - pos.getElevationM();

    if (fabs(distance) < SGLimits<float>::min()) {
        angle = 0;
    } else {
        double sAngle = daltM/distance;
        sAngle = SGMiscd::min(1, SGMiscd::max(-1, sAngle));
        angle = SGMiscd::rad2deg(asin(sAngle));
    }

    return angle;
}

double FGAIBallistic::getBearingLoadToHitch() const {
    //calculate the bearing and range of the second pos from the first
    double az1, az2, distance;

    geo_inverse_wgs_84(pos, hitchpos, &az1, &az2, &distance);

    return az1;
}

double FGAIBallistic::getRelBrgHitchToUser() const {
    //calculate the relative bearing 
    double az1, az2, distance;

    geo_inverse_wgs_84(hitchpos, userpos, &az1, &az2, &distance);

    double rel_brg = az1 - hdg;

    if (rel_brg > 180)
        rel_brg -= 360;

    return rel_brg;
}

double FGAIBallistic::getElevHitchToUser() const {

    //calculate the distance from the user position
    SGVec3d carthitchPos = getCartHitchPos();
    SGVec3d cartuserPos = getCartUserPos();

    SGVec3d diff = cartuserPos - carthitchPos;

    double distance = norm(diff);
    double angle = 0;

    double daltM = userpos.getElevationM() - hitchpos.getElevationM();

    // now the angle, positive angles are upwards
    if (fabs(distance) < SGLimits<float>::min()) {
        angle = 0;
    } else {
        double sAngle = daltM/distance;
        sAngle = SGMiscd::min(1, SGMiscd::max(-1, sAngle));
        angle = SGMiscd::rad2deg(asin(sAngle));
    }

    return angle;
}

void FGAIBallistic::setTgtOffsets(double dt, double coeff){
    double c = dt / (coeff + dt);

    _x_offset = (_tgt_x_offset * c) + (_x_offset * (1 - c));
    _y_offset = (_tgt_y_offset * c) + (_y_offset * (1 - c));
    _z_offset = (_tgt_z_offset * c) + (_z_offset * (1 - c));
}

void FGAIBallistic::formateToAC(double dt){

    setTgtOffsets(dt, 25);
    setHitchPos();
    setHitchVelocity(dt);

    // elapsed time has a random initialisation so that each 
    // wingman moves differently
    _elapsed_time += dt;

    // we derive a sine based factor to give us smoothly 
    // varying error between -1 and 1
    double factor  = sin(SGMiscd::deg2rad(_elapsed_time * 10));
    double r_angle = 5 * factor;
    double p_angle = 2.5 * factor;
    double h_angle = 5 * factor;
    double h_feet  = 3 * factor;

    pos.setLatitudeDeg(hitchpos.getLatitudeDeg());
    pos.setLongitudeDeg(hitchpos.getLongitudeDeg());

    if (getHtAGL()){

        if(_ht_agl_ft <= 10) {
            _height = userpos.getElevationFt();
        } else if (_ht_agl_ft > 10 && _ht_agl_ft <= 150 ) {
            setHt(userpos.getElevationFt(), dt, 1.0);
        } else if (_ht_agl_ft > 150 && _ht_agl_ft <= 250) {
            setHt(hitchpos.getElevationFt()+ h_feet, dt, 0.75);
        } else
            setHt(hitchpos.getElevationFt()+ h_feet, dt, 0.5);

        pos.setElevationFt(_height);
    }

    // these calculations are unreliable at slow speeds
    if(speed >= 10) {
        setHdg(_azimuth + h_angle, dt, 0.9);
        setPch(_elevation + p_angle + _pitch_offset, dt, 0.9);

        if (roll <= 115 && roll >= -115)
            setBnk(manager->get_user_roll() + r_angle + _roll_offset, dt, 0.5);
        else
            roll = manager->get_user_roll() + r_angle + _roll_offset;

    } else {
        setHdg(manager->get_user_heading(), dt, 0.9);
        setPch(manager->get_user_pitch() + _pitch_offset, dt, 0.9);
        setBnk(manager->get_user_roll() + _roll_offset, dt, 0.9);
    }

    setSpeed(speed);
}
// end AIBallistic
