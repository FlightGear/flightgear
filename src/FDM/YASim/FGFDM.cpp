#include <stdio.h>
#include <stdlib.h>

#include <Main/fg_props.hxx>

#include "Jet.hpp"
#include "SimpleJet.hpp"
#include "Gear.hpp"
#include "Atmosphere.hpp"
#include "PropEngine.hpp"
#include "Propeller.hpp"
#include "PistonEngine.hpp"

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
static const float CIN2CM = 1.6387064e-5;

// Stubs, so that this can be compiled without the FlightGear
// binary.  What's the best way to handle this?

//     float fgGetFloat(char* name, float def) { return 0; }
//     void fgSetFloat(char* name, float val) {}

FGFDM::FGFDM()
{
    _nextEngine = 0;

    // Map /controls/elevator to the approach elevator control.  This
    // should probably be settable, but there are very few aircraft
    // who trim their approaches using things other than elevator.
    _airplane.setElevatorControl(parseAxis("/controls/elevator"));
}

FGFDM::~FGFDM()
{
    int i;
    for(i=0; i<_axes.size(); i++) {
	AxisRec* a = (AxisRec*)_axes.get(i);
	delete[] a->name;
	delete a;
    }
    for(i=0; i<_thrusters.size(); i++) {
	EngRec* er = (EngRec*)_thrusters.get(i);
	delete[] er->prefix;
	delete er->eng;
	delete er;
    }
    for(i=0; i<_weights.size(); i++) {
	WeightRec* wr = (WeightRec*)_weights.get(i);
	delete[] wr->prop;
	delete wr;
    }
    for(i=0; i<_controlProps.size(); i++)
        delete (PropOut*)_controlProps.get(i);
}

void FGFDM::iterate(float dt)
{
    getExternalInput(dt);
    _airplane.iterate(dt);
    setOutputProperties();
}

Airplane* FGFDM::getAirplane()
{
    return &_airplane;
}

void FGFDM::init()
{
    // Allows the user to start with something other than full fuel
    _airplane.setFuelFraction(fgGetFloat("/sim/fuel-fraction", 1));

    // This has a nasty habit of being false at startup.  That's not
    // good.
    fgSetBool("/controls/gear-down", true);
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
	_airplane.setApproach(spd, alt, aoa);
	_cruiseCurr = false;
    } else if(eq(name, "cruise")) {
	float spd = attrf(a, "speed") * KTS2MPS;
	float alt = attrf(a, "alt") * FT2M;
	_airplane.setCruise(spd, alt);
	_cruiseCurr = true;
    } else if(eq(name, "cockpit")) {
	v[0] = attrf(a, "x");
	v[1] = attrf(a, "y");
	v[2] = attrf(a, "z");
	_airplane.setPilotPos(v);
    } else if(eq(name, "wing")) {
	_airplane.setWing(parseWing(a, name));
    } else if(eq(name, "hstab")) {
	_airplane.setTail(parseWing(a, name));
    } else if(eq(name, "vstab")) {
	_airplane.addVStab(parseWing(a, name));
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

 	float n1min = attrf(a, "n1-idle", 55);
	float n1max = attrf(a, "n1-max", 102);
 	float n2min = attrf(a, "n2-idle", 73);
	float n2max = attrf(a, "n2-max", 103);
	j->setRPMs(n1min, n1max, n2min, n2max);

	if(a->hasAttribute("tsfc")) j->setTSFC(attrf(a, "tsfc"));
	if(a->hasAttribute("egt"))  j->setEGT(attrf(a, "egt"));
	if(a->hasAttribute("epr"))  j->setEPR(attrf(a, "epr"));
	if(a->hasAttribute("exhaust-speed"))
	    j->setVMax(attrf(a, "exhaust-speed") * KTS2MPS);
	
	j->setPosition(v);
	_airplane.addThruster(j, mass, v);
	sprintf(buf, "/engines/engine[%d]", _nextEngine++);
	EngRec* er = new EngRec();
	er->eng = j;
	er->prefix = dup(buf);
	_thrusters.add(er);
    } else if(eq(name, "gear")) {
	Gear* g = new Gear();
	_currObj = g;
	v[0] = attrf(a, "x");
	v[1] = attrf(a, "y");
	v[2] = attrf(a, "z");
	g->setPosition(v);
	v[0] = 0;
	v[1] = 0;
	v[2] = attrf(a, "compression", 1);
	g->setCompression(v);
        g->setBrake(attrf(a, "skid", 0));
	g->setStaticFriction(attrf(a, "sfric", 0.8));
	g->setDynamicFriction(attrf(a, "dfric", 0.7));
	_airplane.addGear(g);
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
	_airplane.addFuselage(v, b, attrf(a, "width"), taper, mid);
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
    // The control axes
    ControlMap* cm = _airplane.getControlMap();
    cm->reset();
    int i;
    for(i=0; i<_axes.size(); i++) {
	AxisRec* a = (AxisRec*)_axes.get(i);
	float val = fgGetFloat(a->name, 0);
	cm->setInput(a->handle, val);
    }
    cm->applyControls(dt);

    // Weights
    for(i=0; i<_weights.size(); i++) {
	WeightRec* wr = (WeightRec*)_weights.get(i);
	_airplane.setWeight(wr->handle, fgGetFloat(wr->prop));
    }
}

