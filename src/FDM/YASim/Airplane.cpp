#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include "Atmosphere.hpp"
#include "ControlMap.hpp"
#include "Gear.hpp"
#include "Math.hpp"
#include "Glue.hpp"
#include "RigidBody.hpp"
#include "Surface.hpp"
#include "Rotorpart.hpp"
#include "Thruster.hpp"
#include "Hitch.hpp"
#include "Airplane.hpp"
#include "yasim-common.hpp"

namespace yasim {

// gadgets
inline float abs(float f) { return f<0 ? -f : f; }

Airplane::Airplane()
{
}

Airplane::~Airplane()
{
    int i;
    for(i=0; i<_fuselages.size(); i++)
	delete (Fuselage*)_fuselages.get(i);
    for(i=0; i<_tanks.size(); i++)
	delete (Tank*)_tanks.get(i);
    for(i=0; i<_thrusters.size(); i++)
	delete (ThrustRec*)_thrusters.get(i);
    for(i=0; i<_gears.size(); i++) {
	GearRec* g = (GearRec*)_gears.get(i);
        delete g->gear;
        delete g;
    }
    for(i=0; i<_surfs.size(); i++)
	delete (Surface*)_surfs.get(i);    
    for(i=0; i<_contacts.size(); i++) {
        ContactRec* c = (ContactRec*)_contacts.get(i);
        delete c->gear;
        delete c;
    }
    for(i=0; i<_solveWeights.size(); i++)
        delete (SolveWeight*)_solveWeights.get(i);
    for(i=0; i<_config[CRUISE].controls.size(); i++) {
        ControlSetting* c = (ControlSetting*)_config[CRUISE].controls.get(i);
        delete c;        
    }
    for(i=0; i<_config[APPROACH].controls.size(); i++) {
        ControlSetting* c = (ControlSetting*)_config[APPROACH].controls.get(i);
        delete c;
    }
    delete _wing;
    delete _tail;
    for(i=0; i<_vstabs.size(); i++)
        delete (Wing*)_vstabs.get(i);
    for(i=0; i<_weights.size(); i++)
        delete (WeightRec*)_weights.get(i);
}

void Airplane::iterate(float dt)
{
    // The gear might have moved.  Change their aerodynamics.
    updateGearState();
    _model.iterate();
}

void Airplane::calcFuelWeights()
{
    for(int i=0; i<_tanks.size(); i++) {
        Tank* t = (Tank*)_tanks.get(i);
        _model.getBody()->setMass(t->handle, t->fill);
    }
}

void Airplane::getPilotAccel(float* out) 
{
    State* s = _model.getState();

    // Gravity
    Glue::geodUp(s->pos, out);
    Math::mul3(-9.8f, out, out);
    Math::vmul33(s->orient, out, out);
    out[0] = -out[0];

    float acceleration[3];
    // Convert to aircraft coordinates
    Math::vmul33(s->orient, s->acc, acceleration);
    acceleration[1] = -acceleration[1];
    acceleration[2] = -acceleration[2];

    Math::add3(acceleration, out, out);

    // FIXME: rotational & centripetal acceleration needed
}

Wing* Airplane::getWing()
{
    if (_wing == nullptr) {
        _wing = new Wing((Version*)this, true);
    }
    return _wing;
}

Wing* Airplane::getTail()
{
    if (_tail == nullptr) {
        _tail = new Wing((Version*)this, true);
    }
    return _tail;
}

void Airplane::updateGearState()
{
    for(int i=0; i<_gears.size(); i++) {
        GearRec* gr = (GearRec*)_gears.get(i);
        float ext = gr->gear->getExtension();

        gr->surf->setDragCoefficient(ext);
        gr->surf->setYDrag(ext);
        gr->surf->setLiftCoefficient(ext);
    }
}

/// setup a config record
void Airplane::setConfig(Configuration cfg, float speed, float altitude, float fuel, float gla, float aoa)
{
    _config[cfg].id = cfg;
    _config[cfg].speed = speed;
    _config[cfg].altitude = altitude;
    // solver assumes fixed (given) AoA for approach, so setup once; 
    // solver will change this for cruise config
    _config[cfg].state.setupOrientationFromAoa(aoa); 
    _config[cfg].aoa = aoa; // not strictly needed, see runConfig()
    _config[cfg].fuel = fuel;
    _config[cfg].glideAngle = gla;
}

/// set property name for elevator  
void Airplane::setElevatorControl(const char* propName)
{
    _approachElevator = _addControlSetting(APPROACH, propName, 0);
}

/// set property name for hstab trim
void Airplane::setHstabTrimControl(const char* propName)
{
    // there must be only one value for tail incidence but we need it in each 
    // config, so we have a 2nd variable
    _tailIncidence = _addControlSetting(APPROACH, propName, 0);
    _tailIncidenceCopy = _addControlSetting(CRUISE, propName, 0);
}

void Airplane::addControlSetting(Configuration cfg, const char* prop, float val)
{
    _addControlSetting(cfg, prop,val);
}

Airplane::ControlSetting* Airplane::_addControlSetting(Configuration cfg, const char* prop, float val)
{
    ControlSetting* c = new ControlSetting();
    c->propHandle = _controlMap.getInputPropertyHandle(prop);
    c->val = val;
    _config[cfg].controls.add(c);
    return c;
}


/**
 * used by the XML parser in FGFDM and solveAirplane
 */
void Airplane::addControlInput(const char* propName, ControlMap::ControlType type, void* obj, int subobj, int opt, float src0, float src1, float dst0, float dst1)
{
    ControlMap::ObjectID oid = ControlMap::getObjectID(obj, subobj);    
    _controlMap.addMapping(propName, type, oid, opt, src0, src1, dst0, dst1);
    // tail incidence is needed by solver so capture the prop name if used in XML
    if (type == ControlMap::INCIDENCE && obj == _tail) {
        setHstabTrimControl(propName);
    }
}

void Airplane::addSolutionWeight(Configuration cfg, int idx, float wgt)
{
    SolveWeight* w = new SolveWeight();
    w->cfg = cfg;
    w->id = idx;
    w->wgt = wgt;
    _solveWeights.add(w);
}

void Airplane::addFuselage(const float* front, const float* back, float width,
                           float taper, float mid, 
                           float cx, float cy, float cz, float idrag)
{
    Fuselage* f = new Fuselage();
    Math::set3(front, f->front);
    Math::set3(back, f->back);
    f->width = width;
    f->taper = taper;
    f->mid = mid;
    f->_cx=cx;
    f->_cy=cy;
    f->_cz=cz;
    f->_idrag=idrag;
    _fuselages.add(f);
}

int Airplane::addTank(const float* pos, float cap, float density)
{
    Tank* t = new Tank();
    Math::set3(pos, t->pos);
    t->cap = cap;
    t->fill = cap;
    t->density = density;
    t->handle = 0xffffffff;
    return _tanks.add(t);
}

void Airplane::addGear(Gear* gear)
{
    GearRec* g = new GearRec();
    g->gear = gear;
    g->surf = 0;
    _gears.add(g);
}

void Airplane::addThruster(Thruster* thruster, float mass, const float* cg)
{
    ThrustRec* t = new ThrustRec();
    t->thruster = thruster;
    t->mass = mass;
    Math::set3(cg, t->cg);
    _thrusters.add(t);
}

/// Use ballast to redistribute mass, this is NOT added to empty weight.
void Airplane::addBallast(const float* pos, float mass)
{
    _model.getBody()->addMass(mass, pos, true);
    _ballast += mass;
}

/// Masses configurable at runtime, e.g. cargo, pax
int Airplane::addWeight(const float* pos, float size)
{
    WeightRec* wr = new WeightRec();
    wr->handle = _model.getBody()->addMass(0, pos);

    wr->surf = new Surface(this, pos, size*size);
    _model.addSurface(wr->surf);
    _surfs.add(wr->surf);

    return _weights.add(wr);
}

/// Change weight of a previously added mass point
void Airplane::setWeight(int handle, float mass)
{
    WeightRec* wr = (WeightRec*)_weights.get(handle);

    _model.getBody()->setMass(wr->handle, mass);

    // Kill the aerodynamic drag if the mass is exactly zero.  This is
    // how we simulate droppable stores.
    if(mass == 0) {
        wr->surf->setDragCoefficient(0);
        wr->surf->setYDrag(0);
        wr->surf->setLiftCoefficient(0);
    } else {
        wr->surf->setDragCoefficient(1);
        wr->surf->setYDrag(1);
        wr->surf->setLiftCoefficient(1);
    }
}

void Airplane::setFuelFraction(float frac)
{
    for(int i=0; i<_tanks.size(); i++) {
        Tank* t = (Tank*)_tanks.get(i);
        t->fill = frac * t->cap;
        _model.getBody()->setMass(t->handle, t->cap * frac);
    }
}

/**
 * @brief add contact point for crash detection 
 * used to add wingtips and fuselage nose and tail
 * 
 * @param pos ...
 */

void Airplane::addContactPoint(const float* pos)
{
    ContactRec* c = new ContactRec;
    c->gear = 0;
    Math::set3(pos, c->p);
    _contacts.add(c);
}

float Airplane::compileWing(Wing* w)
{
    // Make sure it's initialized.  The surfaces will pop out with
    // total drag coefficients equal to their areas, which is what we
    // want.
    w->compile();

    // The tip of the wing is a contact point
    float tip[3];
    // need compile() before getTip()!
    w->getTip(tip);
    addContactPoint(tip);
    if(w->isMirrored()) {
        tip[1] *= -1;
        addContactPoint(tip);
        tip[1] *= -1; //undo mirror
    }

    float wgt = 0;
    wgt = w->updateModel(&_model);
    return wgt;
}

void Airplane::compileRotorgear()
{
    getRotorgear()->compile();
}

float Airplane::compileFuselage(Fuselage* f)
{
    // The front and back are contact points
    addContactPoint(f->front);
    addContactPoint(f->back);

    float wgt = 0;
    float fwd[3];
    Math::sub3(f->front, f->back, fwd);
    float len = Math::mag3(fwd);
    if (len == 0) {
        _failureMsg = "Zero length fuselage";
    return 0;
    }
    float wid = f->width;
    int segs = (int)Math::ceil(len/wid);
    float segWgt = len*wid/segs;
    int j;
    for(j=0; j<segs; j++) {
        float frac = (j+0.5f) / segs;
        float scale = 1;
        if(frac < f->mid)
            scale = f->taper+(1-f->taper) * (frac / f->mid);
        else {
            if( isVersionOrNewer( YASIM_VERSION_32 ) ) {
                // Correct calculation of width for fuselage taper.
                scale = 1 - (1-f->taper) * (frac - f->mid) / (1 - f->mid);
            } else {
                // Original, incorrect calculation of width for fuselage taper.
                scale = f->taper+(1-f->taper) * (frac - f->mid) / (1 - f->mid);
            }
        }

        // Where are we?
        float pos[3];
        Math::mul3(frac, fwd, pos);
        Math::add3(f->back, pos, pos);

        // _Mass_ weighting goes as surface area^(3/2)
        float mass = scale*segWgt * Math::sqrt(scale*segWgt);
        _model.getBody()->addMass(mass, pos, true);
        wgt += mass;


        // The following is the original YASim value for sideDrag.
        // Originally YASim calculated the fuselage's lateral drag
        // coefficient as (solver drag factor) * (len/wid).
        // However, this greatly underestimates a fuselage's lateral drag.
        float sideDrag = len/wid;

        if ( isVersionOrNewer( YASIM_VERSION_32 ) ) {
            // New YASim assumes a fixed lateral drag coefficient of 0.5.
            // This will not be multiplied by the solver drag factor, because
            // that factor is tuned to match the drag in the direction of
            // flight, which is completely independent of lateral drag.
            // The value of 0.5 is only a ballpark estimate, roughly matching
            // the side-on drag for a long cylinder at the higher Reynolds
            // numbers typical for an aircraft's lateral drag.
            // This fits if the fuselage is long and has a round cross section.
            // For flat-sided fuselages, the value should be increased, up to
            // a limit of around 2 for a long rectangular prism.
            // For very short fuselages, in which the end effects are strong,
            // the value should be reduced.
            // Such adjustments can be made using the fuselage's "cy" and "cz"
            // XML parameters: "cy" for the sides, "cz" for top and bottom.
            sideDrag = 0.5;
        }
        float dragCoefficient = scale*segWgt*f->_cx;
        if( isVersionOrNewer( YASIM_VERSION_32 ) ) {
                dragCoefficient = scale*segWgt;
        }

        // Make a Surface too
        Surface* s = new Surface(this, pos, dragCoefficient);
        if( isVersionOrNewer( YASIM_VERSION_32 ) ) {
                s->setDragCoefficient(f->_cx);
        }
        s->setYDrag(sideDrag*f->_cy);
        s->setLiftCoefficient(sideDrag*f->_cz);
        s->setInducedDrag(f->_idrag);

        // FIXME: fails for fuselages aligned along the Y axis
        float o[9];
        float *x=o, *y=o+3, *z=o+6; // nicknames for the axes
        Math::unit3(fwd, x);
        y[0] = 0; y[1] = 1; y[2] = 0;
        Math::cross3(x, y, z);
        Math::unit3(z, z);
        Math::cross3(z, x, y);
        s->setOrientation(o);

        _model.addSurface(s);
        f->surfs.add(s);
        _surfs.add(s);
    }
    return wgt;
}

// FIXME: should probably add a mass for the gear, too
void Airplane::compileGear(GearRec* gr)
{
    Gear* g = gr->gear;


    // Put the surface at the half-way point on the gear strut, give
    // it a drag coefficient equal to a square of the same dimension
    // (gear are really draggy) and make it symmetric.  Assume that
    // the "length" of the gear is 3x the compression distance
    float pos[3], cmp[3];
    g->getCompression(cmp);
    float length = 3 * Math::mag3(cmp);
    g->getPosition(pos);
    Math::mul3(0.5, cmp, cmp);
    Math::add3(pos, cmp, pos);

    // Make a Surface object for the aerodynamic behavior
    Surface* s = new Surface(this, pos, length*length);
    gr->surf = s;

    _model.addGear(g);
    _model.addSurface(s);
    _surfs.add(s);
}

/**
 * @brief add "fake gear" per contact point
 * 
 * 
 * @return void
 */

void Airplane::compileContactPoints()
{
    // Figure it will compress by 20cm
    float comp[3];
    float DIST = 0.2f;
    comp[0] = 0; comp[1] = 0; comp[2] = DIST;

    // Give it a spring constant such that at full compression it will
    // hold up 10 times the planes mass.  That's about right.  Yeah.
    float mass = _model.getBody()->getTotalMass();
    float spring = (1/DIST) * 9.8f * 10.0f * mass;
    float damp = 2 * Math::sqrt(spring * mass);

    for(int i=0; i<_contacts.size(); i++) {
        ContactRec* c = (ContactRec*)_contacts.get(i);

        Gear* g = new Gear();
        c->gear = g;
        g->setPosition(c->p);
        
        g->setCompression(comp);
        g->setSpring(spring);
        g->setDamping(damp);
        g->setBrake(1);

        // I made these up
        g->setStaticFriction(0.6f);
        g->setDynamicFriction(0.5f);
        g->setContactPoint(1);

        _model.addGear(g);
    }
}

void Airplane::compile(bool verbose)
{
    RigidBody* body = _model.getBody();
    int firstMass = body->numMasses();
    SGPropertyNode_ptr baseN = fgGetNode("/fdm/yasim/model/wings", true);

    // Generate the point masses for the plane.  Just use unitless
    // numbers for a first pass, then go back through and rescale to
    // make the weight right.
    float aeroWgt = 0;

    // The Wing objects
    if (_wing)
    {
        if (baseN != nullptr) {
            _wingsN = baseN->getChild("wing", 0, true);
            _wing->setPropertyNode(_wingsN);
        }
        aeroWgt += compileWing(_wing);
        
        // convert % to absolute x coordinates
        _cgDesiredFront = _wing->getMACx() - _wing->getMACLength()*_cgDesiredMin;
        _cgDesiredAft = _wing->getMACx() - _wing->getMACLength()*_cgDesiredMax;
        if (baseN != 0) {
            SGPropertyNode_ptr n = fgGetNode("/fdm/yasim/model", true);
            n->getNode("cg-x-range-front", true)->setFloatValue(_cgDesiredFront);
            n->getNode("cg-x-range-aft", true)->setFloatValue(_cgDesiredAft);
        }
    }
    if (_tail)
    {
        if (baseN != nullptr) {
            _wingsN = baseN->getChild("tail", 0, true);
            _tail->setPropertyNode(_wingsN);
        }
        aeroWgt += compileWing(_tail);
    }

    for(int i=0; i<_vstabs.size(); i++)
    {
        Wing* vs = (Wing*)_vstabs.get(i);
        if (baseN != 0) {
            _wingsN = baseN->getChild("stab", i, true);
            vs->setPropertyNode(_wingsN);
        }
        aeroWgt += compileWing(vs);
    }

    // The fuselage(s)
    for(int i=0; i<_fuselages.size(); i++)
        aeroWgt += compileFuselage((Fuselage*)_fuselages.get(i));

    // Count up the absolute weight we have
    float nonAeroWgt = _ballast;
    for(int i=0; i<_thrusters.size(); i++)
        nonAeroWgt += ((ThrustRec*)_thrusters.get(i))->mass;

    // Rescale to the specified empty weight
    float wscale = (_emptyWeight-nonAeroWgt)/aeroWgt;
    for(int i=firstMass; i<body->numMasses(); i++) {
        body->setMass(i, body->getMass(i)*wscale);
    }
    //if we have prop tree, give scale factor to each wing so it can export its mass to the prop tree
    if (baseN != nullptr) {
        if (_wing) _wing->weight2mass(wscale);
        if (_tail) _tail->weight2mass(wscale);
        for(int i=0; i<_vstabs.size(); i++)
        {
            ((Wing*)_vstabs.get(i))->weight2mass(wscale);
        }
    }
    // Add the thruster masses
    for(int i=0; i<_thrusters.size(); i++) {
        ThrustRec* t = (ThrustRec*)_thrusters.get(i);
        body->addMass(t->mass, t->cg, true);
    }

    // Add the tanks, empty for now.
    float totalFuel = 0;
    for(int i=0; i<_tanks.size(); i++) { 
        Tank* t = (Tank*)_tanks.get(i); 
        t->handle = body->addMass(0, t->pos);
        totalFuel += t->cap;
    }
    _config[CRUISE].weight = _emptyWeight + totalFuel*_config[CRUISE].fuel;
    _config[APPROACH].weight = _emptyWeight + totalFuel*_config[APPROACH].fuel;

    
    body->recalc();

    // Add surfaces for the landing gear.
    for(int i=0; i<_gears.size(); i++)
        compileGear((GearRec*)_gears.get(i));

    // The Thruster objects
    for(int i=0; i<_thrusters.size(); i++) {
        ThrustRec* tr = (ThrustRec*)_thrusters.get(i);
        tr->handle = _model.addThruster(tr->thruster);
    }
    
    if(_wing) {
        // Ground effect
        // If a double tapered wing is modelled with wing and mstab, wing must 
        // be outboard to get correct wingspan.
        float pos[3];
        float gespan = 0;
        gespan = _wing->getSpan();
        _wing->getBase(pos);
        if(!isVersionOrNewer( Version::YASIM_VERSION_2017_2 )) {
          //old code
          //float span = _length * Math::cos(_sweep) * Math::cos(_dihedral);
          //span = 2*(span + Math::abs(_base[2]));
          gespan -= 2*pos[1]; // cut away base (y-distance)
          gespan += 2*Math::abs(pos[2]); // add (wrong) z-distance
        }
        if (baseN != 0)
          baseN->getChild("wing", 0)->getNode("gnd-eff-span", true)->setFloatValue(gespan);
        // where does the hard coded factor 0.15 come from?
        _model.setGroundEffect(pos, gespan, 0.15f);
    }
    
    // solve function below resets failure message
    // so check if we have any problems and abort here
    if (_failureMsg) return;

    solveGear();
    calculateCGHardLimits();
    
    if(_wing && _tail) solveAirplane(verbose);
    else
    {
       // The rotor(s) mass:
       compileRotorgear(); 
       solveHelicopter(verbose);
    }

    // Do this after solveGear, because it creates "gear" objects that
    // we don't want to affect.
    compileContactPoints();
}

void Airplane::solveGear()
{
    float cg[3], pos[3];
    _model.getBody()->getCG(cg);

    // Calculate spring constant weightings for the gear.  Weight by
    // the inverse of the distance to the c.g. in the XY plane, which
    // should be correct for most gear arrangements.  Add 50cm of
    // "buffer" to keep things from blowing up with aircraft with a
    // single gear very near the c.g. (AV-8, for example).
    float total = 0;
    int i;
    for(i=0; i<_gears.size(); i++) {
        GearRec* gr = (GearRec*)_gears.get(i);
        Gear* g = gr->gear;
        g->getPosition(pos);
	Math::sub3(cg, pos, pos);
        gr->wgt = 1.0f/(0.5f+Math::sqrt(pos[0]*pos[0] + pos[1]*pos[1]));
        if (!g->getIgnoreWhileSolving())
            total += gr->wgt;
    }

    // Renormalize so they sum to 1
    for(i=0; i<_gears.size(); i++)
        ((GearRec*)_gears.get(i))->wgt /= total;
    
    // The force at max compression should be sufficient to stop a
    // plane moving downwards at 2x the approach descent rate.  Assume
    // a 3 degree approach.
    float descentRate = 2.0f*_config[APPROACH].speed/19.1f;

    // Spread the kinetic energy according to the gear weights.  This
    // will results in an equal compression fraction (not distance) of
    // each gear.
    float energy = 0.5f*_config[APPROACH].weight*descentRate*descentRate;

    for(i=0; i<_gears.size(); i++) {
        GearRec* gr = (GearRec*)_gears.get(i);
        float e = energy * gr->wgt;
        float comp[3];
        gr->gear->getCompression(comp);
        float len = Math::mag3(comp)*(1+2*gr->gear->getInitialLoad());

        // Energy in a spring: e = 0.5 * k * len^2
        float k = 2 * e / (len*len);

        gr->gear->setSpring(k * gr->gear->getSpring());

        // Critically damped (too damped, too!)
        gr->gear->setDamping(2*Math::sqrt(k*_config[APPROACH].weight*gr->wgt)
                             * gr->gear->getDamping());
    }
}

void Airplane::calculateCGHardLimits()
{
    _cgMax = -1e6;
    _cgMin = 1e6;
    for (int i = 0; i < _gears.size(); i++) {
        GearRec* gr = (GearRec*)_gears.get(i);
        float pos[3];
        gr->gear->getPosition(pos);
        if (pos[0] > _cgMax) _cgMax = pos[0];
        if (pos[0] < _cgMin) _cgMin = pos[0];
    }
}

void Airplane::initEngines()
{
    for(int i=0; i<_thrusters.size(); i++) {
        ThrustRec* tr = (ThrustRec*)_thrusters.get(i);
        tr->thruster->init();
    }
}

void Airplane::stabilizeThrust()
{
    for(int i=0; i<_thrusters.size(); i++)
	_model.getThruster(i)->stabilize();
}

/// Setup weights for cruise or approach during solve.
void Airplane::setupWeights(Configuration cfg)
{
    int i;
    for(i=0; i<_weights.size(); i++)
        setWeight(i, 0);
    for(i=0; i<_solveWeights.size(); i++) {
        SolveWeight* w = (SolveWeight*)_solveWeights.get(i);
        if(w->cfg == cfg)
            setWeight(w->id, w->wgt);
    }
}

/// used by solver to simulate property input
void Airplane::setControlValues(const Vector& controls)
{
    _controlMap.reset();
    for(int i=0; i < controls.size(); i++) {
        ControlSetting* c = (ControlSetting*)controls.get(i);
        if (c->propHandle >= 0)
            _controlMap.setInput(c->propHandle, c->val);
    }
    _controlMap.applyControls(); 
}

void Airplane::runConfig(Config &cfg)
{
    // aoa is consider to be given for approach so we calculate orientation 
    // for approach only once in setConfig() but everytime for cruise here.
    if (!(cfg.id == APPROACH)) {
        cfg.state.setupOrientationFromAoa(cfg.aoa);
    }
    cfg.state.setupSpeedAndPosition(cfg.speed, cfg.glideAngle);
    _model.setState(&cfg.state);
    _model.setStandardAtmosphere(cfg.altitude);
    setControlValues(cfg.controls);
  
    // The local wind
    float wind[3];
    Math::mul3(-1, cfg.state.v, wind);
    cfg.state.globalToLocal(wind, wind);
    
    setFuelFraction(cfg.fuel);
    setupWeights(cfg.id);
    
    // Set up the thruster parameters and iterate until the thrust
    // stabilizes.
    for(int i=0; i<_thrusters.size(); i++) {
        Thruster* t = ((ThrustRec*)_thrusters.get(i))->thruster;
        t->setWind(wind);
        t->setStandardAtmosphere(cfg.altitude);
    }
    
    stabilizeThrust();
    updateGearState();
    
    // Precompute thrust in the model, and calculate aerodynamic forces
    _model.getBody()->recalc();
    _model.getBody()->reset();
    _model.initIteration();
    _model.calcForces(&cfg.state);
}

/// Used only in solveAirplane() and solveHelicopter(), not at runtime
void Airplane::applyDragFactor(float factor)
{
    float applied = Math::pow(factor, _solverDelta);
    _dragFactor *= applied;
    if(_wing)
      _wing->multiplyDragCoefficient(applied);
    if(_tail)
      _tail->multiplyDragCoefficient(applied);
    int i;
    for(i=0; i<_vstabs.size(); i++) {
        Wing* w = (Wing*)_vstabs.get(i);
        w->multiplyDragCoefficient(applied);
    }
    for(i=0; i<_fuselages.size(); i++) {
        Fuselage* f = (Fuselage*)_fuselages.get(i);
        for(int j=0; j<f->surfs.size(); j++) {
            Surface* s = (Surface*)f->surfs.get(j);
            if( isVersionOrNewer( YASIM_VERSION_32 ) ) {
                // For new YASim, the solver drag factor is only applied to
                // the X axis for Fuselage Surfaces.
                // The solver is tuning the coefficient for longitudinal drag,
                // along the direction of flight. A fuselage's lateral drag
                // is completely independent and is normally much higher;
                // it won't be affected by the streamlining done to reduce
                // longitudinal drag. So the solver should only adjust the
                // fuselage's longitudinal (X axis) drag coefficient.
                s->mulDragCoefficient(applied);
            } else {
                // Originally YASim applied the drag factor to all axes
                // for Fuselage Surfaces.
                s->mulTotalForceCoefficient(applied);
            }
        }
    }
    for(i=0; i<_weights.size(); i++) {
        WeightRec* wr = (WeightRec*)_weights.get(i);
        wr->surf->mulTotalForceCoefficient(applied);
    }
    for(i=0; i<_gears.size(); i++) {
        GearRec* gr = (GearRec*)_gears.get(i);
        gr->surf->mulTotalForceCoefficient(applied);
    }
}

/// Used only in solveAirplane() and solveHelicopter(), not at runtime
/// change lift coefficient cz in surfaces
void Airplane::applyLiftRatio(float factor)
{
    float applied = Math::pow(factor, _solverDelta);
    _liftRatio *= applied;
    if(_wing)
      _wing->multiplyLiftRatio(applied);
    if(_tail)
      _tail->multiplyLiftRatio(applied);
    int i;
    for(i=0; i<_vstabs.size(); i++) {
        Wing* w = (Wing*)_vstabs.get(i);
        w->multiplyLiftRatio(applied);
    }
}

/// Helper for solveAirplane()
float Airplane::normFactor(float f)
{
    if(f < 0) f = -f;
    if(f < 1) f = 1/f;
    return f;
}

///helper for solveAirplane()
float Airplane::_getPitch(Config &cfg)
{
    float tmp[3];
    _model.getBody()->getAngularAccel(tmp);
    cfg.state.localToGlobal(tmp, tmp);
    return tmp[1];
}

///helper for solveAirplane()
float Airplane::_getLiftForce(Config &cfg)
{
    float tmp[3];
    _model.getBody()->getAccel(tmp);
    cfg.state.localToGlobal(tmp, tmp);
    return cfg.weight * tmp[2];
}

///helper for solveAirplane()
float Airplane::_getDragForce(Config &cfg)
{
    float tmp[3];
    _model.getBody()->getAccel(tmp);
    cfg.state.localToGlobal(tmp, tmp);
    return cfg.weight * tmp[0];
}

// This is a heuristic approach to "force" solver converge due to numeric
// problems. To be replaced by a better solution later.
float Airplane::_checkConvergence(float prev, float current)
{
    static int damping {0};
    //different sign and almost same value -> oscilation; 
    if ((prev*current) < 0 && (abs(current + prev) < 0.01f)) {
        if (!damping) fprintf(stderr,"YASim warning: possible convergence problem.\n");
        damping++;
        if (current < 1) current *= abs(current); // quadratic
        else current = sqrt(current);
    }
    return current;
}

void Airplane::solveAirplane(bool verbose)
{
    static const float ARCMIN = 0.0002909f;

    float tmp[3];
    _solutionIterations = 0;
    _failureMsg = 0;

    if (_approachElevator == nullptr) {        
        setElevatorControl(DEF_PROP_ELEVATOR_TRIM);
    }
    
    if (_tailIncidence == nullptr) {
        // no control mapping from XML parser, so we just create "local" 
        // variables for solver instead of full mapping / property
        _tailIncidence = new ControlSetting;
        _tailIncidenceCopy = new ControlSetting;
    }
    if (verbose) {
        fprintf(stdout,"i\tdAoa\tdTail\tcl0\tcp1\n");
    }

    float prevTailDelta {0};
    while(1) {
        if(_solutionIterations++ > _solverMaxIterations) { 
            _failureMsg = "Solution failed to converge!";
            return;
        }
        // Run an iteration at cruise, and extract the needed numbers:
        runConfig(_config[CRUISE]);
        
        _model.getThrust(tmp);
        float thrust = tmp[0] + _config[CRUISE].weight * Math::sin(_config[CRUISE].glideAngle) * 9.81;
        
        float cDragForce = _getDragForce(_config[CRUISE]);
        float clift0 = _getLiftForce(_config[CRUISE]);
        float cpitch0 = _getPitch(_config[CRUISE]);

        // Run an approach iteration, and do likewise
        runConfig(_config[APPROACH]);
        double apitch0 = _getPitch(_config[APPROACH]);
        float alift = _getLiftForce(_config[APPROACH]);

        // Modify the cruise AoA a bit to get a derivative
        float savedAoa = _config[CRUISE].aoa;
        _config[CRUISE].aoa += ARCMIN;
        runConfig(_config[CRUISE]);
        _config[CRUISE].aoa = savedAoa;
            
        float clift1 = _getLiftForce(_config[CRUISE]);

        // Do the same with the tail incidence
        float savedIncidence = _tailIncidence->val;
        // see setHstabTrimControl() for explanation
        _tailIncidenceCopy->val = _tailIncidence->val += ARCMIN;
        if (!_tail->setIncidence(_tailIncidence->val)) {
            _failureMsg = "Tail incidence out of bounds.";
            return;
        };
        runConfig(_config[CRUISE]);
        _tailIncidenceCopy->val = _tailIncidence->val = savedIncidence;
        _tail->setIncidence(_tailIncidence->val);

        float cpitch1 = _getPitch(_config[CRUISE]);

        // Now calculate:
        float awgt = 9.8f * _config[APPROACH].weight;

        float dragFactor = thrust / (thrust-cDragForce);
        float liftFactor = awgt / (awgt+alift);

        // Sanity:
        if(dragFactor <= 0) {
            _failureMsg = "dragFactor < 0 (drag > thrust)";
            break;
        }
        if(liftFactor <= 0) {
            _failureMsg = "liftFactor < 0";
            break;
        }

        // And the elevator control in the approach.  This works just
        // like the tail incidence computation (it's solving for the
        // same thing -- pitching moment -- by diddling a different
        // variable).
        const float ELEVDIDDLE = 0.001f;
        _approachElevator->val += ELEVDIDDLE;
        runConfig(_config[APPROACH]);
        _approachElevator->val -= ELEVDIDDLE;

        double apitch1 = _getPitch(_config[APPROACH]);

        // Now apply the values we just computed.  Note that the
        // "minor" variables are deferred until we get the lift/drag
        // numbers in the right ballpark.

        applyDragFactor(dragFactor);
        applyLiftRatio(liftFactor);

        // DON'T do the following until the above are sane
        if(normFactor(dragFactor) > _solverThreshold*1.0001
        || normFactor(liftFactor) > _solverThreshold*1.0001)
        {
            continue;
        }

        // OK, now we can adjust the minor variables:
        float aoaDelta = -clift0 * (ARCMIN/(clift1-clift0));
        float tailDelta = -cpitch0 * (ARCMIN/(cpitch1-cpitch0));
        // following is a hack against oszilation variables,
        // needs more research to get a fully understood - it works 
        if (_solverMode > 0) {
            tailDelta = _checkConvergence(prevTailDelta, tailDelta);
            prevTailDelta = tailDelta;
        }
        if (verbose) {
            fprintf(stdout,"%4d\t%f\t%f\t%f\t%f\n", _solutionIterations, aoaDelta, tailDelta, clift0, cpitch1);
        }
        _config[CRUISE].aoa += _solverDelta*aoaDelta;
        _tailIncidence->val += _solverDelta*tailDelta;
        
        _config[CRUISE].aoa = Math::clamp(_config[CRUISE].aoa, -0.175f, 0.175f);
        _tailIncidence->val = Math::clamp(_tailIncidence->val, -0.175f, 0.175f);

        if(abs(cDragForce/_config[CRUISE].weight) < _solverThreshold*0.0001 &&
          abs(alift/_config[APPROACH].weight) < _solverThreshold*0.0001 &&
          abs(aoaDelta) < _solverThreshold*.000017 &&
          abs(tailDelta) < _solverThreshold*.000017)
        {
            float elevDelta = -apitch0 * (ELEVDIDDLE/(apitch1-apitch0));
            if (verbose) {
                fprintf(stdout,"%4d dElev %f, ap0 %f,ap1 %f \n", _solutionIterations, elevDelta, apitch0, apitch1);
            }
            // If this finaly value is OK, then we're all done
            if(abs(elevDelta) < _solverThreshold*0.0001)
                break;

            // Otherwise, adjust and do the next iteration
            _approachElevator->val += _solverDelta * elevDelta;
            if(abs(_approachElevator->val) > 1) {
                _failureMsg = "Insufficient elevator to trim for approach.";
                break;
            }
        }
    }

    if(_dragFactor < 1e-06 || _dragFactor > 1e6) {
        _failureMsg = "Drag factor beyond reasonable bounds.";
        return;
    } else if(_liftRatio < 1e-04 || _liftRatio > 1e4) {
        _failureMsg = "Lift ratio beyond reasonable bounds.";
        return;
    } else if(Math::abs(_config[CRUISE].aoa) >= .17453293) {
        _failureMsg = "Cruise AoA > 10 degrees";
        return;
    } else if(Math::abs(_tailIncidence->val) >= .17453293) {
        _failureMsg = "Tail incidence > 10 degrees";
        return;
    }
    // if we have a property tree, export result from solver
    if (_wingsN != nullptr) {
        if (_tailIncidence->propHandle >= 0) {
            fgSetFloat(_controlMap.getProperty(_tailIncidence->propHandle)->name, _tailIncidence->val);
        }
    }
}

void Airplane::solveHelicopter(bool verbose)
{
    _solutionIterations = 0;
    _failureMsg = 0;
    if (getRotorgear()!=0)
    {
        Rotorgear* rg = getRotorgear();
        applyDragFactor(Math::pow(rg->getYasimDragFactor()/1000,
            1/_solverDelta));
        applyLiftRatio(Math::pow(rg->getYasimLiftFactor(),
            1/_solverDelta));
    }
    else
    //huh, no wing and no rotor? (_rotorgear is constructed, 
    //if a rotor is defined
    {
        applyDragFactor(Math::pow(15.7/1000, 1/_solverDelta));
        applyLiftRatio(Math::pow(104, 1/_solverDelta));
    }
    _config[CRUISE].state.setupState(0,0,0);
    _model.setState(&_config[CRUISE].state);
    setupWeights(APPROACH);
    _controlMap.reset();
    _model.getBody()->reset();
    _model.setStandardAtmosphere(_config[CRUISE].altitude);    
}

float Airplane::getCGMAC()
{ 
    if (_wing) {
      float cg[3];
      _model.getBody()->getCG(cg);
      return (_wing->getMACx() - cg[0]) / _wing->getMACLength();
    }
    return 0;
}

float Airplane::getWingSpan() const
{
    if (_wing == nullptr) return -1;
    return  _wing->getSpan();
}

float Airplane::getWingArea() const
{
    if (_wing == nullptr) return -1;
    return  _wing->getArea();
}

float Airplane::_getWingLoad(float mass) const
{
    if (_wing == nullptr) return -1;
    float area =  _wing->getArea();
    if (area == 0) return -1;
    else return mass / area;
}

/// get x-distance between CG and 25% MAC of w
float Airplane::_getWingLever(const Wing* w) const
{
    if (w == nullptr) return -1;
    float cg[3];
    _model.getCG(cg);
    // aerodynamic center is at 25% of MAC
    float ac = w->getMACx() - 0.25f * w->getMACLength();
    return ac - cg[0];
}

/// get max thrust with standard atmosphere at sea level
float Airplane::getMaxThrust()
{
    float wind[3] {0,0,0};
    float thrust[3] {0,0,0};
    float sum[3] {0,0,0};
    for(int i=0; i<_thrusters.size(); i++) {
        Thruster* t = ((ThrustRec*)_thrusters.get(i))->thruster;
        t->setWind(wind);
        t->setStandardAtmosphere(0);
        t->setThrottle(1);
        t->stabilize();
        t->getThrust(thrust);
        Math::add3(thrust, sum, sum);
    }    
    return sum[0];
}

float Airplane::getTailIncidence() const 
{
    if (_tailIncidence != nullptr) { 
        return _tailIncidence->val;        
    }
    else return 0;
}

float Airplane::getApproachElevator() const 
{
    if (_approachElevator != nullptr) { 
        return _approachElevator->val;        
    }
    else return 0;
}

}; // namespace yasim
