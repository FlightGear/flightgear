#ifndef _AIRPLANE_HPP
#define _AIRPLANE_HPP

#include "ControlMap.hpp"
#include "Model.hpp"
#include "Wing.hpp"
#include "Rotor.hpp"
#include "Vector.hpp"
#include "Version.hpp"
#include <simgear/props/props.hxx>

namespace yasim {

class Gear;
class Hook;
class Launchbar;
class Thruster;
class Hitch;

/// The Airplane class ties together the different components
class Airplane : public Version {
    SGPropertyNode_ptr _wingsN;
public:
    Airplane();
    ~Airplane();

    void iterate(float dt);
    void calcFuelWeights();

    ControlMap* getControlMap() { return &_controls; }
    Model* getModel() { return &_model; }

    void setPilotPos(float* pos) { Math::set3(pos, _pilotPos); }
    void getPilotPos(float* out) { Math::set3(_pilotPos, out); }

    const void getPilotAccel(float* out);

    void setEmptyWeight(float weight) {  _emptyWeight = weight; }

    void setWing(Wing* wing) { _wing = wing; }
    Wing* getWing() { return _wing; }
    void setTail(Wing* tail) { _tail = tail; }
    void addVStab(Wing* vstab) { _vstabs.add(vstab); }

    void addFuselage(float* front, float* back, float width,
                     float taper=1, float mid=0.5,
                     float cx=1, float cy=1, float cz=1, float idrag=1);
    int addTank(float* pos, float cap, float fuelDensity);
    void addGear(Gear* g);
    void addHook(Hook* h) { _model.addHook(h); }
    void addLaunchbar(Launchbar* l) { _model.addLaunchbar(l); }
    void addThruster(Thruster* t, float mass, float* cg);
    void addBallast(float* pos, float mass);
    void addHitch(Hitch* h) { _model.addHitch(h); }

    int addWeight(float* pos, float size);
    void setWeight(int handle, float mass);

    void setApproach(float speed, float altitude, float aoa, float fuel, float gla);
    void setCruise(float speed, float altitude, float fuel, float gla);

    void setElevatorControl(int control);
    void addApproachControl(int control, float val);
    void addCruiseControl(int control, float val);

    void addSolutionWeight(bool approach, int idx, float wgt);

    const int numGear() { return _gears.size(); }
    Gear* getGear(int g) { return ((GearRec*)_gears.get(g))->gear; }
    Hook* getHook() { return _model.getHook(); }
    const int numHitches() { return _hitches.size(); }
    Hitch* getHitch(int h);
    Rotorgear* getRotorgear() { return _model.getRotorgear(); }
    Launchbar* getLaunchbar() { return _model.getLaunchbar(); }

    const int numThrusters() { return _thrusters.size(); }
    Thruster* getThruster(int n) {
        return ((ThrustRec*)_thrusters.get(n))->thruster; }
    
    const int numTanks() { return _tanks.size(); }
    void setFuelFraction(float frac); // 0-1, total amount of fuel
    // get fuel in kg
    const float getFuel(int tank) { return ((Tank*)_tanks.get(tank))->fill; }
    // set fuel in kg
    float setFuel(int tank, float fuel) { return ((Tank*)_tanks.get(tank))->fill = fuel; }
    // get fuel density in kg/m^3
    const float getFuelDensity(int tank) { return ((Tank*)_tanks.get(tank))->density; }
    const float getTankCapacity(int tank) { return ((Tank*)_tanks.get(tank))->cap; }

    void compile(); // generate point masses & such, then solve
    void initEngines();
    void stabilizeThrust();

    // Solution output values
    const int getSolutionIterations() { return _solutionIterations; }
    const float getDragCoefficient() { return _dragFactor; }
    const float getLiftRatio() { return _liftRatio; }
    const float getCruiseAoA() { return _cruiseAoA; }
    const float getTailIncidence() { return _tailIncidence; }
    const float getApproachElevator() { return _approachElevator.val; }
    const char* getFailureMsg() { return _failureMsg; }

