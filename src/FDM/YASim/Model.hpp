#ifndef _MODEL_HPP
#define _MODEL_HPP

#include "Integrator.hpp"
#include "RigidBody.hpp"
#include "BodyEnvironment.hpp"
#include "Vector.hpp"
#include "Turbulence.hpp"
#include "Rotor.hpp"
#include "Atmosphere.hpp"
#include <simgear/props/props.hxx>

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
    void getCG(float* cg) const { return _body.getCG(cg); }
    float getMass() const {return _body.getTotalMass(); }
    Integrator* getIntegrator() { return &_integrator; }

    void setTurbulence(Turbulence* turb) { _turb = turb; }

    State* getState() const { return _s; }
    void setState(State* s);

    bool isCrashed() const { return _crashed; } 
    void setCrashed(bool crashed) { _crashed = crashed; }
    float getAGL() const { return _agl; }

    void iterate();

    // Externally-managed subcomponents
    int addThruster(Thruster* t) { return _thrusters.add(t); }
    int addSurface(Surface* surf) { return _surfaces.add(surf); }
    int addGear(Gear* gear) { return _gears.add(gear); }
    void addHook(Hook* hook) { _hook = hook; }
    void addLaunchbar(Launchbar* launchbar) { _launchbar = launchbar; }
    Surface* getSurface(int handle) const { return (Surface*)_surfaces.get(handle); }
    Rotorgear* getRotorgear(void) { return &_rotorgear; }
    Hook* getHook(void) const { return _hook; }
    int addHitch(Hitch* hitch) { return _hitches.add(hitch); }
    Launchbar* getLaunchbar(void) const { return _launchbar; }

    // Semi-private methods for use by the Airplane solver.
    int numThrusters() const { return _thrusters.size(); }
    Thruster* getThruster(int handle) { return (Thruster*)_thrusters.get(handle); }
    void setThruster(int handle, Thruster* t) { _thrusters.set(handle, t); }
    void initIteration();
    void getThrust(float* out) const;

    void setGroundCallback(Ground* ground_cb);
    Ground* getGroundCallback(void) { return _ground_cb; }

    //
    // Per-iteration settables
    //
    void setGroundEffect(const float* pos, float span, float mul);
    void setWind(float* wind) { Math::set3(wind, _wind); }
    void setAtmosphere(Atmosphere a) { _atmo = a; };
    void setStandardAtmosphere(float altitude) { _atmo.setStandard(altitude); };

    void updateGround(State* s);

    // BodyEnvironment callbacks
    virtual void calcForces(State* s);
    virtual void newState(State* s);

private:
    void initRotorIteration();
    void calcGearForce(Gear* g, float* v, float* rot, float* ground);
    float gearFriction(float wgt, float v, Gear* g);
    void localWind(const float* pos, const yasim::State* s, float* out, float alt, bool is_rotor = false);

    Integrator _integrator;
    RigidBody _body;

    Turbulence* _turb {nullptr};

    Vector _thrusters;
    Vector _surfaces;
    Rotorgear _rotorgear;
    Vector _gears;
    Hook* _hook {nullptr};
    Launchbar* _launchbar {nullptr};
    Vector _hitches;

    float _wingSpan {0};
    float _groundEffect {0};
    float _geRefPoint[3] {0,0,0};

    Ground* _ground_cb;
    double _global_ground[4] {0,0,1, -1e5};
    Atmosphere _atmo;
    float _wind[3] {0,0,0};
    

    // Accumulators for the total internal gyro and engine torque
    float _gyro[3] {0,0,0};
    float _torque[3] {0,0,0};

    State* _s;
    bool _crashed {false};
    float _agl {0};
    SGPropertyNode_ptr _modelN;  
    SGPropertyNode_ptr _fAeroXN;
    SGPropertyNode_ptr _fAeroYN;
    SGPropertyNode_ptr _fAeroZN;
    SGPropertyNode_ptr _fSumXN;
    SGPropertyNode_ptr _fSumYN;
    SGPropertyNode_ptr _fSumZN;
    SGPropertyNode_ptr _fGravXN;
    SGPropertyNode_ptr _fGravYN;
    SGPropertyNode_ptr _fGravZN;
    SGPropertyNode_ptr _gefxN;
    SGPropertyNode_ptr _gefyN;
    SGPropertyNode_ptr _gefzN;
    SGPropertyNode_ptr _wgdistN;
};

}; // namespace yasim
#endif // _MODEL_HPP
