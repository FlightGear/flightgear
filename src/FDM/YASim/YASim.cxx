#include <stdio.h>

#include <simgear/misc/sg_path.hxx>
#include <simgear/debug/logstream.hxx>
#include <simgear/easyxml.hxx>
#include <Main/globals.hxx>
#include <Main/fg_props.hxx>
#include <Scenery/scenery.hxx>

#include "FGFDM.hpp"
#include "Atmosphere.hpp"
#include "Math.hpp"
#include "Airplane.hpp"
#include "Model.hpp"
#include "Integrator.hpp"
#include "Glue.hpp"
#include "Gear.hpp"
#include "PropEngine.hpp"
#include "PistonEngine.hpp"

#include "YASim.hxx"

using namespace yasim;

static const float RAD2DEG = 180/3.14159265358979323846;
static const float RAD2RPM = 9.54929658551;
static const float M2FT = 3.2808399;
static const float FT2M = 0.3048;
static const float MPS2KTS = 3600.0/1852.0;
static const float CM2GALS = 264.172037284; // gallons/cubic meter
static const float KG2LBS = 2.20462262185;
static const float W2HP = 1.3416e-3;

void YASim::printDEBUG()
{
    static int debugCount = 0;
    
    debugCount++;
    if(debugCount >= 3) {
	debugCount = 0;

// 	printf("RPM %.1f FF %.1f\n",
// 	       fgGetFloat("/engines/engine[0]/rpm"),
// 	       fgGetFloat("/engines/engine[0]/fuel-flow-gph"));

// 	printf("gear: %f\n", fgGetFloat("/controls/gear-down"));

//    	printf("alpha %5.1f beta %5.1f\n", get_Alpha()*57.3, get_Beta()*57.3);

// 	printf("pilot: %f %f %f\n",
// 	       fgGetDouble("/sim/view/pilot/x-offset-m"),
// 	       fgGetDouble("/sim/view/pilot/y-offset-m"),
// 	       fgGetDouble("/sim/view/pilot/z-offset-m"));
    }
}

YASim::YASim(double dt)
{
    set_delta_t(dt);
    _fdm = new FGFDM();

    // Because the integration method is via fourth-order Runge-Kutta,
    // we only get an "output" state for every 4 times the internal
    // forces are calculated.  So divide dt by four to account for
    // this, and only run an iteration every fourth time through
    // update.
    _dt = dt * 4;
    _fdm->getAirplane()->getModel()->getIntegrator()->setInterval(_dt);
    _updateCount = 0;
}

void YASim::report()
{
    Airplane* a = _fdm->getAirplane();

    float aoa = a->getCruiseAoA() * RAD2DEG;
    float tail = -1 * a->getTailIncidence() * RAD2DEG;
    float drag = 1000 * a->getDragCoefficient();

    SG_LOG(SG_FLIGHT,SG_INFO,"YASim solution results:");
    SG_LOG(SG_FLIGHT,SG_INFO,"      Iterations: "<<a->getSolutionIterations());
    SG_LOG(SG_FLIGHT,SG_INFO,"Drag Coefficient: "<< drag);
    SG_LOG(SG_FLIGHT,SG_INFO,"      Lift Ratio: "<<a->getLiftRatio());
    SG_LOG(SG_FLIGHT,SG_INFO,"      Cruise AoA: "<< aoa);
    SG_LOG(SG_FLIGHT,SG_INFO,"  Tail Incidence: "<< tail);

    float cg[3];
    char buf[256];
    a->getModel()->getBody()->getCG(cg);
    sprintf(buf, "            CG: %.1f, %.1f, %.1f", cg[0], cg[1], cg[2]);
    SG_LOG(SG_FLIGHT, SG_INFO, buf);

    if(a->getFailureMsg()) {
        SG_LOG(SG_FLIGHT, SG_ALERT, "YASim SOLUTION FAILURE:");
        SG_LOG(SG_FLIGHT, SG_ALERT, a->getFailureMsg());
        exit(1);
    }
}

