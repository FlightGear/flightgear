#include "Atmosphere.hpp"
#include "ControlMap.hpp"
#include "Gear.hpp"
#include "Math.hpp"
#include "Glue.hpp"
#include "RigidBody.hpp"
#include "Surface.hpp"
#include "Thruster.hpp"

#include "Airplane.hpp"
namespace yasim {

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
    _approachP = 0;
    _approachT = 0;
    _approachSpeed = 0;
    _approachAoA = 0;
    _approachWeight = 0;

    _dragFactor = 1;
    _liftRatio = 1;
    _cruiseAoA = 0;
    _tailIncidence = 0;
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
    for(i=0; i<_gears.size(); i++)
	delete (GearRec*)_gears.get(i);
    for(i=0; i<_surfs.size(); i++)
	delete (Surface*)_surfs.get(i);    
}

void Airplane::iterate(float dt)
{
    _model.iterate();

    // FIXME: Consume fuel
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
    Math::mul3(-9.8, out, out);

    // The regular acceleration
    float tmp[3];
    Math::mul3(-1, s->acc, tmp);
    Math::add3(tmp, out, out);

    // Convert to aircraft coordinates
    Math::vmul33(s->orient, out, out);

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

void Airplane::setGearState(bool down, float dt)
{
    int i;
    for(i=0; i<_gears.size(); i++) {
        GearRec* gr = (GearRec*)_gears.get(i);
        if(gr->time == 0) {
            // Non-extensible
            gr->gear->setExtension(1);
            gr->surf->setXDrag(1);
            gr->surf->setYDrag(1);
            gr->surf->setZDrag(1);
            continue;
        }

        float diff = dt / gr->time;
        if(!down) diff = -diff;
        float ext = gr->gear->getExtension() + diff;
        if(ext < 0) ext = 0;
        if(ext > 1) ext = 1;

        gr->gear->setExtension(ext);
        gr->surf->setXDrag(ext);
        gr->surf->setYDrag(ext);
        gr->surf->setZDrag(ext);
    }
}

void Airplane::setApproach(float speed, float altitude)
{
    // The zero AoA will become a calculated stall AoA in compile()
    setApproach(speed, altitude, 0);
}

void Airplane::setApproach(float speed, float altitude, float aoa)
{
    _approachSpeed = speed;
    _approachP = Atmosphere::getStdPressure(altitude);
    _approachT = Atmosphere::getStdTemperature(altitude);
    _approachAoA = aoa;
}
 
void Airplane::setCruise(float speed, float altitude)
{
    _cruiseSpeed = speed;
    _cruiseP = Atmosphere::getStdPressure(altitude);
    _cruiseT = Atmosphere::getStdTemperature(altitude);
    _cruiseAoA = 0;
    _tailIncidence = 0;
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

int Airplane::numTanks()
{
    return _tanks.size();
}

float Airplane::getFuel(int tank)
{
    return ((Tank*)_tanks.get(tank))->fill;
}

float Airplane::getFuelDensity(int tank)
{
    return ((Tank*)_tanks.get(tank))->density;
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
                           float taper, float mid)
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

void Airplane::addGear(Gear* gear, float transitionTime)
{
    GearRec* g = new GearRec();
    g->gear = gear;
    g->surf = 0;
    g->time = transitionTime;
    _gears.add(g);
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

char* Airplane::getFailureMsg()
{
    return _failureMsg;
}

int Airplane::getSolutionIterations()
{
    return _solutionIterations;
}

void Airplane::setupState(float aoa, float speed, State* s)
{
    float cosAoA = Math::cos(aoa);
    float sinAoA = Math::sin(aoa);
    s->orient[0] =  cosAoA; s->orient[1] = 0; s->orient[2] = sinAoA;
    s->orient[3] =       0; s->orient[4] = 1; s->orient[5] =      0;
    s->orient[6] = -sinAoA; s->orient[7] = 0; s->orient[8] = cosAoA;

    s->v[0] = speed; s->v[1] = 0; s->v[2] = 0;

    int i;
    for(i=0; i<3; i++)
	s->pos[i] = s->rot[i] = s->acc[i] = s->racc[i] = 0;

    // Put us 1m above the origin, or else the gravity computation in
    // Model goes nuts
    s->pos[2] = 1;
}

float Airplane::compileWing(Wing* w)
{
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

float Airplane::compileFuselage(Fuselage* f)
{
    float wgt = 0;
    float fwd[3];
    Math::sub3(f->front, f->back, fwd);
    float len = Math::mag3(fwd);
    float wid = f->width;
    int segs = (int)Math::ceil(len/wid);
    float segWgt = len*wid/segs;
    int j;
    for(j=0; j<segs; j++) {
        float frac = (j+0.5) / segs;

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
        s->setYDrag(sideDrag);
        s->setZDrag(sideDrag);
        s->setTotalDrag(scale*segWgt);

        // FIXME: fails for fuselages aligned along the Y axis
        float o[9];
        float *x=o, *y=o+3, *z=o+6; // nicknames for the axes
        Math::unit3(fwd, x);
        y[0] = 0; y[1] = 1; y[2] = 0;
        Math::cross3(x, y, z);
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

void Airplane::compile()
{
    double ground[3];
    ground[0] = 0; ground[1] = 0; ground[2] = 1;
    _model.setGroundPlane(ground, -100000);

    RigidBody* body = _model.getBody();
    int firstMass = body->numMasses();

    // Generate the point masses for the plane.  Just use unitless
    // numbers for a first pass, then go back through and rescale to
    // make the weight right.
    float aeroWgt = 0;

    // The Wing objects
    aeroWgt += compileWing(_wing);
    aeroWgt += compileWing(_tail);
    int i;
    for(i=0; i<_vstabs.size(); i++) {
        aeroWgt += compileWing((Wing*)_vstabs.get(i)); 
    }
    
    // The fuselage(s)
    for(i=0; i<_fuselages.size(); i++) {
        aeroWgt += compileFuselage((Fuselage*)_fuselages.get(i));
    }

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
    _cruiseWeight = _emptyWeight + totalFuel*0.5;
    _approachWeight = _emptyWeight + totalFuel*0.2;

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
    float gepos[3];
    float gespan = _wing->getGroundEffect(gepos);
    _model.setGroundEffect(gepos, gespan, .3);

    solveGear();
    solve();

    // Drop the gear (use a really big dt)
    setGearState(true, 1000000);
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
        gr->wgt = 1/(0.5+Math::sqrt(pos[0]*pos[0] + pos[1]*pos[1]));
        total += gr->wgt;
    }

    // Renormalize so they sum to 1
    for(i=0; i<_gears.size(); i++)
        ((GearRec*)_gears.get(i))->wgt /= total;
    
    // The force at max compression should be sufficient to stop a
    // plane moving downwards at 3x the approach descent rate.  Assume
    // a 3 degree approach.
    float descentRate = 3*_approachSpeed/19.1;

    // Spread the kinetic energy according to the gear weights.  This
    // will results in an equal compression fraction (not distance) of
    // each gear.
    float energy = 0.5*_approachWeight*descentRate*descentRate;

    for(i=0; i<_gears.size(); i++) {
        GearRec* gr = (GearRec*)_gears.get(i);
        float e = energy * gr->wgt;
        float comp[3];
        gr->gear->getCompression(comp);
        float len = Math::mag3(comp);

        // Energy in a spring: e = 0.5 * k * len^2
        float k = 2 * e / (len*len);

        gr->gear->setSpring(k);

        // Critically damped (too damped, too!)
        gr->gear->setDamping(2*Math::sqrt(k*_approachWeight*gr->wgt));

        // These are pretty generic
        gr->gear->setStaticFriction(0.8);
        gr->gear->setDynamicFriction(0.7);
    }
}

void Airplane::stabilizeThrust()
{
    int i;
    for(i=0; i<_thrusters.size(); i++)
	_model.getThruster(i)->stabilize();
}

void Airplane::runCruise()
{
    setupState(_cruiseAoA, _cruiseSpeed, &_cruiseState);
    _model.setState(&_cruiseState);
    _model.setAir(_cruiseP, _cruiseT);

    // The control configuration
    _controls.reset();
    int i;
    for(i=0; i<_cruiseControls.size(); i++) {
	Control* c = (Control*)_cruiseControls.get(i);
	_controls.setInput(c->control, c->val);
    }
    _controls.applyControls();

    // The local wind
    float wind[3];
    Math::mul3(-1, _cruiseState.v, wind);
    Math::vmul33(_cruiseState.orient, wind, wind);
 
    // Gear are up (if they're non-retractable, this is a noop)
    setGearState(false, 100000);
    
    // Cruise is by convention at 50% tank capacity
    setFuelFraction(0.5);
   
    // Set up the thruster parameters and iterate until the thrust
    // stabilizes.
    for(i=0; i<_thrusters.size(); i++) {
	Thruster* t = ((ThrustRec*)_thrusters.get(i))->thruster;
	t->setWind(wind);
	t->setAir(_cruiseP, _cruiseT);
    }
    stabilizeThrust();

    // Precompute thrust in the model, and calculate aerodynamic forces
    _model.getBody()->reset();
    _model.initIteration();
    _model.calcForces(&_cruiseState);
}

void Airplane::runApproach()
{
    setupState(_approachAoA, _approachSpeed, &_approachState);
    _model.setState(&_approachState);
    _model.setAir(_approachP, _approachT);

    // The control configuration
    _controls.reset();
    int i;
    for(i=0; i<_approachControls.size(); i++) {
	Control* c = (Control*)_approachControls.get(i);
	_controls.setInput(c->control, c->val);
    }
    _controls.applyControls();

    // The local wind
    float wind[3];
    Math::mul3(-1, _approachState.v, wind);
    Math::vmul33(_approachState.orient, wind, wind);
    
    // Approach is by convention at 20% tank capacity
    setFuelFraction(0.2);

    // Gear are down
    setGearState(true, 100000);

    // Run the thrusters until they get to a stable setting.  FIXME:
    // this is lots of wasted work.
    for(i=0; i<_thrusters.size(); i++) {
	Thruster* t = ((ThrustRec*)_thrusters.get(i))->thruster;
	t->setWind(wind);
	t->setAir(_approachP, _approachT);
    }
    stabilizeThrust();

    // Precompute thrust in the model, and calculate aerodynamic forces
    _model.getBody()->reset();
    _model.initIteration();
    _model.calcForces(&_approachState);
}

void Airplane::applyDragFactor(float factor)
{
    float applied = Math::sqrt(factor);
    _dragFactor *= applied;
    _wing->setDragScale(_wing->getDragScale() * applied);
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
    float applied = Math::sqrt(factor);
    _liftRatio *= applied;
    _wing->setLiftRatio(_wing->getLiftRatio() * applied);
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
    static const float ARCMIN = 0.0002909;

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
	float thrust = tmp[0];

	_model.getBody()->getAccel(tmp);
	float xforce = _cruiseWeight * tmp[0];
	float clift0 = _cruiseWeight * tmp[2];

	_model.getBody()->getAngularAccel(tmp);
	float pitch0 = tmp[1];

	// Run an approach iteration, and do likewise
	runApproach();

	_model.getBody()->getAccel(tmp);
	float alift = _approachWeight * tmp[2];

	// Modify the cruise AoA a bit to get a derivative
	_cruiseAoA += ARCMIN;
	runCruise();
	_cruiseAoA -= ARCMIN;

	_model.getBody()->getAccel(tmp);
	float clift1 = _cruiseWeight * tmp[2];

	// Do the same with the tail incidence
	_tail->setIncidence(_tailIncidence + ARCMIN);
	runCruise();
	_tail->setIncidence(_tailIncidence);

	_model.getBody()->getAngularAccel(tmp);
	float pitch1 = tmp[1];

	// Now calculate:
	float awgt = 9.8 * _approachWeight;

	float dragFactor = thrust / (thrust-xforce);
	float liftFactor = awgt / (awgt+alift);
	float aoaDelta = -clift0 * (ARCMIN/(clift1-clift0));
	float tailDelta = -pitch0 * (ARCMIN/(pitch1-pitch0));

        // Sanity:
        if(dragFactor <= 0) {
            _failureMsg = "Zero or negative drag adjustment.";
            return;
        } else if(liftFactor <= 0) {
            _failureMsg = "Zero or negative lift adjustment.";
            return;
        }

	// And apply:
	applyDragFactor(dragFactor);
	applyLiftRatio(liftFactor);

	// DON'T do the following until the above are sane
	if(normFactor(dragFactor) > 1.1
	   || normFactor(liftFactor) > 1.1)
	{
	    continue;
	}

	// OK, now we can adjust the minor variables
	_cruiseAoA += 0.5*aoaDelta;
	_tailIncidence += 0.5*tailDelta;
	
	_cruiseAoA = clamp(_cruiseAoA, -.174, .174);
	_tailIncidence = clamp(_tailIncidence, -.174, .174);

        if(dragFactor < 1.00001 && liftFactor < 1.00001 &&
           aoaDelta < .000017   && tailDelta < .000017)
        {
            break;
        }
    }

    if(_dragFactor < 1e-06 || _dragFactor > 1e6) {
	_failureMsg = "Drag factor beyond reasonable bounds.";
	return;
    } else if(_liftRatio < 1e-04 || _liftRatio > 1e4) {
	_failureMsg = "Lift ratio beyond reasonable bounds.";
	return;
    } else if(Math::abs(_cruiseAoA) >= .174) {
	_failureMsg = "Cruise AoA > 10 degrees";
	return;
    } else if(Math::abs(_tailIncidence) >= .174) {
	_failureMsg = "Tail incidence > 10 degrees";
	return;
    }
}
}; // namespace yasim
