#include <Main/fg_props.hxx>
#include "Math.hpp"
#include "RigidBody.hpp"

namespace yasim {

RigidBody::RigidBody()
{
    // Allocate space for 16 masses initially.  More space will be
    // allocated dynamically.
    _nMasses = 0;
    _massesAlloced = 16;
    _masses = new Mass[_massesAlloced];
    _gyro[0] = _gyro[1] = _gyro[2] = 0;
    _spin[0] = _spin[1] = _spin[2] = 0;
    _bodyN = fgGetNode("/fdm/yasim/model/masses", true);
}

RigidBody::~RigidBody()
{
    delete[] _masses;
}

/// add new point mass to body
/// isStatic: set to true for masses that do not change per iteration (everything but fuel?)
int RigidBody::addMass(float mass, float* pos, bool isStatic)
{
    // If out of space, reallocate twice as much
    if(_nMasses == _massesAlloced) {
        _massesAlloced *= 2;
        Mass *m2 = new Mass[_massesAlloced];
        int i;
        for(i=0; i<_nMasses; i++)
            m2[i] = _masses[i];
        delete[] _masses;
        _masses = m2;
    }
    setMass(_nMasses, mass, pos, isStatic);
    return _nMasses++;
}

/// change mass
/// handle: returned by addMass
void RigidBody::setMass(int handle, float mass)
{
    if (_masses[handle].m  == mass) 
      return;
    _masses[handle].m = mass;
    // if static mass is changed, reset pre-calculated mass
    // may apply to weights like cargo, pax, that usually do not change with FDM rate 
    if (_masses[handle].isStatic)
      _staticMass.m = 0;
    if (_bodyN != 0)
      _bodyN->getChild("mass", handle, true)->getNode("mass", true)->setFloatValue(mass);
}

void RigidBody::setMass(int handle, float mass, float* pos, bool isStatic)
{
    _masses[handle].m = mass;
    _masses[handle].isStatic = isStatic;
    Math::set3(pos, _masses[handle].p);
    if (_bodyN != 0) {
      SGPropertyNode_ptr n = _bodyN->getChild("mass", handle, true);
      n->getNode("isStatic", true)->setValue(isStatic);
      n->getNode("mass", true)->setFloatValue(mass);
      n->getNode("pos-x", true)->setFloatValue(pos[0]);
      n->getNode("pos-y", true)->setFloatValue(pos[1]);
      n->getNode("pos-z", true)->setFloatValue(pos[2]);	
    }
}

int RigidBody::numMasses()
{
    return _nMasses;
}

float RigidBody::getMass(int handle)
{
    return _masses[handle].m;
}

void RigidBody::getMassPosition(int handle, float* out)
{
    out[0] = _masses[handle].p[0];
    out[1] = _masses[handle].p[1];
    out[2] = _masses[handle].p[2];
}

float RigidBody::getTotalMass()
{
    return _totalMass;
}

// Calcualtes the rotational velocity of a particular point.  All
// coordinates are local!
void RigidBody::pointVelocity(float* pos, float* rot, float* out)
{
    Math::sub3(pos, _cg, out);   //  out = pos-cg
    Math::cross3(rot, out, out); //      = rot cross (pos-cg)
}

void RigidBody::setGyro(float* angularMomentum)
{
    Math::set3(angularMomentum, _gyro);
}

void RigidBody::_recalcStatic()
{
	// aggregate all masses that do not change (e.g. fuselage, wings) into one point mass
	_staticMass.m = 0;
	_staticMass.p[0] = 0;
	_staticMass.p[1] = 0;
	_staticMass.p[2] = 0;
	int i;
	int s = 0;
	for(i=0; i<_nMasses; i++) {
		if (_masses[i].isStatic) {
			s++;
			float m = _masses[i].m;
			_staticMass.m += m;
			_staticMass.p[0] += m * _masses[i].p[0];
			_staticMass.p[1] += m * _masses[i].p[1];
			_staticMass.p[2] += m * _masses[i].p[2];
		}
	}
	Math::mul3(1/_staticMass.m, _staticMass.p, _staticMass.p);
  if (_bodyN != 0) {
    _bodyN->getNode("aggregated-mass", true)->setFloatValue(_staticMass.m);
    _bodyN->getNode("aggregated-count", true)->setIntValue(s);
    _bodyN->getNode("aggregated-pos-x", true)->setFloatValue(_staticMass.p[0]);
    _bodyN->getNode("aggregated-pos-y", true)->setFloatValue(_staticMass.p[1]);
    _bodyN->getNode("aggregated-pos-z", true)->setFloatValue(_staticMass.p[2]);	
  }
	// Now the inertia tensor:
	for(i=0; i<9; i++)
		_tI_static[i] = 0;

	for(i=0; i<_nMasses; i++) {
		if (_masses[i].isStatic) {
			float m = _masses[i].m;

			float x = _masses[i].p[0] - _staticMass.p[0];
			float y = _masses[i].p[1] - _staticMass.p[1];
			float z = _masses[i].p[2] - _staticMass.p[2];

			float xy = m*x*y; float yz = m*y*z; float zx = m*z*x;
			float x2 = m*x*x; float y2 = m*y*y; float z2 = m*z*z;

			// tensor is symmetric, so we can save some calculations in the loop
			_tI_static[0] += y2+z2;  _tI_static[1] -=    xy;  _tI_static[2] -=    zx;
			                         _tI_static[4] += x2+z2;  _tI_static[5] -=    yz;
			                                                  _tI_static[8] += x2+y2;
		}
	}
	// copy symmetric elements
	_tI_static[3] = _tI_static[1];
	_tI_static[6] = _tI_static[2];
	_tI_static[7] = _tI_static[5];
}

/// calculate the total mass, centre of gravity and inertia tensor
/**
  recalc is used when compiling the model but more important it is called in
  Model::iterate() e.g. at FDM rate (120 Hz)
  We can save some CPU due to the symmetry of the tensor and by aggregating
  masses that do not change during flight.
 */
void RigidBody::recalc()
{
    //aggregate static masses into one mass
    if (_staticMass.m == 0) _recalcStatic();

    // Calculate the c.g and total mass
    // init with pre-calculated static mass
    _totalMass = _staticMass.m;
    _cg[0] = _staticMass.m * _staticMass.p[0];
    _cg[1] = _staticMass.m * _staticMass.p[1];
    _cg[2] = _staticMass.m * _staticMass.p[2];
    int i;
    for(i=0; i<_nMasses; i++) {
      // only masses we did not aggregate
      if (!_masses[i].isStatic) { 
        float m = _masses[i].m;
        _totalMass += m;
        _cg[0] += m * _masses[i].p[0];
        _cg[1] += m * _masses[i].p[1];
        _cg[2] += m * _masses[i].p[2];
      }
    }
    Math::mul3(1/_totalMass, _cg, _cg);

    // Now the inertia tensor:
    for(i=0; i<9; i++)
      _tI[i] = _tI_static[i];

    for(i=0; i<_nMasses; i++) {
      if (!_masses[i].isStatic) {
        float m = _masses[i].m;

        float x = _masses[i].p[0] - _cg[0];
        float y = _masses[i].p[1] - _cg[1];
        float z = _masses[i].p[2] - _cg[2];
        float mx = m*x;
        float my = m*y;
        float mz = m*z;
        
        float xy = mx*y; float yz = my*z; float zx = mz*x;
        float x2 = mx*x; float y2 = my*y; float z2 = mz*z;

        _tI[0] += y2+z2;  _tI[1] -=    xy;  _tI[2] -=    zx;
                          _tI[4] += x2+z2;  _tI[5] -=    yz;
                                            _tI[8] += x2+y2;
      }
    }
    // copy symmetric elements 
    _tI[3] = _tI[1];
    _tI[6] = _tI[2];
    _tI[7] = _tI[5];

    //calculate inverse
    Math::invert33_sym(_tI, _invI);
}

void RigidBody::reset()
{
    _torque[0] = _torque[1] = _torque[2] = 0;
    _force[0] = _force[1] = _force[2] = 0;
}

void RigidBody::addForce(float* force)
{
    Math::add3(_force, force, _force);
}

void RigidBody::addTorque(float* torque)
{
    Math::add3(_torque, torque, _torque);
}

void RigidBody::addForce(float* pos, float* force)
{
    addForce(force);
    
    // For a force F at position X, the torque about the c.g C is:
    // torque = F cross (C - X)
    float v[3], t[3];
    Math::sub3(_cg, pos, v);
    Math::cross3(force, v, t);
    addTorque(t);
}

void RigidBody::setBodySpin(float* rotation)
{
    Math::set3(rotation, _spin);
}

void RigidBody::getCG(float* cgOut)
{
    Math::set3(_cg, cgOut);
}

void RigidBody::getAccel(float* accelOut)
{
    Math::mul3(1/_totalMass, _force, accelOut);
}

void RigidBody::getAccel(float* pos, float* accelOut)
{
    getAccel(accelOut);

    // Turn the "spin" vector into a normalized spin axis "a" and a
    // radians/sec scalar "rate".
    float a[3];
    float rate = Math::mag3(_spin);
    Math::set3(_spin, a);
    if (rate !=0 )
        Math::mul3(1/rate, a, a);
    //an else branch is not neccesary. a, which is a=(0,0,0) in the else case, is only used in a dot product
    float v[3];
    Math::sub3(_cg, pos, v);             // v = cg - pos
    Math::mul3(Math::dot3(v, a), a, a);  // a = a * (v dot a)
    Math::add3(v, a, v);                 // v = v + a

    // Now v contains the vector from pos to the rotation axis.
    // Multiply by the square of the rotation rate to get the linear
    // acceleration.
    Math::mul3(rate*rate, v, v);
    Math::add3(v, accelOut, accelOut);
}

void RigidBody::getAngularAccel(float* accelOut)
{
    // Compute "tau" as the externally applied torque, plus the
    // counter-torque due to the internal gyro.
    float tau[3]; // torque
    Math::cross3(_gyro, _spin, tau);
    Math::add3(_torque, tau, tau);

    // Now work the equation of motion.  Use "v" as a notational
    // shorthand, as the value isn't an acceleration until the end.
    float *v = accelOut;
    Math::vmul33(_tI, _spin, v);  // v = I*omega
    Math::cross3(_spin, v, v);   // v = omega X I*omega
    Math::add3(tau, v, v);       // v = tau + (omega X I*omega)
    Math::vmul33(_invI, v, v);   // v = invI*(tau + (omega X I*omega))
}

void RigidBody::getInertiaMatrix(float* inertiaOut)
{
    // valid only after a call to RigidBody::recalc()
    // See comment at top of RigidBody.hpp on units.
    for(int i=0;i<9;i++)
    {
       inertiaOut[i] = _tI[i];
    }
}

}; // namespace yasim
