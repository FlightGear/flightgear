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

    RigidBody* getBody();
    Integrator* getIntegrator();

    void setTurbulence(Turbulence* turb) { _turb = turb; }

    State* getState();
    void setState(State* s);

    bool isCrashed();
    void setCrashed(bool crashed);
    float getAGL();

    void iterate();

    // Externally-managed subcomponents
    int addThruster(Thruster* t);
    int addSurface(Surface* surf);
    int addGear(Gear* gear);
    void addHook(Hook* hook);
    void addLaunchbar(Launchbar* launchbar);
    Surface* getSurface(int handle);
    Rotorgear* getRotorgear(void);
    Gear* getGear(int handle);
    Hook* getHook(void);
    int addHitch(Hitch* hitch);
    Launchbar* getLaunchbar(void);

    // Semi-private methods for use by the Airplane solver.
    int numThrusters();
    Thruster* getThruster(int handle);
    void setThruster(int handle, Thruster* t);
    void initIteration();
    void getThrust(float* out);

    void setGroundCallback(Ground* ground_cb);
    Ground* getGroundCallback(void);

    //
    // Per-iteration settables
    //
    void setGroundEffect(float* pos, float span, float mul);
    void setWind(float* wind);
    void setAir(float pressure, float temp, float density);

    void updateGround(State* s);

    // BodyEnvironment callbacks
    virtual void calcForces(State* s);
    virtual void newState(State* s);

private:
    void initRotorIteration();
    void calcGearForce(Gear* g, float* v, float* rot, float* ground);
    float gearFriction(float wgt, float v, Gear* g);
    void localWind(float* pos, State* s, float* out, float alt,
        bool is_rotor = false);

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

    float _groundEffectSpan;
    float _groundEffect;
    float _wingCenter[3];

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