void YASim::init()
{
    Airplane* a = _fdm->getAirplane();
    Model* m = a->getModel();

    // Superclass hook
    common_init();

    m->setCrashed(false);

    // Build a filename and parse it
    SGPath f(globals->get_fg_root());
    f.append("Aircraft-yasim");
    f.append(fgGetString("/sim/aero"));
    f.concat(".xml");
    readXML(f.str(), *_fdm);

    // Compile it into a real airplane, and tell the user what they got
    a->compile();
    report();

    _fdm->init();

    // Create some FG{Eng|Gear}Interface objects
    int i;
    for(i=0; i<a->numGear(); i++) {
        Gear* g = a->getGear(i);
        FGGearInterface fgg;
        float pos[3];
        g->getPosition(pos);
        fgg.SetX(pos[0]); fgg.SetY(-pos[1]); fgg.SetZ(-pos[2]);
        add_gear_unit(fgg);
    }
    for(i=0; i<m->numThrusters(); i++) {
        FGEngInterface fge;
        add_engine(fge);

	// Sanify the initial input conditions
	char buf[64];
 	sprintf(buf, "/controls/throttle[%d]", i);        fgSetFloat(buf, 0);
	sprintf(buf, "/controls/mixture[%d]", i);         fgSetFloat(buf, 1);
	sprintf(buf, "/controls/propeller-pitch[%d]", i); fgSetFloat(buf, 1);
	sprintf(buf, "/controls/afterburner[%d]", i);     fgSetFloat(buf, 0);
    }
    

    // Are we at ground level?  If so, lift the plane up so the gear
    // clear the ground
    if(get_Altitude() - get_Runway_altitude() < 50) {
	float minGearZ = 1e18;
	for(i=0; i<a->numGear(); i++) {
	    Gear* g = a->getGear(i);
	    float pos[3];
	    g->getPosition(pos);
	    if(pos[2] < minGearZ)
		minGearZ = pos[2];
	}
	_set_Altitude(get_Runway_altitude() - minGearZ*M2FT);
    }

    // The pilot's eyepoint
    float pilot[3];
    a->getPilotPos(pilot);
    fgSetFloat("/sim/view/pilot/x-offset-m", -pilot[0]);
    fgSetFloat("/sim/view/pilot/y-offset-m", -pilot[1]);
    fgSetFloat("/sim/view/pilot/z-offset-m", pilot[2]);

    // Blank the state, and copy in ours
    State s;
    m->setState(&s);

    copyToYASim(true);
    set_inited(true);
}

bool YASim::update(int iterations)
{
    // If we're crashed, then we don't care
    if(_fdm->getAirplane()->getModel()->isCrashed())
	return true;

    int i;
    for(i=0; i<iterations; i++) {
        // Remember, update only every 4th call
        _updateCount++;
        if(_updateCount >= 4) {

	    copyToYASim(false);
	    _fdm->iterate(_dt);
	    copyFromYASim();

	    printDEBUG();

            _updateCount = 0;
        }
    }

    return true; // what does this mean?
}