    static void setupState(const float aoa, const float speed, const float gla, yasim::State* s); // utility
    void loadApproachControls() { loadControls(_approachControls); }
    void loadCruiseControls() { loadControls(_cruiseControls); }
    
    const float getCGHardLimitXMin() { return _cgMin; } // get min x-coordinate for c.g (from main gear)
    const float getCGHardLimitXMax() { return _cgMax; } // get max x-coordinate for c.g (from nose gear)
    const float getCGMAC(); // return c.g. x as fraction of MAC
    // set desired range for C.G. in % of MAC, 0% = leading edge, 100% trailing edge
    void  setDesiredCGRangeInPercentOfMAC(float MACPercentMin, float MACPercentMax) { _cgDesiredMin = MACPercentMin; _cgDesiredMax = MACPercentMax; }
    const float getCGSoftLimitXMin() { return _cgDesiredAft; }   // get x-coordinate limit calculated from MAC and setCGRange values
    const float getCGSoftLimitXMax() { return _cgDesiredFront; } // get x-coordinate limit calculated from MAC and setCGRange values
    void  setAutoBallast(bool allowed) { _autoBallast = allowed; }
    
private:
    struct Tank { float pos[3]; float cap; float fill;
        float density; int handle; };
    struct Fuselage { float front[3], back[3], width, taper, mid, _cx, _cy, _cz, _idrag;
        Vector surfs; };
    struct GearRec { Gear* gear; Surface* surf; float wgt; };
    struct ThrustRec { Thruster* thruster;
        int handle; float cg[3]; float mass; };
    struct Control { int control; float val; };
    struct WeightRec { int handle; Surface* surf; };
    struct SolveWeight { bool approach; int idx; float wgt; };
    struct ContactRec { Gear* gear; float p[3]; };

    void loadControls(Vector &controls);
    void runCruise();
    void runApproach();
    void solveGear();
    void solve();
    void solveHelicopter();
    float compileWing(Wing* w);
    void compileRotorgear();
    float compileFuselage(Fuselage* f);
    void compileGear(GearRec* gr);
    void applyDragFactor(float factor);
    void applyLiftRatio(float factor);
    void addContactPoint(float* pos);
    void compileContactPoints();
    float normFactor(float f);
    void updateGearState();
    void setupWeights(bool isApproach);
    void calculateCGHardLimits();
    
    Model _model;
    ControlMap _controls;

    float _emptyWeight;
    float _pilotPos[3];

    Wing* _wing;
    Wing* _tail;

    Vector _fuselages;
    Vector _vstabs;
    Vector _tanks;
    Vector _thrusters;
    float _ballast;

    Vector _gears;
    Vector _contacts; // non-gear ground contact points
    Vector _weights;
    Vector _surfs; // NON-wing Surfaces
    Vector _hitches; //for airtow and winch

    Vector _solveWeights;

    Vector _cruiseControls;
    State _cruiseState;
    float _cruiseP;
    float _cruiseT;
    float _cruiseSpeed;
    float _cruiseWeight;
    float _cruiseFuel;
    float _cruiseGlideAngle;

    Vector _approachControls;
    State _approachState;
    float _approachP;
    float _approachT;
    float _approachSpeed;
    float _approachAoA;
    float _approachWeight;
    float _approachFuel;
    float _approachGlideAngle;

    int _solutionIterations;
    float _dragFactor;
    float _liftRatio;
    float _cruiseAoA;
    float _tailIncidence;
    Control _approachElevator;
    const char* _failureMsg;

    float _cgMax;          // hard limits for cg from gear position
    float _cgMin;          // hard limits for cg from gear position
    float _cgDesiredMax;   // desired cg max in %MAC from config
    float _cgDesiredMin;   // desired cg min in %MAC from config
    float _cgDesiredFront; // calculated desired cg x max
    float _cgDesiredAft;   // calculated desired cg x min 
    bool _autoBallast = false;
};

}; // namespace yasim
#endif // _AIRPLANE_HPP
