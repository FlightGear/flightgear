#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <cstring>

#include <Main/fg_props.hxx>

#include "yasim-common.hpp"
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
#include "ElectricEngine.hpp"
#include "Rotor.hpp"
#include "Rotorpart.hpp"
#include "Hitch.hpp"
#include "Surface.hpp"

#include "FGFDM.hpp"

namespace yasim {

// Stubs, so that this can be compiled without the FlightGear
// binary.  What's the best way to handle this?

//     float fgGetFloat(char* name, float def) { return 0; }
//     void fgSetFloat(char* name, float val) {}

FGFDM::FGFDM()
{
    // FIXME: read seed from somewhere?
    int seed = 0;
    _turb = new Turbulence(10, seed);
}

FGFDM::~FGFDM()
{
    for(int i=0; i<_thrusters.size(); i++) {
        EngRec* er = (EngRec*)_thrusters.get(i);
        delete er->eng;
        delete er;
    }

    for(int i=0; i<_weights.size(); i++) {
        WeightRec* wr = (WeightRec*)_weights.get(i);
        delete wr;
    }

    for(int i=0; i<_controlOutputs.size(); i++)
        delete (ControlOutput*)_controlOutputs.get(i);

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
    //reset id generator, needed on simulator reset/re-init
    Surface::resetIDgen();
    _turb_magnitude_norm = fgGetNode("/environment/turbulence/magnitude-norm", true);
    _turb_rate_hz        = fgGetNode("/environment/turbulence/rate-hz", true);

    _yasimN = fgGetNode("/fdm/yasim", true);
    _gross_weight_lbs = _yasimN->getNode("gross-weight-lbs", true);

    // alias to older name
    fgGetNode("/yasim/gross-weight-lbs", true)->alias(_gross_weight_lbs);

    // write some compile time information to property tree
    _yasimN->getNode("config-version",true)->setIntValue(_airplane.getVersion());
    _yasimN->getNode("model/cg-x-min",true)->setFloatValue(_airplane.getCGHardLimitXMin());
    _yasimN->getNode("model/cg-x-max",true)->setFloatValue(_airplane.getCGHardLimitXMax());

    // prepare nodes for write at runtime
    _cg_x = _yasimN->getNode("cg-x-m", true);
    _cg_xmacN = _yasimN->getNode("cg-x-mac", true);
    _cg_y = _yasimN->getNode("cg-y-m", true);
    _cg_z = _yasimN->getNode("cg-z-m", true);
    _vxN = _yasimN->getNode("velocities/v-x", true);
    _vyN = _yasimN->getNode("velocities/v-y", true);
    _vzN = _yasimN->getNode("velocities/v-z", true);
    _vrxN = _yasimN->getNode("velocities/vrot-x", true);
    _vryN = _yasimN->getNode("velocities/vrot-y", true);
    _vrzN = _yasimN->getNode("velocities/vrot-z", true);
    _axN = _yasimN->getNode("accelerations/a-x", true);
    _ayN = _yasimN->getNode("accelerations/a-y", true);
    _azN = _yasimN->getNode("accelerations/a-z", true);
    _arxN = _yasimN->getNode("accelerations/arot-x", true);
    _aryN = _yasimN->getNode("accelerations/arot-y", true);
    _arzN = _yasimN->getNode("accelerations/arot-z", true);

    // Allows the user to start with something other than full fuel
    _airplane.setFuelFraction(fgGetFloat("/sim/fuel-fraction", 1));

    // stash engine/thruster properties
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

    // stash properties for fuel state
    _fuel_props.clear();
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

    if (_yasimN->getBoolValue("respect-external-gear-state") == false) {
        fgSetBool("/controls/gear/gear-down", true);
    }

    _airplane.getModel()->setTurbulence(_turb);
}

// Not the worlds safest parser.  But it's short & sweet.
void FGFDM::startElement(const char* name, const XMLAttributes &a)
{
    //XMLAttributes* a = (XMLAttributes*)&atts;
    float v[3] {0,0,0};

    if(!strcmp(name, "airplane")) { parseAirplane(&a); }
    else if(!strcmp(name, "approach") || !strcmp(name, "cruise")) {
        parseApproachCruise(&a, name);
    }
    else if(!strcmp(name, "solve-weight")) { parseSolveWeight(&a); }
    else if(!strcmp(name, "cockpit")) { parseCockpit(&a); }
    else if(!strcmp(name, "rotor")) { parseRotor(&a, name); }
    else if(!strcmp(name, "rotorgear")) { parseRotorGear(&a); }
    else if(!strcmp(name, "wing") || !strcmp(name, "hstab") || !strcmp(name, "vstab") || !strcmp(name, "mstab")) {
        parseWing(&a, name, &_airplane);
    }
    else if(!strcmp(name, "piston-engine")) { parsePistonEngine(&a); }
    else if(!strcmp(name, "turbine-engine")) { parseTurbineEngine(&a); }
    else if(!strcmp(name, "electric-engine")) { parseElectricEngine(&a); }
    else if(!strcmp(name, "propeller")) { parsePropeller(&a); }
    else if(!strcmp(name, "thruster")) { parseThruster(&a); }
    else if(!strcmp(name, "jet")) { parseJet(&a); }
    else if(!strcmp(name, "hitch")) { parseHitch(&a); }
    else if(!strcmp(name, "tow")) { parseTow(&a); }
    else if(!strcmp(name, "winch")) { parseWinch(&a); }
    else if(!strcmp(name, "gear")) { parseGear(&a); }
    else if(!strcmp(name, "hook")) { parseHook(&a); }
    else if(!strcmp(name, "launchbar")) { parseLaunchbar(&a); }
    else if(!strcmp(name, "fuselage")) { parseFuselage(&a); }
    else if(!strcmp(name, "tank")) { parseTank(&a); }
    else if(!strcmp(name, "ballast")) { parseBallast(&a); }
    else if(!strcmp(name, "weight")) { parseWeight(&a); }
    else if(!strcmp(name, "stall")) { parseStall(&a); }
    else if(!strcmp(name, "flap0") || !strcmp(name, "flap1") || !strcmp(name, "spoiler") || !strcmp(name, "slat")) {
        parseFlap(&a, name);
    }
    else if(!strcmp(name, "actionpt")) {
        attrf_xyz(&a, v);
        ((Thruster*)_currObj)->setPosition(v);
    }
    else if(!strcmp(name, "dir")) {
        attrf_xyz(&a, v);
        ((Thruster*)_currObj)->setDirection(v);
    }
    else if(!strcmp(name, "control-setting")) { parseControlSetting(&a); }
    else if(!strcmp(name, "control-input")) { parseControlIn(&a); }
    else if(!strcmp(name, "control-output")) { parseControlOut(&a); }
    else if(!strcmp(name, "control-speed")) { parseControlSpeed(&a); }
    else {
        SG_LOG(SG_FLIGHT,SG_ALERT,"Unexpected tag '" << name << "' found in YASim aircraft description");
        exit(1);
    }
} // startElement

void FGFDM::parseAirplane(const XMLAttributes* a)
{
    float f {0};
    if(a->hasAttribute("mass")) { f = attrf(a, "mass") * LBS2KG; }
    else if (a->hasAttribute("mass-lbs")) { f = attrf(a, "mass-lbs") * LBS2KG; }
    else if (a->hasAttribute("mass-kg")) { f = attrf(a, "mass-kg"); }
    else {
        SG_LOG(SG_FLIGHT,SG_ALERT,"YASim fatal: missing attribute, airplane needs one of {mass-lbs, mass-kg}");
        exit(1);
    }
    _airplane.setEmptyWeight(f);
    if(a->hasAttribute("version")) { _airplane.setVersion(a->getValue("version")); }
    if( !_airplane.isVersionOrNewer( Version::YASIM_VERSION_CURRENT ) ) {
        SG_LOG(SG_FLIGHT, SG_DEV_ALERT, "This aircraft does not use the latest yasim configuration version.");
    }
    _airplane.setDesiredCGRangeInPercentOfMAC(attrf(a, "cg-min", 0.25f), attrf(a, "cg-max", 0.3f));

    if (a->hasAttribute("mtow-lbs")) { _airplane.setMTOW(attrf(a, "mtow-lbs") * LBS2KG); }
    else if (a->hasAttribute("mtow-kg")) { _airplane.setMTOW(attrf(a, "mtow-kg")); }
    if (a->hasAttribute("solver-mode")) { _airplane.setSolverMode(attri(a, "solver-mode")); }
}

void FGFDM::parseApproachCruise(const XMLAttributes* a, const char* name)
{
    float spd, alt = 0;
    if (a->hasAttribute("speed")) { spd = attrf(a, "speed") * KTS2MPS; }
    else if (a->hasAttribute("speed-kt")) { spd = attrf(a, "speed-kt") * KTS2MPS; }
    else if (a->hasAttribute("speed-kmh")) { spd = attrf(a, "speed-kmh") * KMH2MPS; }
    else {
        SG_LOG(SG_FLIGHT,SG_ALERT,"YASim fatal: missing attribute, "<< name << " needs one of {speed-kt, speed-kmh}");
        exit(1);
    }
    if (a->hasAttribute("alt")) { alt = attrf(a, "alt") * FT2M; }
    else if (a->hasAttribute("alt-ft")) { alt = attrf(a, "alt-ft") * FT2M; }
    else if (a->hasAttribute("alt-m")) { alt = attrf(a, "alt-m"); }
    float gla = attrf(a, "glide-angle", 0) * DEG2RAD;
    if (!strcmp(name, "approach")) {
        float aoa = attrf(a, "aoa", 0) * DEG2RAD;
        _airplaneCfg = Airplane::Configuration::APPROACH;
        _airplane.setConfig(_airplaneCfg, spd, alt, attrf(a, "fuel", 0.2), gla, aoa);
    }
    else {
        _airplaneCfg = Airplane::Configuration::CRUISE;
        _airplane.setConfig(_airplaneCfg, spd, alt, attrf(a, "fuel", 0.5), gla);
    }
}

void FGFDM::parseSolveWeight(const XMLAttributes* a)
{
    float f {0};
    int idx = attri(a, "idx");
    if(a->hasAttribute("weight")) { f = attrf(a, "weight") * LBS2KG; }
    else if(a->hasAttribute("weight-lbs")) { f = attrf(a, "weight-lbs") * LBS2KG; }
    else if(a->hasAttribute("weight-kg")) { f = attrf(a, "weight-kg"); }
    else {
        SG_LOG(SG_FLIGHT,SG_ALERT,"YASim fatal: missing attribute, solve-weight needs one of {weight-lbs, weight-kg}");
        exit(1);
    }
    _airplane.addSolutionWeight(_airplaneCfg, idx, f);
}

void FGFDM::parseCockpit(const XMLAttributes* a)
{
    float v[3];
    attrf_xyz(a, v);
    _airplane.setPilotPos(v);
}


void FGFDM::getExternalInput(float dt)
{
    char buf[256];

    _turb->setMagnitude(_turb_magnitude_norm->getFloatValue());
    _turb->update(dt, _turb_rate_hz->getFloatValue());

    // The control axes
    ControlMap* cm = _airplane.getControlMap();
    cm->reset();

    for(int i=0; i < cm->numProperties(); i++) {
        ControlMap::PropHandle *p = cm->getProperty(i);
        float val = fgGetFloat(p->name, 0);
        cm->setInput(p->handle, val);
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
            sprintf(buf, "%s/rpm", er->prefix.c_str());
            p->setOmega(fgGetFloat(buf, 500) * RPM2RAD);
        }
    }
}

