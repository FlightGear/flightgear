#ifndef _MODEL_HPP
#define _MODEL_HPP

#include "Integrator.hpp"
#include "RigidBody.hpp"
#include "BodyEnvironment.hpp"
#include "Vector.hpp"
#include "Turbulence.hpp"
#include "Rotor.hpp"

namespace yasim {

// Declare the types whose pointers get passed around here
class Integrator;
class Thruster;
class Surface;
class Rotorpart;
class Gear;
class Ground;
class Hook;
class Launchbar;
class Hitch;

class Model : public BodyEnvironment {
public:
    Model();
    virtual ~Model();

    RigidBody* getBody() { return &_body; }
    Integrator* getIntegrator() { return &_integrator; }

    void setTurbulence(Turbulence* turb) { _turb = turb; }

    State* getState() { return _s; }
    void setState(State* s);

    bool isCrashed() { return _crashed; } 
    void setCrashed(bool crashed) { _crashed = crashed; }
    float getAGL() { return _agl; }

    void iterate();

    // Externally-managed subcomponents
    int addThruster(Thruster* t) { return _thrusters.add(t); }
    int addSurface(Surface* surf) { return _surfaces.add(surf); }
    int addGear(Gear* gear) { return _gears.add(gear); }
    void addHook(Hook* hook) { _hook = hook; }
    void addLaunchbar(Launchbar* launchbar) { _launchbar = launchbar; }
    Surface* getSurface(int handle) { return (Surface*)_surfaces.get(handle); }
    Rotorgear* getRotorgear(void) { return &_rotorgear; }
    Gear* getGear(int handle);
    Hook* getHook(void) { return _hook; }
    int addHitch(Hitch* hitch) { return _hitches.add(hitch); }
    Launchbar* getLaunchbar(void) { return _launchbar; }

    // Semi-private methods for use by the Airplane solver.
    int numThrusters() { return _thrusters.size(); }
    Thruster* getThruster(int handle) { return (Thruster*)_thrusters.get(handle); }
    void setThruster(int handle, Thruster* t) { _thrusters.set(handle, t); }
    void initIteration();
    void getThrust(float* out);

    void setGroundCallback(Ground* ground_cb);
    Ground* getGroundCallback(void) { return _ground_cb; }

    //
    // Per-iteration settables
    //
    void setGroundEffect(const float* pos, const float span, const float mul);
    void setWind(float* wind) { Math::set3(wind, _wind); }
    void setAir(const float pressure, const float temp, const float density);

    void updateGround(State* s);

    // BodyEnvironment callbacks
    virtual void calcForces(State* s);
    virtual void newState(State* s);

private:
    void initRotorIteration();
    void calcGearForce(Gear* g, float* v, float* rot, float* ground);
    float gearFriction(float wgt, float v, Gear* g);
    void localWind(const float* pos, State* s, float* out, float alt, bool is_rotor = false);

    Integrator _integrator;
    RigidBody _body;

    Turbulence* _turb;

    Vector _thrusters;
    Vector _surfaces;
    Rotorgear _rotorgear;
    Vector _gears;
    Hook* _hook;
    Launchbar* _launchbar;
    Vector _hitches;

    float _wingSpan;
    float _groundEffect;
    float _geRefPoint[3];

    Ground* _ground_cb;
    double _global_ground[4];
    float _pressure;
    float _temp;
    float _rho;
    float _wind[3];

    // Accumulators for the total internal gyro and engine torque
    float _gyro[3];
    float _torque[3];

    State* _s;
    bool _crashed;
    float _agl;
};

}; // namespace yasim
#endif // _MODEL_HPP
