#ifndef _MODEL_HPP
#define _MODEL_HPP

#include "Integrator.hpp"
#include "RigidBody.hpp"
#include "BodyEnvironment.hpp"
#include "Vector.hpp"

namespace yasim {

// Declare the types whose pointers get passed around here
class Integrator;
class Thruster;
class Surface;
class Rotorpart;
class Rotorblade;
class Rotor;
class Gear;

class Model : public BodyEnvironment {
public:
    Model();
    virtual ~Model();

    RigidBody* getBody();
    Integrator* getIntegrator();

    State* getState();
    void setState(State* s);

    void resetState();
    bool isCrashed();
    void setCrashed(bool crashed);
    float getAGL();

    void iterate(float dt);

    // Externally-managed subcomponents
    int addThruster(Thruster* t);
    int addSurface(Surface* surf);
    int addRotorpart(Rotorpart* rpart);
    int addRotorblade(Rotorblade* rblade);
    int addRotor(Rotor* rotor);
    int addGear(Gear* gear);
    Surface* getSurface(int handle);
    Rotorpart* getRotorpart(int handle);
    Rotorblade* getRotorblade(int handle);
    Rotor* getRotor(int handle);
    Gear* getGear(int handle);

    // Semi-private methods for use by the Airplane solver.
    int numThrusters();
    Thruster* getThruster(int handle);
    void setThruster(int handle, Thruster* t);
    void initIteration(float dt);
    void getThrust(float* out);

    //
    // Per-iteration settables
    //
    void setGroundPlane(double* planeNormal, double fromOrigin);
    void setGroundEffect(float* pos, float span, float mul);
    void setWind(float* wind);
    void setAir(float pressure, float temp, float density);

    // BodyEnvironment callbacks
    virtual void calcForces(State* s);
    virtual void newState(State* s);

private:
    void calcGearForce(Gear* g, float* v, float* rot, float* ground);
    float gearFriction(float wgt, float v, Gear* g);
    float localGround(State* s, float* out);
    void localWind(float* pos, State* s, float* out);

    Integrator _integrator;
    RigidBody _body;

    Vector _thrusters;
    Vector _surfaces;
    Vector _rotorparts;
    Vector _rotorblades;
    Vector _rotors;
    Vector _gears;

    float _groundEffectSpan;
    float _groundEffect;
    float _wingCenter[3];

    double _ground[4];
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
