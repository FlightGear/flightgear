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

    enum Configuration {
        NONE,
        APPROACH,
        CRUISE,
        TAKEOFF,  // for testing
        TEST,     // for testing
    };
    
    void iterate(float dt);
    void calcFuelWeights();

    ControlMap* getControlMap() { return &_controlMap; }
    Model* getModel() { return &_model; }

    void setPilotPos(const float* pos) { Math::set3(pos, _pilotPos); }
    void getPilotPos(float* out) { Math::set3(_pilotPos, out); }
    void getPilotAccel(float* out);

    void  setEmptyWeight(float weight) {  _emptyWeight = weight; }
    float getEmptyWeight() const {  return _emptyWeight; }

    Wing* getWing();
    bool  hasWing() const { return (_wing != nullptr); }
    Wing* getTail(); 
    void  addVStab(Wing* vstab) { _vstabs.add(vstab); }

    void addFuselage(const float* front, const float* back, float width,
                     float taper=1, float mid=0.5f,
                     float cx=1, float cy=1, float cz=1, float idrag=1);
    int  addTank(const float* pos, float cap, float fuelDensity);
    void addGear(Gear* g);
    void addHook(Hook* h) { _model.addHook(h); }
    void addLaunchbar(Launchbar* l) { _model.addLaunchbar(l); }
    void addThruster(Thruster* thruster, float mass, const float* cg);
    void addBallast(const float* pos, float mass);
    void addHitch(Hitch* h) { _model.addHitch(h); }

    int  addWeight(const float* pos, float size);
    void setWeight(int handle, float mass);

    void setConfig(Configuration cfg, float speed, float altitude, float fuel, 
                   float gla = 0, float aoa = 0);
    
    /// add (fixed) control setting to approach/cruise config (for solver)
    void addControlSetting(Configuration cfg, const char* prop, float val);
    /// add a control input mapping for runtime
    void addControlInput(const char* propName, ControlMap::ControlType type, void* obj, int subobj, int opt, float src0, float src1, float dst0, float dst1);
    void addSolutionWeight(Configuration cfg, int idx, float wgt);

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
    /// get fuel in kg
    float getFuel(int tank) const { return ((Tank*)_tanks.get(tank))->fill; }
    /// set fuel in kg
    float setFuel(int tank, float fuel) { return ((Tank*)_tanks.get(tank))->fill = fuel; }
    /// get fuel density in kg/m^3
    float getFuelDensity(int tank) const { return ((Tank*)_tanks.get(tank))->density; }
    float getTankCapacity(int tank) const { return ((Tank*)_tanks.get(tank))->cap; }

    void compile(bool verbose = false); // generate point masses & such, then solve
    void initEngines();
    void stabilizeThrust();

    // Solution output values
    int getSolutionIterations() const { return _solutionIterations; }
    float getDragCoefficient() const { return _dragFactor; }
    float getLiftRatio() const { return _liftRatio; }
    float getCruiseAoA() const { return _config[CRUISE].aoa; }
    float getTailIncidence()const;
    float getApproachElevator() const;
    const char* getFailureMsg() const { return _failureMsg; }
    float getMass() const { return _model.getMass(); };
    
    // next two are used only in yasim CLI tool
    void setApproachControls() { setControlValues(_config[APPROACH].controls); }
    void setCruiseControls() { setControlValues(_config[CRUISE].controls); }
    
    float getCGHardLimitXMin() const { return _cgMin; } // get min x-coordinate for c.g (from main gear)
    float getCGHardLimitXMax() const { return _cgMax; } // get max x-coordinate for c.g (from nose gear)
    float getCGMAC(); // return c.g. x as fraction of MAC
    /// set desired range for C.G. in % of MAC, 0% = leading edge, 100% trailing edge
    void  setDesiredCGRangeInPercentOfMAC(float MACPercentMin, float MACPercentMax) { _cgDesiredMin = MACPercentMin; _cgDesiredMax = MACPercentMax; }
    float getCGSoftLimitXMin() const { return _cgDesiredAft; }   // get x-coordinate limit calculated from MAC and setCGRange values
    float getCGSoftLimitXMax() const { return _cgDesiredFront; } // get x-coordinate limit calculated from MAC and setCGRange values

    void  setMTOW(float mtow) { _mtow = mtow; }
    float getMTOW() const { return _mtow; }
    float getWingSpan() const;
    float getWingArea() const;
    float getWingLoadEmpty() const { return _getWingLoad(_emptyWeight); };
    float getWingLoadMTOW() const { return _getWingLoad(_mtow); };
    /// get x-distance between CG and 25% MAC of wing
    float getWingLever() const { return _getWingLever(_wing); };
    /// get x-distance between CG and 25% MAC of tail
    float getTailLever() const { return _getWingLever(_tail); };
    float getMaxThrust();
    float getThrust2WeightEmpty() { return getMaxThrust()/(_emptyWeight * KG2N); };
    float getThrust2WeightMTOW() { return getMaxThrust()/(_mtow*KG2N); };
    void  setSolverTweak(float f) { _solverDelta = f; };
    void  setSolverThreshold(float threshold) { _solverThreshold = threshold; };
    void  setSolverMaxIterations(int i) { _solverMaxIterations = i; };
    void  setSolverMode(int i) { _solverMode = i; };
    
