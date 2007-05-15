// FGAIBallistic - FGAIBase-derived class creates a ballistic object
//
// Written by David Culp, started November 2003.
// - davidculp2@comcast.net
//
// With major additions by Mathias Froehlich & Vivian Meazza 2004-2007
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
#include <simgear/scene/material/mat.hxx>
#include <math.h>
#include <vector>

#include <Scenery/scenery.hxx>

#include "AIBallistic.hxx"

SG_USING_STD(vector);

const double FGAIBallistic::slugs_to_kgs = 14.5939029372;

FGAIBallistic::FGAIBallistic() :
    FGAIBase(otBallistic),
    _aero_stabilised(false),
    _drag_area(0.007),
    _life_timer(0.0),
    _gravity(32),
    //  _buoyancy(64),
    _ht_agl_ft(0),
    _load_resistance(0),
    _solid(false),
    _impact_data(false),
    _impact_energy(0),
    _impact_speed(0),
    _impact_lat(0),
    _impact_lon(0),
    _impact_elev(0),
    _mat_name("")
{
    no_roll = false;
}

FGAIBallistic::~FGAIBallistic() {
}

void FGAIBallistic::readFromScenario(SGPropertyNode* scFileNode) {
    if (!scFileNode)
        return;

    FGAIBase::readFromScenario(scFileNode);

    setAzimuth(scFileNode->getDoubleValue("azimuth", 0.0));
    setElevation(scFileNode->getDoubleValue("elevation", 0.0));
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
    setName(scFileNode->getStringValue("name", "Bomb"));
}

bool FGAIBallistic::init(bool search_in_AI_path) {
    FGAIBase::init(search_in_AI_path);

    props->setStringValue("material/name", _mat_name.c_str());
    props->setStringValue("name", _name.c_str());

    // start with high value so that animations don't trigger yet
    _ht_agl_ft = 10000000;
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
    props->tie("impact/latitude-deg",
                SGRawValuePointer<double>(&_impact_lat));
    props->tie("impact/longitude-deg",
                SGRawValuePointer<double>(&_impact_lon));
    props->tie("impact/elevation-m",
                SGRawValuePointer<double>(&_impact_elev));
    props->tie("impact/speed-mps",
                SGRawValuePointer<double>(&_impact_speed));
    props->tie("impact/energy-kJ",
                SGRawValuePointer<double>(&_impact_energy));
}

void FGAIBallistic::unbind() {
    //    FGAIBase::unbind();
    props->untie("sim/time/elapsed-sec");
    props->untie("material/load-resistance");
    props->untie("material/solid");
    props->untie("altitude-agl-ft");
    props->untie("impact/latitude-deg");
    props->untie("impact/longitude-deg");
    props->untie("impact/elevation-m");
    props->untie("impact/speed-mps");
    props->untie("impact/energy-kJ");
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
    _impact = i;
}

void FGAIBallistic::setName(const string& n) {
    _name = n;
}

void FGAIBallistic::Run(double dt) {
    _life_timer += dt;
    //    cout << "life timer 1" << _life_timer <<  dt << endl;
    if (_life_timer > life)
        setDie(true);

    double speed_north_deg_sec;
    double speed_east_deg_sec;
    double wind_speed_from_north_deg_sec;
    double wind_speed_from_east_deg_sec;
    double Cdm;      // Cd adjusted by Mach Number

    //randomise Cd by +- 5%
    if (_random)
        _Cd = _Cd * 0.95 + (0.05 * sg_random());

    // Adjust Cd by Mach number. The equations are based on curves
    // for a conventional shell/bullet (no boat-tail).
    if (Mach >= 1.2)
        Cdm = 0.2965 * pow ( Mach, -1.1506 ) + _Cd;
    else if (Mach >= 0.7)
        Cdm = 0.3742 * pow ( Mach, 2) - 0.252 * Mach + 0.0021 + _Cd;
    else
        Cdm = 0.0125 * Mach + _Cd;

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

    // calculate vertical and horizontal speed components
    vs = sin( pitch * SG_DEGREES_TO_RADIANS ) * speed_fps;
    double hs = cos( pitch * SG_DEGREES_TO_RADIANS ) * speed_fps;

    // convert horizontal speed (fps) to degrees per second
    speed_north_deg_sec = cos(hdg / SG_RADIANS_TO_DEGREES) * hs / ft_per_deg_lat;
    speed_east_deg_sec  = sin(hdg / SG_RADIANS_TO_DEGREES) * hs / ft_per_deg_lon;

    // if wind not required, set to zero
    if (!_wind) {
        _wind_from_north = 0;
        _wind_from_east = 0;
    }

    // convert wind speed (fps) to degrees per second
    wind_speed_from_north_deg_sec = _wind_from_north / ft_per_deg_lat;
    wind_speed_from_east_deg_sec  = _wind_from_east / ft_per_deg_lon;

    // set new position
    pos.setLatitudeDeg( pos.getLatitudeDeg()
        + (speed_north_deg_sec - wind_speed_from_north_deg_sec) * dt );
    pos.setLongitudeDeg( pos.getLongitudeDeg()
        + (speed_east_deg_sec - wind_speed_from_east_deg_sec) * dt );

    // adjust vertical speed for acceleration of gravity and buoyancy
    vs -= (_gravity - _buoyancy) * dt;

    // adjust altitude (feet)
    altitude_ft += vs * dt;
    pos.setElevationFt(altitude_ft);

    // recalculate pitch (velocity vector) if aerostabilized
    /*cout << _name << ": " << "aero_stabilised " << _aero_stabilised
    << " pitch " << pitch <<" vs "  << vs <<endl ;*/

    if (_aero_stabilised)
        pitch = atan2( vs, hs ) * SG_RADIANS_TO_DEGREES;

    // recalculate total speed
    speed = sqrt( vs * vs + hs * hs) / SG_KT_TO_FPS;

    if (_impact && !_impact_data && vs < 0)
        handle_impact();

    // set destruction flag if altitude less than sea level -1000
    if (altitude_ft < -1000.0)
        setDie(true);

}  // end Run

double FGAIBallistic::_getTime() const {
    //    cout << "life timer 2" << _life_timer << endl;
    return _life_timer;
}

void FGAIBallistic::handle_impact() {
    double elevation_m;
    const SGMaterial* material;

    // try terrain intersection
    if (!globals->get_scenery()->get_elevation_m(pos.getLatitudeDeg(), pos.getLongitudeDeg(),
            10000.0, elevation_m, &material))
        return;

    if (material) {
        const vector<string> names = material->get_names();

        if (!names.empty())
            _mat_name = names[0].c_str();

        _solid = material->get_solid();
        _load_resistance = material->get_load_resistance();
        props->setStringValue("material/name", _mat_name.c_str());
        //cout << "material " << _mat_name << " solid " << _solid << " load " << _load_resistance << endl;
    }

    _ht_agl_ft = pos.getElevationFt() - elevation_m * SG_METER_TO_FEET;

    // report impact by setting tied variables
    if (_ht_agl_ft <= 0) {
        _impact_lat = pos.getLatitudeDeg();
        _impact_lon = pos.getLongitudeDeg();
        _impact_elev = elevation_m;
        _impact_speed = speed * SG_KT_TO_MPS;
        _impact_energy = (_mass * slugs_to_kgs) * _impact_speed
                * _impact_speed / (2 * 1000);

        props->setBoolValue("impact/signal", true); // for listeners
        _impact_data = true;
    }
}

// end AIBallistic