void YASim::copyToYASim(bool copyState)
{
    // Physical state
    float lat = get_Latitude();
    float lon = get_Longitude();
    float alt = get_Altitude() * FT2M;
    float roll = get_Phi();
    float pitch = get_Theta();
    float hdg = get_Psi();

    // Environment
    float wind[3];
    wind[0] = get_V_north_airmass() * FT2M;
    wind[1] = get_V_east_airmass() * FT2M;
    wind[2] = get_V_down_airmass() * FT2M;
    double ground = get_Runway_altitude() * FT2M;

    // You'd this this would work, but it doesn't.  These values are
    // always zero.  Use a "standard" pressure intstead.
    //
    // float pressure = get_Static_pressure();
    // float temp = get_Static_temperature();
    float pressure = Atmosphere::getStdPressure(alt);
    float temp = Atmosphere::getStdTemperature(alt);

    // Convert and set:
    Model* model = _fdm->getAirplane()->getModel();
    State s;
    float xyz2ned[9];
    Glue::xyz2nedMat(lat, lon, xyz2ned);

    // position
    Glue::geod2xyz(lat, lon, alt, s.pos);

    // orientation
    Glue::euler2orient(roll, pitch, hdg, s.orient);
    Math::mmul33(s.orient, xyz2ned, s.orient);

    // Copy in the existing velocity for now...
    Math::set3(model->getState()->v, s.v);

    if(copyState)
	model->setState(&s);

    // wind
    Math::tmul33(xyz2ned, wind, wind);
    model->setWind(wind);

    // ground.  Calculate a cartesian coordinate for the ground under
    // us, find the (geodetic) up vector normal to the ground, then
    // use that to find the final (radius) term of the plane equation.
    double xyz[3], gplane[3]; float up[3];
    Glue::geod2xyz(lat, lon, ground, xyz);
    Glue::geodUp(xyz, up); // FIXME, needless reverse computation...
    int i;
    for(i=0; i<3; i++) gplane[i] = up[i];
    double rad = gplane[0]*xyz[0] + gplane[1]*xyz[1] + gplane[2]*xyz[2];
    model->setGroundPlane(gplane, rad);

    // air
    model->setAir(pressure, temp);
}

// All the settables:
//
// These are set below:
// _set_Accels_Local
// _set_Accels_Body
// _set_Accels_CG_Body 
// _set_Accels_Pilot_Body
// _set_Accels_CG_Body_N 
// _set_Velocities_Local
// _set_Velocities_Ground
// _set_Velocities_Wind_Body
// _set_Omega_Body
// _set_Euler_Rates
// _set_Euler_Angles
// _set_V_rel_wind
// _set_V_ground_speed
// _set_V_equiv_kts
// _set_V_calibrated_kts
// _set_Alpha
// _set_Beta
// _set_Mach_number
// _set_Climb_Rate
// _set_Tank1Fuel
// _set_Tank2Fuel
// _set_Altitude_AGL
// _set_Geodetic_Position
// _set_Runway_altitude

// Ignoring these, because they're unused:
// _set_Geocentric_Position
// _set_Geocentric_Rates
// _set_Cos_phi
// _set_Cos_theta
// _set_Earth_position_angle (WTF?)
// _set_Gamma_vert_rad
// _set_Inertias
// _set_T_Local_to_Body
// _set_CG_Position
// _set_Sea_Level_Radius

