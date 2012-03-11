
#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include "Atmosphere.hpp"
#include "Thruster.hpp"
#include "Math.hpp"
#include "RigidBody.hpp"
#include "Integrator.hpp"
#include "Propeller.hpp"
#include "PistonEngine.hpp"
#include "Gear.hpp"
#include "Hook.hpp"
#include "Launchbar.hpp"
#include "Surface.hpp"
#include "Rotor.hpp"
#include "Rotorpart.hpp"
#include "Hitch.hpp"
#include "Glue.hpp"
#include "Ground.hpp"

#include "Model.hpp"
namespace yasim {

#if 0
void printState(State* s)
{
    State tmp = *s;
    Math::vmul33(tmp.orient, tmp.v, tmp.v);
    Math::vmul33(tmp.orient, tmp.acc, tmp.acc);
    Math::vmul33(tmp.orient, tmp.rot, tmp.rot);
    Math::vmul33(tmp.orient, tmp.racc, tmp.racc);

    printf("\nNEW STATE (LOCAL COORDS)\n");
    printf("pos: %10.2f %10.2f %10.2f\n", tmp.pos[0], tmp.pos[1], tmp.pos[2]);
    printf("o:   ");
    int i;
    for(i=0; i<3; i++) {
	if(i != 0) printf("     ");
	printf("%6.2f %6.2f %6.2f\n",
	       tmp.orient[3*i+0], tmp.orient[3*i+1], tmp.orient[3*i+2]);
    }
    printf("v:   %6.2f %6.2f %6.2f\n", tmp.v[0], tmp.v[1], tmp.v[2]);
    printf("acc: %6.2f %6.2f %6.2f\n", tmp.acc[0], tmp.acc[1], tmp.acc[2]);
    printf("rot: %6.2f %6.2f %6.2f\n", tmp.rot[0], tmp.rot[1], tmp.rot[2]);
    printf("rac: %6.2f %6.2f %6.2f\n", tmp.racc[0], tmp.racc[1], tmp.racc[2]);
}
#endif

Model::Model()
{
    int i;
    for(i=0; i<3; i++) _wind[i] = 0;

    _integrator.setBody(&_body);
    _integrator.setEnvironment(this);

    // Default value of 30 Hz
    _integrator.setInterval(1.0f/30.0f);

    _agl = 0;
    _crashed = false;
    _turb = 0;
    _ground_cb = new Ground();
    _hook = 0;
    _launchbar = 0;

    _groundEffectSpan = 0;
    _groundEffect = 0;
    for(i=0; i<3; i++) _wingCenter[i] = 0;

    _global_ground[0] = 0; _global_ground[1] = 0; _global_ground[2] = 1;
    _global_ground[3] = -100000;

}

Model::~Model()
{
    // FIXME: who owns these things?  Need a policy
    delete _ground_cb;
    delete _hook;
    delete _launchbar;
    for(int i=0; i<_hitches.size();i++)
        delete (Hitch*)_hitches.get(i);

}

void Model::getThrust(float* out)
{
    float tmp[3];
    out[0] = out[1] = out[2] = 0;
    int i;
    for(i=0; i<_thrusters.size(); i++) {
	Thruster* t = (Thruster*)_thrusters.get(i);
	t->getThrust(tmp);
	Math::add3(tmp, out, out);
    }
}

void Model::initIteration()
{
    // Precompute torque and angular momentum for the thrusters
    int i;
    for(i=0; i<3; i++)
	_gyro[i] = _torque[i] = 0;

    // Need a local altitude for the wind calculation
    float lground[4];
    _s->planeGlobalToLocal(_global_ground, lground);
    float alt = Math::abs(lground[3]);

    for(i=0; i<_thrusters.size(); i++) {
	Thruster* t = (Thruster*)_thrusters.get(i);
	
        // Get the wind velocity at the thruster location
        float pos[3], v[3];
	t->getPosition(pos);
        localWind(pos, _s, v, alt);

	t->setWind(v);
	t->setAir(_pressure, _temp, _rho);
	t->integrate(_integrator.getInterval());

	t->getTorque(v);
	Math::add3(v, _torque, _torque);

	t->getGyro(v);
	Math::add3(v, _gyro, _gyro);
    }

    // Displace the turbulence coordinates according to the local wind.
    if(_turb) {
        float toff[3];
        Math::mul3(_integrator.getInterval(), _wind, toff);
        _turb->offset(toff);
    }

    for(i=0; i<_hitches.size(); i++) {
        Hitch* h = (Hitch*)_hitches.get(i);
        h->integrate(_integrator.getInterval());
    }

    
}

// This function initializes some variables for the rotor calculation
// Furthermore it integrates in "void Rotorpart::inititeration
// (float dt,float *rot)" the "rotor orientation" by omega*dt for the 
// 3D-visualization of the heli only. and it compensates the rotation 
// of the fuselage. The rotor does not follow the rotation of the fuselage.
// Therefore its rotation must be subtracted from the orientation of the 
// rotor.
// Maik
void Model::initRotorIteration()
{
    float dt = _integrator.getInterval();
    float lrot[3];
    if (!_rotorgear.isInUse()) return;
    Math::vmul33(_s->orient, _s->rot, lrot);
    Math::mul3(dt,lrot,lrot);
    _rotorgear.initRotorIteration(lrot,dt);
}

void Model::iterate()
{
    initIteration();
    initRotorIteration();
    _body.recalc(); // FIXME: amortize this, somehow
    _integrator.calcNewInterval();
}

bool Model::isCrashed()
{
    return _crashed;
}

void Model::setCrashed(bool crashed)
{
    _crashed = crashed;
}

float Model::getAGL()
{
    return _agl;
}

State* Model::getState()
{
    return _s;
}

void Model::setState(State* s)
{
    _integrator.setState(s);
    _s = _integrator.getState();
}

RigidBody* Model::getBody()
{
    return &_body;
}

Integrator* Model::getIntegrator()
{
    return &_integrator;
}

Surface* Model::getSurface(int handle)
{
    return (Surface*)_surfaces.get(handle);
}

Rotorgear* Model::getRotorgear(void)
{
    return &_rotorgear;
}

int Model::addThruster(Thruster* t)
{
    return _thrusters.add(t);
}

Hook* Model::getHook(void)
{
    return _hook;
}

Launchbar* Model::getLaunchbar(void)
{
    return _launchbar;
}

int Model::numThrusters()
{
    return _thrusters.size();
}

Thruster* Model::getThruster(int handle)
{
    return (Thruster*)_thrusters.get(handle);
}

void Model::setThruster(int handle, Thruster* t)
{
    _thrusters.set(handle, t);
}

int Model::addSurface(Surface* surf)
{
    return _surfaces.add(surf);
}

int Model::addGear(Gear* gear)
{
    return _gears.add(gear);
}

void Model::addHook(Hook* hook)
{
    _hook = hook;
}

void Model::addLaunchbar(Launchbar* launchbar)
{
    _launchbar = launchbar;
}

int Model::addHitch(Hitch* hitch)
{
    return _hitches.add(hitch);
}

void Model::setGroundCallback(Ground* ground_cb)
{
    delete _ground_cb;
    _ground_cb = ground_cb;
}

Ground* Model::getGroundCallback(void)
{
    return _ground_cb;
}

void Model::setGroundEffect(float* pos, float span, float mul)
{
    Math::set3(pos, _wingCenter);
    _groundEffectSpan = span;
    _groundEffect = mul;
}

void Model::setAir(float pressure, float temp, float density)
{
    _pressure = pressure;
    _temp = temp;
    _rho = density;
}

void Model::setWind(float* wind)
{
    Math::set3(wind, _wind);
}

void Model::updateGround(State* s)
{
    float dummy[3];
    _ground_cb->getGroundPlane(s->pos, _global_ground, dummy);

    int i;
    // The landing gear
    for(i=0; i<_gears.size(); i++) {
	Gear* g = (Gear*)_gears.get(i);

	// Get the point of ground contact
        float pos[3], cmpr[3];
	g->getPosition(pos);
	g->getCompression(cmpr);

	Math::mul3(g->getCompressFraction(), cmpr, cmpr);
	Math::add3(cmpr, pos, pos);
        // Transform the local coordinates of the contact point to
        // global coordinates.
        double pt[3];
        s->posLocalToGlobal(pos, pt);

        // Ask for the ground plane in the global coordinate system
        double global_ground[4];
        float global_vel[3];
        const SGMaterial* material;
        _ground_cb->getGroundPlane(pt, global_ground, global_vel, &material);
        g->setGlobalGround(global_ground, global_vel, pt[0], pt[1], material);
    }

    for(i=0; i<_hitches.size(); i++) {
        Hitch* h = (Hitch*)_hitches.get(i);

        // Get the point of interest
        float pos[3];
        h->getPosition(pos);

        // Transform the local coordinates of the contact point to
        // global coordinates.
        double pt[3];
        s->posLocalToGlobal(pos, pt);

        // Ask for the ground plane in the global coordinate system
        double global_ground[4];
        float global_vel[3];
        _ground_cb->getGroundPlane(pt, global_ground, global_vel);
        h->setGlobalGround(global_ground, global_vel);
    }

    for(i=0; i<_rotorgear.getRotors()->size(); i++) {
        Rotor* r = (Rotor*)_rotorgear.getRotors()->get(i);
        r->findGroundEffectAltitude(_ground_cb,s);
    }

    // The arrester hook
    if(_hook) {
        double pt[3];
        _hook->getTipGlobalPosition(s, pt);
        double global_ground[4];
        _ground_cb->getGroundPlane(pt, global_ground, dummy);
        _hook->setGlobalGround(global_ground);
    }

    // The launchbar/holdback
    if(_launchbar) {
        double pt[3];
        _launchbar->getTipGlobalPosition(s, pt);
        double global_ground[4];
        _ground_cb->getGroundPlane(pt, global_ground, dummy);
        _launchbar->setGlobalGround(global_ground);
    }
}

void Model::calcForces(State* s)
{
    // Add in the pre-computed stuff.  These values aren't part of the
    // Runge-Kutta integration (they don't depend on position or
    // velocity), and are therefore constant across the four calls to
    // calcForces.  They get computed before we begin the integration
    // step.
    _body.setGyro(_gyro);
    _body.addTorque(_torque);
    int i,j;
    for(i=0; i<_thrusters.size(); i++) {
	Thruster* t = (Thruster*)_thrusters.get(i);
	float thrust[3], pos[3];
	t->getThrust(thrust);
	t->getPosition(pos);
	_body.addForce(pos, thrust);
    }

    // Get a ground plane in local coordinates.  The first three
    // elements are the normal vector, the final one is the distance
    // from the local origin along that vector to the ground plane
    // (negative for objects "above" the ground)
    float ground[4];
    s->planeGlobalToLocal(_global_ground, ground);
    float alt = Math::abs(ground[3]);

    // Gravity, convert to a force, then to local coordinates
    float grav[3];
    Glue::geodUp(s->pos, grav);
    Math::mul3(-9.8f * _body.getTotalMass(), grav, grav);
    Math::vmul33(s->orient, grav, grav);
    _body.addForce(grav);

    // Do each surface, remembering that the local velocity at each
    // point is different due to rotation.
    float faero[3];
    faero[0] = faero[1] = faero[2] = 0;
    for(i=0; i<_surfaces.size(); i++) {
	Surface* sf = (Surface*)_surfaces.get(i);

	// Vsurf = wind - velocity + (rot cross (cg - pos))
	float vs[3], pos[3];
	sf->getPosition(pos);
        localWind(pos, s, vs, alt);

	float force[3], torque[3];
	sf->calcForce(vs, _rho, force, torque);
	Math::add3(faero, force, faero);

	_body.addForce(pos, force);
	_body.addTorque(torque);
    }
    for (j=0; j<_rotorgear.getRotors()->size();j++)
    {
        Rotor* r = (Rotor *)_rotorgear.getRotors()->get(j);
        float vs[3], pos[3];
        r->getPosition(pos);
        localWind(pos, s, vs, alt);
        r->calcLiftFactor(vs, _rho,s);
        float tq=0; 
        // total torque of rotor (scalar) for calculating new rotor rpm

        for(i=0; i<r->_rotorparts.size(); i++) {
            float torque_scalar=0;
            Rotorpart* rp = (Rotorpart*)r->_rotorparts.get(i);

            // Vsurf = wind - velocity + (rot cross (cg - pos))
            float vs[3], pos[3];
            rp->getPosition(pos);
            localWind(pos, s, vs, alt,true);

            float force[3], torque[3];
            rp->calcForce(vs, _rho, force, torque, &torque_scalar);
            tq+=torque_scalar;
            rp->getPositionForceAttac(pos);

            _body.addForce(pos, force);
            _body.addTorque(torque);
        }
        r->setTorque(tq);
    }
    if (_rotorgear.isInUse())
    {
        float torque[3];
        _rotorgear.calcForces(torque);
        _body.addTorque(torque);
    }

    // Account for ground effect by multiplying the vertical force
    // component by an amount linear with the fraction of the wingspan
    // above the ground.
    if ((_groundEffectSpan != 0) && (_groundEffect != 0 ))
    {
        float dist = ground[3] - Math::dot3(ground, _wingCenter);
        if(dist > 0 && dist < _groundEffectSpan) {
        float fz = Math::dot3(faero, ground);
            fz *= (_groundEffectSpan - dist) / _groundEffectSpan;
            fz *= _groundEffect;
        Math::mul3(fz, ground, faero);
        _body.addForce(faero);
        }
    }
    
    // Convert the velocity and rotation vectors to local coordinates
    float lrot[3], lv[3];
    Math::vmul33(s->orient, s->rot, lrot);
    Math::vmul33(s->orient, s->v, lv);

    // The landing gear
    for(i=0; i<_gears.size(); i++) {
	float force[3], contact[3];
	Gear* g = (Gear*)_gears.get(i);

	g->calcForce(&_body, s, lv, lrot);
	g->getForce(force, contact);
	_body.addForce(contact, force);
    }

    // The arrester hook
    if(_hook) {
        _hook->calcForce(_ground_cb, &_body, s, lv, lrot);
	float force[3], contact[3];
        _hook->getForce(force, contact);
        _body.addForce(contact, force);
    }

    // The launchbar/holdback
    if(_launchbar) {
        _launchbar->calcForce(_ground_cb, &_body, s, lv, lrot);
	float forcelb[3], contactlb[3], forcehb[3], contacthb[3];
        _launchbar->getForce(forcelb, contactlb, forcehb, contacthb);
        _body.addForce(contactlb, forcelb);
        _body.addForce(contacthb, forcehb);
    }

// The hitches
    for(i=0; i<_hitches.size(); i++) {
        float force[3], contact[3];
        Hitch* h = (Hitch*)_hitches.get(i);
        h->calcForce(_ground_cb,&_body, s);
        h->getForce(force, contact);
        _body.addForce(contact, force);
    }
}
void Model::newState(State* s)
{
    _s = s;

    // Some simple collision detection
    float min = 1e8;
    int i;
    for(i=0; i<_gears.size(); i++) {
	Gear* g = (Gear*)_gears.get(i);

        if (!g->getSubmergable())
        {
	    // Get the point of ground contact
            float pos[3], cmpr[3];
	    g->getPosition(pos);
	    g->getCompression(cmpr);
	    Math::mul3(g->getCompressFraction(), cmpr, cmpr);
	    Math::add3(cmpr, pos, pos);

            // The plane transformed to local coordinates.
            double global_ground[4];
            g->getGlobalGround(global_ground);
            float ground[4];
            s->planeGlobalToLocal(global_ground, ground);
	    float dist = ground[3] - Math::dot3(pos, ground);

	    // Find the lowest one
	    if(dist < min)
	        min = dist;
        }
    }
    _agl = min;
    if(_agl < -1) // Allow for some integration slop
	_crashed = true;
}

// Calculates the airflow direction at the given point and for the
// specified aircraft velocity.
void Model::localWind(float* pos, State* s, float* out, float alt, bool is_rotor)
{
    float tmp[3], lwind[3], lrot[3], lv[3];

    // Get a global coordinate for our local position, and calculate
    // turbulence.
    if(_turb) {
        double gpos[3]; float up[3];
        Math::tmul33(s->orient, pos, tmp);
        for(int i=0; i<3; i++) {
            gpos[i] = s->pos[i] + tmp[i];
        }
        Glue::geodUp(gpos, up);
        _turb->getTurbulence(gpos, alt, up, lwind);
        Math::add3(_wind, lwind, lwind);
    } else {
        Math::set3(_wind, lwind);
    }

    // Convert to local coordinates
    Math::vmul33(s->orient, lwind, lwind);
    Math::vmul33(s->orient, s->rot, lrot);
    Math::vmul33(s->orient, s->v, lv);

    _body.pointVelocity(pos, lrot, out); // rotational velocity
    Math::mul3(-1, out, out);      //  (negated)
    Math::add3(lwind, out, out);    //  + wind
    Math::sub3(out, lv, out);       //  - velocity

    //add the downwash of the rotors if it is not self a rotor
    if (_rotorgear.isInUse()&&!is_rotor)
    {
        _rotorgear.getDownWash(pos,lv,tmp);
        Math::add3(out,tmp, out);    //  + downwash
    }


}

}; // namespace yasim
