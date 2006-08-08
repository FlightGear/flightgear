#include "Atmosphere.hpp"
#include "Math.hpp"
#include "Jet.hpp"
namespace yasim {

Jet::Jet()
{
    _maxThrust = 0;
    _abFactor = 1;
    _reheat = 0;
    _rotControl = 0;
    _maxRot = 0;
    _reverseThrust = false;

    // Initialize parameters for an early-ish subsonic turbojet.  More
    // recent turbofans will typically have a lower vMax, epr0, and
    // tsfc.
    _vMax = 800;
    _epr0 = 3.0;
    _tsfc = 0.8f;
    _egt0 = 1050;
    _n1Min = 55;
    _n1Max = 102;
    _n2Min = 73;
    _n2Max = 103;
    setSpooling(4); // 4 second spool time? s'bout right.

    // And initialize to an engine that is idling
    _n1 = _n1Min;
    _n2 = _n2Min;

    // And sanify the remaining junk, just in case.
    _running = true;
    _cranking = false;
    _fuel = true;
    _epr = 1;
    _fuelFlow = 0;
    _egt = 273;
    _tempCorrect = 1;
    _pressureCorrect = 1;
}

void Jet::stabilize()
{
    // Just run it for an hour, there's no need to iterate given the
    // algorithms used.
    integrate(3600);
}

void Jet::setMaxThrust(float thrust, float afterburner)
{
    _maxThrust = thrust;
    if(afterburner == 0) _abFactor = 1;
    else                 _abFactor = afterburner/thrust;
}

void Jet::setVMax(float spd)
{
    _vMax = spd;
}

void Jet::setTSFC(float tsfc)
{
    _tsfc = tsfc;
}

void Jet::setRPMs(float idleN1, float maxN1, float idleN2, float maxN2)
{
    _n1Min = idleN1;
    _n1Max = maxN1;
    _n2Min = idleN2;
    _n2Max = maxN2;
}

void Jet::setEGT(float takeoffEGT)
{
    _egt0 = takeoffEGT;
}

void Jet::setEPR(float takeoffEPR)
{
    _epr0 = takeoffEPR;
}

void Jet::setSpooling(float time)
{
    // 2.3 = -ln(0.1), i.e. x=2.3 is the 90% point we're defining
    // The extra fudge factor is there because the N1 speed (which
    // determines thrust) lags the N2 speed.
    _decay = 1.5f * 2.3f / time;
}

void Jet::setVectorAngle(float angle)
{
    _maxRot = angle;
}

void Jet::setReheat(float reheat)
{
    _reheat = Math::clamp(reheat, 0, 1);
}

void Jet::setRotation(float rot)
{
    if(rot < 0) rot = 0;
    if(rot > 1) rot = 1;
    _rotControl = rot;
}

float Jet::getN1()
{
    return _n1 * _tempCorrect;
}

float Jet::getN2()
{
    return _n2 * _tempCorrect;
}

float Jet::getEPR()
{
    return _epr;
}

float Jet::getEGT()
{
    // Exactly zero means "off" -- return the ambient temperature
    if(_egt == 0) return _temp;

    return _egt * _tempCorrect * _tempCorrect;
}

float Jet::getFuelFlow()
{
    return _fuelFlow * _pressureCorrect;
}

void Jet::integrate(float dt)
{
    // Sea-level values
    const static float P0 = Atmosphere::getStdPressure(0);
    const static float T0 = Atmosphere::getStdTemperature(0);
    const static float D0 = Atmosphere::getStdDensity(0);

    float spd = -Math::dot3(_wind, _dir);

    float statT, statP, statD;
    Atmosphere::calcStaticAir(_pressure, _temp, _rho, spd,
                              &statP, &statT, &statD);
    _pressureCorrect = statP/P0;
    _tempCorrect = Math::sqrt(statT/T0);

    // Handle running out of fuel.  This is a hack.  What should
    // really happen is a simulation of ram air torque on the
    // turbine.  This just forces the engine into ground idle.
    if(_fuel == false)
        _throttle = 0;

    // Linearly taper maxThrust to zero at vMax
    float vCorr = spd<0 ? 1 : (spd<_vMax ? 1-spd/_vMax : 0);

    float maxThrust = _maxThrust * vCorr * (statD/D0);
    float setThrust = maxThrust * _throttle;

    // Now get a "beta" (i.e. EPR - 1) value.  The output values are
    // expressed as functions of beta.
    float ibeta0 = 1/(_epr0 - 1);
    float betaTarget = (_epr0 - 1) * (setThrust/_maxThrust) * (P0/_pressure)
	* (_temp/statT);
    float n2Target = _n2Min + (betaTarget*ibeta0) * (_n2Max - _n2Min);

    // Note that this "first" beta value is used to compute a target
    // for N2 only Integrate the N2 speed and back-calculate a beta1
    // target.  The N1 speed will seek to this.
    _n2 = (_n2 + dt*_decay * n2Target) / (1 + dt*_decay);

    float betaN2 = (_epr0-1) * (_n2 - _n2Min) / (_n2Max - _n2Min);
    float n1Target = _n1Min + betaN2*ibeta0 * (_n1Max - _n1Min);
    _n1 = (_n1 + dt*_decay * n1Target) / (1 + dt*_decay);

    // The actual thrust produced is keyed to the N1 speed.  Add the
    // afterburners in at the end.
    float betaN1 =  (_epr0-1) * (_n1 - _n1Min) / (_n1Max - _n1Min);
    _thrust = _maxThrust * betaN1/((_epr0-1)*(P0/_pressure)*(_temp/statT));
    _thrust *= 1 + _reheat*(_abFactor-1);

    // Finally, calculate the output variables.   Use a 80/20 mix of
    // the N2/N1 speeds as the key.
    float beta = 0.8f*betaN2 + 0.2f*betaN1;
    _epr = beta + 1;
    float ff0 = _maxThrust*_tsfc*(1/(3600.0f*9.8f)); // takeoff fuel flow, kg/s
    _fuelFlow = ff0 * beta*ibeta0;
    _fuelFlow *= 1 + (3.5f * _reheat * _abFactor); // Afterburners take
						  // 3.5 times as much
						  // fuel per thrust unit
    _egt = T0 + beta*ibeta0 * (_egt0 - T0);

    // Thrust reverse handling:
    if(_reverseThrust) _thrust *= -_reverseEff;
}

bool Jet::isRunning()
{
    return _running;
}

bool Jet::isCranking()
{
    return _cranking;
}

void Jet::getThrust(float* out)
{
    Math::mul3(_thrust, _dir, out);

    // Rotate about the Y axis for thrust vectoring
    float angle = _rotControl * _maxRot;
    float s = Math::sin(angle);
    float c = Math::cos(angle);
    float o0 = out[0];
    out[0] =  c * o0 + s * out[2];
    out[2] = -s * o0 + c * out[2];
}

void Jet::getTorque(float* out)
{
    out[0] = out[1] = out[2] = 0;
    return;
}

void Jet::getGyro(float* out)
{
    out[0] = out[1] = out[2] = 0;
    return;
}

}; // namespace yasim
