#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>

#include <Main/fg_props.hxx>

#include "Math.hpp"
#include "Jet.hpp"
#include "SimpleJet.hpp"
#include "Gear.hpp"
#include "Hook.hpp"
#include "Launchbar.hpp"
#include "Atmosphere.hpp"
#include "PropEngine.hpp"
#include "Propeller.hpp"
#include "PistonEngine.hpp"
#include "TurbineEngine.hpp"
#include "Rotor.hpp"
#include "Rotorpart.hpp"
#include "Hitch.hpp"

#include "FGFDM.hpp"

namespace yasim {

// Some conversion factors
static const float KTS2MPS = 0.514444444444;
static const float FT2M = 0.3048;
static const float DEG2RAD = 0.0174532925199;
static const float RPM2RAD = 0.10471975512;
static const float LBS2N = 4.44822;
static const float LBS2KG = 0.45359237;
static const float KG2LBS = 2.2046225;
static const float CM2GALS = 264.172037284;
static const float HP2W = 745.700;
static const float INHG2PA = 3386.389;
static const float K2DEGF = 1.8;
static const float K2DEGFOFFSET = -459.4;
static const float CIN2CM = 1.6387064e-5;
static const float YASIM_PI = 3.14159265358979323846;

static const float NM2FTLB = (1/(LBS2N*FT2M));

// Stubs, so that this can be compiled without the FlightGear
// binary.  What's the best way to handle this?

//     float fgGetFloat(char* name, float def) { return 0; }
//     void fgSetFloat(char* name, float val) {}

FGFDM::FGFDM()
{
    _vehicle_radius = 0.0f;

    _nextEngine = 0;

    // Map /controls/flight/elevator to the approach elevator control.  This
    // should probably be settable, but there are very few aircraft
    // who trim their approaches using things other than elevator.
    _airplane.setElevatorControl(parseAxis("/controls/flight/elevator-trim"));

    // FIXME: read seed from somewhere?
    int seed = 0;
    _turb = new Turbulence(10, seed);
}

FGFDM::~FGFDM()
{
    for(int i=0; i<_axes.size(); i++) {
        AxisRec* a = (AxisRec*)_axes.get(i);
        delete[] a->name;
        delete a;
    }

    for(int i=0; i<_thrusters.size(); i++) {
        EngRec* er = (EngRec*)_thrusters.get(i);
        delete[] er->prefix;
        delete er->eng;
        delete er;
    }

    for(int i=0; i<_weights.size(); i++) {
        WeightRec* wr = (WeightRec*)_weights.get(i);
        delete[] wr->prop;
        delete wr;
    }

    for(int i=0; i<_controlProps.size(); i++)
        delete (PropOut*)_controlProps.get(i);

    delete _turb;
}

void FGFDM::iterate(float dt)
{
    getExternalInput(dt);
    _airplane.iterate(dt);

    // Do fuel stuff
    for(int i=0; i<_airplane.numThrusters(); i++) {
        Thruster* t = _airplane.getThruster(i);

        bool out_of_fuel = _fuel_props[i]._out_of_fuel->getBoolValue();
        t->setFuelState(!out_of_fuel);

        double consumed = _fuel_props[i]._fuel_consumed_lbs->getDoubleValue();
        _fuel_props[i]._fuel_consumed_lbs->setDoubleValue(
                consumed + dt * KG2LBS * t->getFuelFlow());
    }
    for(int i=0; i<_airplane.numTanks(); i++) {
        _airplane.setFuel(i, LBS2KG * _tank_level_lbs[i]->getFloatValue());
    }
    _airplane.calcFuelWeights();
    
    setOutputProperties(dt);
}

Airplane* FGFDM::getAirplane()
{
    return &_airplane;
}

void FGFDM::init()
{
    _turb_magnitude_norm = fgGetNode("/environment/turbulence/magnitude-norm", true);
    _turb_rate_hz        = fgGetNode("/environment/turbulence/rate-hz", true);
    _gross_weight_lbs    = fgGetNode("/yasim/gross-weight-lbs", true);

    // Allows the user to start with something other than full fuel
    _airplane.setFuelFraction(fgGetFloat("/sim/fuel-fraction", 1));

    // Read out the resulting fuel state and stash engine/thruster properties
    _thrust_props.clear();
    for (int i=0; i<_thrusters.size(); i++) {
        SGPropertyNode_ptr node = fgGetNode("engines/engine", i, true);
        Thruster* t = ((EngRec*)_thrusters.get(i))->eng;

        ThrusterProps tp;
        tp._running =       node->getChild("running", 0, true);
        tp._cranking =      node->getChild("cranking", 0, true);
        tp._prop_thrust =   node->getChild("prop-thrust", 0, true); // Deprecated name
        tp._thrust_lbs =    node->getChild("thrust-lbs", 0, true);
        tp._fuel_flow_gph = node->getChild("fuel-flow-gph", 0, true);

        if(t->getPropEngine())
        {
            tp._rpm = node->getChild("rpm", 0, true);
            tp._torque_ftlb = node->getChild("torque-ftlb", 0, true);

            PropEngine* p = t->getPropEngine();
            if(p->getEngine()->isPistonEngine())
            {
                tp._mp_osi =   node->getChild("mp-osi",   0, true);
                tp._mp_inhg =  node->getChild("mp-inhg",  0, true);
                tp._egt_degf = node->getChild("egt-degf", 0, true);

                tp._oil_temperature_degf = node->getChild("oil-temperature-degf", 0, true);
                tp._boost_gauge_inhg =     node->getChild("boost-gauge-inhg", 0, true);
            } else if(p->getEngine()->isTurbineEngine()) {
                tp._n2 = node->getChild("n2", 0, true);
            }
        }

        if(t->getJet())
        {
            tp._n1 =       node->getChild("n1",       0, true);
            tp._n2 =       node->getChild("n2",       0, true);
            tp._epr =      node->getChild("epr",      0, true);
            tp._egt_degf = node->getChild("egt-degf", 0, true);
        }
        _thrust_props.push_back(tp);
    }

    // stash properties for engines/fuel state
    _thrust_props.clear();
    for(int i=0; i<_airplane.numThrusters(); i++) {
        SGPropertyNode_ptr e = fgGetNode("engines/engine", i, true);
        FuelProps f;
        f._out_of_fuel       = e->getChild("out-of-fuel", 0, true);
        f._fuel_consumed_lbs = e->getChild("fuel-consumed-lbs", 0, true);
        _fuel_props.push_back(f);
    }

    // initialize tanks and stash properties for tank level
    _tank_level_lbs.clear();
    for(int i=0; i<_airplane.numTanks(); i++) {
        char buf[256];
        sprintf(buf, "/consumables/fuel/tank[%d]/level-lbs", i);
        fgSetDouble(buf, _airplane.getFuel(i) * KG2LBS);
        _tank_level_lbs.push_back(fgGetNode(buf, true));

        double density = _airplane.getFuelDensity(i);
        sprintf(buf, "/consumables/fuel/tank[%d]/density-ppg", i);
        fgSetDouble(buf, density * (KG2LBS/CM2GALS));

// set in TankProperties class
//        sprintf(buf, "/consumables/fuel/tank[%d]/level-gal_us", i);
//        fgSetDouble(buf, _airplane.getFuel(i) * CM2GALS / density);

        sprintf(buf, "/consumables/fuel/tank[%d]/capacity-gal_us", i);
        fgSetDouble(buf, CM2GALS * _airplane.getTankCapacity(i)/density);
    }

    // This has a nasty habit of being false at startup.  That's not
    // good.
    fgSetBool("/controls/gear/gear-down", true);

    _airplane.getModel()->setTurbulence(_turb);
}

// Not the worlds safest parser.  But it's short & sweet.
void FGFDM::startElement(const char* name, const XMLAttributes &atts)
{
    XMLAttributes* a = (XMLAttributes*)&atts;
    float v[3];
    char buf[64];

    if(eq(name, "airplane")) {
	_airplane.setWeight(attrf(a, "mass") * LBS2KG);
    } else if(eq(name, "approach")) {
	float spd = attrf(a, "speed") * KTS2MPS;
	float alt = attrf(a, "alt", 0) * FT2M;
	float aoa = attrf(a, "aoa", 0) * DEG2RAD;
        float gla = attrf(a, "glide-angle", 0) * DEG2RAD;
	_airplane.setApproach(spd, alt, aoa, attrf(a, "fuel", 0.2),gla);
	_cruiseCurr = false;
    } else if(eq(name, "cruise")) {
	float spd = attrf(a, "speed") * KTS2MPS;
	float alt = attrf(a, "alt") * FT2M;
        float gla = attrf(a, "glide-angle", 0) * DEG2RAD;
	_airplane.setCruise(spd, alt, attrf(a, "fuel", 0.5),gla);
	_cruiseCurr = true;
    } else if(eq(name, "solve-weight")) {
        int idx = attri(a, "idx");
        float wgt = attrf(a, "weight") * LBS2KG;
        _airplane.addSolutionWeight(!_cruiseCurr, idx, wgt);
    } else if(eq(name, "cockpit")) {
	v[0] = attrf(a, "x");
	v[1] = attrf(a, "y");
	v[2] = attrf(a, "z");
	_airplane.setPilotPos(v);
    } else if(eq(name, "rotor")) {
        _airplane.getModel()->getRotorgear()->addRotor(parseRotor(a, name));
    } else if(eq(name, "rotorgear")) {
        Rotorgear* r = _airplane.getModel()->getRotorgear();
	_currObj = r;
        #define p(x) if (a->hasAttribute(#x)) r->setParameter((char *)#x,attrf(a,#x) );
        #define p2(x,y) if (a->hasAttribute(y)) r->setParameter((char *)#x,attrf(a,y) );
        p2(max_power_engine,"max-power-engine")
        p2(engine_prop_factor,"engine-prop-factor")
        p(yasimdragfactor)
        p(yasimliftfactor)
        p2(max_power_rotor_brake,"max-power-rotor-brake")
        p2(rotorgear_friction,"rotorgear-friction")
        p2(engine_accel_limit,"engine-accel-limit")
        #undef p
        #undef p2
        r->setInUse();
    } else if(eq(name, "wing")) {
	_airplane.setWing(parseWing(a, name));
    } else if(eq(name, "hstab")) {
	_airplane.setTail(parseWing(a, name));
    } else if(eq(name, "vstab") || eq(name, "mstab")) {
	_airplane.addVStab(parseWing(a, name));
    } else if(eq(name, "piston-engine")) {
        parsePistonEngine(a);
    } else if(eq(name, "turbine-engine")) {
        parseTurbineEngine(a);
    } else if(eq(name, "propeller")) {
	parsePropeller(a);
    } else if(eq(name, "thruster")) {
	SimpleJet* j = new SimpleJet();
	_currObj = j;
	v[0] = attrf(a, "x"); v[1] = attrf(a, "y"); v[2] = attrf(a, "z");
	j->setPosition(v);
	_airplane.addThruster(j, 0, v);
	v[0] = attrf(a, "vx"); v[1] = attrf(a, "vy"); v[2] = attrf(a, "vz");
	j->setDirection(v);
	j->setThrust(attrf(a, "thrust") * LBS2N);
    } else if(eq(name, "jet")) {
	Jet* j = new Jet();
	_currObj = j;
	v[0] = attrf(a, "x");
	v[1] = attrf(a, "y");
	v[2] = attrf(a, "z");
	float mass = attrf(a, "mass") * LBS2KG;
	j->setMaxThrust(attrf(a, "thrust") * LBS2N,
			attrf(a, "afterburner", 0) * LBS2N);
	j->setVectorAngle(attrf(a, "rotate", 0) * DEG2RAD);
        j->setReverseThrust(attrf(a, "reverse", 0.2));

 	float n1min = attrf(a, "n1-idle", 55);
	float n1max = attrf(a, "n1-max", 102);
 	float n2min = attrf(a, "n2-idle", 73);
	float n2max = attrf(a, "n2-max", 103);
	j->setRPMs(n1min, n1max, n2min, n2max);

	j->setTSFC(attrf(a, "tsfc", 0.8));
	if(a->hasAttribute("egt"))  j->setEGT(attrf(a, "egt"));
	if(a->hasAttribute("epr"))  j->setEPR(attrf(a, "epr"));
	if(a->hasAttribute("exhaust-speed"))
	    j->setVMax(attrf(a, "exhaust-speed") * KTS2MPS);
	if(a->hasAttribute("spool-time"))
	    j->setSpooling(attrf(a, "spool-time"));
	
	j->setPosition(v);
	_airplane.addThruster(j, mass, v);
	sprintf(buf, "/engines/engine[%d]", _nextEngine++);
	EngRec* er = new EngRec();
	er->eng = j;
	er->prefix = dup(buf);
	_thrusters.add(er);
    } else if(eq(name, "hitch")) {
        Hitch* h = new Hitch(a->getValue("name"));
        _currObj = h;
        v[0] = attrf(a, "x");
        v[1] = attrf(a, "y");
        v[2] = attrf(a, "z");
        h->setPosition(v);
        if(a->hasAttribute("force-is-calculated-by-other")) h->setForceIsCalculatedByOther(attrb(a,"force-is-calculated-by-other"));
        _airplane.addHitch(h);
    } else if(eq(name, "tow")) {
        Hitch* h = (Hitch*)_currObj;
        if(a->hasAttribute("length"))
            h->setTowLength(attrf(a, "length"));
        if(a->hasAttribute("elastic-constant"))
            h->setTowElasticConstant(attrf(a, "elastic-constant"));
        if(a->hasAttribute("break-force"))
            h->setTowBreakForce(attrf(a, "break-force"));
        if(a->hasAttribute("weight-per-meter"))
            h->setTowWeightPerM(attrf(a, "weight-per-meter"));
        if(a->hasAttribute("mp-auto-connect-period"))
            h->setMpAutoConnectPeriod(attrf(a, "mp-auto-connect-period"));
    } else if(eq(name, "winch")) {
        Hitch* h = (Hitch*)_currObj;
        double pos[3];
        pos[0] = attrd(a, "x",0);
        pos[1] = attrd(a, "y",0);
        pos[2] = attrd(a, "z",0);
        h->setWinchPosition(pos);
        if(a->hasAttribute("max-speed"))
            h->setWinchMaxSpeed(attrf(a, "max-speed"));
        if(a->hasAttribute("power"))
            h->setWinchPower(attrf(a, "power") * 1000);
        if(a->hasAttribute("max-force"))
            h->setWinchMaxForce(attrf(a, "max-force"));
        if(a->hasAttribute("initial-tow-length"))
            h->setWinchInitialTowLength(attrf(a, "initial-tow-length"));
        if(a->hasAttribute("max-tow-length"))
            h->setWinchMaxTowLength(attrf(a, "max-tow-length"));
        if(a->hasAttribute("min-tow-length"))
            h->setWinchMinTowLength(attrf(a, "min-tow-length"));
    } else if(eq(name, "gear")) {
	Gear* g = new Gear();
	_currObj = g;
	v[0] = attrf(a, "x");
	v[1] = attrf(a, "y");
	v[2] = attrf(a, "z");
	g->setPosition(v);
        float nrm = Math::mag3(v);
        if (_vehicle_radius < nrm)
            _vehicle_radius = nrm;
        if(a->hasAttribute("upx")) {
            v[0] = attrf(a, "upx");
            v[1] = attrf(a, "upy");
            v[2] = attrf(a, "upz");
            Math::unit3(v, v);
        } else {
            v[0] = 0;
            v[1] = 0;
            v[2] = 1;
        }
        for(int i=0; i<3; i++)
            v[i] *= attrf(a, "compression", 1);
	g->setCompression(v);
        g->setBrake(attrf(a, "skid", 0));
        g->setInitialLoad(attrf(a, "initial-load", 0));
	g->setStaticFriction(attrf(a, "sfric", 0.8));
	g->setDynamicFriction(attrf(a, "dfric", 0.7));
        g->setSpring(attrf(a, "spring", 1));
        g->setDamping(attrf(a, "damp", 1));
        if(a->hasAttribute("on-water")) g->setOnWater(attrb(a,"on-water"));
        if(a->hasAttribute("on-solid")) g->setOnSolid(attrb(a,"on-solid"));
        if(a->hasAttribute("ignored-by-solver")) g->setIgnoreWhileSolving(attrb(a,"ignored-by-solver"));
        g->setSpringFactorNotPlaning(attrf(a, "spring-factor-not-planing", 1));
        g->setSpeedPlaning(attrf(a, "speed-planing", 0) * KTS2MPS);
        g->setReduceFrictionByExtension(attrf(a, "reduce-friction-by-extension", 0));
	_airplane.addGear(g);
    } else if(eq(name, "hook")) {
	Hook* h = new Hook();
	_currObj = h;
	v[0] = attrf(a, "x");
	v[1] = attrf(a, "y");
	v[2] = attrf(a, "z");
	h->setPosition(v);
        float length = attrf(a, "length", 1.0);
        h->setLength(length);
        float nrm = length+Math::mag3(v);
        if (_vehicle_radius < nrm)
            _vehicle_radius = nrm;
        h->setDownAngle(attrf(a, "down-angle", 70) * DEG2RAD);
        h->setUpAngle(attrf(a, "up-angle", 0) * DEG2RAD);
 	_airplane.addHook(h);
    } else if(eq(name, "launchbar")) {
	Launchbar* l = new Launchbar();
	_currObj = l;
	v[0] = attrf(a, "x");
	v[1] = attrf(a, "y");
	v[2] = attrf(a, "z");
	l->setLaunchbarMount(v);
	v[0] = attrf(a, "holdback-x", v[0]);
	v[1] = attrf(a, "holdback-y", v[1]);
	v[2] = attrf(a, "holdback-z", v[2]);
	l->setHoldbackMount(v);
        float length = attrf(a, "length", 1.0);
        l->setLength(length);
        l->setDownAngle(attrf(a, "down-angle", 45) * DEG2RAD);
        l->setUpAngle(attrf(a, "up-angle", -45) * DEG2RAD);
        l->setHoldbackLength(attrf(a, "holdback-length", 2.0));
 	_airplane.addLaunchbar(l);
    } else if(eq(name, "fuselage")) {
	float b[3];
	v[0] = attrf(a, "ax");
	v[1] = attrf(a, "ay");
	v[2] = attrf(a, "az");
	b[0] = attrf(a, "bx");
	b[1] = attrf(a, "by");
	b[2] = attrf(a, "bz");
        float taper = attrf(a, "taper", 1);
        float mid = attrf(a, "midpoint", 0.5);
        float cx = attrf(a, "cx", 1);
        float cy = attrf(a, "cy", 1);
        float cz = attrf(a, "cz", 1);
	float idrag = attrf(a, "idrag", 1);
	_airplane.addFuselage(v, b, attrf(a, "width"), taper, mid, 
            cx, cy, cz, idrag);
    } else if(eq(name, "tank")) {
	v[0] = attrf(a, "x");
	v[1] = attrf(a, "y");
	v[2] = attrf(a, "z");
	float density = 6.0; // gasoline, in lbs/gal
	if(a->hasAttribute("jet")) density = 6.72; 
	density *= LBS2KG*CM2GALS;
	_airplane.addTank(v, attrf(a, "capacity") * LBS2KG, density);
    } else if(eq(name, "ballast")) {
	v[0] = attrf(a, "x");
	v[1] = attrf(a, "y");
	v[2] = attrf(a, "z");
	_airplane.addBallast(v, attrf(a, "mass") * LBS2KG);
    } else if(eq(name, "weight")) {
	parseWeight(a);
    } else if(eq(name, "stall")) {
	Wing* w = (Wing*)_currObj;
	w->setStall(attrf(a, "aoa") * DEG2RAD);
	w->setStallWidth(attrf(a, "width", 2) * DEG2RAD);
	w->setStallPeak(attrf(a, "peak", 1.5));
    } else if(eq(name, "flap0")) {
	((Wing*)_currObj)->setFlap0(attrf(a, "start"), attrf(a, "end"),
				    attrf(a, "lift"), attrf(a, "drag"));
    } else if(eq(name, "flap1")) {
	((Wing*)_currObj)->setFlap1(attrf(a, "start"), attrf(a, "end"),
				    attrf(a, "lift"), attrf(a, "drag"));
    } else if(eq(name, "slat")) {
	((Wing*)_currObj)->setSlat(attrf(a, "start"), attrf(a, "end"),
				   attrf(a, "aoa"), attrf(a, "drag"));
    } else if(eq(name, "spoiler")) {
	((Wing*)_currObj)->setSpoiler(attrf(a, "start"), attrf(a, "end"),
				      attrf(a, "lift"), attrf(a, "drag"));
    /* } else if(eq(name, "collective")) {
        ((Rotor*)_currObj)->setcollective(attrf(a, "min"), attrf(a, "max"));
    } else if(eq(name, "cyclic")) {
        ((Rotor*)_currObj)->setcyclic(attrf(a, "ail"), attrf(a, "ele"));
    */                               
    } else if(eq(name, "actionpt")) {
 	v[0] = attrf(a, "x");
	v[1] = attrf(a, "y");
	v[2] = attrf(a, "z");
	((Thruster*)_currObj)->setPosition(v);
    } else if(eq(name, "dir")) {
 	v[0] = attrf(a, "x");
	v[1] = attrf(a, "y");
	v[2] = attrf(a, "z");
	((Thruster*)_currObj)->setDirection(v);
    } else if(eq(name, "control-setting")) {
	// A cruise or approach control setting
	const char* axis = a->getValue("axis");
	float value = attrf(a, "value", 0);
	if(_cruiseCurr)
	    _airplane.addCruiseControl(parseAxis(axis), value);
	else
	    _airplane.addApproachControl(parseAxis(axis), value);
    } else if(eq(name, "control-input")) {

	// A mapping of input property to a control
        int axis = parseAxis(a->getValue("axis"));
	int control = parseOutput(a->getValue("control"));
	int opt = 0;
	opt |= a->hasAttribute("split") ? ControlMap::OPT_SPLIT : 0;
	opt |= a->hasAttribute("invert") ? ControlMap::OPT_INVERT : 0;
	opt |= a->hasAttribute("square") ? ControlMap::OPT_SQUARE : 0;
	
	ControlMap* cm = _airplane.getControlMap();
	if(a->hasAttribute("src0")) {
                           cm->addMapping(axis, control, _currObj, opt,
			   attrf(a, "src0"), attrf(a, "src1"), 
			   attrf(a, "dst0"), attrf(a, "dst1"));
	} else {
            cm->addMapping(axis, control, _currObj, opt);
	}
    } else if(eq(name, "control-output")) {
        // A property output for a control on the current object
        ControlMap* cm = _airplane.getControlMap();
        int type = parseOutput(a->getValue("control"));
        int handle = cm->getOutputHandle(_currObj, type);

	PropOut* p = new PropOut();
	p->prop = fgGetNode(a->getValue("prop"), true);
	p->handle = handle;
	p->type = type;
	p->left = !(a->hasAttribute("side") &&
                        eq("right", a->getValue("side")));
	p->min = attrf(a, "min", cm->rangeMin(type));
	p->max = attrf(a, "max", cm->rangeMax(type));
	_controlProps.add(p);

    } else if(eq(name, "control-speed")) {
        ControlMap* cm = _airplane.getControlMap();
        int type = parseOutput(a->getValue("control"));
        int handle = cm->getOutputHandle(_currObj, type);
        float time = attrf(a, "transition-time", 0);
        
        cm->setTransitionTime(handle, time);
    } else {
        SG_LOG(SG_FLIGHT,SG_ALERT,"Unexpected tag '"
               << name << "' found in YASim aircraft description");
        exit(1);
    }
}

void FGFDM::getExternalInput(float dt)
{
    char buf[256];

    _turb->setMagnitude(_turb_magnitude_norm->getFloatValue());
    _turb->update(dt, _turb_rate_hz->getFloatValue());

    // The control axes
    ControlMap* cm = _airplane.getControlMap();
    cm->reset();

    for(int i=0; i<_axes.size(); i++) {
        AxisRec* a = (AxisRec*)_axes.get(i);
        float val = fgGetFloat(a->name, 0);
        cm->setInput(a->handle, val);
    }
    cm->applyControls(dt);

    // Weights
    for(int i=0; i<_weights.size(); i++) {
        WeightRec* wr = (WeightRec*)_weights.get(i);
        _airplane.setWeight(wr->handle, LBS2KG * fgGetFloat(wr->prop));
    }

    for(int i=0; i<_thrusters.size(); i++) {
        EngRec* er = (EngRec*)_thrusters.get(i);
        Thruster* t = er->eng;

        if(t->getPropEngine()) {
            PropEngine* p = t->getPropEngine();
            sprintf(buf, "%s/rpm", er->prefix);
            p->setOmega(fgGetFloat(buf, 500) * RPM2RAD);
        }
    }
}

// Linearly "seeks" a property by the specified fraction of the way to
// the target value.  Used to emulate "slowly changing" output values.
static void moveprop(SGPropertyNode* node, const char* prop,
                    float target, float frac)
{
    float val = node->getFloatValue(prop);
    if(frac > 1) frac = 1;
    if(frac < 0) frac = 0;
    val += (target - val) * frac;
    node->setFloatValue(prop, val);
}

void FGFDM::setOutputProperties(float dt)
{
    float grossWgt = _airplane.getModel()->getBody()->getTotalMass() * KG2LBS;
    _gross_weight_lbs->setFloatValue(grossWgt);

    ControlMap* cm = _airplane.getControlMap();
    for(int i=0; i<_controlProps.size(); i++) {
        PropOut* p = (PropOut*)_controlProps.get(i);
        float val = (p->left
                     ? cm->getOutput(p->handle)
                     : cm->getOutputR(p->handle));
        float rmin = cm->rangeMin(p->type);
        float rmax = cm->rangeMax(p->type);
        float frac = (val - rmin) / (rmax - rmin);
        val = frac*(p->max - p->min) + p->min;
        p->prop->setFloatValue(val);
    }

    for(int i=0; i<_airplane.getRotorgear()->getNumRotors(); i++) {
        Rotor*r=(Rotor*)_airplane.getRotorgear()->getRotor(i);
        int j = 0;
        float f;
        char b[256];
        while((j = r->getValueforFGSet(j, b, &f)))
            if(b[0]) fgSetFloat(b,f);
        j=0;
        while((j = _airplane.getRotorgear()->getValueforFGSet(j, b, &f)))
            if(b[0]) fgSetFloat(b,f);
        for(j=0; j < r->numRotorparts(); j+=r->numRotorparts()>>2) {
            Rotorpart* s = (Rotorpart*)r->getRotorpart(j);
            char *b;
            int k;
            for(k=0; k<2; k++) {
                b=s->getAlphaoutput(k);
                if(b[0]) fgSetFloat(b, s->getAlpha(k));
            }
        }
    }

    // Use the density of the first tank, or a dummy value if no tanks
    float fuelDensity = 1.0;
    if(_airplane.numTanks())
        fuelDensity = _airplane.getFuelDensity(0);
    for(int i=0; i<_thrusters.size(); i++) {
        EngRec* er = (EngRec*)_thrusters.get(i);
        Thruster* t = er->eng;
        SGPropertyNode * node = fgGetNode("engines/engine", i, true);

        ThrusterProps& tp = _thrust_props[i];

        // Set: running, cranking, prop-thrust, max-hp, power-pct
        tp._running->setBoolValue(t->isRunning());
        tp._cranking->setBoolValue(t->isCranking());

        float tmp[3];
        t->getThrust(tmp);
        float lbs = Math::mag3(tmp) * (KG2LBS/9.8);
        tp._prop_thrust->setFloatValue(lbs); // Deprecated name
        tp._thrust_lbs->setFloatValue(lbs);
        tp._fuel_flow_gph->setFloatValue(
                (t->getFuelFlow()/fuelDensity) * 3600 * CM2GALS);

        if(t->getPropEngine()) {
            PropEngine* p = t->getPropEngine();
            tp._rpm->setFloatValue(p->getOmega() * (1/RPM2RAD));
            tp._torque_ftlb->setFloatValue(
                    p->getEngine()->getTorque() * NM2FTLB);

            if(p->getEngine()->isPistonEngine()) {
                PistonEngine* pe = p->getEngine()->isPistonEngine();
                tp._mp_osi->setFloatValue(pe->getMP() * (1/INHG2PA));
                tp._mp_inhg->setFloatValue(pe->getMP() * (1/INHG2PA));
                tp._egt_degf->setFloatValue(
                        pe->getEGT() * K2DEGF + K2DEGFOFFSET);
                tp._oil_temperature_degf->setFloatValue(
                        pe->getOilTemp() * K2DEGF + K2DEGFOFFSET);
                tp._boost_gauge_inhg->setFloatValue(
                        pe->getBoost() * (1/INHG2PA));
            } else if(p->getEngine()->isTurbineEngine()) {
                TurbineEngine* te = p->getEngine()->isTurbineEngine();
                tp._n2->setFloatValue(te->getN2());
            }
        }

        if(t->getJet()) {
            Jet* j = t->getJet();
            tp._n1->setFloatValue(j->getN1());
            tp._n2->setFloatValue(j->getN2());
            tp._epr->setFloatValue(j->getEPR());
            tp._egt_degf->setFloatValue(
                    j->getEGT() * K2DEGF + K2DEGFOFFSET);

            // These are "unmodeled" values that are still needed for
            // many cockpits.  Tie them all to the N1 speed, but
            // normalize the numbers to the range [0:1] so the
            // cockpit code can scale them to the right values.
            float pnorm = j->getPerfNorm();
            moveprop(node, "oilp-norm", pnorm, dt/3); // 3s seek time
            moveprop(node, "oilt-norm", pnorm, dt/30); // 30s 
            moveprop(node, "itt-norm", pnorm, dt/1); // 1s
        }
    }
}

Wing* FGFDM::parseWing(XMLAttributes* a, const char* type)
{
    Wing* w = new Wing();

    float defDihed = 0;
    if(eq(type, "vstab"))
	defDihed = 90;
    else
	w->setMirror(true);

    float pos[3];
    pos[0] = attrf(a, "x");
    pos[1] = attrf(a, "y");
    pos[2] = attrf(a, "z");
    w->setBase(pos);

    w->setLength(attrf(a, "length"));
    w->setChord(attrf(a, "chord"));
    w->setSweep(attrf(a, "sweep", 0) * DEG2RAD);
    w->setTaper(attrf(a, "taper", 1));
    w->setDihedral(attrf(a, "dihedral", defDihed) * DEG2RAD);
    w->setCamber(attrf(a, "camber", 0));

    // These come in with positive indicating positive AoA, but the
    // internals expect a rotation about the left-pointing Y axis, so
    // invert the sign.
    w->setIncidence(attrf(a, "incidence", 0) * DEG2RAD * -1);
    w->setTwist(attrf(a, "twist", 0) * DEG2RAD * -1);

    // The 70% is a magic number that sorta kinda seems to match known
    // throttle settings to approach speed.
    w->setInducedDrag(0.7*attrf(a, "idrag", 1));

    float effect = attrf(a, "effectiveness", 1);
    w->setDragScale(w->getDragScale()*effect);

    _currObj = w;
    return w;
}

Rotor* FGFDM::parseRotor(XMLAttributes* a, const char* type)
{
    Rotor* w = new Rotor();

    // float defDihed = 0;

    float pos[3];
    pos[0] = attrf(a, "x");
    pos[1] = attrf(a, "y");
    pos[2] = attrf(a, "z");
    w->setBase(pos);

    float normal[3];
    normal[0] = attrf(a, "nx");
    normal[1] = attrf(a, "ny");
    normal[2] = attrf(a, "nz");
    w->setNormal(normal);

    float forward[3];
    forward[0] = attrf(a, "fx");
    forward[1] = attrf(a, "fy");
    forward[2] = attrf(a, "fz");
    w->setForward(forward);

    w->setMaxCyclicail(attrf(a, "maxcyclicail", 7.6));
    w->setMaxCyclicele(attrf(a, "maxcyclicele", 4.94));
    w->setMinCyclicail(attrf(a, "mincyclicail", -7.6));
    w->setMinCyclicele(attrf(a, "mincyclicele", -4.94));
    w->setMaxCollective(attrf(a, "maxcollective", 15.8));
    w->setMinCollective(attrf(a, "mincollective", -0.2));
    w->setDiameter(attrf(a, "diameter", 10.2));
    w->setWeightPerBlade(attrf(a, "weightperblade", 44));
    w->setNumberOfBlades(attrf(a, "numblades", 4));
    w->setRelBladeCenter(attrf(a, "relbladecenter", 0.7));
    w->setDynamic(attrf(a, "dynamic", 0.7));
    w->setDelta3(attrf(a, "delta3", 0));
    w->setDelta(attrf(a, "delta", 0));
    w->setTranslift(attrf(a, "translift", 0.05));
    w->setC2(attrf(a, "dragfactor", 1));
    w->setStepspersecond(attrf(a, "stepspersecond", 120));
    w->setPhiNull((attrf(a, "phi0", 0))*YASIM_PI/180);
    w->setRPM(attrf(a, "rpm", 424));
    w->setRelLenHinge(attrf(a, "rellenflaphinge", 0.07));
    w->setAlpha0((attrf(a, "flap0", -5))*YASIM_PI/180);
    w->setAlphamin((attrf(a, "flapmin", -15))/180*YASIM_PI);
    w->setAlphamax((attrf(a, "flapmax",  15))*YASIM_PI/180);
    w->setAlpha0factor(attrf(a, "flap0factor", 1));
    w->setTeeterdamp(attrf(a,"teeterdamp",.0001));
    w->setMaxteeterdamp(attrf(a,"maxteeterdamp",1000));
    w->setRelLenTeeterHinge(attrf(a,"rellenteeterhinge",0.01));
    w->setBalance(attrf(a,"balance",1.0));
    w->setMinTiltYaw(attrf(a,"mintiltyaw",0.0));
    w->setMinTiltPitch(attrf(a,"mintiltpitch",0.0));
    w->setMinTiltRoll(attrf(a,"mintiltroll",0.0));
    w->setMaxTiltYaw(attrf(a,"maxtiltyaw",0.0));
    w->setMaxTiltPitch(attrf(a,"maxtiltpitch",0.0));
    w->setMaxTiltRoll(attrf(a,"maxtiltroll",0.0));
    w->setTiltCenterX(attrf(a,"tiltcenterx",0.0));
    w->setTiltCenterY(attrf(a,"tiltcentery",0.0));
    w->setTiltCenterZ(attrf(a,"tiltcenterz",0.0));
    w->setDownwashFactor(attrf(a, "downwashfactor", 1));
    if(attrb(a,"ccw"))
       w->setCcw(1); 
    if(attrb(a,"sharedflaphinge"))
       w->setSharedFlapHinge(true); 

    if(a->hasAttribute("name"))
       w->setName(a->getValue("name") );
    if(a->hasAttribute("alphaout0"))
       w->setAlphaoutput(0,a->getValue("alphaout0") );
    if(a->hasAttribute("alphaout1"))  w->setAlphaoutput(1,a->getValue("alphaout1") );
    if(a->hasAttribute("alphaout2"))  w->setAlphaoutput(2,a->getValue("alphaout2") );
    if(a->hasAttribute("alphaout3"))  w->setAlphaoutput(3,a->getValue("alphaout3") );
    if(a->hasAttribute("coneout"))  w->setAlphaoutput(4,a->getValue("coneout") );
    if(a->hasAttribute("yawout"))   w->setAlphaoutput(5,a->getValue("yawout") );
    if(a->hasAttribute("rollout"))  w->setAlphaoutput(6,a->getValue("rollout") );

    w->setPitchA(attrf(a, "pitch-a", 10));
    w->setPitchB(attrf(a, "pitch-b", 10));
    w->setForceAtPitchA(attrf(a, "forceatpitch-a", 3000));
    w->setPowerAtPitch0(attrf(a, "poweratpitch-0", 300));
    w->setPowerAtPitchB(attrf(a, "poweratpitch-b", 3000));
    if(attrb(a,"notorque"))
       w->setNotorque(1); 

#define p(x) if (a->hasAttribute(#x)) w->setParameter((char *)#x,attrf(a,#x) );
#define p2(x,y) if (a->hasAttribute(y)) w->setParameter((char *)#x,attrf(a,y) );
    p2(translift_ve,"translift-ve")
    p2(translift_maxfactor,"translift-maxfactor")
    p2(ground_effect_constant,"ground-effect-constant")
    p2(vortex_state_lift_factor,"vortex-state-lift-factor")
    p2(vortex_state_c1,"vortex-state-c1")
    p2(vortex_state_c2,"vortex-state-c2")
    p2(vortex_state_c3,"vortex-state_c3")
    p2(vortex_state_e1,"vortex-state-e1")
    p2(vortex_state_e2,"vortex-state-e2")
    p(twist)
    p2(number_of_segments,"number-of-segments")
    p2(number_of_parts,"number-of-parts")
    p2(rel_len_where_incidence_is_measured,"rel-len-where-incidence-is-measured")
    p(chord)
    p(taper)
    p2(airfoil_incidence_no_lift,"airfoil-incidence-no-lift")
    p2(rel_len_blade_start,"rel-len-blade-start")
    p2(incidence_stall_zero_speed,"incidence-stall-zero-speed")
    p2(incidence_stall_half_sonic_speed,"incidence-stall-half-sonic-speed")
    p2(lift_factor_stall,"lift-factor-stall")
    p2(stall_change_over,"stall-change-over")
    p2(drag_factor_stall,"drag-factor-stall")
    p2(airfoil_lift_coefficient,"airfoil-lift-coefficient")
    p2(airfoil_drag_coefficient0,"airfoil-drag-coefficient0")
    p2(airfoil_drag_coefficient1,"airfoil-drag-coefficient1")
    p2(cyclic_factor,"cyclic-factor")
    p2(rotor_correction_factor,"rotor-correction-factor")
#undef p
#undef p2
    _currObj = w;
    return w;
}

void FGFDM::parsePistonEngine(XMLAttributes* a)
{
    float engP = attrf(a, "eng-power") * HP2W;
    float engS = attrf(a, "eng-rpm") * RPM2RAD;

    PistonEngine* eng = new PistonEngine(engP, engS);

    if(a->hasAttribute("displacement"))
        eng->setDisplacement(attrf(a, "displacement") * CIN2CM);

    if(a->hasAttribute("compression"))
        eng->setCompression(attrf(a, "compression"));        

    if(a->hasAttribute("min-throttle"))
        eng->setMinThrottle(attrf(a, "min-throttle"));

    if(a->hasAttribute("turbo-mul")) {
        float mul = attrf(a, "turbo-mul");
        float mp = attrf(a, "wastegate-mp", 1e6) * INHG2PA;
        eng->setTurboParams(mul, mp);
        eng->setTurboLag(attrf(a, "turbo-lag", 2));
    }

    if(a->hasAttribute("supercharger"))
        eng->setSupercharger(attrb(a, "supercharger"));

    ((PropEngine*)_currObj)->setEngine(eng);
}

void FGFDM::parseTurbineEngine(XMLAttributes* a)
{
    float power = attrf(a, "eng-power") * HP2W;
    float omega = attrf(a, "eng-rpm") * RPM2RAD;
    float alt = attrf(a, "alt") * FT2M;
    float flatRating = attrf(a, "flat-rating") * HP2W;
    TurbineEngine* eng = new TurbineEngine(power, omega, alt, flatRating);

    if(a->hasAttribute("n2-low-idle"))
        eng->setN2Range(attrf(a, "n2-low-idle"), attrf(a, "n2-high-idle"),
                        attrf(a, "n2-max"));

    // Nasty units conversion: lbs/hr per hp -> kg/s per watt
    if(a->hasAttribute("bsfc"))
        eng->setFuelConsumption(attrf(a, "bsfc") * (LBS2KG/(3600*HP2W)));

    ((PropEngine*)_currObj)->setEngine(eng);
}

void FGFDM::parsePropeller(XMLAttributes* a)
{
    // Legacy Handling for the old engines syntax:
    PistonEngine* eng = 0;
    if(a->hasAttribute("eng-power")) {
        SG_LOG(SG_FLIGHT,SG_ALERT, "WARNING: "
               << "Legacy engine definition in YASim configuration file.  "
               << "Please fix.");
        float engP = attrf(a, "eng-power") * HP2W;
        float engS = attrf(a, "eng-rpm") * RPM2RAD;
        eng = new PistonEngine(engP, engS);
        if(a->hasAttribute("displacement"))
            eng->setDisplacement(attrf(a, "displacement") * CIN2CM);
        if(a->hasAttribute("compression"))
            eng->setCompression(attrf(a, "compression"));        
        if(a->hasAttribute("turbo-mul")) {
            float mul = attrf(a, "turbo-mul");
            float mp = attrf(a, "wastegate-mp", 1e6) * INHG2PA;
            eng->setTurboParams(mul, mp);
        }
    }

    // Now parse the actual propeller definition:
    float cg[3];
    cg[0] = attrf(a, "x");
    cg[1] = attrf(a, "y");
    cg[2] = attrf(a, "z");
    float mass = attrf(a, "mass") * LBS2KG;
    float moment = attrf(a, "moment");
    float radius = attrf(a, "radius");
    float speed = attrf(a, "cruise-speed") * KTS2MPS;
    float omega = attrf(a, "cruise-rpm") * RPM2RAD;
    float power = attrf(a, "cruise-power") * HP2W;
    float rho = Atmosphere::getStdDensity(attrf(a, "cruise-alt") * FT2M);

    Propeller* prop = new Propeller(radius, speed, omega, rho, power);
    PropEngine* thruster = new PropEngine(prop, eng, moment);
    _airplane.addThruster(thruster, mass, cg);

    // Set the stops (fine = minimum pitch, coarse = maximum pitch)
    float fine_stop = attrf(a, "fine-stop", 0.25f);
    float coarse_stop = attrf(a, "coarse-stop", 4.0f);
    prop->setStops(fine_stop, coarse_stop);

    if(a->hasAttribute("takeoff-power")) {
	float power0 = attrf(a, "takeoff-power") * HP2W;
	float omega0 = attrf(a, "takeoff-rpm") * RPM2RAD;
	prop->setTakeoff(omega0, power0);
    }

    if(a->hasAttribute("max-rpm")) {
	float max = attrf(a, "max-rpm") * RPM2RAD;
	float min = attrf(a, "min-rpm") * RPM2RAD;
	thruster->setVariableProp(min, max);
    }

    if(attrb(a, "contra"))
        thruster->setContraPair(true);

    if(a->hasAttribute("manual-pitch")) {
	prop->setManualPitch();
    }

    thruster->setGearRatio(attrf(a, "gear-ratio", 1));

    char buf[64];
    sprintf(buf, "/engines/engine[%d]", _nextEngine++);
    EngRec* er = new EngRec();
    er->eng = thruster;
    er->prefix = dup(buf);
    _thrusters.add(er);

    _currObj = thruster;
}

// Turns a string axis name into an integer for use by the
// ControlMap.  Creates a new axis if this one hasn't been defined
// yet.
int FGFDM::parseAxis(const char* name)
{
    for(int i=0; i<_axes.size(); i++) {
        AxisRec* a = (AxisRec*)_axes.get(i);
        if(eq(a->name, name))
            return a->handle;
    }

    // Not there, make a new one.
    AxisRec* a = new AxisRec();
    a->name = dup(name);
    fgGetNode( a->name, true ); // make sure the property name exists
    a->handle = _airplane.getControlMap()->newInput();
    _axes.add(a);
    return a->handle;
}

int FGFDM::parseOutput(const char* name)
{
    if(eq(name, "THROTTLE"))  return ControlMap::THROTTLE;
    if(eq(name, "MIXTURE"))   return ControlMap::MIXTURE;
    if(eq(name, "CONDLEVER")) return ControlMap::CONDLEVER;
    if(eq(name, "STARTER"))   return ControlMap::STARTER;
    if(eq(name, "MAGNETOS"))  return ControlMap::MAGNETOS;
    if(eq(name, "ADVANCE"))   return ControlMap::ADVANCE;
    if(eq(name, "REHEAT"))    return ControlMap::REHEAT;
    if(eq(name, "BOOST"))     return ControlMap::BOOST;
    if(eq(name, "VECTOR"))    return ControlMap::VECTOR;
    if(eq(name, "PROP"))      return ControlMap::PROP;
    if(eq(name, "BRAKE"))     return ControlMap::BRAKE;
    if(eq(name, "STEER"))     return ControlMap::STEER;
    if(eq(name, "EXTEND"))    return ControlMap::EXTEND;
    if(eq(name, "HEXTEND"))   return ControlMap::HEXTEND;
    if(eq(name, "LEXTEND"))   return ControlMap::LEXTEND;
        if(eq(name, "LACCEL"))    return ControlMap::LACCEL;
    if(eq(name, "INCIDENCE")) return ControlMap::INCIDENCE;
    if(eq(name, "FLAP0"))     return ControlMap::FLAP0;
    if(eq(name, "FLAP0EFFECTIVENESS"))   return ControlMap::FLAP0EFFECTIVENESS;
    if(eq(name, "FLAP1"))     return ControlMap::FLAP1;
    if(eq(name, "FLAP1EFFECTIVENESS"))   return ControlMap::FLAP1EFFECTIVENESS;
    if(eq(name, "SLAT"))      return ControlMap::SLAT;
    if(eq(name, "SPOILER"))   return ControlMap::SPOILER;
    if(eq(name, "CASTERING")) return ControlMap::CASTERING;
    if(eq(name, "PROPPITCH")) return ControlMap::PROPPITCH;
    if(eq(name, "PROPFEATHER")) return ControlMap::PROPFEATHER;
    if(eq(name, "COLLECTIVE")) return ControlMap::COLLECTIVE;
    if(eq(name, "CYCLICAIL")) return ControlMap::CYCLICAIL;
    if(eq(name, "CYCLICELE")) return ControlMap::CYCLICELE;
    if(eq(name, "TILTROLL")) return ControlMap::TILTROLL;
    if(eq(name, "TILTPITCH")) return ControlMap::TILTPITCH;
    if(eq(name, "TILTYAW")) return ControlMap::TILTYAW;
    if(eq(name, "ROTORGEARENGINEON")) return ControlMap::ROTORENGINEON;
    if(eq(name, "ROTORBRAKE")) return ControlMap::ROTORBRAKE;
    if(eq(name, "ROTORENGINEMAXRELTORQUE")) 
        return ControlMap::ROTORENGINEMAXRELTORQUE;
    if(eq(name, "ROTORRELTARGET")) return ControlMap::ROTORRELTARGET;
    if(eq(name, "ROTORBALANCE")) return ControlMap::ROTORBALANCE;
    if(eq(name, "REVERSE_THRUST")) return ControlMap::REVERSE_THRUST;
    if(eq(name, "WASTEGATE")) return ControlMap::WASTEGATE;
    if(eq(name, "WINCHRELSPEED")) return ControlMap::WINCHRELSPEED;
    if(eq(name, "HITCHOPEN")) return ControlMap::HITCHOPEN;
    if(eq(name, "PLACEWINCH")) return ControlMap::PLACEWINCH;
    if(eq(name, "FINDAITOW")) return ControlMap::FINDAITOW;

    SG_LOG(SG_FLIGHT,SG_ALERT,"Unrecognized control type '"
           << name << "' in YASim aircraft description.");
    exit(1);

}

void FGFDM::parseWeight(XMLAttributes* a)
{
    WeightRec* wr = new WeightRec();

    float v[3];
    v[0] = attrf(a, "x");
    v[1] = attrf(a, "y");
    v[2] = attrf(a, "z");

    wr->prop = dup(a->getValue("mass-prop"));
    wr->size = attrf(a, "size", 0);
    wr->handle = _airplane.addWeight(v, wr->size);

    _weights.add(wr);
}

bool FGFDM::eq(const char* a, const char* b)
{
    // Figure it out for yourself. :)
    while(*a && *b && *a == *b) { a++; b++; }
    return !(*a || *b);
}

char* FGFDM::dup(const char* s)
{
    int len=0;
    while(s[len++]);
    char* s2 = new char[len+1];
    char* p = s2;
    while((*p++ = *s++));
    s2[len] = 0;
    return s2;
}

int FGFDM::attri(XMLAttributes* atts, const char* attr)
{
    if(!atts->hasAttribute(attr)) {
        SG_LOG(SG_FLIGHT,SG_ALERT,"Missing '" << attr <<
               "' in YASim aircraft description");
        exit(1);
    }
    return attri(atts, attr, 0);
}

int FGFDM::attri(XMLAttributes* atts, const char* attr, int def)
{
    const char* val = atts->getValue(attr);
    if(val == 0) return def;
    else         return atol(val);
}

float FGFDM::attrf(XMLAttributes* atts, const char* attr)
{
    if(!atts->hasAttribute(attr)) {
        SG_LOG(SG_FLIGHT,SG_ALERT,"Missing '" << attr <<
               "' in YASim aircraft description");
        exit(1);
    }
    return attrf(atts, attr, 0);
}

float FGFDM::attrf(XMLAttributes* atts, const char* attr, float def)
{
    const char* val = atts->getValue(attr);
    if(val == 0) return def;
    else         return (float)atof(val);    
}

double FGFDM::attrd(XMLAttributes* atts, const char* attr)
{
    if(!atts->hasAttribute(attr)) {
        SG_LOG(SG_FLIGHT,SG_ALERT,"Missing '" << attr <<
               "' in YASim aircraft description");
        exit(1);
    }
    return attrd(atts, attr, 0);
}

double FGFDM::attrd(XMLAttributes* atts, const char* attr, double def)
{
    const char* val = atts->getValue(attr);
    if(val == 0) return def;
    else         return atof(val);
}

// ACK: the dreaded ambiguous string boolean.  Remind me to shoot Maik
// when I have a chance. :).  Unless you have a parser that can check
// symbol constants (we don't), this kind of coding is just a Bad
// Idea.  This implementation, for example, silently returns a boolean
// falsehood for values of "1", "yes", "True", and "TRUE".  Which is
// especially annoying preexisting boolean attributes in the same
// parser want to see "1" and will choke on a "true"...
//
// Unfortunately, this usage creeped into existing configuration files
// while I wasn't active, and it's going to be hard to remove.  Issue
// a warning to nag people into changing their ways for now...
bool FGFDM::attrb(XMLAttributes* atts, const char* attr)
{
    const char* val = atts->getValue(attr);
    if(val == 0) return false;

    if(eq(val,"true")) {
        SG_LOG(SG_FLIGHT, SG_ALERT, "Warning: " <<
               "deprecated 'true' boolean in YASim configuration file.  " <<
               "Use numeric booleans (attribute=\"1\") instead");
        return true;
    }
    return attri(atts, attr, 0) ? true : false;
}

}; // namespace yasim
