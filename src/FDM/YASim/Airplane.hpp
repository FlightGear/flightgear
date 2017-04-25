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

    void getPilotAccel(float* out);

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

    int numGear() const { return _gears.size(); }
    Gear* getGear(int g) { return ((GearRec*)_gears.get(g))->gear; }
    Hook* getHook() const { return _model.getHook(); }
    int numHitches() const { return _hitches.size(); }
    Hitch* getHitch(int h);
    Rotorgear* getRotorgear() { return _model.getRotorgear(); }
    Launchbar* getLaunchbar() const { return _model.getLaunchbar(); }

    int numThrusters() const { return _thrusters.size(); }
    Thruster* getThruster(int n) {
        return ((ThrustRec*)_thrusters.get(n))->thruster; }
    
    int numTanks() const { return _tanks.size(); }
    void setFuelFraction(float frac); // 0-1, total amount of fuel
    // get fuel in kg
    float getFuel(int tank) const { return ((Tank*)_tanks.get(tank))->fill; }
    // set fuel in kg
    float setFuel(int tank, float fuel) { return ((Tank*)_tanks.get(tank))->fill = fuel; }
    // get fuel density in kg/m^3
    float getFuelDensity(int tank) const { return ((Tank*)_tanks.get(tank))->density; }
    float getTankCapacity(int tank) const { return ((Tank*)_tanks.get(tank))->cap; }

    void compile(); // generate point masses & such, then solve
    void initEngines();
    void stabilizeThrust();

    // Solution output values
    int getSolutionIterations() const { return _solutionIterations; }
    float getDragCoefficient() const { return _dragFactor; }
    float getLiftRatio() const { return _liftRatio; }
    float getCruiseAoA() const { return _cruiseAoA; }
    float getTailIncidence() const { return _tailIncidence; }
    float getApproachElevator() const { return _approachElevator.val; }
    const char* getFailureMsg() const { return _failureMsg; }

    void loadApproachControls() { loadControls(_approachControls); }
    void loadCruiseControls() { loadControls(_cruiseControls); }
    
    float getCGHardLimitXMin() const { return _cgMin; } // get min x-coordinate for c.g (from main gear)
    float getCGHardLimitXMax() const { return _cgMax; } // get max x-coordinate for c.g (from nose gear)
    float getCGMAC(); // return c.g. x as fraction of MAC
    // set desired range for C.G. in % of MAC, 0% = leading edge, 100% trailing edge
    void  setDesiredCGRangeInPercentOfMAC(float MACPercentMin, float MACPercentMax) { _cgDesiredMin = MACPercentMin; _cgDesiredMax = MACPercentMax; }
    float getCGSoftLimitXMin() const { return _cgDesiredAft; }   // get x-coordinate limit calculated from MAC and setCGRange values
    float getCGSoftLimitXMax() const { return _cgDesiredFront; } // get x-coordinate limit calculated from MAC and setCGRange values
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

    void loadControls(const Vector& controls);
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

    float _emptyWeight {0};
    float _pilotPos[3] {0, 0, 0};

    Wing* _wing {nullptr};
    Wing* _tail {nullptr};

    Vector _fuselages;
    Vector _vstabs;
    Vector _tanks;
    Vector _thrusters;
    float _ballast {0};

    Vector _gears;
    Vector _contacts; // non-gear ground contact points
    Vector _weights;
    Vector _surfs; // NON-wing Surfaces
    Vector _hitches; //for airtow and winch

    Vector _solveWeights;

    Vector _cruiseControls;
    State _cruiseState;
    Atmosphere _cruiseAtmo;
    float _cruiseSpeed{0};
    float _cruiseWeight{0};
    float _cruiseFuel{0};
    float _cruiseGlideAngle{0};

    Vector _approachControls;
    State _approachState;
    Atmosphere _approachAtmo;
    float _approachSpeed {0};
    float _approachAoA {0};
    float _approachWeight {0};
    float _approachFuel {0};
    float _approachGlideAngle {0};

    int _solutionIterations {0};
    float _dragFactor {1};
    float _liftRatio {1};
    float _cruiseAoA {0};
    float _tailIncidence {0};
    Control _approachElevator;
    const char* _failureMsg {0};
    
    float _cgMax {-1e6};         // hard limits for cg from gear position
    float _cgMin {1e6};          // hard limits for cg from gear position
    float _cgDesiredMax {0.3f};  // desired cg max in %MAC from config
    float _cgDesiredMin {0.25f}; // desired cg min in %MAC from config
    float _cgDesiredFront {0};   // calculated desired cg x max
    float _cgDesiredAft {0};     // calculated desired cg x min 
    bool _autoBallast = false;
};

}; // namespace yasim
#endif // _AIRPLANE_HPP
