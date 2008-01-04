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

#include <simgear/math/point3d.hxx>
#include <simgear/math/sg_random.h>
#include <simgear/math/sg_geodesy.hxx>

#include <Scenery/scenery.hxx>

#include "AIBallistic.hxx"

const double FGAIBallistic::slugs_to_kgs = 14.5939029372;
const double FGAIBallistic::slugs_to_lbs = 32.1740485564;

FGAIBallistic::FGAIBallistic() :
    FGAIBase(otBallistic),
    _elevation(0),
    _aero_stabilised(false),
    _drag_area(0.007),
    _life_timer(0.0),
_gravity(32.1740485564),
    _buoyancy(0),
    _random(false),
    _ht_agl_ft(0),
    _load_resistance(0),
    _solid(false),
    _report_collision(false),
    _report_impact(false),
_wind(true),
    _impact_report_node(fgGetNode("/ai/models/model-impact", true)),
    _external_force(false)

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
    setMass(scFileNode->getDoubleValue("mass", 0.007));
    setStabilisation(scFileNode->getBoolValue("aero_stabilized", false));
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
    setForceStabilisation(scFileNode->getBoolValue("force_stabilized", false));
    setXOffset(scFileNode->getDoubleValue("x-offset", 0.0));
    setYOffset(scFileNode->getDoubleValue("y-offset", 0.0));
    setZOffset(scFileNode->getDoubleValue("z-offset", 0.0));
}

bool FGAIBallistic::init(bool search_in_AI_path) {
    FGAIBase::init(search_in_AI_path);

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
    props->tie("material/load-resistance",
                SGRawValuePointer<double>(&_load_resistance));
    props->tie("material/solid",
                SGRawValuePointer<bool>(&_solid));
    props->tie("altitude-agl-ft",
                SGRawValuePointer<double>(&_ht_agl_ft));
}

void FGAIBallistic::unbind() {
    //    FGAIBase::unbind();
    props->untie("sim/time/elapsed-sec");
    props->untie("material/load-resistance");
    props->untie("material/solid");
    props->untie("altitude-agl-ft");
}

void FGAIBallistic::update(double dt) {
    FGAIBase::update(dt);
    Run(dt);
    Transform();
}

void FGAIBallistic::setAzimuth(double az) {
    hdg = _azimuth = az;
}

void FGAIBallistic::setElevation(double el) {
    pitch = _elevation = el;
}

