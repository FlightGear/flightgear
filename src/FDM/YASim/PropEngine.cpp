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

    _variable = false;

    _prop = prop;
    _eng = eng;
    _moment = moment;
}

PropEngine::~PropEngine()
{
    delete _prop;
    delete _eng;
}

void PropEngine::setAdvance(float advance)
{
    _advance = Math::clamp(advance, 0, 1);
}

void PropEngine::setVariableProp(float min, float max)
{
    _variable = true;
    _minOmega = min;
    _maxOmega = max;
}

float PropEngine::getOmega()
{
    return _omega;
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

void PropEngine::stabilize()
{
    float speed = -Math::dot3(_wind, _dir);
    _eng->setThrottle(_throttle);
    _eng->setMixture(_mixture);

    if(_variable) {
	_omega = _minOmega + _advance * (_maxOmega - _minOmega);
	_prop->modPitch(1e6); // Start at maximum pitch and move down
    } else {
	_omega = 52;
    }

    bool goingUp = false;
    float step = 10;
    while(true) {
	float etau, ptau, dummy;
	_prop->calc(_rho, speed, _omega, &dummy, &ptau);
	_eng->calc(_P, _T, _omega, &etau, &dummy);
	float tdiff = etau - ptau;

	if(Math::abs(tdiff/_moment) < 0.1)
	    break;

	if(tdiff > 0) {
	    if(!goingUp) step *= 0.5;
	    goingUp = true;
 	    if(!_variable)  _omega += step;
	    else            _prop->modPitch(1+(step*0.005));
	} else {
	    if(goingUp) step *= 0.5;
	    goingUp = false;
 	    if(!_variable)  _omega -= step;
	    else            _prop->modPitch(1-(step*0.005));
	}
    }
}

void PropEngine::integrate(float dt)
{
    float speed = -Math::dot3(_wind, _dir);

    float propTorque, engTorque, thrust;

    _eng->setThrottle(_throttle);
    _eng->setMixture(_mixture);
    
    _prop->calc(_rho, speed, _omega,
		&thrust, &propTorque);
    _eng->calc(_P, _T, _omega, &engTorque, &_fuelFlow);

    // Turn the thrust into a vector and save it
    Math::mul3(thrust, _dir, _thrust);

    // Euler-integrate the RPM.  This doesn't need the full-on
    // Runge-Kutta stuff.
    float rotacc = (engTorque-propTorque)/Math::abs(_moment);
    _omega += dt * rotacc;

    // Clamp to a 500 rpm idle.  This should probably be settable, and
    // needs to go away when the startup code gets written.
    if(_omega < 52.3) _omega = 52.3;

    // FIXME: Integrate the propeller governor here, when that gets
    // implemented...

    // Store the total angular momentum into _gyro
    Math::mul3(_omega*_moment, _dir, _gyro);

    // Accumulate the engine torque, it acts on the body as a whole.
    // (Note: engine torque, not propeller torque.  They can be
    // different, but the difference goes to accelerating the
    // rotation.  It is the engine torque that is felt at the shaft
    // and works on the body.)
    float tau = _moment < 0 ? engTorque : -engTorque;
    Math::mul3(tau, _dir, _torque);

    // Play with the variable propeller, but only if the propeller is
    // vaguely stable alread (accelerating less than 100 rpm/s)
    if(_variable && Math::abs(rotacc) < 20) {
	float target = _minOmega + _advance*(_maxOmega-_minOmega);
	float mod = 1.04;
	if(target > _omega) mod = 1/mod;
	float diff = Math::abs(target - _omega);
	if(diff < 1) mod = 1 + (mod-1)*diff;
	if(thrust < 0) mod = 1;
	_prop->modPitch(mod);
    }
}

}; // namespace yasim
