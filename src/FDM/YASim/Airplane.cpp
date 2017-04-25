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
    _approachConfig.isApproach = true;
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
    for(i=0; i<_cruiseConfig.controls.size(); i++)
        delete (Control*)_cruiseConfig.controls.get(i);
    for(i=0; i<_approachConfig.controls.size(); i++) {
        Control* c = (Control*)_approachConfig.controls.get(i);
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
    _approachConfig.speed = speed;
    _approachConfig.altitude = altitude;
    _approachConfig.state.setupOrientationFromAoa(aoa); // see runConfig()
    _approachConfig.aoa = aoa; // not strictly needed, see runConfig()
    _approachConfig.fuel = fuel;
    _approachConfig.glideAngle = gla;
}
 
void Airplane::setCruise(float speed, float altitude, float fuel, float gla)
{
    _cruiseConfig.speed = speed;
    _cruiseConfig.altitude = altitude;
    _cruiseConfig.aoa = 0;
    _tailIncidence = 0;
    _cruiseConfig.fuel = fuel;
    _cruiseConfig.glideAngle = gla;
}

void Airplane::setElevatorControl(int control)
{
    _approachElevator.control = control;
    _approachElevator.val = 0;
    _approachConfig.controls.add(&_approachElevator);
}

void Airplane::addApproachControl(int control, float val)
{
    Control* c = new Control();
    c->control = control;
    c->val = val;
    _approachConfig.controls.add(c);
}

void Airplane::addCruiseControl(int control, float val)
{
    Control* c = new Control();
    c->control = control;
    c->val = val;
    _cruiseConfig.controls.add(c);
}