// Externally set via the weather code:
// _set_Velocities_Local_Airmass
// _set_Density
// _set_Static_pressure
// _set_Static_temperature
void YASim::copyFromYASim()
{
    Airplane* airplane = _fdm->getAirplane();
    Model* model = airplane->getModel();
    State* s = model->getState();

    // position
    double lat, lon, alt;
    Glue::xyz2geod(s->pos, &lat, &lon, &alt);
    _set_Geodetic_Position(lat, lon, alt*M2FT);

    // UNUSED
    //_set_Geocentric_Position(Glue::geod2geocLat(lat), lon, alt*M2FT);

    _set_Altitude_AGL(model->getAGL() * M2FT);

    // useful conversion matrix
    float xyz2ned[9];
    Glue::xyz2nedMat(lat, lon, xyz2ned);

    // velocity
    float v[3];
    Math::vmul33(xyz2ned, s->v, v);
    _set_Velocities_Local(M2FT*v[0], M2FT*v[1], M2FT*v[2]);
    _set_V_ground_speed(Math::sqrt(M2FT*v[0]*M2FT*v[0] +
				   M2FT*v[1]*M2FT*v[1]));
    _set_Climb_Rate(-M2FT*v[2]);

    // The HUD uses this, but inverts down (?!)
    _set_Velocities_Ground(M2FT*v[0], M2FT*v[1], -M2FT*v[2]);

    // _set_Geocentric_Rates(M2FT*v[0], M2FT*v[1], M2FT*v[2]); // UNUSED

    Math::vmul33(s->orient, s->v, v);
    _set_Velocities_Wind_Body(v[0]*M2FT, -v[1]*M2FT, -v[2]*M2FT);
    _set_V_rel_wind(Math::mag3(v)*M2FT); // units?

    // These don't work, use a dummy based on altitude
    // float P = get_Static_pressure();
    // float T = get_Static_temperature();
    float P = Atmosphere::getStdPressure(alt);
    float T = Atmosphere::getStdTemperature(alt);
    _set_V_equiv_kts(Atmosphere::calcVEAS(v[0], P, T)*MPS2KTS);
    _set_V_calibrated_kts(Atmosphere::calcVCAS(v[0], P, T)*MPS2KTS);
    _set_Mach_number(Atmosphere::calcMach(v[0], T));

    // acceleration
    Math::vmul33(xyz2ned, s->acc, v);
    _set_Accels_Local(M2FT*v[0], M2FT*v[1], M2FT*v[2]);

    Math::vmul33(s->orient, s->acc, v);
    _set_Accels_Body(M2FT*v[0], -M2FT*v[1], -M2FT*v[2]);
    _set_Accels_CG_Body(M2FT*v[0], -M2FT*v[1], -M2FT*v[2]);

    _fdm->getAirplane()->getPilotAccel(v);
    _set_Accels_Pilot_Body(M2FT*v[0], -M2FT*v[1], -M2FT*v[2]);

    // The one appears (!) to want inverted pilot acceleration
    // numbers, in G's...
    Math::mul3(1.0/9.8, v, v);
    _set_Accels_CG_Body_N(v[0], -v[1], -v[2]);

    // orientation
    float alpha, beta;
    Glue::calcAlphaBeta(s, &alpha, &beta);
    _set_Alpha(alpha);
    _set_Beta(beta);

    float tmp[9];
    Math::trans33(xyz2ned, tmp);
    Math::mmul33(s->orient, tmp, tmp);
    float roll, pitch, hdg;
    Glue::orient2euler(tmp, &roll, &pitch, &hdg);
    _set_Euler_Angles(roll, pitch, hdg);

    // rotation
    Math::vmul33(s->orient, s->rot, v);
    _set_Omega_Body(v[0], -v[1], -v[2]);

    Glue::calcEulerRates(s, &roll, &pitch, &hdg);
    _set_Euler_Rates(roll, pitch, hdg);

    // Fill out our engine and gear objects
    int i;
    for(i=0; i<get_num_gear(); i++) {
        FGGearInterface* fgg = get_gear_unit(i);
        Gear* g = airplane->getGear(i);
        if(g->getBrake() != 0)
            fgg->SetBrake(true);
        if(g->getCompressFraction() != 0)
            fgg->SetWoW(true);
	else
	    fgg->SetWoW(false);
        fgg->SetPosition(g->getExtension());
    }

    for(i=0; i<get_num_engines(); i++) {
        FGEngInterface* fge = get_engine(i);
        Thruster* t = model->getThruster(i);

        fge->set_Running_Flag(true);
        fge->set_Cranking_Flag(false);

        // Note: assumes all tanks have the same fuel density!
        fge->set_Fuel_Flow(CM2GALS * t->getFuelFlow()
                           / airplane->getFuelDensity(0));

        float tmp[3];
        t->getThrust(tmp);
        fge->set_prop_thrust(Math::mag3(tmp) * KG2LBS / 9.8);

        PropEngine* pe = t->getPropEngine();
        if(pe) {
            fge->set_RPM(pe->getOmega() * RAD2RPM);

            pe->getTorque(tmp);
            float power = Math::mag3(tmp) * pe->getOmega();
            float maxPower = pe->getPistonEngine()->getPower();

            fge->set_MaxHP(maxPower * W2HP);
            fge->set_Percentage_Power(100 * power/maxPower);
        }
    }
}
