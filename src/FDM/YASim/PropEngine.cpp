#include "Math.hpp"
#include "Propeller.hpp"
#include "PistonEngine.hpp"
#include "PropEngine.hpp"
namespace yasim {

PropEngine::PropEngine(Propeller* prop, PistonEngine* eng, float moment)
{
    // Start off at 500rpm, because the start code doesn't exist yet
    _omega = 52.3;
    _dir[0] = 1; _dir[1] = 0; _dir[2] = 0;

    _prop = prop;
    _eng = eng;
    _moment = moment;
}

PropEngine::~PropEngine()
{
    delete _prop;
    delete _eng;
}

float PropEngine::getOmega()
{
    return _omega;
}

Thruster* PropEngine::clone()
{
    // FIXME: bloody mess
    PropEngine* p = new PropEngine(_prop, _eng, _moment);
    cloneInto(p);
    p->_prop = new Propeller(*_prop);
    p->_eng = new PistonEngine(*_eng);
    return p;
}

void PropEngine::getThrust(float* out)
{
    for(int i=0; i<3; i++) out[i] = _thrust[i];    
}

void PropEngine::getTorque(float* out)
{
    for(int i=0; i<3; i++) out[i] = _torque[i];
}

void PropEngine::getGyro(float* out)
{
    for(int i=0; i<3; i++) out[i] = _gyro[i];
}

float PropEngine::getFuelFlow()
{
    return _fuelFlow;
}

void PropEngine::integrate(float dt)
{
    float speed = -Math::dot3(_wind, _dir);

    float propTorque, engTorque, thrust;

    _eng->setThrottle(_throttle);
    _eng->setMixture(_mixture);
    
    _prop->calc(_rho, speed, _omega,
		&thrust, &propTorque);
    _eng->calc(_rho, _omega, &engTorque, &_fuelFlow);

    // Turn the thrust into a vector and save it
    Math::mul3(thrust, _dir, _thrust);

    // Euler-integrate the RPM.  This doesn't need the full-on
    // Runge-Kutta stuff.
    _omega += dt*(engTorque-propTorque)/Math::abs(_moment);

    // FIXME: Integrate the propeller governor here, when that gets
    // implemented...

    // Store the total angular momentum into _gyro
    Math::mul3(_omega*_moment, _dir, _gyro);

    // Accumulate the engine torque, it acta on the body as a whole.
    // (Note: engine torque, not propeller torque.  They can be
    // different, but the difference goes to accelerating the
    // rotation.  It is the engine torque that is felt at the shaft
    // and works on the body.)
    float tau = _moment < 0 ? engTorque : -engTorque;
    Math::mul3(tau, _dir, _torque);
}

}; // namespace yasim