void Airplane::addSolutionWeight(bool approach, int idx, float wgt)
{
    SolveWeight* w = new SolveWeight();
    w->approach = approach;
    w->idx = idx;
    w->wgt = wgt;
    _solveWeights.add(w);
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

void Airplane::addThruster(Thruster* thruster, float mass, float* cg)
{
    ThrustRec* t = new ThrustRec();
    t->thruster = thruster;
    t->mass = mass;
    int i;
    for(i=0; i<3; i++) t->cg[i] = cg[i];
    _thrusters.add(t);
}

/// Use ballast to redistribute mass, this is NOT added to empty weight.
void Airplane::addBallast(float* pos, float mass)
{
    _model.getBody()->addMass(mass, pos, true);
    _ballast += mass;
}

/// Masses configurable at runtime, e.g. cargo, pax
int Airplane::addWeight(float* pos, float size)
{
    WeightRec* wr = new WeightRec();
    wr->handle = _model.getBody()->addMass(0, pos);

    wr->surf = new Surface(this);
    wr->surf->setPosition(pos);
    wr->surf->setTotalDrag(size*size);
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

void Airplane::addContactPoint(float* pos)
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
    if (_wingsN != 0) {
      _wingsN->getNode("tip-x", true)->setFloatValue(tip[0]);
      _wingsN->getNode("tip-y", true)->setFloatValue(tip[1]);
      _wingsN->getNode("tip-z", true)->setFloatValue(tip[2]);
      w->getBase(tip);
      _wingsN->getNode("base-x", true)->setFloatValue(tip[0]);
      _wingsN->getNode("base-y", true)->setFloatValue(tip[1]);
      _wingsN->getNode("base-z", true)->setFloatValue(tip[2]);
      _wingsN->getNode("wing-span", true)->setFloatValue(w->getSpan());
      _wingsN->getNode("wing-area", true)->setFloatValue(w->getArea());
      _wingsN->getNode("aspect-ratio", true)->setFloatValue(w->getAspectRatio());
      _wingsN->getNode("standard-mean-chord", true)->setFloatValue(w->getSMC());
      _wingsN->getNode("mac", true)->setFloatValue(w->getMAC());
      _wingsN->getNode("mac-x", true)->setFloatValue(w->getMACx());
      _wingsN->getNode("mac-y", true)->setFloatValue(w->getMACy());
    }

    float wgt = 0;
    float dragSum = 0;
    for(int i=0; i<w->numSurfaces(); i++) {
        Surface* s = (Surface*)w->getSurface(i);
        float td = s->getTotalDrag();
        int sid = s->getID();

        _model.addSurface(s);

        float mass = w->getSurfaceWeight(i);
        mass = mass * Math::sqrt(mass);
        float pos[3];
        s->getPosition(pos);
        int mid = _model.getBody()->addMass(mass, pos, true);
        if (_wingsN != 0) {
          SGPropertyNode_ptr n = _wingsN->getNode("surfaces", true)->getChild("surface", sid, true);
          n->getNode("drag", true)->setFloatValue(td);
          n->getNode("mass-id", true)->setIntValue(mid);
        }
        wgt += mass;
        dragSum += td;
    }
    if (_wingsN != 0)  {
      _wingsN->getNode("weight", true)->setFloatValue(wgt);	
      _wingsN->getNode("drag", true)->setFloatValue(dragSum);	
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

        // Make a Surface too
        Surface* s = new Surface(this);
        s->setPosition(pos);

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

	if( isVersionOrNewer( YASIM_VERSION_32 ) ) {
        	s->setXDrag(f->_cx);
	}
        s->setYDrag(sideDrag*f->_cy);
        s->setZDrag(sideDrag*f->_cz);
	if( isVersionOrNewer( YASIM_VERSION_32 ) ) {
        	s->setTotalDrag(scale*segWgt);
	} else {
		s->setTotalDrag(scale*segWgt*f->_cx);
	}
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

    // Make a Surface object for the aerodynamic behavior
    Surface* s = new Surface(this);
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

void Airplane::compile()
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
      if (baseN != 0) _wingsN = baseN->getChild("wing", 0, true);
      aeroWgt += compileWing(_wing);
      
      // convert % to absolute x coordinates
      _cgDesiredFront = _wing->getMACx() - _wing->getMAC()*_cgDesiredMin;
      _cgDesiredAft = _wing->getMACx() - _wing->getMAC()*_cgDesiredMax;
      if (baseN != 0) {
        SGPropertyNode_ptr n = fgGetNode("/fdm/yasim/model", true);
        n->getNode("cg-x-range-front", true)->setFloatValue(_cgDesiredFront);
        n->getNode("cg-x-range-aft", true)->setFloatValue(_cgDesiredAft);
      }
    }
    if (_tail)
    {
      if (baseN != 0) _wingsN = baseN->getChild("tail", 0, true);
      aeroWgt += compileWing(_tail);
    }
    int i;
    for(i=0; i<_vstabs.size(); i++)
    {
      if (baseN != 0) _wingsN = baseN->getChild("stab", i, true);
      aeroWgt += compileWing((Wing*)_vstabs.get(i));
    }

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
        body->addMass(t->mass, t->cg, true);
    }

    // Add the tanks, empty for now.
    float totalFuel = 0;
    for(i=0; i<_tanks.size(); i++) { 
        Tank* t = (Tank*)_tanks.get(i); 
        t->handle = body->addMass(0, t->pos);
        totalFuel += t->cap;
    }
    _cruiseConfig.weight = _emptyWeight + totalFuel*_cruiseConfig.fuel;
    _approachConfig.weight = _emptyWeight + totalFuel*_approachConfig.fuel;

    
    body->recalc();

    // Add surfaces for the landing gear.
    for(i=0; i<_gears.size(); i++)
        compileGear((GearRec*)_gears.get(i));

    // The Thruster objects
    for(i=0; i<_thrusters.size(); i++) {
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
    float descentRate = 2.0f*_approachConfig.speed/19.1f;

    // Spread the kinetic energy according to the gear weights.  This
    // will results in an equal compression fraction (not distance) of
    // each gear.
    float energy = 0.5f*_approachConfig.weight*descentRate*descentRate;

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
        gr->gear->setDamping(2*Math::sqrt(k*_approachConfig.weight*gr->wgt)
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

/// load values for controls as defined in cruise configuration
void Airplane::loadControls(const Vector& controls)
{
  _controls.reset();
  for(int i=0; i < controls.size(); i++) {
    Control* c = (Control*)controls.get(i);
    _controls.setInput(c->control, c->val);
  }
  _controls.applyControls(); 
}

/// Helper for solve()
void Airplane::runConfig(Config &cfg)
{
  // aoa is consider to be given for approach so we calculate orientation 
  // only once in setApproach()
  if (!cfg.isApproach) {
    cfg.state.setupOrientationFromAoa(cfg.aoa);
  }
  cfg.state.setupSpeedAndPosition(cfg.speed, cfg.glideAngle);
  _model.setState(&cfg.state);
  _model.setStandardAtmosphere(cfg.altitude);
  loadControls(cfg.controls);
  
  // The local wind
  float wind[3];
  Math::mul3(-1, cfg.state.v, wind);
  cfg.state.globalToLocal(wind, wind);
  
  setFuelFraction(cfg.fuel);
  setupWeights(cfg.isApproach);
  
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
/// Used only in Airplane::solve() and solveHelicopter(), not at runtime
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
    for(i=0; i<_fuselages.size(); i++) {
	Fuselage* f = (Fuselage*)_fuselages.get(i);
	int j;
	for(j=0; j<f->surfs.size(); j++) {
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
		s->setXDrag(s->getXDrag() * applied);
	    } else {
		// Originally YASim applied the drag factor to all axes
		// for Fuselage Surfaces.
		s->setTotalDrag(s->getTotalDrag() * applied);
	    }
	}
    }
    for(i=0; i<_weights.size(); i++) {
	WeightRec* wr = (WeightRec*)_weights.get(i);
	wr->surf->setTotalDrag(wr->surf->getTotalDrag() * applied);
    }
    for(i=0; i<_gears.size(); i++) {
	GearRec* gr = (GearRec*)_gears.get(i);
	gr->surf->setTotalDrag(gr->surf->getTotalDrag() * applied);
    }
}

/// Used only in Airplane::solve() and solveHelicopter(), not at runtime
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

/// Helper for solve()
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
	runConfig(_cruiseConfig);

	_model.getThrust(tmp);
        float thrust = tmp[0] + _cruiseConfig.weight * Math::sin(_cruiseConfig.glideAngle) * 9.81;

	_model.getBody()->getAccel(tmp);
        _cruiseConfig.state.localToGlobal(tmp, tmp);
	float xforce = _cruiseConfig.weight * tmp[0];
	float clift0 = _cruiseConfig.weight * tmp[2];

	_model.getBody()->getAngularAccel(tmp);
        _cruiseConfig.state.localToGlobal(tmp, tmp);
	float pitch0 = tmp[1];

	// Run an approach iteration, and do likewise
        runConfig(_approachConfig);

	_model.getBody()->getAngularAccel(tmp);
        _approachConfig.state.localToGlobal(tmp, tmp);
	double apitch0 = tmp[1];

	_model.getBody()->getAccel(tmp);
        _approachConfig.state.localToGlobal(tmp, tmp);
	float alift = _approachConfig.weight * tmp[2];

	// Modify the cruise AoA a bit to get a derivative
	_cruiseConfig.aoa += ARCMIN;
        runConfig(_cruiseConfig);
        _cruiseConfig.aoa -= ARCMIN;
        
	_model.getBody()->getAccel(tmp);
        _cruiseConfig.state.localToGlobal(tmp, tmp);
	float clift1 = _cruiseConfig.weight * tmp[2];

	// Do the same with the tail incidence
	_tail->setIncidence(_tailIncidence + ARCMIN);
        runConfig(_cruiseConfig);
        _tail->setIncidence(_tailIncidence);

	_model.getBody()->getAngularAccel(tmp);
        _cruiseConfig.state.localToGlobal(tmp, tmp);
	float pitch1 = tmp[1];

	// Now calculate:
	float awgt = 9.8f * _approachConfig.weight;

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
        runConfig(_approachConfig);
        _approachElevator.val -= ELEVDIDDLE;

	_model.getBody()->getAngularAccel(tmp);
        _approachConfig.state.localToGlobal(tmp, tmp);
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
	_cruiseConfig.aoa += SOLVE_TWEAK*aoaDelta;
	_tailIncidence += SOLVE_TWEAK*tailDelta;
	
	_cruiseConfig.aoa = Math::clamp(_cruiseConfig.aoa, -0.175f, 0.175f);
	_tailIncidence = Math::clamp(_tailIncidence, -0.175f, 0.175f);

        if(abs(xforce/_cruiseConfig.weight) < STHRESH*0.0001 &&
           abs(alift/_approachConfig.weight) < STHRESH*0.0001 &&
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
    } else if(Math::abs(_cruiseConfig.aoa) >= .17453293) {
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
    _cruiseConfig.state.setupState(0,0,0);
    _model.setState(&_cruiseConfig.state);
    setupWeights(true);
    _controls.reset();
    _model.getBody()->reset();
    _model.setStandardAtmosphere(_cruiseConfig.altitude);    
}

float Airplane::getCGMAC()
{ 
    float cg[3];
    _model.getBody()->getCG(cg);
    return (_wing->getMACx() - cg[0]) / _wing->getMAC();
}

}; // namespace yasim