private:
    struct Tank { 
      float pos[3] {0,0,0};
      float cap {0}, fill {0}, density {0};
      int handle {-1};
    };
    struct Fuselage { 
      float  front[3] {0,0,0}, back[3] {0,0,0};
      float  width, taper, mid, _cx, _cy, _cz, _idrag;
      Vector surfs;      
    };
    struct GearRec { 
      Gear* gear;
      Surface* surf;
      float wgt {0};
    };
    struct ThrustRec { 
      int handle {-1};
      Thruster* thruster {nullptr};
      float cg[3] {0,0,0};
      float mass {0};
    };
    struct ControlSetting { 
      int propHandle {-1};
      float val {0};
    };
    struct WeightRec { 
      int handle {-1};
      Surface* surf {nullptr};
    };
    struct SolveWeight { 
      int id {-1};
      Configuration cfg {APPROACH};
      float wgt {0};
    };
    struct ContactRec {
      Gear* gear {nullptr};
      float p[3] {0,0,0};
    };
    struct Config {
        Configuration id;
        float speed {0};
        float fuel {0};
        float glideAngle {0};
        float aoa {0};
        float altitude {0};
        float weight {0};
        State state;
        Vector controls;
    };
    Config _config[Configuration::TEST];

    /// load values for controls as defined in cruise/approach configuration
    void setControlValues(const Vector& controls);
    /// Helper for solve()
    void runConfig(Config &cfg);
    void solveGear();
    float _getPitch(Config &cfg);
    float _getLiftForce(Config &cfg);
    float _getDragForce(Config &cfg);
    float _checkConvergence(float prev, float current);
    void solveAirplane(bool verbose = false);
    void solveHelicopter(bool verbose = false);
    float compileWing(Wing* w);
    void compileRotorgear();
    float compileFuselage(Fuselage* f);
    void compileGear(GearRec* gr);
    void applyDragFactor(float factor);
    void applyLiftRatio(float factor);
    void addContactPoint(const float* pos);
    void compileContactPoints();
    float normFactor(float f);
    void updateGearState();
    void setupWeights(Configuration cfg);
    void calculateCGHardLimits();
    ///calculate mass divided by area of main wing
    float _getWingLoad(float mass) const;
    ///calculate distance between CGx and AC of wing w
    float _getWingLever(const Wing* w) const;
    ControlSetting* _addControlSetting(Configuration cfg, const char* prop, float val);
     ///set name of property controlling the elevator
    void setElevatorControl(const char* propName);
    /// set property name controling tail trim (incidence)
    void setHstabTrimControl(const char* propName);
    
    int   _solverMode {1};
    float _solverDelta {0.3226f};
    // How close to the solution are we trying get?  
    // Trying too hard can result in oscillations (no convergence). 
    float _solverThreshold {1};
    int   _solverMaxIterations {10000};
    Model _model;
    ControlMap _controlMap;

    float _emptyWeight {0};
    ///max take of weight
    float _mtow {0};
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

    int _solutionIterations {0};
    float _dragFactor {1};
    float _liftRatio {1};
    ControlSetting* _tailIncidence {nullptr}; // added to approach config so solver can change it
    // Copy of _tailIncidence added to cruise config. See setHstabTrimControl() for explanation.
    ControlSetting* _tailIncidenceCopy {nullptr}; 
    ControlSetting* _approachElevator {nullptr};
    const char* _failureMsg {0};
    /// hard limits for cg from gear position
    float _cgMax {-1e6};         
    /// hard limits for cg from gear position
    float _cgMin {1e6};          
    /// desired cg max in %MAC from config
    float _cgDesiredMax {0.3f};  
    /// desired cg min in %MAC from config
    float _cgDesiredMin {0.25f}; 
    /// calculated desired cg x max
    float _cgDesiredFront {0};   
    /// calculated desired cg x min 
    float _cgDesiredAft {0};     
};

}; // namespace yasim
#endif // _AIRPLANE_HPP