void FGAIBallistic::setRoll(double rl) {
    _rotation = rl;
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

void FGAIBallistic::setRandom(bool r) {
    _random = r;
}

void FGAIBallistic::setImpact(bool i) {
    _report_impact = i;
}

void FGAIBallistic::setCollision(bool c) {
    _report_collision = c;
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

void FGAIBallistic::setXOffset(double x) {
    _x_offset = x;
}

void FGAIBallistic::setYOffset(double y) {
    _y_offset = y;
}

void FGAIBallistic::setZOffset(double z) {
    _z_offset = z;
}

void FGAIBallistic::setSubID(int i) {
    _subID = i;
    //cout << "sub id " << _subID << " name " << _name << endl;
}

void FGAIBallistic::setSubmodel(const string& s) {
    _submodel = s;
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

    if (globals->get_scenery()->get_elevation_m(pos.getLatitudeDeg(), pos.getLongitudeDeg(),
        10000.0, _elevation_m, &_material)){
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

void FGAIBallistic::setPitch(double e, double dt, double coeff){
    double c = dt / (coeff + dt);
    pitch = (e * c) + (pitch * (1 - c));
}

void FGAIBallistic::setHdg(double dt, double coeff){
    double recip = getRecip(hdg);
    double c = dt / (coeff + dt);
    //we need to ensure that we turn the short way to the new hdg
    if (_azimuth < recip && _azimuth < hdg && hdg > 180) {
        hdg = ((_azimuth + 360) * c) + (hdg * (1 - c));
    } else if (_azimuth > recip && _azimuth > hdg && hdg <= 180){
        hdg = ((_azimuth - 360) * c) + (hdg * (1 - c));
    } else {
        hdg = (_azimuth * c) + (hdg * (1 - c));
    }
}

void FGAIBallistic::Run(double dt) {
    _life_timer += dt;

    if (_life_timer > life && life != -1)
        setDie(true);

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

    //cout << " Mach , " << Mach << " , Cdm , " << Cdm << " ballistic speed kts //"<< speed <<  endl;

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
    double hs;

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
    double vs_force_fps = 0;
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

            if ( _ht_agl_ft <= (0 + _z_offset + deadzone) && _solid){
                normal_force_lbs = (_mass * slugs_to_lbs) - v_force_lbs;
                pos.setElevationFt((_elevation_m * SG_METER_TO_FEET) + _z_offset);
                vs = 0;

                // calculate friction
                // we assume a static Coefficient of Friction (mu) of 0.62 (wood on concrete)
                double mu = 0.62;

                static_friction_force_lbs = mu * normal_force_lbs * _frictionFactor;

                //adjust horizontal force
                if (h_force_lbs <= static_friction_force_lbs && hs <= 0.1)
                    h_force_lbs = hs = 0;
                else 
                    dynamic_friction_force_lbs = (static_friction_force_lbs * 0.75);

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

        double friction_force_speed_north_fps = cos(getRecip(hdg) * SG_DEGREES_TO_RADIANS) * friction_force_fps;
        double friction_force_speed_east_fps  = sin(getRecip(hdg) * SG_DEGREES_TO_RADIANS) * friction_force_fps;

        // convert horizontal speed (fps) to degrees per second
        force_speed_north_deg_sec = force_speed_north_fps / ft_per_deg_lat;
        force_speed_east_deg_sec  = force_speed_east_fps / ft_per_deg_lon;

        friction_force_speed_north_deg_sec = friction_force_speed_north_fps / ft_per_deg_lat;
        friction_force_speed_east_deg_sec  = friction_force_speed_east_fps / ft_per_deg_lon;
    }

    // convert wind speed (fps) to degrees lat/lon per second
    double wind_speed_from_north_deg_sec = _wind_from_north / ft_per_deg_lat;
    double wind_speed_from_east_deg_sec  = _wind_from_east / ft_per_deg_lon;

    // set new position
    pos.setLatitudeDeg( pos.getLatitudeDeg()
        + (speed_north_deg_sec - wind_speed_from_north_deg_sec 
        + force_speed_north_deg_sec + friction_force_speed_north_deg_sec) * dt );
    pos.setLongitudeDeg( pos.getLongitudeDeg()
        + (speed_east_deg_sec - wind_speed_from_east_deg_sec 
        + force_speed_east_deg_sec + friction_force_speed_east_deg_sec) * dt );

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

    // adjust altitude (feet) and set new elevation
    altitude_ft = pos.getElevationFt();
    altitude_ft += vs * dt;
    pos.setElevationFt(altitude_ft);

    // recalculate total speed
    if ( vs == 0 && hs == 0)
        speed = 0;
    else
        speed = sqrt( vs * vs + hs * hs) / SG_KT_TO_FPS;

    // recalculate elevation and azimuth (velocity vectors)
    _elevation = atan2( vs, hs ) * SG_RADIANS_TO_DEGREES;
    _azimuth =  atan2((speed_east_fps + force_speed_east_fps), 
        (speed_north_fps + force_speed_north_fps)) * SG_RADIANS_TO_DEGREES;

    // rationalise azimuth
    if (_azimuth < 0)
        _azimuth += 360;

    if (_aero_stabilised) { // we simulate rotational moment of inertia by using a filter
        const double coeff = 0.9;

        // we assume a symetrical MI about the pitch and yaw axis
        setPitch(_elevation, dt, coeff);
        setHdg(dt, coeff);

    } else if (_force_stabilised) { // we simulate rotational moment of inertia by using a filter
        const double coeff = 0.9;
        double ratio = h_force_lbs/(_mass * slugs_to_lbs);

        double force_pitch = acos(ratio) * SG_RADIANS_TO_DEGREES;

        if (force_pitch <= force_elevation_deg)
            force_pitch = force_elevation_deg;

        // we assume a symetrical MI about the pitch and yaw axis
        setPitch(force_pitch,dt, coeff);
        setHdg(dt, coeff);
    }

    //do impacts and collisions
    if (_report_impact && !_impact_reported)
        handle_impact();

    if (_report_collision && !_collision_reported)
        handle_collision();

    // set destruction flag if altitude less than sea level -1000
    if (altitude_ft < -1000.0)
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

        // kill the AIObject if there is no subsubmodel
        if (_subID == 0)
            setDie(true);
    }
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

// end AIBallistic

