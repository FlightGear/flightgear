#ifndef _AIRPLANE_HPP
#define _AIRPLANE_HPP

#include "ControlMap.hpp"
#include "Model.hpp"
#include "Wing.hpp"
#include "Rotor.hpp"
#include "Vector.hpp"

namespace yasim {

class Gear;
class Hook;
class Launchbar;
class Thruster;
class Hitch;

class Airplane {
public:
    Airplane();
    ~Airplane();

    void iterate(float dt);
    void calcFuelWeights();

    ControlMap* getControlMap();
    Model* getModel();

    void setPilotPos(float* pos);
    void getPilotPos(float* out);

    void getPilotAccel(float* out);

    void setWeight(float weight);

    void setWing(Wing* wing);
    void setTail(Wing* tail);
    void addVStab(Wing* vstab);

    void addFuselage(float* front, float* back, float width,
                     float taper=1, float mid=0.5,
                     float cx=1, float cy=1, float cz=1, float idrag=1);
    int addTank(float* pos, float cap, float fuelDensity);
    void addGear(Gear* g);
    void addHook(Hook* h);
    void addLaunchbar(Launchbar* l);
    void addThruster(Thruster* t, float mass, float* cg);
    void addBallast(float* pos, float mass);
    void addHitch(Hitch* h);

    int addWeight(float* pos, float size);
    void setWeight(int handle, float mass);

    void setApproach(float speed, float altitude, float aoa, float fuel, float gla);
    void setCruise(float speed, float altitude, float fuel, float gla);

    void setElevatorControl(int control);
    void addApproachControl(int control, float val);
    void addCruiseControl(int control, float val);

    void addSolutionWeight(bool approach, int idx, float wgt);

    int numGear();
    Gear* getGear(int g);
    Hook* getHook();
    int numHitches() { return _hitches.size(); }
    Hitch* getHitch(int h);
    Rotorgear* getRotorgear();
    Launchbar* getLaunchbar();

    int numThrusters() { return _thrusters.size(); }
    Thruster* getThruster(int n) {
        return ((ThrustRec*)_thrusters.get(n))->thruster; }
    
    int numTanks();
    void setFuelFraction(float frac); // 0-1, total amount of fuel
    float getFuel(int tank); // in kg!
    float setFuel(int tank, float fuel); // in kg!
    float getFuelDensity(int tank); // kg/m^3
    float getTankCapacity(int tank);

    void compile(); // generate point masses & such, then solve
    void initEngines();
    void stabilizeThrust();

    // Solution output values
    int getSolutionIterations();
    float getDragCoefficient();
    float getLiftRatio();
    float getCruiseAoA();
    float getTailIncidence();
    float getApproachElevator() { return _approachElevator.val; }
    const char* getFailureMsg();

    static void setupState(float aoa, float speed, float gla, State* s); // utility

private:
    struct Tank { float pos[3]; float cap; float fill;
	          float density; int handle; };
    struct Fuselage { float front[3], back[3], width, taper, mid, _cx, _cy, _cz, _idrag; };
    struct GearRec { Gear* gear; Surface* surf; float wgt; };
    struct ThrustRec { Thruster* thruster;
	               int handle; float cg[3]; float mass; };
    struct Control { int control; float val; };
    struct WeightRec { int handle; Surface* surf; };
    struct SolveWeight { bool approach; int idx; float wgt; };
    struct ContactRec { Gear* gear; float p[3]; };

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
    float clamp(float val, float min, float max);
    void addContactPoint(float* pos);
    void compileContactPoints();
    float normFactor(float f);
    void updateGearState();
    void setupWeights(bool isApproach);

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
};

}; // namespace yasim
#endif // _AIRPLANE_HPP