// Linearly "seeks" a property by the specified fraction of the way to
// the target value.  Used to emulate "slowly changing" output values.
static void moveprop(SGPropertyNode* node, const std::string& prop,
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

    float cg[3];
    _airplane.getModel()->getBody()->getCG(cg);
    _cg_x->setFloatValue(cg[0]);
    _cg_y->setFloatValue(cg[1]);
    _cg_z->setFloatValue(cg[2]);
    _cg_xmacN->setFloatValue(_airplane.getCGMAC());

    State* s = _airplane.getModel()->getState();
    float v[3], acc[3], rot[3], racc[3];
    Math::vmul33(s->orient, s->v, v);
    Math::vmul33(s->orient, s->acc, acc);
    Math::vmul33(s->orient, s->rot, rot);
    Math::vmul33(s->orient, s->racc, racc);

    _vxN->setFloatValue(v[0]);
    _vyN->setFloatValue(v[1]);
    _vzN->setFloatValue(v[2]);
    _vrxN->setFloatValue(rot[0]);
    _vryN->setFloatValue(rot[1]);
    _vrzN->setFloatValue(rot[2]);
    _axN->setFloatValue(acc[0]);
    _ayN->setFloatValue(acc[1]);
    _azN->setFloatValue(acc[2]);
    _arxN->setFloatValue(racc[0]);
    _aryN->setFloatValue(racc[1]);
    _arzN->setFloatValue(racc[2]);

    ControlMap* cm = _airplane.getControlMap();
    for(int i=0; i<_controlOutputs.size(); i++) {
        ControlOutput* p = (ControlOutput*)_controlOutputs.get(i);
        float val = (p->left
                     ? cm->getOutput(p->handle)
                     : cm->getOutputR(p->handle));
        float rmin = cm->rangeMin(p->control);
        float rmax = cm->rangeMax(p->control);
        float frac = (val - rmin) / (rmax - rmin);
        // clamp output
        val = frac*(p->max - p->min) + p->min;
        p->prop->setFloatValue(val);
    }

    for(int i=0; i<_airplane.getRotorgear()->getNumRotors(); i++) {
        Rotor*r=(Rotor*)_airplane.getRotorgear()->getRotor(i);
        int j = 0;
        float f;
        char b[300];
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
        if (tmp[0] < 0) lbs = -lbs; // Show negative thrust in properties.
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

void FGFDM::parseWing(const XMLAttributes* a, const char* type, Airplane* airplane)
{
    float defDihed = 0;
    bool mirror = true;

    if(!strcmp(type, "vstab")) {
        defDihed = 90;
        mirror = false;
    }

    float base[3] {0,0,0};
    float chord {0};
    float length = attrf(a, "length");

    // positive incidence/twist means positive AoA (leading edge up).
    // Due to the coordinate system used in class Surface the sign will be inverted (only) there.
    float incidence {0};
    float twist = attrf(a, "twist", 0) * DEG2RAD;

    // if this element is declared as section of a wing, skip attributes
    // that are ignored in class Wing anyway because they are calculated
    float isSection = attrb(a, "append");
    if (!isSection) {
        attrf_xyz(a, base);
        chord = attrf(a, "chord");
        incidence = attrf(a, "incidence", 0) * DEG2RAD;
    }
    else {
        if (a->hasAttribute("x") || a->hasAttribute("y") || a->hasAttribute("z") ||
            a->hasAttribute("chord") || a->hasAttribute("incidence")
        ) {
            SG_LOG(SG_FLIGHT, SG_WARN, "YASim warning: redundant attribute in wing definition \n"
            "when using <wing append=\"1\" ...> x, y, z, chord and incidence will be ignored. ");
        }
    }

    // optional attributes (with defaults)
    float sweep = attrf(a, "sweep", 0) * DEG2RAD;
    float taper = attrf(a, "taper", 1);
    float dihedral = attrf(a, "dihedral", defDihed) * DEG2RAD;


    float camber = attrf(a, "camber", 0);
    if (!airplane->isVersionOrNewer(Version::YASIM_VERSION_2017_2) && (camber == 0)) {
        SG_LOG(SG_FLIGHT, SG_DEV_WARN, "YASIM warning: versions before 2017.2 are buggy for wings with camber=0");
    }

    // The 70% is a magic number that sorta kinda seems to match known
    // throttle settings to approach speed.
    float idrag = 0.7*attrf(a, "idrag", 1);

    // get wing object by type
    Wing* w;
    if (!strcmp(type, "wing"))
    {
        w = airplane->getWing();
    }
    else if (!strcmp(type, "hstab")) {
        w = airplane->getTail();
        if (a->hasAttribute("incidence-max-deg")) w->setIncidenceMax(attrf(a, "incidence-max-deg") * DEG2RAD);
        if (a->hasAttribute("incidence-min-deg")) w->setIncidenceMin(attrf(a, "incidence-min-deg") * DEG2RAD);
    } else {
        w = new Wing(airplane, mirror);
    }
    // add section; if wing object has already section, base will be overridden
    // by tip of last section
    _wingSection = w->addWingSection(base, chord, length, taper, sweep, dihedral, twist, camber, idrag, incidence);
    if (!strcmp(type, "vstab") || !strcmp(type, "mstab"))
    {
        airplane->addVStab(w);
    }

    float dragFactor = attrf(a, "pdrag", 1);
    if (a->hasAttribute("effectiveness")) {
/* FIXME:
 * check if all attibutes have "good" names and update parser AND documentation together
 * only after that issue warnings
        SG_LOG(SG_FLIGHT, SG_ALERT, "Warning: " <<
               "deprecated attribute 'effectiveness' in YASim configuration file.  " <<
               "Use 'pdrag' instead to add parasitic drag.");
*/
        dragFactor = attrf(a, "effectiveness", 1);
    }
    w->setSectionDrag(_wingSection, dragFactor);
    if (a->hasAttribute("flow")) {
        const char* flowregime = a->getValue("flow");
        if (!strcmp(flowregime,"TRANSONIC")) {
            w->setFlowRegime(FLOW_TRANSONIC);
            const float mcrit = attrf(a,"mcrit", 0.6f);
            if ( (mcrit > 0.0f) && (mcrit <= 1.0f)) {
                w->setCriticalMachNumber(mcrit);
            } else {
                SG_LOG(SG_FLIGHT, SG_ALERT, "YASim warning: invalid input for critical mach number. Defaulting to mcrit=0.6.");
            }
        }
    } else {
        w->setFlowRegime(FLOW_SUBSONIC);
    }
    _currObj = w;
}

void FGFDM::parseRotor(const XMLAttributes* a, const char* type)
{
    Rotor* w = new Rotor();

    // float defDihed = 0;

    float pos[3];
    attrf_xyz(a, pos);
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

    if(a->hasAttribute("name"))      w->setName(a->getValue("name"));
    if(a->hasAttribute("alphaout0")) w->setAlphaoutput(0,a->getValue("alphaout0"));
    if(a->hasAttribute("alphaout1")) w->setAlphaoutput(1,a->getValue("alphaout1"));
    if(a->hasAttribute("alphaout2")) w->setAlphaoutput(2,a->getValue("alphaout2"));
    if(a->hasAttribute("alphaout3")) w->setAlphaoutput(3,a->getValue("alphaout3"));
    if(a->hasAttribute("coneout"))   w->setAlphaoutput(4,a->getValue("coneout"));
    if(a->hasAttribute("yawout"))    w->setAlphaoutput(5,a->getValue("yawout"));
    if(a->hasAttribute("rollout"))   w->setAlphaoutput(6,a->getValue("rollout"));

    w->setPitchA(attrf(a, "pitch-a", 10));
    w->setPitchB(attrf(a, "pitch-b", 10));
    w->setForceAtPitchA(attrf(a, "forceatpitch-a", 3000));
    w->setPowerAtPitch0(attrf(a, "poweratpitch-0", 300));
    w->setPowerAtPitchB(attrf(a, "poweratpitch-b", 3000));
    if(attrb(a,"notorque")) w->setNotorque(1);

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
    _airplane.getModel()->getRotorgear()->addRotor(w);
} //parseRotor

void FGFDM::parseRotorGear(const XMLAttributes* a)
{
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
}

void FGFDM::parsePistonEngine(const XMLAttributes* a)
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

void FGFDM::parseTurbineEngine(const XMLAttributes* a)
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

void FGFDM::parseElectricEngine(const XMLAttributes* a)
{
    float Kv = attrf(a, "Kv") * RPM2RAD; //Kv is expected to be given as RPM per volt in XML
    float voltage = attrf(a, "voltage"); // voltage applied at the motor windings in volts
    float Rm = attrf(a, "Rm"); // winding resistance in Ohms
    ElectricEngine* eng = new ElectricEngine(voltage, Kv, Rm);

    ((PropEngine*)_currObj)->setEngine(eng);
}


void FGFDM::parsePropeller(const XMLAttributes* a)
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
    attrf_xyz(a, cg);
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
    er->prefix = buf;
    _thrusters.add(er);

    _currObj = thruster;
}

void FGFDM::parseThruster(const XMLAttributes* a)
{
    float v[3];
    SimpleJet* j = new SimpleJet();
    _currObj = j;
    attrf_xyz(a, v);
    j->setPosition(v);
    _airplane.addThruster(j, 0, v);
    v[0] = attrf(a, "vx"); v[1] = attrf(a, "vy"); v[2] = attrf(a, "vz");
    j->setDirection(v);
    j->setThrust(attrf(a, "thrust") * LBS2N);
}

void FGFDM::parseJet(const XMLAttributes* a)
{
    float v[3];
    Jet* j = new Jet();
    _currObj = j;
    attrf_xyz(a, v);
    float mass;
    if(a->hasAttribute("mass")) { mass = attrf(a, "mass") * LBS2KG; }
    else if(a->hasAttribute("mass-lbs")) { mass = attrf(a, "mass-lbs") * LBS2KG; }
    else if(a->hasAttribute("mass-kg")) { mass = attrf(a, "mass-kg"); }
    else {
        SG_LOG(SG_FLIGHT,SG_ALERT,"YASim fatal: missing attribute, jet needs one of {mass-lbs, mass-kg}");
        exit(1);
    }
    j->setMaxThrust(attrf(a, "thrust") * LBS2N, attrf(a, "afterburner", 0) * LBS2N);
    j->setVectorAngle(attrf(a, "rotate", 0) * DEG2RAD);
    j->setReverseThrust(attrf(a, "reverse", 0.2));

    float n1min = attrf(a, "n1-idle", 55);
    float n1max = attrf(a, "n1-max", 102);
    float n2min = attrf(a, "n2-idle", 73);
    float n2max = attrf(a, "n2-max", 103);
    j->setRPMs(n1min, n1max, n2min, n2max);

    j->setTSFC(attrf(a, "tsfc", 0.8));
    j->setATSFC(attrf(a, "atsfc", 0.0));
    if(a->hasAttribute("egt"))  j->setEGT(attrf(a, "egt"));
    if(a->hasAttribute("epr"))  j->setEPR(attrf(a, "epr"));
    if(a->hasAttribute("exhaust-speed"))
        j->setVMax(attrf(a, "exhaust-speed") * KTS2MPS);
    if(a->hasAttribute("spool-time"))
        j->setSpooling(attrf(a, "spool-time"));

    j->setPosition(v);
    _airplane.addThruster(j, mass, v);
    char buf[64];
    sprintf(buf, "/engines/engine[%d]", _nextEngine++);
    EngRec* er = new EngRec();
    er->eng = j;
    er->prefix = buf;
    _thrusters.add(er);
}

void FGFDM::parseHitch(const XMLAttributes* a)
{
    float v[3];
    Hitch* h = new Hitch(a->getValue("name"));
    _currObj = h;
    attrf_xyz(a, v);
    h->setPosition(v);
    if(a->hasAttribute("force-is-calculated-by-other")) h->setForceIsCalculatedByOther(attrb(a,"force-is-calculated-by-other"));
    _airplane.addHitch(h);
}

void FGFDM::parseTow(const XMLAttributes* a)
{
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
}

void FGFDM::parseWinch(const XMLAttributes* a)
{
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
}

void FGFDM::parseGear(const XMLAttributes* a)
{
    float v[3];
    Gear* g = new Gear();
    _currObj = g;
    attrf_xyz(a, v);
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
    g->setStiction(attrf(a, "stiction", 0));
    g->setStictionABS(attrf(a, "stiction-abs", 0));
    _airplane.addGear(g);
}

void FGFDM::parseHook(const XMLAttributes* a)
{
    float v[3];
    Hook* h = new Hook();
    _currObj = h;
    attrf_xyz(a, v);
    h->setPosition(v);
    float length = attrf(a, "length", 1.0);
    h->setLength(length);
    float nrm = length+Math::mag3(v);
    if (_vehicle_radius < nrm)
        _vehicle_radius = nrm;
    h->setDownAngle(attrf(a, "down-angle", 70) * DEG2RAD);
    h->setUpAngle(attrf(a, "up-angle", 0) * DEG2RAD);
    _airplane.addHook(h);
}

void FGFDM::parseLaunchbar(const XMLAttributes* a)
{
    float v[3];
    Launchbar* l = new Launchbar();
    _currObj = l;
    attrf_xyz(a, v);
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
}

void FGFDM::parseFuselage(const XMLAttributes* a)
{
    float v[3];
    float b[3];
    v[0] = attrf(a, "ax");
    v[1] = attrf(a, "ay");
    v[2] = attrf(a, "az");
    b[0] = attrf(a, "bx");
    b[1] = attrf(a, "by");
    b[2] = attrf(a, "bz");
    float taper = attrf(a, "taper", 1);
    float mid = attrf(a, "midpoint", 0.5);
    if (_airplane.isVersionOrNewer(Version::YASIM_VERSION_32)) {
        // A fuselage's "midpoint" XML attribute is defined from the
            // fuselage's front end, but the Fuselage object's internal
            // "mid" attribute is actually defined from the rear end.
        // Thus YASim's original interpretation of "midpoint" was wrong.
            // Complement the "midpoint" value to ensure the fuselage
            // points the right way.
        mid = 1 - mid;
    }
    float cx = attrf(a, "cx", 1);
    float cy = attrf(a, "cy", 1);
    float cz = attrf(a, "cz", 1);
    float idrag = attrf(a, "idrag", 1);
    _airplane.addFuselage(v, b, attrf(a, "width"), taper, mid, cx, cy, cz, idrag);
}

void FGFDM::parseTank(const XMLAttributes* a)
{
    float v[3];
    attrf_xyz(a, v);
    float density = 6.0; // gasoline, in lbs/gal
    if(a->hasAttribute("jet")) density = 6.72;
    density *= LBS2KG*CM2GALS;
    float capacity = 0;
    if(a->hasAttribute("capacity")) { capacity = attrf(a, "capacity") * LBS2KG; }
    else if(a->hasAttribute("capacity-lbs")) { capacity = attrf(a, "capacity-lbs") * LBS2KG; }
    else if(a->hasAttribute("capacity-kg")) { capacity = attrf(a, "capacity-kg"); }
    else {
        SG_LOG(SG_FLIGHT,SG_ALERT,"YASim fatal: missing attribute, tank needs one of {capacity-lbs, capacity-kg}");
        exit(1);
    }
    _airplane.addTank(v, capacity, density);
}

void FGFDM::parseBallast(const XMLAttributes* a)
{
    float v[3];
    float f;
    attrf_xyz(a, v);
    if(a->hasAttribute("mass")) { f = attrf(a, "mass") * LBS2KG; }
    else if (a->hasAttribute("mass-lbs")) { f = attrf(a, "mass-lbs") * LBS2KG; }
    else if (a->hasAttribute("mass-kg")) { f = attrf(a, "mass-kg"); }
    else {
        SG_LOG(SG_FLIGHT,SG_ALERT,"YASim fatal: missing attribute, airplane needs one of {mass-lbs, mass-kg}");
        exit(1);
    }
    _airplane.addBallast(v, f);
}

/*
void FGFDM::parseXXX(const XMLAttributes* a)
{
    float v[3];
}
*/

void FGFDM::parseWeight(const XMLAttributes* a)
{
    WeightRec* wr = new WeightRec();

    float v[3];
    attrf_xyz(a, v);
    wr->prop = std::string{a->getValue("mass-prop")};
    wr->size = attrf(a, "size", 0);
    wr->handle = _airplane.addWeight(v, wr->size);
    _weights.add(wr);
}

void FGFDM::parseStall(const XMLAttributes* a)
{
    Wing* w = (Wing*)_currObj;
    StallParams sp;
    sp.aoa = attrf(a, "aoa") * DEG2RAD;
    sp.width = attrf(a, "width", 2) * DEG2RAD;
    sp.peak = attrf(a, "peak", 1.5);
    w->setSectionStallParams(_wingSection, sp);
}

void FGFDM::parseFlap(const XMLAttributes* a, const char* name)
{
    FlapParams fp;
    fp.start = attrf(a, "start");
    fp.end = attrf(a, "end");
    if (!strcmp(name, "slat")) {
        fp.aoa = attrf(a, "aoa");
    }
    else {
        fp.lift =  attrf(a, "lift");
    }
    fp.drag = attrf(a, "drag");
    if (!strcmp(name, "flap0")) {
        ((Wing*)_currObj)->setFlapParams(_wingSection, WING_FLAP0, fp);
    }
    if (!strcmp(name, "flap1")) {
        ((Wing*)_currObj)->setFlapParams(_wingSection, WING_FLAP1, fp);
    }
    if (!strcmp(name, "spoiler")) {
        ((Wing*)_currObj)->setFlapParams(_wingSection, WING_SPOILER, fp);
    }
    if (!strcmp(name, "slat")) {
        ((Wing*)_currObj)->setFlapParams(_wingSection, WING_SLAT, fp);
    }
}

void FGFDM::parseControlSetting(const XMLAttributes* a)
{
    // A cruise or approach control setting
    float value = attrf(a, "value", 0);
    _airplane.addControlSetting(_airplaneCfg, a->getValue("axis"), value);
}

void FGFDM::parseControlIn(const XMLAttributes* a)
{
    // map input property to a YASim control
    ControlMap::ControlType control = ControlMap::parseControl(a->getValue("control"));
    int opt = 0;
    opt |= a->hasAttribute("split") ? ControlMap::OPT_SPLIT : 0;
    opt |= a->hasAttribute("invert") ? ControlMap::OPT_INVERT : 0;
    opt |= a->hasAttribute("square") ? ControlMap::OPT_SQUARE : 0;
    float src0, src1, dst0, dst1;
    src0 = dst0 = ControlMap::rangeMin(control);
    src1 = dst1 = ControlMap::rangeMax(control);
    if(a->hasAttribute("src0")) {
        src0 = attrf(a, "src0");
        src1 = attrf(a, "src1");
        dst0 = attrf(a, "dst0");
        dst1 = attrf(a, "dst1");
    }
    _airplane.addControlInput(a->getValue("axis"), control, _currObj, _wingSection, opt, src0, src1, dst0, dst1);
}

void FGFDM::parseControlOut(const XMLAttributes* a)
{
    // A property output for a control on the current object
    ControlMap* cm = _airplane.getControlMap();
    ControlMap::ControlType control = ControlMap::parseControl(a->getValue("control"));
    ControlMap::ObjectID oid = ControlMap::getObjectID(_currObj, _wingSection);

    ControlOutput* p = new ControlOutput();
    p->prop = fgGetNode(a->getValue("prop"), true);
    p->handle = cm->getOutputHandle(oid, control);
    p->control = control;
    p->left = !(a->hasAttribute("side") &&
                    !strcmp("right", a->getValue("side")));
    // for output clamping
    p->min = attrf(a, "min", cm->rangeMin(control));
    p->max = attrf(a, "max", cm->rangeMax(control));
    _controlOutputs.add(p);
}

void FGFDM::parseControlSpeed(const XMLAttributes* a)
{
    ControlMap* cm = _airplane.getControlMap();
    ControlMap::ControlType control = ControlMap::parseControl(a->getValue("control"));
    ControlMap::ObjectID oid = ControlMap::getObjectID(_currObj, _wingSection);
    int handle = cm->getOutputHandle(oid, control);
    float time = attrf(a, "transition-time", 0);
    cm->setTransitionTime(handle, time);
}

int FGFDM::attri(const XMLAttributes* atts, const char* attr)
{
    if(!atts->hasAttribute(attr)) {
        SG_LOG(SG_FLIGHT,SG_ALERT,"Missing '" << attr <<
               "' in YASim aircraft description");
        exit(1);
    }
    return attri(atts, attr, 0);
}

int FGFDM::attri(const XMLAttributes* atts, const char* attr, int def)
{
    const char* val = atts->getValue(attr);
    if(val == 0) return def;
    else         return atol(val);
}

float FGFDM::attrf(const XMLAttributes* atts, const char* attr)
{
    if(!atts->hasAttribute(attr)) {
        SG_LOG(SG_FLIGHT,SG_ALERT,"Missing '" << attr <<
               "' in YASim aircraft description");
        exit(1);
    }
    return attrf(atts, attr, 0);
}

float FGFDM::attrf(const XMLAttributes* atts, const char* attr, float def)
{
    const char* val = atts->getValue(attr);
    if(val == 0) return def;
    else         return (float)atof(val);
}

void FGFDM::attrf_xyz(const XMLAttributes* atts, float* out)
{
    out[0] = attrf(atts, "x");
    out[1] = attrf(atts, "y");
    out[2] = attrf(atts, "z");
}

double FGFDM::attrd(const XMLAttributes* atts, const char* attr)
{
    if(!atts->hasAttribute(attr)) {
        SG_LOG(SG_FLIGHT,SG_ALERT,"Missing '" << attr <<
               "' in YASim aircraft description");
        exit(1);
    }
    return attrd(atts, attr, 0);
}

double FGFDM::attrd(const XMLAttributes* atts, const char* attr, double def)
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
bool FGFDM::attrb(const XMLAttributes* atts, const char* attr)
{
    const char* val = atts->getValue(attr);
    if(val == 0) return false;

    if(!strcmp(val,"true")) {
        SG_LOG(SG_FLIGHT, SG_ALERT, "Warning: " <<
               "deprecated 'true' boolean in YASim configuration file.  " <<
               "Use numeric booleans (attribute=\"1\") instead");
        return true;
    }
    return attri(atts, attr, 0) ? true : false;
}

}; // namespace yasim
