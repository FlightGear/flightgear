#ifndef _JET_HPP
#define _JET_HPP

#include "Thruster.hpp"

namespace yasim {

class Jet : public Thruster {
public:
    Jet();

    virtual Jet* getJet() { return this; }
    
    void setMaxThrust(float thrust, float afterburner=0);
    void setVMax(float spd);
    void setTSFC(float tsfc);
    void setRPMs(float idleN1, float maxN1, float idleN2, float maxN2);
    void setEGT(float takeoffEGT);
    void setEPR(float takeoffEPR);
    void setVectorAngle(float angle);

    // The time it takes the engine to reach 90% thrust from idle
    void setSpooling(float time);

    // Sets the reheat control
    void setReheat(float reheat);

    // Sets the thrust vector control (0-1)
    void setRotation(float rot);

    // Thrust reverser control.
    void setReverse(bool reverse) { _reverseThrust = reverse; }

    // Thrust reverser effectiveness.
    void setReverseThrust(float eff) { _reverseEff = eff; }

    float getN1();
    float getN2();
    float getEPR();
    float getEGT();

    // Normalized "performance" number.  Used for fuzzy numbers in FGFDM
    float getPerfNorm() { return (_n1 - _n1Min) / (_n1Max - _n1Min); }

    // From Thruster:
    virtual bool isRunning();
    virtual bool isCranking();
    virtual void getThrust(float* out);
    virtual void getTorque(float* out);
    virtual void getGyro(float* out);
    virtual float getFuelFlow();
    virtual void integrate(float dt);
    virtual void stabilize();

private:
    float _reheat;
    bool _reverseThrust;

    float _maxThrust; // Max dry thrust at sea level
    float _abFactor;  // Afterburner thrust multiplier

    float _maxRot;
    float _rotControl;

    float _decay;  // time constant for the exponential integration
    float _vMax;   // speed at which thrust is zero
    float _epr0;   // EPR at takeoff thrust
    float _tsfc;   // TSFC ((lb/hr)/lb) at takeoff thrust and zero airspeed
    float _egt0;   // EGT at takeoff thrust
    float _n1Min;  // N1 at ground idle
    float _n1Max;  // N1 at takeoff thrust
    float _n2Min;  // N2 at ground idle
    float _n2Max;  // N2 at takeoff thrust
    float _reverseEff; // Thrust reverser effectiveness (fraction)

    bool _running;   // Is the engine running?
    bool _cranking;  // Is the engine cranking?
    float _thrust;   // Current thrust
    float _epr;      // Current EPR
    float _n1;       // Current UNCORRECTED N1 (percent)
    float _n2;       // Current UNCORRECTED N2 (percent)
    float _fuelFlow; // Current UNCORRECTED fuel flow (kg/s)
    float _egt;      // Current UNCORRECTED EGT (kelvin)

    float _tempCorrect; // Intake temp / std temp (273 K)
    float _pressureCorrect; // Intake pressure / std pressure
};

}; // namespace yasim
#endif // _JET_HPP
