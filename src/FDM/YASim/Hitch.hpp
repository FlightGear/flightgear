#ifndef _HITCH_HPP
#define _HITCH_HPP

#include <string>

#include <Main/fg_props.hxx>
#include <simgear/props/tiedpropertylist.hxx>

namespace yasim {

class Ground;
class RigidBody;
struct State;

class Hitch {
public:
    Hitch(const char *name);
    ~Hitch();

    // Externally set values
    void setPosition(float* position);
    void setOpen(bool isOpen);
    //void setName(const char *text);

    void setTowLength(float length);
    void setTowElasticConstant(float sc);
    void setTowBreakForce(float bf);
    void setTowWeightPerM(float rw);
    void setWinchMaxSpeed(float mws);
    void setWinchRelSpeed(float rws);
    void setWinchPosition(double *winchPosition); //in global coordinates!
    void setWinchPositionAuto(bool doit);
    void findBestAIObject(bool doit,bool running_as_autoconnect=false);
    void setWinchInitialTowLength(float length);
    void setWinchPower(float power);
    void setWinchMaxForce(float force);
    void setWinchMaxTowLength(float length);
    void setWinchMinTowLength(float length);
    void setMpAutoConnectPeriod(float dt);
    void setForceIsCalculatedByOther(bool b);
    
    void setGlobalGround(double *global_ground, float *global_vel);

    void getPosition(float* out);
    float getTowLength(void);

    void calcForce(Ground *g_cb, RigidBody* body, State* s);

    // Computed values: total force
    void getForce(float* force, float* off);

    void integrate (float dt);

    std::string getConnectedPropertyNode() const;
    void setConnectedPropertyNode(const char *nodename);

private:
    float _pos[3];
    bool _open;
    bool _oldOpen;
    float _towLength;
    float _towElasticConstant;
    float _towBrakeForce;
    float _towWeightPerM;
    float _winchMaxSpeed;
    float _winchRelSpeed;
    float _winchInitialTowLength;
    float _winchMaxTowLength;
    float _winchMinTowLength;
    float _winchPower;
    float _winchMaxForce;
    float _winchActualForce;
    double _winchPos[3];
    float _force[3];
    float _towEndForce[3];
    float _reportTowEndForce[3];
    float _forceMagnitude;
    double _global_ground[4];
    float _global_vel[3];
    char _name[256];
    State* _state;
    float _dist;
    float _timeLagCorrectedDist;
    SGPropertyNode_ptr _towEndNode;
    const char *_towEndPropertyName;
    bool _towEndIsConnectedToProperty;
    bool _nodeIsMultiplayer;
    bool _nodeIsAiAircraft;
    bool _forceIsCalculatedByMaster;
    int _nodeID;
    //const char *_ai_MP_callsign;
    bool _isSlave;
    float _mpAutoConnectPeriod;
    float _timeToNextAutoConnectTry;
    float _timeToNextReConnectTry;
    float _height_above_ground;
    float _winch_height_above_ground;
    float _loPosFrac;
    float _lowest_tow_height;
    float _speed_in_tow_direction;
    float _mp_time_lag;
    float _mp_last_reported_dist;
    float _mp_last_reported_v;
    float _mp_v;
    float _mp_dist;
    float _mp_lpos[3];
    float _mp_force[3];
    bool _mp_is_slave;
    bool _mp_open_last_state;

    bool _displayed_len_lower_dist_message;
    bool _last_wish;

    SGPropertyNode_ptr _node;
    simgear::TiedPropertyList _tiedProperties;
};

}; // namespace yasim
#endif // _HITCH_HPP
