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

namespace yasim {

// gadgets
inline float norm(float f) { return f<1 ? 1/f : f; }
inline float abs(float f) { return f<0 ? -f : f; }

// Solver threshold.  How close to the solution are we trying
// to get?  Trying too hard can result in oscillations about
// the correct solution, which is bad.  Stick this in as a
// compile time constant for now, and consider making it
// settable per-model.
const float STHRESH = 1;

// How slowly do we change values in the solver.  Too slow, and
// the solution converges very slowly.  Too fast, and it can
// oscillate.
const float SOLVE_TWEAK = 0.3226;

Airplane::Airplane()
{
    _emptyWeight = 0;
    _pilotPos[0] = _pilotPos[1] = _pilotPos[2] = 0;
    _wing = 0;
    _tail = 0;
    _ballast = 0;
    _cruiseP = 0;
    _cruiseT = 0;
    _cruiseSpeed = 0;
    _cruiseWeight = 0;
    _cruiseGlideAngle = 0;
    _approachP = 0;
    _approachT = 0;
    _approachSpeed = 0;
    _approachAoA = 0;
    _approachWeight = 0;
    _approachGlideAngle = 0;

    _dragFactor = 1;
    _liftRatio = 1;
    _cruiseAoA = 0;
    _tailIncidence = 0;

    _failureMsg = 0;
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
    for(i=0; i<_cruiseControls.size(); i++)
        delete (Control*)_cruiseControls.get(i);
    for(i=0; i<_approachControls.size(); i++) {
        Control* c = (Control*)_approachControls.get(i);
        if(c != &_approachElevator)
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

ControlMap* Airplane::getControlMap()
{
    return &_controls;
}

Model* Airplane::getModel()
{
    return &_model;
}

void Airplane::getPilotAccel(float* out)
{
    State* s = _model.getState();

    // Gravity
    Glue::geodUp(s->pos, out);
    Math::mul3(-9.8f, out, out);
    Math::vmul33(s->orient, out, out);
    out[0] = -out[0];

    // The regular acceleration
    float tmp[3];
    // Convert to aircraft coordinates
    Math::vmul33(s->orient, s->acc, tmp);
    tmp[1] = -tmp[1];
    tmp[2] = -tmp[2];

    Math::add3(tmp, out, out);

    // FIXME: rotational & centripetal acceleration needed
}

void Airplane::setPilotPos(float* pos)
{
    int i;
    for(i=0; i<3; i++) _pilotPos[i] = pos[i];
}

void Airplane::getPilotPos(float* out)
{
    int i;
    for(i=0; i<3; i++) out[i] = _pilotPos[i];
}

int Airplane::numGear()
{
    return _gears.size();
}

Gear* Airplane::getGear(int g)
{
    return ((GearRec*)_gears.get(g))->gear;
}

Hook* Airplane::getHook()
{
    return _model.getHook();
}

Launchbar* Airplane::getLaunchbar()
{
    return _model.getLaunchbar();
}

Rotorgear* Airplane::getRotorgear()
{
    return _model.getRotorgear();
}

void Airplane::updateGearState()
{
    for(int i=0; i<_gears.size(); i++) {
        GearRec* gr = (GearRec*)_gears.get(i);
        float ext = gr->gear->getExtension();

        gr->surf->setXDrag(ext);
        gr->surf->setYDrag(ext);
        gr->surf->setZDrag(ext);
    }
}

void Airplane::setApproach(float speed, float altitude, float aoa, float fuel, float gla)
{
    _approachSpeed = speed;
    _approachP = Atmosphere::getStdPressure(altitude);
    _approachT = Atmosphere::getStdTemperature(altitude);
    _approachAoA = aoa;
    _approachFuel = fuel;
    _approachGlideAngle = gla;
}
 
void Airplane::setCruise(float speed, float altitude, float fuel, float gla)
{
    _cruiseSpeed = speed;
    _cruiseP = Atmosphere::getStdPressure(altitude);
    _cruiseT = Atmosphere::getStdTemperature(altitude);
    _cruiseAoA = 0;
    _tailIncidence = 0;
    _cruiseFuel = fuel;
    _cruiseGlideAngle = gla;
}

void Airplane::setElevatorControl(int control)
{
    _approachElevator.control = control;
    _approachElevator.val = 0;
    _approachControls.add(&_approachElevator);
}

void Airplane::addApproachControl(int control, float val)
{
    Control* c = new Control();
    c->control = control;
    c->val = val;
    _approachControls.add(c);
}

void Airplane::addCruiseControl(int control, float val)
{
    Control* c = new Control();
    c->control = control;
    c->val = val;
    _cruiseControls.add(c);
}

void Airplane::addSolutionWeight(bool approach, int idx, float wgt)
{
    SolveWeight* w = new SolveWeight();
    w->approach = approach;
    w->idx = idx;
    w->wgt = wgt;
    _solveWeights.add(w);
}

int Airplane::numTanks()
{
    return _tanks.size();
}

float Airplane::getFuel(int tank)
{
    return ((Tank*)_tanks.get(tank))->fill;
}

float Airplane::setFuel(int tank, float fuel)
{
    return ((Tank*)_tanks.get(tank))->fill = fuel;
}

float Airplane::getFuelDensity(int tank)
{
    return ((Tank*)_tanks.get(tank))->density;
}

float Airplane::getTankCapacity(int tank)
{
    return ((Tank*)_tanks.get(tank))->cap;
}

void Airplane::setWeight(float weight)
{
    _emptyWeight = weight;
}

void Airplane::setWing(Wing* wing)
{
    _wing = wing;
}

void Airplane::setTail(Wing* tail)
{
    _tail = tail;
}

void Airplane::addVStab(Wing* vstab)
{
    _vstabs.add(vstab);
}

void Airplane::addFuselage(float* front, float* back, float width,
                           float taper, float mid, 
                           float cx, float cy, float cz, float idrag)
{
    Fuselage* f = new Fuselage();
    int i;
    for(i=0; i<3; i++) {
	f->front[i] = front[i];
	f->back[i]  = back[i];
    }
    f->width = width;
    f->taper = taper;
    f->mid = mid;
    f->_cx=cx;
    f->_cy=cy;
    f->_cz=cz;
    f->_idrag=idrag;
    _fuselages.add(f);
}

int Airplane::addTank(float* pos, float cap, float density)
{
    Tank* t = new Tank();
    int i;
    for(i=0; i<3; i++) t->pos[i] = pos[i];
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

void Airplane::addHook(Hook* hook)
{
    _model.addHook(hook);
}

void Airplane::addHitch(Hitch* hitch)
{
    _model.addHitch(hitch);
}

void Airplane::addLaunchbar(Launchbar* launchbar)
{
    _model.addLaunchbar(launchbar);
}

void Airplane::addThruster(Thruster* thruster, float mass, float* cg)
{
    ThrustRec* t = new ThrustRec();
    t->thruster = thruster;
    t->mass = mass;
    int i;
    for(i=0; i<3; i++) t->cg[i] = cg[i];
    _thrusters.add(t);
}

void Airplane::addBallast(float* pos, float mass)
{
    _model.getBody()->addMass(mass, pos);
    _ballast += mass;
}

int Airplane::addWeight(float* pos, float size)
{
    WeightRec* wr = new WeightRec();
    wr->handle = _model.getBody()->addMass(0, pos);

    wr->surf = new Surface();
    wr->surf->setPosition(pos);
    wr->surf->setTotalDrag(size*size);
    _model.addSurface(wr->surf);
    _surfs.add(wr->surf);

    return _weights.add(wr);
}

void Airplane::setWeight(int handle, float mass)
{
    WeightRec* wr = (WeightRec*)_weights.get(handle);

    _model.getBody()->setMass(wr->handle, mass);

    // Kill the aerodynamic drag if the mass is exactly zero.  This is
    // how we simulate droppable stores.
    if(mass == 0) {
	wr->surf->setXDrag(0);
	wr->surf->setYDrag(0);
	wr->surf->setZDrag(0);
    } else {
	wr->surf->setXDrag(1);
	wr->surf->setYDrag(1);
	wr->surf->setZDrag(1);
    }
}

void Airplane::setFuelFraction(float frac)
{
    int i;
    for(i=0; i<_tanks.size(); i++) {
        Tank* t = (Tank*)_tanks.get(i);
        t->fill = frac * t->cap;
        _model.getBody()->setMass(t->handle, t->cap * frac);
    }
}

float Airplane::getDragCoefficient()
{
    return _dragFactor;
}

float Airplane::getLiftRatio()
{
    return _liftRatio;
}

float Airplane::getCruiseAoA()
{
    return _cruiseAoA;
}

float Airplane::getTailIncidence()
{
    return _tailIncidence;
}

const char* Airplane::getFailureMsg()
{
    return _failureMsg;
}

int Airplane::getSolutionIterations()
{
    return _solutionIterations;
}

void Airplane::setupState(float aoa, float speed, float gla, State* s)
{
    float cosAoA = Math::cos(aoa);
    float sinAoA = Math::sin(aoa);
    s->orient[0] =  cosAoA; s->orient[1] = 0; s->orient[2] = sinAoA;
    s->orient[3] =       0; s->orient[4] = 1; s->orient[5] =      0;
    s->orient[6] = -sinAoA; s->orient[7] = 0; s->orient[8] = cosAoA;

    s->v[0] = speed*Math::cos(gla); s->v[1] = -speed*Math::sin(gla); s->v[2] = 0;

    int i;
    for(i=0; i<3; i++)
	s->pos[i] = s->rot[i] = s->acc[i] = s->racc[i] = 0;

    // Put us 1m above the origin, or else the gravity computation in
    // Model goes nuts
    s->pos[2] = 1;
}

void Airplane::addContactPoint(float* pos)
{
    ContactRec* c = new ContactRec;
    c->gear = 0;
    c->p[0] = pos[0];
    c->p[1] = pos[1];
    c->p[2] = pos[2];
    _contacts.add(c);
}

float Airplane::compileWing(Wing* w)
{
    // The tip of the wing is a contact point
    float tip[3];
    w->getTip(tip);
    addContactPoint(tip);
    if(w->isMirrored()) {
        tip[1] *= -1;
        addContactPoint(tip);
    }

    // Make sure it's initialized.  The surfaces will pop out with
    // total drag coefficients equal to their areas, which is what we
    // want.
    w->compile();

    float wgt = 0;
    int i;
    for(i=0; i<w->numSurfaces(); i++) {
        Surface* s = (Surface*)w->getSurface(i);

	float td = s->getTotalDrag();
	s->setTotalDrag(td);

        _model.addSurface(s);

        float mass = w->getSurfaceWeight(i);
        mass = mass * Math::sqrt(mass);
        float pos[3];
        s->getPosition(pos);
        _model.getBody()->addMass(mass, pos);
        wgt += mass;
    }
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
        else
            scale = f->taper+(1-f->taper) * (frac - f->mid) / (1 - f->mid);

        // Where are we?
        float pos[3];
        Math::mul3(frac, fwd, pos);
        Math::add3(f->back, pos, pos);

        // _Mass_ weighting goes as surface area^(3/2)
        float mass = scale*segWgt * Math::sqrt(scale*segWgt);
        _model.getBody()->addMass(mass, pos);
        wgt += mass;

        // Make a Surface too
        Surface* s = new Surface();
        s->setPosition(pos);
	float sideDrag = len/wid;
        s->setYDrag(sideDrag*f->_cy);
        s->setZDrag(sideDrag*f->_cz);
        s->setTotalDrag(scale*segWgt*f->_cx);
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
        _surfs.add(s);
    }
    return wgt;
}

// FIXME: should probably add a mass for the gear, too
void Airplane::compileGear(GearRec* gr)
{
    Gear* g = gr->gear;

    // Make a Surface object for the aerodynamic behavior
    Surface* s = new Surface();
    gr->surf = s;

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

    s->setPosition(pos);
    s->setTotalDrag(length*length);

    _model.addGear(g);
    _model.addSurface(s);
    _surfs.add(s);
}

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

    int i;
    for(i=0; i<_contacts.size(); i++) {
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

void Airplane::compile()
{
    RigidBody* body = _model.getBody();
    int firstMass = body->numMasses();

    // Generate the point masses for the plane.  Just use unitless
    // numbers for a first pass, then go back through and rescale to
    // make the weight right.
    float aeroWgt = 0;

    // The Wing objects
    if (_wing)
      aeroWgt += compileWing(_wing);
    if (_tail)
      aeroWgt += compileWing(_tail);
    int i;
    for(i=0; i<_vstabs.size(); i++)
        aeroWgt += compileWing((Wing*)_vstabs.get(i)); 


    // The fuselage(s)
    for(i=0; i<_fuselages.size(); i++)
        aeroWgt += compileFuselage((Fuselage*)_fuselages.get(i));

    // Count up the absolute weight we have
    float nonAeroWgt = _ballast;
    for(i=0; i<_thrusters.size(); i++)
        nonAeroWgt += ((ThrustRec*)_thrusters.get(i))->mass;

    // Rescale to the specified empty weight
    float wscale = (_emptyWeight-nonAeroWgt)/aeroWgt;
    for(i=firstMass; i<body->numMasses(); i++)
        body->setMass(i, body->getMass(i)*wscale);

    // Add the thruster masses
    for(i=0; i<_thrusters.size(); i++) {
        ThrustRec* t = (ThrustRec*)_thrusters.get(i);
        body->addMass(t->mass, t->cg);
    }

    // Add the tanks, empty for now.
    float totalFuel = 0;
    for(i=0; i<_tanks.size(); i++) { 
        Tank* t = (Tank*)_tanks.get(i); 
        t->handle = body->addMass(0, t->pos);
        totalFuel += t->cap;
    }
    _cruiseWeight = _emptyWeight + totalFuel*_cruiseFuel;
    _approachWeight = _emptyWeight + totalFuel*_approachFuel;

    body->recalc();

    // Add surfaces for the landing gear.
    for(i=0; i<_gears.size(); i++)
        compileGear((GearRec*)_gears.get(i));

    // The Thruster objects
    for(i=0; i<_thrusters.size(); i++) {
        ThrustRec* tr = (ThrustRec*)_thrusters.get(i);
        tr->handle = _model.addThruster(tr->thruster);
    }

    // Ground effect
    if(_wing) {
        float gepos[3];
        float gespan = 0;
        gespan = _wing->getGroundEffect(gepos);
        _model.setGroundEffect(gepos, gespan, 0.15f);
    }

    // solve function below resets failure message
    // so check if we have any problems and abort here
    if (_failureMsg) return;

    solveGear();
    if(_wing && _tail) solve();
    else
    {
       // The rotor(s) mass:
       compileRotorgear(); 
       solveHelicopter();
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
    float descentRate = 2.0f*_approachSpeed/19.1f;

    // Spread the kinetic energy according to the gear weights.  This
    // will results in an equal compression fraction (not distance) of
    // each gear.
    float energy = 0.5f*_approachWeight*descentRate*descentRate;

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
        gr->gear->setDamping(2*Math::sqrt(k*_approachWeight*gr->wgt)
                             * gr->gear->getDamping());
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
    int i;
    for(i=0; i<_thrusters.size(); i++)
	_model.getThruster(i)->stabilize();
}

void Airplane::setupWeights(bool isApproach)
{
    int i;
    for(i=0; i<_weights.size(); i++)
        setWeight(i, 0);
    for(i=0; i<_solveWeights.size(); i++) {
        SolveWeight* w = (SolveWeight*)_solveWeights.get(i);
        if(w->approach == isApproach)
            setWeight(w->idx, w->wgt);
    }
}

void Airplane::runCruise()
{
    setupState(_cruiseAoA, _cruiseSpeed,_cruiseGlideAngle, &_cruiseState);
    _model.setState(&_cruiseState);
    _model.setAir(_cruiseP, _cruiseT,
                  Atmosphere::calcStdDensity(_cruiseP, _cruiseT));

    // The control configuration
    _controls.reset();
    int i;
    for(i=0; i<_cruiseControls.size(); i++) {
	Control* c = (Control*)_cruiseControls.get(i);
	_controls.setInput(c->control, c->val);
    }
    _controls.applyControls(1000000); // Huge dt value

    // The local wind
    float wind[3];
    Math::mul3(-1, _cruiseState.v, wind);
    Math::vmul33(_cruiseState.orient, wind, wind);
 
    setFuelFraction(_cruiseFuel);
    setupWeights(false);
   
    // Set up the thruster parameters and iterate until the thrust
    // stabilizes.
    for(i=0; i<_thrusters.size(); i++) {
	Thruster* t = ((ThrustRec*)_thrusters.get(i))->thruster;
	t->setWind(wind);
	t->setAir(_cruiseP, _cruiseT,
                  Atmosphere::calcStdDensity(_cruiseP, _cruiseT));
    }
    stabilizeThrust();

    updateGearState();

    // Precompute thrust in the model, and calculate aerodynamic forces
    _model.getBody()->recalc();
    _model.getBody()->reset();
    _model.initIteration();
    _model.calcForces(&_cruiseState);
}

void Airplane::runApproach()
{
    setupState(_approachAoA, _approachSpeed,_approachGlideAngle, &_approachState);
    _model.setState(&_approachState);
    _model.setAir(_approachP, _approachT,
                  Atmosphere::calcStdDensity(_approachP, _approachT));

    // The control configuration
    _controls.reset();
    int i;
    for(i=0; i<_approachControls.size(); i++) {
	Control* c = (Control*)_approachControls.get(i);
	_controls.setInput(c->control, c->val);
    }
    _controls.applyControls(1000000);

    // The local wind
    float wind[3];
    Math::mul3(-1, _approachState.v, wind);
    Math::vmul33(_approachState.orient, wind, wind);
    
    setFuelFraction(_approachFuel);

    setupWeights(true);

    // Run the thrusters until they get to a stable setting.  FIXME:
    // this is lots of wasted work.
    for(i=0; i<_thrusters.size(); i++) {
	Thruster* t = ((ThrustRec*)_thrusters.get(i))->thruster;
	t->setWind(wind);
	t->setAir(_approachP, _approachT,
                  Atmosphere::calcStdDensity(_approachP, _approachT));
    }
    stabilizeThrust();

    updateGearState();

    // Precompute thrust in the model, and calculate aerodynamic forces
    _model.getBody()->recalc();
    _model.getBody()->reset();
    _model.initIteration();
    _model.calcForces(&_approachState);
}

void Airplane::applyDragFactor(float factor)
{
    float applied = Math::pow(factor, SOLVE_TWEAK);
    _dragFactor *= applied;
    if(_wing)
      _wing->setDragScale(_wing->getDragScale() * applied);
    if(_tail)
      _tail->setDragScale(_tail->getDragScale() * applied);
    int i;
    for(i=0; i<_vstabs.size(); i++) {
	Wing* w = (Wing*)_vstabs.get(i);
	w->setDragScale(w->getDragScale() * applied);
    }
    for(i=0; i<_surfs.size(); i++) {
	Surface* s = (Surface*)_surfs.get(i);
	s->setTotalDrag(s->getTotalDrag() * applied);
    }
}

void Airplane::applyLiftRatio(float factor)
{
    float applied = Math::pow(factor, SOLVE_TWEAK);
    _liftRatio *= applied;
    if(_wing)
      _wing->setLiftRatio(_wing->getLiftRatio() * applied);
    if(_tail)
      _tail->setLiftRatio(_tail->getLiftRatio() * applied);
    int i;
    for(i=0; i<_vstabs.size(); i++) {
        Wing* w = (Wing*)_vstabs.get(i);
        w->setLiftRatio(w->getLiftRatio() * applied);
    }
}

float Airplane::clamp(float val, float min, float max)
{
    if(val < min) return min;
    if(val > max) return max;
    return val;
}

float Airplane::normFactor(float f)
{
    if(f < 0) f = -f;
    if(f < 1) f = 1/f;
    return f;
}

void Airplane::solve()
{
    static const float ARCMIN = 0.0002909f;

    float tmp[3];
    _solutionIterations = 0;
    _failureMsg = 0;

    while(1) {
        if(_solutionIterations++ > 10000) { 
            _failureMsg = "Solution failed to converge after 10000 iterations";
            return;
        }

	// Run an iteration at cruise, and extract the needed numbers:
	runCruise();

	_model.getThrust(tmp);
        float thrust = tmp[0] + _cruiseWeight * Math::sin(_cruiseGlideAngle) * 9.81;

	_model.getBody()->getAccel(tmp);
        Math::tmul33(_cruiseState.orient, tmp, tmp);
	float xforce = _cruiseWeight * tmp[0];
	float clift0 = _cruiseWeight * tmp[2];

	_model.getBody()->getAngularAccel(tmp);
        Math::tmul33(_cruiseState.orient, tmp, tmp);
	float pitch0 = tmp[1];

	// Run an approach iteration, and do likewise
	runApproach();

	_model.getBody()->getAngularAccel(tmp);
        Math::tmul33(_approachState.orient, tmp, tmp);
	double apitch0 = tmp[1];

	_model.getBody()->getAccel(tmp);
        Math::tmul33(_approachState.orient, tmp, tmp);
	float alift = _approachWeight * tmp[2];

	// Modify the cruise AoA a bit to get a derivative
	_cruiseAoA += ARCMIN;
	runCruise();
	_cruiseAoA -= ARCMIN;

	_model.getBody()->getAccel(tmp);
        Math::tmul33(_cruiseState.orient, tmp, tmp);
	float clift1 = _cruiseWeight * tmp[2];

	// Do the same with the tail incidence
	_tail->setIncidence(_tailIncidence + ARCMIN);
	runCruise();
	_tail->setIncidence(_tailIncidence);

	_model.getBody()->getAngularAccel(tmp);
        Math::tmul33(_cruiseState.orient, tmp, tmp);
	float pitch1 = tmp[1];

	// Now calculate:
	float awgt = 9.8f * _approachWeight;

	float dragFactor = thrust / (thrust-xforce);
	float liftFactor = awgt / (awgt+alift);
	float aoaDelta = -clift0 * (ARCMIN/(clift1-clift0));
	float tailDelta = -pitch0 * (ARCMIN/(pitch1-pitch0));

        // Sanity:
        if(dragFactor <= 0 || liftFactor <= 0)
            break;

        // And the elevator control in the approach.  This works just
        // like the tail incidence computation (it's solving for the
        // same thing -- pitching moment -- by diddling a different
        // variable).
        const float ELEVDIDDLE = 0.001f;
        _approachElevator.val += ELEVDIDDLE;
        runApproach();
        _approachElevator.val -= ELEVDIDDLE;

	_model.getBody()->getAngularAccel(tmp);
        Math::tmul33(_approachState.orient, tmp, tmp);
	double apitch1 = tmp[1];
        float elevDelta = -apitch0 * (ELEVDIDDLE/(apitch1-apitch0));

        // Now apply the values we just computed.  Note that the
        // "minor" variables are deferred until we get the lift/drag
        // numbers in the right ballpark.

	applyDragFactor(dragFactor);
	applyLiftRatio(liftFactor);

	// DON'T do the following until the above are sane
	if(normFactor(dragFactor) > STHRESH*1.0001
	   || normFactor(liftFactor) > STHRESH*1.0001)
	{
	    continue;
	}

	// OK, now we can adjust the minor variables:
	_cruiseAoA += SOLVE_TWEAK*aoaDelta;
	_tailIncidence += SOLVE_TWEAK*tailDelta;
	
	_cruiseAoA = clamp(_cruiseAoA, -0.175f, 0.175f);
	_tailIncidence = clamp(_tailIncidence, -0.175f, 0.175f);

        if(abs(xforce/_cruiseWeight) < STHRESH*0.0001 &&
           abs(alift/_approachWeight) < STHRESH*0.0001 &&
           abs(aoaDelta) < STHRESH*.000017 &&
           abs(tailDelta) < STHRESH*.000017)
        {
            // If this finaly value is OK, then we're all done
            if(abs(elevDelta) < STHRESH*0.0001)
                break;

            // Otherwise, adjust and do the next iteration
            _approachElevator.val += SOLVE_TWEAK * elevDelta;
            if(abs(_approachElevator.val) > 1) {
                _failureMsg = "Insufficient elevator to trim for approach";
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
    } else if(Math::abs(_cruiseAoA) >= .17453293) {
	_failureMsg = "Cruise AoA > 10 degrees";
	return;
    } else if(Math::abs(_tailIncidence) >= .17453293) {
	_failureMsg = "Tail incidence > 10 degrees";
	return;
    }
}

void Airplane::solveHelicopter()
{
    _solutionIterations = 0;
    _failureMsg = 0;
    if (getRotorgear()!=0)
    {
        Rotorgear* rg = getRotorgear();
        applyDragFactor(Math::pow(rg->getYasimDragFactor()/1000,
            1/SOLVE_TWEAK));
        applyLiftRatio(Math::pow(rg->getYasimLiftFactor(),
            1/SOLVE_TWEAK));
    }
    else
    //huh, no wing and no rotor? (_rotorgear is constructed, 
    //if a rotor is defined
    {
        applyDragFactor(Math::pow(15.7/1000, 1/SOLVE_TWEAK));
        applyLiftRatio(Math::pow(104, 1/SOLVE_TWEAK));
    }
    setupState(0,0,0, &_cruiseState);
    _model.setState(&_cruiseState);
    setupWeights(true);
    _controls.reset();
    _model.getBody()->reset();
    _model.setAir(_cruiseP, _cruiseT,
                  Atmosphere::calcStdDensity(_cruiseP, _cruiseT));
    
}

}; // namespace yasim
