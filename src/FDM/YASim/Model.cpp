#include <stdio.h>

#include "Atmosphere.hpp"
#include "Thruster.hpp"
#include "Math.hpp"
#include "RigidBody.hpp"
#include "Integrator.hpp"
#include "Propeller.hpp"
#include "PistonEngine.hpp"
#include "Gear.hpp"
#include "Surface.hpp"
#include "Glue.hpp"

#include "Model.hpp"
namespace yasim {

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
}

Model::~Model()
{
    // FIXME: who owns these things?  Need a policy
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
    for(i=0; i<_thrusters.size(); i++) {
	Thruster* t = (Thruster*)_thrusters.get(i);
	
        // Get the wind velocity at the thruster location
        float pos[3], v[3];
	t->getPosition(pos);
        localWind(pos, _s, v);

	t->setWind(v);
	t->setAir(_pressure, _temp, _rho);
	t->integrate(_integrator.getInterval());

	t->getTorque(v);
	Math::add3(v, _torque, _torque);

	t->getGyro(v);
	Math::add3(v, _gyro, _gyro);
    }
}

void Model::iterate()
{
    initIteration();
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

int Model::addThruster(Thruster* t)
{
    return _thrusters.add(t);
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

void Model::setGroundEffect(float* pos, float span, float mul)
{
    Math::set3(pos, _wingCenter);
    _groundEffectSpan = span;
    _groundEffect = mul;
}

// The first three elements are a unit vector pointing from the global
// origin to the plane, the final element is the distance from the
// origin (the radius of the earth, in most implementations).  So
// (v dot _ground)-_ground[3] gives the distance AGL.
void Model::setGroundPlane(double* planeNormal, double fromOrigin)
{
    int i;
    for(i=0; i<3; i++) _ground[i] = planeNormal[i];
    _ground[3] = fromOrigin;
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

void Model::calcForces(State* s)
{
    // Add in the pre-computed stuff.  These values aren't part of the
    // Runge-Kutta integration (they don't depend on position or
    // velocity), and are therefore constant across the four calls to
    // calcForces.  They get computed before we begin the integration
    // step.
    _body.setGyro(_gyro);
    _body.addTorque(_torque);
    int i;
    for(i=0; i<_thrusters.size(); i++) {
	Thruster* t = (Thruster*)_thrusters.get(i);
	float thrust[3], pos[3];
	t->getThrust(thrust);
	t->getPosition(pos);
	_body.addForce(pos, thrust);
    }

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
        localWind(pos, s, vs);

	float force[3], torque[3];
	sf->calcForce(vs, _rho, force, torque);
	Math::add3(faero, force, faero);
	_body.addForce(pos, force);
	_body.addTorque(torque);
    }

    // Get a ground plane in local coordinates.  The first three
    // elements are the normal vector, the final one is the distance
    // from the local origin along that vector to the ground plane
    // (negative for objects "above" the ground)
    float ground[4];
    ground[3] = localGround(s, ground);

    // Account for ground effect by multiplying the vertical force
    // component by an amount linear with the fraction of the wingspan
    // above the ground.
    float dist = ground[3] - Math::dot3(ground, _wingCenter);
    if(dist > 0 && dist < _groundEffectSpan) {
	float fz = Math::dot3(faero, ground);
	Math::mul3(fz * _groundEffect * dist/_groundEffectSpan,
		   ground, faero);
	_body.addForce(faero);
    }
    
    // Convert the velocity and rotation vectors to local coordinates
    float lrot[3], lv[3];
    Math::vmul33(s->orient, s->rot, lrot);
    Math::vmul33(s->orient, s->v, lv);

    // The landing gear
    for(i=0; i<_gears.size(); i++) {
	float force[3], contact[3];
	Gear* g = (Gear*)_gears.get(i);
	g->calcForce(&_body, lv, lrot, ground);
	g->getForce(force, contact);
	_body.addForce(contact, force);
    }
}

void Model::newState(State* s)
{
    _s = s;

    //printState(s);

    // Some simple collision detection
    float min = 1e8;
    float ground[4], pos[3], cmpr[3];
    ground[3] = localGround(s, ground);
    int i;
    for(i=0; i<_gears.size(); i++) {
	Gear* g = (Gear*)_gears.get(i);

	// Get the point of ground contact
	g->getPosition(pos);
	g->getCompression(cmpr);
	Math::mul3(g->getCompressFraction(), cmpr, cmpr);
	Math::add3(cmpr, pos, pos);
	float dist = ground[3] - Math::dot3(pos, ground);

	// Find the lowest one
	if(dist < min)
	    min = dist;
    }
    _agl = min;
    if(_agl < -1) // Allow for some integration slop
	_crashed = true;
}

// Returns a unit "down" vector for the ground in out, and the
// distance from the local origin to the ground as the return value.
// So for a given position V, "dist - (V dot out)" will be the height
// AGL.
float Model::localGround(State* s, float* out)
{
    // Get the ground's "down" vector, this can be in floats, because
    // we don't need positioning accuracy.  The direction has plenty
    // of accuracy after truncation.
    out[0] = -(float)_ground[0];
    out[1] = -(float)_ground[1];
    out[2] = -(float)_ground[2];
    Math::vmul33(s->orient, out, out);

    // The distance from the ground to the Aircraft's origin:
    double dist = (s->pos[0]*_ground[0]
                   + s->pos[1]*_ground[1]
                   + s->pos[2]*_ground[2] - _ground[3]);

    return (float)dist;
}

// Calculates the airflow direction at the given point and for the
// specified aircraft velocity.
void Model::localWind(float* pos, State* s, float* out)
{
    // Most of the input is in global coordinates.  Fix that.
    float lwind[3], lrot[3], lv[3];
    Math::vmul33(s->orient, _wind, lwind);
    Math::vmul33(s->orient, s->rot, lrot);
    Math::vmul33(s->orient, s->v, lv);

    _body.pointVelocity(pos, lrot, out); // rotational velocity
    Math::mul3(-1, out, out);      //  (negated)
    Math::add3(lwind, out, out);    //  + wind
    Math::sub3(out, lv, out);       //  - velocity
}

}; // namespace yasim
