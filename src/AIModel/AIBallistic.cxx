/*
 * SPDX-FileName: AIBallistic.cxx
 * SPDX-FileComment: GAIBase-derived class creates a ballistic object
 * SPDX-FileCopyrightText: Copyright (C) 2003  David P. Culp - davidculp2@comcast.net
 * SPDX-FileContributor: With major additions by Mathias Froehlich & Vivian Meazza 2004-2008
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <config.h>

#include <simgear/math/sg_random.hxx>
#include <simgear/math/sg_geodesy.hxx>
#include <simgear/scene/model/modellib.hxx>

#include <Scenery/scenery.hxx>

#include "AIBallistic.hxx"

#include <Main/util.hxx>
#include <Environment/gravity.hxx>
#include <Environment/atmosphere.hxx>
#include <Main/fg_props.hxx>

using namespace simgear;
using std::string;

const double FGAIBallistic::slugs_to_kgs = 14.5939029372;
const double FGAIBallistic::slugs_to_lbs = 32.1740485564;

FGAIBallistic::FGAIBallistic(object_type ot) :
FGAIBase(ot, false),
_height(0.0),
_speed(0),
_ht_agl_ft(0.0),
_azimuth(0.0),
_elevation(0.0),
_rotation(0.0),
hs(0),
_elapsed_time(0),
_az_random_error(0.0),
_el_random_error(0.0),
_aero_stabilised(false),
_drag_area(0.007),
_cd(0.029),
_init_cd(0.029),
_cd_randomness(0.0),
_buoyancy(0),
_life_timer(0.0),
_wind(true),
_mass(0),
_random(false),
_life_randomness(0.0),
_load_resistance(0),
_solid(false),
_force_stabilised(false),
_slave_to_ac(false),
_slave_load_to_ac(false),
_contents_lb(0),
_report_collision(false),
_report_impact(false),
_external_force(false),
_report_expiry(false),
_impact_report_node(fgGetNode("/ai/models/model-impact", true))

{
    no_roll = false;
}


void FGAIBallistic::readFromScenario(SGPropertyNode* scFileNode) {
    if (!scFileNode){
        return;
    }

    FGAIBase::readFromScenario(scFileNode);

    //setPath(scFileNode->getStringValue("model", "Models/Geometry/rocket.ac")); 
    setRandom(scFileNode->getBoolValue("random", false));
    setAzimuth(scFileNode->getDoubleValue("azimuth", 0.0));
    setElevation(scFileNode->getDoubleValue("elevation", 0));
    setDragArea(scFileNode->getDoubleValue("eda", 0.007));
    setLife(scFileNode->getDoubleValue("life", 900.0));
    setBuoyancy(scFileNode->getDoubleValue("buoyancy", 0));
    //setWind_from_east(scFileNode->getDoubleValue("wind_from_east", 0));
    //setWind_from_north(scFileNode->getDoubleValue("wind_from_north", 0));
    setWind(scFileNode->getBoolValue("wind", false));
    setRoll(scFileNode->getDoubleValue("roll", 0.0));
    setCd(scFileNode->getDoubleValue("cd", 0.029));
    //setMass(scFileNode->getDoubleValue("mass", 0.007));
    setWeight(scFileNode->getDoubleValue("weight", 0.25));
    setStabilisation(scFileNode->getBoolValue("aero-stabilised", false));
    setNoRoll(scFileNode->getBoolValue("no-roll", false));
    setImpact(scFileNode->getBoolValue("impact", false));
    setExpiry(scFileNode->getBoolValue("expiry", false));
    setCollision(scFileNode->getBoolValue("collision", false));
    setImpactReportNode(scFileNode->getStringValue("impact-reports"));
    setName(scFileNode->getStringValue("name", "Rocket"));
    setFuseRange(scFileNode->getDoubleValue("fuse-range", 0.0));
    setSMPath(scFileNode->getStringValue("submodel-path", ""));
    setSubID(scFileNode->getIntValue("SubID", 0));
    setExternalForce(scFileNode->getBoolValue("external-force", false));
    setForcePath(scFileNode->getStringValue("force-path", ""));
    setForceStabilisation(scFileNode->getBoolValue("force-stabilised", false));
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
    setContentsPath(scFileNode->getStringValue("contents"));
    setParentName(scFileNode->getStringValue("parent"));
}

bool FGAIBallistic::init(ModelSearchOrder searchOrder)
{
    FGAIBase::init(searchOrder);
    reinit();
    return true;
}

void FGAIBallistic::reinit() {
    _impact_reported = false;
    _collision_reported = false;
    _expiry_reported = false;

    _impact_lat = 0;
    _impact_lon = 0;
    _impact_elev = 0;
    _impact_hdg = 0;
    _impact_pitch = 0;
    _impact_roll = 0;
    _impact_speed = 0;

    invisible = false;

    _elapsed_time += (sg_random() * 100);

    _life_timer = 0;

    props->setStringValue("material/name", "");
    props->setStringValue("name", _name.c_str());
    props->setStringValue("submodels/path", _path.c_str());

    if (_slave_to_ac) {
        props->setStringValue("force/path", _force_path.c_str());
        props->setStringValue("contents/path", _contents_path.c_str());
    }

    //cout << "init: name " << _name.c_str() << " _life_timer " << _life_timer 
    //    << endl;

    //if(_parent != ""){
    //    setParentNode();
    //}

    //setParentNodes(_selected_ac);

    //props->setStringValue("vector/path", _vector_path.c_str());

    // start with high value so that animations don't trigger yet
    _ht_agl_ft = 1e10;
    hdg = _azimuth;
    pitch = _elevation;
    roll = _rotation;

    Transform();

    if (_parent != "") {
        setParentNode();
    }

    setParentNodes(_selected_ac);

    FGAIBase::reinit();
}

void FGAIBallistic::bind() {
    //    FGAIBase::bind();

    _tiedProperties.setRoot(props);
    tie("sim/time/elapsed-sec",
        SGRawValueMethods<FGAIBallistic,double>(*this,
        &FGAIBallistic::_getTime, &FGAIBallistic::setTime));
    //tie("mass-slug",
    //    SGRawValueMethods<FGAIBallistic,double>(*this,
    //    &FGAIBallistic::getMass));

    tie("material/solid",
        SGRawValuePointer<bool>(&_solid));
    tie("altitude-agl-ft",
        SGRawValuePointer<double>(&_ht_agl_ft));
    tie("controls/slave-to-ac",
        SGRawValueMethods<FGAIBallistic,bool>
        (*this, &FGAIBallistic::getSlaved, &FGAIBallistic::setSlaved));
    tie("controls/invisible",
        SGRawValuePointer<bool>(&invisible));

    if (_external_force || _slave_to_ac) {
        tie("controls/force_stabilized",
            SGRawValuePointer<bool>(&_force_stabilised));
        tie("position/global-x",
            SGRawValueMethods<FGAIBase,double>(*this, &FGAIBase::_getCartPosX, 0));
        tie("position/global-y",
            SGRawValueMethods<FGAIBase,double>(*this, &FGAIBase::_getCartPosY, 0));
        tie("position/global-z",
            SGRawValueMethods<FGAIBase,double>(*this, &FGAIBase::_getCartPosZ, 0));
        tie("velocities/vertical-speed-fps",
            SGRawValueMethods<FGAIBase,double>(*this, &FGAIBase::_getVS_fps, &FGAIBase::_setVS_fps));
        tie("velocities/true-airspeed-kt",
            SGRawValuePointer<double>(&speed));
        tie("velocities/horizontal-speed-fps",
            SGRawValuePointer<double>(&hs));
        tie("position/altitude-ft",
            SGRawValueMethods<FGAIBase,double>(*this, &FGAIBase::_getElevationFt, &FGAIBase::_setAltitude));
        tie("position/latitude-deg",
            SGRawValueMethods<FGAIBase,double>(*this, &FGAIBase::_getLatitude, &FGAIBase::_setLatitude));
        tie("position/longitude-deg",
            SGRawValueMethods<FGAIBase,double>(*this, &FGAIBase::_getLongitude, &FGAIBase::_setLongitude));
        tie("orientation/hdg-deg",
            SGRawValuePointer<double>(&hdg));
        tie("orientation/pitch-deg",
            SGRawValuePointer<double>(&pitch));
        tie("orientation/roll-deg",
            SGRawValuePointer<double>(&roll));
        tie("controls/slave-load-to-ac",
            SGRawValueMethods<FGAIBallistic,bool>
            (*this, &FGAIBallistic::getSlavedLoad, &FGAIBallistic::setSlavedLoad));
        tie("position/load-offset",
            SGRawValueMethods<FGAIBallistic,double>
            (*this, &FGAIBallistic::getLoadOffset, &FGAIBallistic::setLoadOffset));
        tie("load/distance-to-hitch-ft",
            SGRawValueMethods<FGAIBallistic,double>
            (*this, &FGAIBallistic::getDistanceToHitch));
        tie("load/elevation-to-hitch-deg",
            SGRawValueMethods<FGAIBallistic,double>
            (*this, &FGAIBallistic::getElevToHitch));
        tie("load/bearing-to-hitch-deg",
            SGRawValueMethods<FGAIBallistic,double>
            (*this, &FGAIBallistic::getBearingToHitch));
        tie("material/load-resistance",
        SGRawValuePointer<double>(&_load_resistance));
    }
}

void FGAIBallistic::update(double dt)
{
    FGAIBase::update(dt);

    if (_slave_to_ac) {
        slaveToAC(dt);
        Transform();
    }
    else if (!invisible) {
        Run(dt);
        Transform();
    }

}

void FGAIBallistic::setAzimuth(double az) {
    if (_random)
        hdg = _azimuth = az - _az_random_error + 2 * _az_random_error * sg_random();
    else 
        hdg = _azimuth = az;
}

void FGAIBallistic::setAzimuthRandomError(double error) {
    _az_random_error = error;
}

void FGAIBallistic::setElevationRandomError(double error) {
    _el_random_error = error;
}

void FGAIBallistic::setElevation(double el) {
    if (_random)
        pitch = _elevation = el - _el_random_error + 2 * _el_random_error * sg_random();
    else
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
    if (_random)
        life = seconds * _life_randomness + (seconds * (1 -_life_randomness) * sg_random());
    else
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

void FGAIBallistic::setCd(double cd) {
    _cd = cd;
    _init_cd = cd;
}

void FGAIBallistic::setCdRandomness(double randomness) {
    _cd_randomness = randomness;
}

void FGAIBallistic::setMass(double m) {
    _mass = m;
}

void FGAIBallistic::setWeight(double w) {
    _weight_lb = w;
}

void FGAIBallistic::setLifeRandomness(double randomness) {
    _life_randomness = randomness;
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
}

void FGAIBallistic::setExternalForce(bool f) {
    _external_force = f;
}

void FGAIBallistic::setImpactReportNode(const string& path) {
    if (!path.empty())
        _impact_report_node = fgGetNode(path.c_str(), true);
}

void FGAIBallistic::setSMPath(const string& s) {
    _path = s;
    //cout << "submodel path " << _path << endl;
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

void FGAIBallistic::setContentsPath(const string& path) {
    _contents_path = path;

    if (!path.empty()) {
        _contents_node = fgGetNode(path.c_str(), true);
    }
}

void FGAIBallistic::setContentsNode(SGPropertyNode_ptr node) {
    if (node != 0) {
        _contents_node = node;
        _contents_path = _contents_node->getDisplayName();
    }
}

void FGAIBallistic::setParentNodes(SGPropertyNode_ptr node) {
    if (node != 0) {
        _pnode = node;
        _p_pos_node = _pnode->getChild("position", 0, true);
        _p_lat_node = _p_pos_node->getChild("latitude-deg", 0, true);
        _p_lon_node = _p_pos_node->getChild("longitude-deg", 0, true);
        _p_alt_node = _p_pos_node->getChild("altitude-ft", 0, true);
        _p_agl_node = _p_pos_node->getChild("altitude-agl-ft", 0, true);


        _p_ori_node = _pnode->getChild("orientation", 0, true);
        _p_pch_node = _p_ori_node->getChild("pitch-deg", 0, true);
        _p_rll_node = _p_ori_node->getChild("roll-deg", 0, true);
        _p_hdg_node = _p_ori_node->getChild("true-heading-deg",0, true);

        _p_vel_node = _pnode->getChild("velocities", 0, true);
        _p_spd_node = _p_vel_node->getChild("true-airspeed-kt", 0, true);
    }
}

void FGAIBallistic::setParentPos() {
    if (_pnode != 0) { 
        double lat = _p_lat_node->getDoubleValue();
        double lon = _p_lon_node->getDoubleValue();
        double alt = _p_alt_node->getDoubleValue();

        _parentpos.setLongitudeDeg(lon);
        _parentpos.setLatitudeDeg(lat);
        _parentpos.setElevationFt(alt);
    }
}

bool FGAIBallistic::getSlaved() const {
    return _slave_to_ac;
}

double FGAIBallistic::getMass() const {
    return _mass;
}

double FGAIBallistic::getContents() {
    if (_contents_node) {
        _contents_lb = _contents_node->getChild("level-lbs", 0, 1)->getDoubleValue();
    }
    return _contents_lb;
}

void FGAIBallistic::setContents(double c) {
    if (_contents_node)
        _contents_lb = _contents_node->getChild("level-gal_us", 0, 1)->setDoubleValue(c);
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

bool FGAIBallistic::getHtAGL(double start) {
    const simgear::BVHMaterial* mat = 0;
    if (getGroundElevationM(SGGeod::fromGeodM(pos, start),
        _elevation_m, &mat)) {
            const SGMaterial* material = dynamic_cast<const SGMaterial*>(mat);
            _ht_agl_ft = pos.getElevationFt() - _elevation_m * SG_METER_TO_FEET;

            if (material) {
                const std::vector<string>& names = material->get_names();
                _solid = material->get_solid();
                _load_resistance = material->get_load_resistance();
                _frictionFactor = material->get_friction_factor();

                if (!names.empty())
                    props->setStringValue("material/name", names[0].c_str());
                else
                    props->setStringValue("material/name", "");

                _mat_name = names[0];

                //cout << "material " << _mat_name 
                //<< " solid " << _solid 
                //<< " load " << _load_resistance
                //<< " frictionFactor " << _frictionFactor
                //<< endl;
            }

            return true;
    }
    else {
        return false;
    }
}

double FGAIBallistic::getRecip(double az) {
    // calculate the reciprocal of the input azimuth 
    if (az - 180 < 0) {
        return az + 180;
    }
    else {
        return az - 180; 
    }
}

void FGAIBallistic::setPch(double e, double dt, double coeff) {
    double c = dt / (coeff + dt);
    pitch = (e * c) + (pitch * (1 - c));
}

void FGAIBallistic::setBnk(double r, double dt, double coeff) {
    double c = dt / (coeff + dt);
    roll = (r * c) + (roll * (1 - c));
}

void FGAIBallistic::setSpd(double s, double dt, double coeff) {
    double c = dt / (coeff + dt);
    _speed = (s * c) + (_speed * (1 - c));
}

void FGAIBallistic::setHt(double h, double dt, double coeff) {
    double c = dt / (coeff + dt);
    _height = (h * c) + (_height * (1 - c));
}

int FGAIBallistic::setHdg(double tgt_hdg, double dt, double coeff) {
    double recip = getRecip(hdg);
    double c = dt / (coeff + dt);
    //cout << "set heading " << tgt_hdg << endl;
    //we need to ensure that we turn the short way to the new hdg
    if (tgt_hdg < recip && tgt_hdg < hdg && hdg > 180) {
        hdg = ((tgt_hdg + 360) * c) + (hdg * (1 - c));
//        cout << "case 1: right turn" << endl;
    } else if (tgt_hdg > recip && tgt_hdg > hdg && hdg <= 180){
        hdg = ((tgt_hdg - 360) * c) + (hdg * (1 - c));
//        cout << "case 2: left turn" << endl;
    } else {
        hdg = (tgt_hdg * c) + (hdg * (1 - c));
//        cout << "case 4: left turn" << endl;
    }
    return -1;
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

void FGAIBallistic::setTgtXOffset(double x) {
    _tgt_x_offset = x;
}

void FGAIBallistic::setTgtYOffset(double y) {
    _tgt_y_offset = y;
}

void FGAIBallistic::setTgtZOffset(double z) {
    _tgt_z_offset = z;
}

void FGAIBallistic::slaveToAC(double dt) {
    if (invisible)
        return;

    double hdg, pch, rll;//, agl = 0;

    if (_pnode != 0) {
        setParentPos();
        hdg = _p_hdg_node->getDoubleValue();
        pch = _p_pch_node->getDoubleValue();
        rll = _p_rll_node->getDoubleValue();
//        agl = _p_agl_node->getDoubleValue();
        setOffsetPos(_parentpos, hdg, pch, rll);
        setSpeed(_p_spd_node->getDoubleValue());
    }
    else {
        hdg = manager->get_user_heading();
        pch = manager->get_user_pitch();
        rll = manager->get_user_roll();
//        agl = manager->get_user_agl();
        setOffsetPos(globals->get_aircraft_position(), hdg, pch, rll);
        setSpeed(manager->get_user_speed());
    }

    pos.setLatitudeDeg(_offsetpos.getLatitudeDeg());
    pos.setLongitudeDeg(_offsetpos.getLongitudeDeg());
    pos.setElevationFt(_offsetpos.getElevationFt());
    setHeading(hdg);
    setPitch(pch + _pitch_offset);
    setBank(rll + _roll_offset);
    setOffsetVelocity(dt, pos);
    setTime(0);

    //update the mass (slugs)
    _mass = (_weight_lb + getContents()) / slugs_to_lbs;

    _impact_reported = false;

    //cout << _name << " _mass "<<_mass <<" " << getContents() 
    //<< " " << getContents() / slugs_to_lbs << " weight " << _weight_lb << endl;
    //    cout << _name << " update hs " << hs << " vs " << vs << endl;
}

void FGAIBallistic::Run(double dt) {
    _life_timer += dt;
    
    //_pass += 1;
    //cout<<"AIBallistic run: name " << _name.c_str() 
    //    << " dt " << dt <<  " _life_timer " << _life_timer << " pass " << _pass << endl;

    // if life = -1 the object does not die
    if (_life_timer > life && life != -1) {
        if (_report_expiry && !_expiry_reported && !_impact_reported && !_collision_reported) {
            //cout<<"AIBallistic run: name " << _name.c_str() << " expiry " 
                //<< " _life_timer " << _life_timer<< endl;
            handle_expiry();
        }
        else {
            //cout<<"AIBallistic run: name " << _name.c_str() 
            //    << " die " <<  " _life_timer " << _life_timer << endl;
            setDie(true);
        }

        setTime(0);
    }

    // Set the contents in the appropriate tank or other property in the parent to zero
    setContents(0);

    if (_random) {
        // Keep the new Cd within +- 10% of the current Cd to avoid a fluctuating value
        double cd_min = _cd * 0.9;
        double cd_max = _cd * 1.1;

        // Randomize Cd by +- a certain percentage of the initial Cd
        _cd = _init_cd * (1 - _cd_randomness + 2 * _cd_randomness * sg_random());

        if (_cd < cd_min) _cd = cd_min;
        if (_cd > cd_max) _cd = cd_max;
    }

    // Adjust Cd by Mach number. The equations are based on curves
    // for a conventional shell/bullet (no boat-tail).
    double Cdm;

    const double Mach = FGAtmo::machFromKnotsAtAltitudeFt(speed, altitude_ft);
    const double rhoKgM3 =  FGAtmo::densityAtAltitudeFt(altitude_ft);
    const double rho = rhoKgM3 / SG_SLUGFT3_TO_KGPM3;
    
    if (Mach < 0.7)
        Cdm = 0.0125 * Mach + _cd;
    else if (Mach < 1.2)
        Cdm = 0.3742 * pow(Mach, 2) - 0.252 * Mach + 0.0021 + _cd;
    else
        Cdm = 0.2965 * pow(Mach, -1.1506) + _cd;

    //cout <<_name << " Mach " << Mach << " Cdm " << Cdm 
    //    << " ballistic speed kts "<< speed <<  endl;

    // drag = Cd * 0.5 * rho * speed * speed * drag_area;
    // rho is adjusted for altitude in void FGAIBase::update,
    // using Standard Atmosphere (sealevel temperature 15C)
    // acceleration = drag/mass;
    // adjust speed by drag
    //
    // speed is in kt, rest is in ft and sec, so we convert speed^2 to fps (* SG_KT_TO_FPS^2)
    // then the result back to kt (/ SG_KT_TO_FPS), which is the same as just * SG_KT_TO_FPS.
    speed -= (Cdm * 0.5 * rho * speed * speed * _drag_area/_mass) * dt * SG_KT_TO_FPS;

    // don't let speed become negative
    if (speed < 0.0)
        speed = 0.0;

//    double speed_fps = speed * SG_KT_TO_FPS;

    // calculate vertical and horizontal speed components
    calcVSHS();

    //resolve horizontal speed into north and east components:
    //and convert horizontal speed (fps) to degrees per second
    calcNE();

    // If wind not required, set to zero
    if (!_wind) {
        _wind_from_north = 0;
        _wind_from_east = 0;
    }
    else {
        _wind_from_north = manager->get_wind_from_north();
        _wind_from_east = manager->get_wind_from_east();
    }

    // Calculate velocity due to external force
    double force_speed_north_deg_sec = 0.0;
    double force_speed_east_deg_sec = 0.0;
    double v_force_acc_fpss = 0.0;
    double force_speed_north_fps = 0.0;
    double force_speed_east_fps = 0.0;
    double h_force_lbs = 0.0;
    double normal_force_fpss = 0.0;
    double friction_force_speed_north_fps = 0.0;
    double friction_force_speed_east_fps = 0.0;
    double friction_force_speed_north_deg_sec = 0.0;
    double friction_force_speed_east_deg_sec = 0.0;
    double force_elevation_deg = 0.0;

    if (_external_force) {
        //cout << _name << " external force " <<  hdg << " az " << _azimuth << endl;

        SGPropertyNode *n = fgGetNode(_force_path.c_str(), true);
        double force_lbs = n->getChild("force-lb", 0, true)->getDoubleValue();
        force_elevation_deg  = n->getChild("force-elevation-deg", 0, true)->getDoubleValue();
        double force_azimuth_deg = n->getChild("force-azimuth-deg", 0, true)->getDoubleValue();
        
        // Resolve force into vertical and horizontal components:
        double v_force_lbs = force_lbs * sin( force_elevation_deg * SG_DEGREES_TO_RADIANS );
        h_force_lbs = force_lbs * cos( force_elevation_deg * SG_DEGREES_TO_RADIANS );

        double normal_force_lbs = 0.0;
        double dynamic_friction_force_lbs = 0.0;

        // Perform ground interaction if impacts are not calculated
        if (!_report_impact && getHtAGL(10000)) {
            double deadzone = 0.1;

            if (_ht_agl_ft <= (_ground_offset + deadzone) && _solid) {
                normal_force_lbs = (_mass * slugs_to_lbs) - v_force_lbs;

                if (normal_force_lbs < 0.0)
                    normal_force_lbs = 0.0;

                pos.setElevationFt(0 + _ground_offset);
                if (vs_fps < 0.0)
                    vs_fps *= -0.5;

                // Calculate friction. We assume a static coefficient of
                // friction (mu) of 0.62 (wood on concrete)
                double mu = 0.62;

                double static_friction_force_lbs = mu * normal_force_lbs * _frictionFactor;

                // Adjust horizontal force. We assume that a speed of <= 5 fps is static
                if (h_force_lbs <= static_friction_force_lbs && hs <= 5) {
                    h_force_lbs = hs = 0;
                    _speed_north_fps = _speed_east_fps = 0;
                }
                else
                    dynamic_friction_force_lbs = static_friction_force_lbs * 0.95;

                // Ignore wind when on the ground for now
                // TODO: fix this
                _wind_from_north = 0;
                _wind_from_east = 0;
            }
        }

        //acceleration = (force(lbsf)/mass(slugs))
        v_force_acc_fpss = v_force_lbs / _mass;
        normal_force_fpss = normal_force_lbs / _mass;
        double h_force_acc_fpss = h_force_lbs / _mass;
        double dynamic_friction_acc_fpss = dynamic_friction_force_lbs / _mass;

        // velocity = acceleration * dt
        double hs_force_fps = h_force_acc_fpss * dt;
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
    hs = sqrt(((_speed_north_fps + force_speed_north_fps + friction_force_speed_north_fps) 
        * (_speed_north_fps + force_speed_north_fps + friction_force_speed_north_fps))
        + ((_speed_east_fps + force_speed_east_fps + friction_force_speed_east_fps) 
        * (_speed_east_fps + force_speed_east_fps + friction_force_speed_east_fps)));

    if (hs <= 0.00001)
        hs = 0;

    // adjust vertical speed for acceleration of gravity, buoyancy, and vertical force
    double gravity = SG_METER_TO_FEET * (Environment::Gravity::instance()->getGravity(pos));
    vs_fps -= (gravity - _buoyancy - v_force_acc_fpss - normal_force_fpss) * dt;

    if (vs_fps <= 0.00001 && vs_fps >= -0.00001)
        vs_fps = 0;

    // set new position
    if (_slave_load_to_ac) {
        setOffsetPos(pos, 
            manager->get_user_heading(),
            manager->get_user_pitch(), 
            manager->get_user_roll());
        pos.setLatitudeDeg(_offsetpos.getLatitudeDeg());
        pos.setLongitudeDeg(_offsetpos.getLongitudeDeg());
        pos.setElevationFt(_offsetpos.getElevationFt());

        if (getHtAGL(10000.0)) {
            double deadzone = 0.1;

            if (_ht_agl_ft <= (0 + _ground_offset + deadzone) && _solid) {
                pos.setElevationFt(0 + _ground_offset);
            }
            else {
                pos.setElevationFt(_offsetpos.getElevationFt() + _load_offset);
            }
        }
    }
    else {
        pos.setLatitudeDeg( pos.getLatitudeDeg()
            + (speed_north_deg_sec - wind_speed_from_north_deg_sec 
            + force_speed_north_deg_sec + friction_force_speed_north_deg_sec) * dt );
        pos.setLongitudeDeg( pos.getLongitudeDeg()
            + (speed_east_deg_sec - wind_speed_from_east_deg_sec 
            + force_speed_east_deg_sec + friction_force_speed_east_deg_sec) * dt );
        pos.setElevationFt(pos.getElevationFt() + vs_fps * dt);
    }

//    cout << _name << " run hs " << hs << " vs " << vs << endl;

    // recalculate total speed
    if ( vs_fps == 0 && hs == 0)
        speed = 0;
    else
        speed = sqrt( vs_fps * vs_fps + hs * hs) / SG_KT_TO_FPS;

    // recalculate elevation and azimuth (velocity vectors)
    _elevation = atan2( vs_fps, hs ) * SG_RADIANS_TO_DEGREES;
    _azimuth =  atan2((_speed_east_fps + force_speed_east_fps + friction_force_speed_east_fps), 
        (_speed_north_fps + force_speed_north_fps + friction_force_speed_north_fps))
        * SG_RADIANS_TO_DEGREES;

    // rationalise azimuth
    if (_azimuth < 0)
        _azimuth += 360;

    if (_aero_stabilised) { // we simulate rotational moment of inertia by using a filter
        //cout<< "_aero_stabilised " << hdg << " az " << _azimuth << endl;
        const double coeff = 0.9;

        // we assume a symmetrical MI about the pitch and yaw axis
        setPch(_elevation, dt, coeff);
        setHdg(_azimuth, dt, coeff);
    }
    else if (_force_stabilised) { // we simulate rotational moment of inertia by using a filter
        //cout<< "_force_stabilised "<< endl;
        
        const double coeff = 0.9;
        double ratio = h_force_lbs/(_mass * slugs_to_lbs);

        if (ratio >  1) ratio =  1;
        if (ratio < -1) ratio = -1;

        double force_pitch = acos(ratio) * SG_RADIANS_TO_DEGREES;

        if (force_pitch <= force_elevation_deg)
            force_pitch = force_elevation_deg;

        // we assume a symmetrical MI about the pitch and yaw axis
        setPch(force_pitch,dt, coeff);
        setHdg(_azimuth, dt, coeff);
    }

    // Do impacts and collisions
    if (_report_impact && !_impact_reported)
        handle_impact();

    if (_report_collision && !_collision_reported)
        handle_collision();

    // Set destruction flag if altitude less than sea level -1000
    if (altitude_ft < -1000.0 && life != -1)
        setDie(true);
}

double FGAIBallistic::_getTime() const {
    return _life_timer;
}

void FGAIBallistic::setTime(double s) {
    _life_timer = s;
}

void FGAIBallistic::handleEndOfLife(double elevation) {
    report_impact(elevation);

    // Make the submodel invisible if the submodel is immortal, otherwise kill it if it has no subsubmodels
    if (life == -1) {
        invisible = true;
    }
    else if (_subID == 0) {
        // Kill the AIObject if there is no subsubmodel
        setDie(true);
    }
}

void FGAIBallistic::handle_impact() {
    // Try terrain intersection
    double start = pos.getElevationM() + 100;

    if (!getHtAGL(start))
        return;

    if (_ht_agl_ft <= 0) {
        SG_LOG(SG_AI, SG_DEBUG, "AIBallistic: terrain impact material" << _mat_name);
        _impact_reported = true;
        handleEndOfLife(_elevation_m);
    } 
}

void FGAIBallistic::handle_expiry() {
    _expiry_reported = true;
    handleEndOfLife(pos.getElevationM());
}

void FGAIBallistic::handle_collision()
{
    const FGAIBase *object = manager->calcCollision(pos.getElevationFt(),
        pos.getLatitudeDeg(),pos.getLongitudeDeg(), _fuse_range);

    if (object) {
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
        n->setStringValue("type", static_cast<std::string>(object->getTypeString()));
    else
        n->setStringValue("type", "terrain");

    SG_LOG(SG_AI, SG_DEBUG, "AIBallistic: object impact " << _name 
        << " lon " <<_impact_lon << " lat " <<_impact_lat << " sec " << _life_timer);

    n->setDoubleValue("longitude-deg", _impact_lon);
    n->setDoubleValue("latitude-deg", _impact_lat);
    n->setDoubleValue("elevation-m", _impact_elev);
    n->setDoubleValue("heading-deg", _impact_hdg);
    n->setDoubleValue("pitch-deg", _impact_pitch);
    n->setDoubleValue("roll-deg", _impact_roll);
    n->setDoubleValue("speed-mps", _impact_speed);

    _impact_report_node->setStringValue(props->getPath());
}

SGVec3d FGAIBallistic::getCartHitchPos() const {
    // convert geodetic positions to geocentered
    SGVec3d cartuserPos = globals->get_aircraft_position_cart();
    
    //SGVec3d cartPos = getCartPos();

    // Transform to the right coordinate frame, configuration is done in
    // the x-forward, y-right, z-up coordinates (feet), computation
    // in the simulation usual body x-forward, y-right, z-down coordinates
    // (meters) )
    SGVec3d _off(_x_offset * SG_FEET_TO_METER,
            _y_offset * SG_FEET_TO_METER,
            -_z_offset * SG_FEET_TO_METER);

    // Transform the user position to the horizontal local coordinate system.
    SGQuatd hlTrans = SGQuatd::fromLonLat(globals->get_aircraft_position());

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

void FGAIBallistic::setOffsetPos(SGGeod inpos, double heading, double pitch, double roll) {
    // Convert the hitch geocentered position to geodetic
    SGVec3d cartoffsetPos = getCartOffsetPos(inpos, heading, pitch, roll);
    SGGeodesy::SGCartToGeod(cartoffsetPos, _offsetpos);
}

double FGAIBallistic::getDistanceToHitch() const {
    //calculate the distance load to hitch 
    SGVec3d carthitchPos = getCartHitchPos();
    SGVec3d cartPos = getCartPos();

    SGVec3d diff = carthitchPos - cartPos;
    double distance = norm(diff);
    return distance * SG_METER_TO_FEET;
}

double FGAIBallistic::getElevToHitch() const {
    // now the angle, positive angles are upwards
    double distance = getDistanceToHitch() * SG_FEET_TO_METER;
    double angle = 0;
    double daltM = _offsetpos.getElevationM() - pos.getElevationM();

    if (fabs(distance) < SGLimits<float>::min()) {
        angle = 0;
    } else {
        double sAngle = daltM/distance;
        sAngle = SGMiscd::min(1, SGMiscd::max(-1, sAngle));
        angle = SGMiscd::rad2deg(asin(sAngle));
    }

    return angle;
}

double FGAIBallistic::getBearingToHitch() const {
    //calculate the bearing and range of the second pos from the first
    double distance = getDistanceToHitch() * SG_FEET_TO_METER;
    double az1, az2;

    geo_inverse_wgs_84(pos, _offsetpos, &az1, &az2, &distance);

    return az1;
}

double FGAIBallistic::getRelBrgHitchToUser() const {
    //calculate the relative bearing 
    double az1, az2, distance;

    geo_inverse_wgs_84(_offsetpos, globals->get_aircraft_position(), &az1, &az2, &distance);

    double rel_brg = az1 - hdg;

    SG_NORMALIZE_RANGE(rel_brg, -180.0, 180.0);

    return rel_brg;
}

double FGAIBallistic::getElevHitchToUser() const {
    // Calculate the distance from the user position
    SGVec3d carthitchPos = getCartHitchPos();
    SGVec3d cartuserPos = globals->get_aircraft_position_cart();

    SGVec3d diff = cartuserPos - carthitchPos;

    double distance = norm(diff);
    double angle = 0;

    double daltM = globals->get_aircraft_position().getElevationM() - _offsetpos.getElevationM();

    // Now the angle, positive angles are upwards
    if (fabs(distance) < SGLimits<float>::min()) {
        angle = 0;
    }
    else {
        double sAngle = daltM/distance;
        sAngle = SGMiscd::min(1, SGMiscd::max(-1, sAngle));
        angle = SGMiscd::rad2deg(asin(sAngle));
    }

    return angle;
}

void FGAIBallistic::setTgtOffsets(double dt, double coeff) {
    double c = dt / (coeff + dt);

    _x_offset = (_tgt_x_offset * c) + (_x_offset * (1 - c));
    _y_offset = (_tgt_y_offset * c) + (_y_offset * (1 - c));
    _z_offset = (_tgt_z_offset * c) + (_z_offset * (1 - c));
}

void FGAIBallistic::calcVSHS() {
    // Calculate vertical and horizontal speed components
    double speed_fps = speed * SG_KT_TO_FPS;

    if (speed == 0.0) {
        hs = vs_fps = 0.0;
    }
    else {
        vs_fps = sin( _elevation * SG_DEGREES_TO_RADIANS ) * speed_fps;
        hs = cos( _elevation * SG_DEGREES_TO_RADIANS ) * speed_fps;
    }
}

void FGAIBallistic::calcNE() {
    // Resolve horizontal speed into north and east components:
    _speed_north_fps = cos(_azimuth / SG_RADIANS_TO_DEGREES) * hs;
    _speed_east_fps = sin(_azimuth / SG_RADIANS_TO_DEGREES) * hs;

    // Convert horizontal speed (fps) to degrees per second
    speed_north_deg_sec = _speed_north_fps / ft_per_deg_lat;
    speed_east_deg_sec  = _speed_east_fps / ft_per_deg_lon;
}

SGVec3d FGAIBallistic::getCartOffsetPos(SGGeod inpos, double user_heading, 
                                        double user_pitch, double user_roll
                                        ) const {
    // Convert geodetic positions to geocentered
    SGVec3d cartuserPos = SGVec3d::fromGeod(inpos);

    // Transform to the right coordinate frame, configuration is done in
    // the x-forward, y-right, z-up coordinates (feet), computation
    // in the simulation usual body x-forward, y-right, z-down coordinates
    // (meters) )
    SGVec3d _off(_x_offset * SG_FEET_TO_METER,
            _y_offset * SG_FEET_TO_METER,
            -_z_offset * SG_FEET_TO_METER);

    // Transform the user position to the horizontal local coordinate system.
    SGQuatd hlTrans = SGQuatd::fromLonLat(inpos);

    // And postrotate the orientation of the user model wrt the horizontal
    // local frame
    hlTrans *= SGQuatd::fromYawPitchRollDeg(
        user_heading,
        user_pitch,
        user_roll);

    // The offset converted to the usual body fixed coordinate system
    // rotated to the earth-fixed coordinates axis
    SGVec3d off = hlTrans.backTransform(_off);

    // Add the position offset of the user model to get the geocentered position
    SGVec3d offsetPos = cartuserPos + off;

    return offsetPos;
}

void FGAIBallistic::setOffsetVelocity(double dt, SGGeod offsetpos) {
    // Calculate the distance from the previous offset position
    SGVec3d cartoffsetPos = SGVec3d::fromGeod(offsetpos);
    SGVec3d diff = cartoffsetPos - _oldcartoffsetPos;

    double distance = norm(diff);
    // Calculate speed knots
    speed = (distance / dt) * SG_MPS_TO_KT;

    // Now calulate the angle between the old and current postion positions (degrees)
    double angle = 0;
    double daltM = offsetpos.getElevationM() - _oldoffsetpos.getElevationM();

    if (fabs(distance) < SGLimits<float>::min()) {
        angle = 0;
    }
    else {
        double sAngle = daltM / distance;
        sAngle = SGMiscd::min(1, SGMiscd::max(-1, sAngle));
        angle = SGMiscd::rad2deg(asin(sAngle));
    }

    _elevation = angle;

    // Calculate vertical and horizontal speed components
    calcVSHS();

    // Calculate the bearing of the new offset position from the old
    // Don't do this if speed is low
    //cout << "speed " << speed << endl;
    if (speed > 0.1) {
        double az1, az2, dist;
        geo_inverse_wgs_84(_oldoffsetpos, offsetpos, &az1, &az2, &dist);
        _azimuth = az1;
        //cout << "offset az " << _azimuth << endl;
    }
    else {
        _azimuth = hdg;
        //cout << " slow offset az " << _azimuth << endl;
    }

    // Resolve horizontal speed into north and east components
    calcNE();

    // And finally store the new values
    _oldcartoffsetPos = cartoffsetPos;
    _oldoffsetpos = offsetpos;
}