void FGFDM::setOutputProperties()
{
    char buf[256];
    int i;

    float grossWgt = _airplane.getModel()->getBody()->getTotalMass() * KG2LBS;
    fgSetFloat("/yasim/gross-weight-lbs", grossWgt);

    ControlMap* cm = _airplane.getControlMap();
    for(i=0; i<_controlProps.size(); i++) {
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

    float fuelDensity = 718.95; // default to gasoline: ~6 lb/gal
    for(i=0; i<_airplane.numTanks(); i++) {
        fuelDensity = _airplane.getFuelDensity(i);
	sprintf(buf, "/consumables/fuel/tank[%d]/level-gal_us", i);
	fgSetFloat(buf, CM2GALS*_airplane.getFuel(i)/fuelDensity);
    }

    for(i=0; i<_thrusters.size(); i++) {
	EngRec* er = (EngRec*)_thrusters.get(i);
        Thruster* t = er->eng;

	sprintf(buf, "%s/fuel-flow-gph", er->prefix);
	fgSetFloat(buf, (t->getFuelFlow()/fuelDensity) * 3600 * CM2GALS);

	if(t->getPropEngine()) {
            PropEngine* p = t->getPropEngine();

            sprintf(buf, "%s/rpm", er->prefix);
            fgSetFloat(buf, p->getOmega() / RPM2RAD);
        }

        if(t->getPistonEngine()) {
            PistonEngine* p = t->getPistonEngine();
	    
            sprintf(buf, "%s/mp-osi", er->prefix);
	    fgSetFloat(buf, p->getMP() * (1/INHG2PA));

	    sprintf(buf, "%s/egt-degf", er->prefix);
	    fgSetFloat(buf, p->getEGT() * K2DEGF + 459.4);
        }

        if(t->getJet()) {
            Jet* j = t->getJet();

            sprintf(buf, "%s/n1", er->prefix);
            fgSetFloat(buf, j->getN1());

            sprintf(buf, "%s/n2", er->prefix);
            fgSetFloat(buf, j->getN2());

            sprintf(buf, "%s/epr", er->prefix);
            fgSetFloat(buf, j->getEPR());

            sprintf(buf, "%s/egt-degf", er->prefix);
            fgSetFloat(buf, j->getEGT() * K2DEGF + 459.4);
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
    w->setIncidence(attrf(a, "incidence", 0) * DEG2RAD);

    float effect = attrf(a, "effectiveness", 1);
    w->setDragScale(w->getDragScale()*effect);

    _currObj = w;
    return w;
}

void FGFDM::parsePropeller(XMLAttributes* a)
{
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

    // Hack, fix this pronto:
    float engP = attrf(a, "eng-power") * HP2W;
    float engS = attrf(a, "eng-rpm") * RPM2RAD;

    Propeller* prop = new Propeller(radius, speed, omega, rho, power);
    PistonEngine* eng = new PistonEngine(engP, engS);
    PropEngine* thruster = new PropEngine(prop, eng, moment);
    _airplane.addThruster(thruster, mass, cg);

    if(a->hasAttribute("displacement"))
        eng->setDisplacement(attrf(a, "displacement") * CIN2CM);

    if(a->hasAttribute("compression"))
        eng->setCompression(attrf(a, "compression"));        

    if(a->hasAttribute("turbo-mul")) {
        float mul = attrf(a, "turbo-mul");
        float mp = attrf(a, "wastegate-mp", 1e6) * INHG2PA;
        eng->setTurboParams(mul, mp);
    }

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
    int i;
    for(i=0; i<_axes.size(); i++) {
	AxisRec* a = (AxisRec*)_axes.get(i);
	if(eq(a->name, name))
	    return a->handle;
    }

    // Not there, make a new one.
    AxisRec* a = new AxisRec();
    a->name = dup(name);
    a->handle = _airplane.getControlMap()->newInput();
    _axes.add(a);
    return a->handle;
}

int FGFDM::parseOutput(const char* name)
{
    if(eq(name, "THROTTLE"))  return ControlMap::THROTTLE;
    if(eq(name, "MIXTURE"))   return ControlMap::MIXTURE;
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
    if(eq(name, "INCIDENCE")) return ControlMap::INCIDENCE;
    if(eq(name, "FLAP0"))     return ControlMap::FLAP0;
    if(eq(name, "FLAP1"))     return ControlMap::FLAP1;
    if(eq(name, "SLAT"))      return ControlMap::SLAT;
    if(eq(name, "SPOILER"))   return ControlMap::SPOILER;
    if(eq(name, "CASTERING")) return ControlMap::CASTERING;
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

int FGFDM::attri(XMLAttributes* atts, char* attr)
{
    if(!atts->hasAttribute(attr)) {
        SG_LOG(SG_FLIGHT,SG_ALERT,"Missing '" << attr <<
               "' in YASim aircraft description");
        exit(1);
    }
    return attri(atts, attr, 0);
}

int FGFDM::attri(XMLAttributes* atts, char* attr, int def)
{
    const char* val = atts->getValue(attr);
    if(val == 0) return def;
    else         return atol(val);
}

float FGFDM::attrf(XMLAttributes* atts, char* attr)
{
    if(!atts->hasAttribute(attr)) {
        SG_LOG(SG_FLIGHT,SG_ALERT,"Missing '" << attr <<
               "' in YASim aircraft description");
        exit(1);
    }
    return attrf(atts, attr, 0);
}

float FGFDM::attrf(XMLAttributes* atts, char* attr, float def)
{
    const char* val = atts->getValue(attr);
    if(val == 0) return def;
    else         return (float)atof(val);    
}

}; // namespace yasim
