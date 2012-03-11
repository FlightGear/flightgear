#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include "Math.hpp"
#include "BodyEnvironment.hpp"
#include "RigidBody.hpp"
#include <string.h>
#include <sstream>



#include "Hitch.hpp"

using std::vector;

namespace yasim {
Hitch::Hitch(const char *name)
{
    if (fgGetNode("/sim/hitches", true))
        _node = fgGetNode("/sim/hitches", true)->getNode(name, true);
    else _node = 0;
    int i;
    for(i=0; i<3; i++)
        _pos[i] = _force[i] = _winchPos[i] = _mp_lpos[i]=_towEndForce[i]=_mp_force[i]=0;
    for(i=0; i<2; i++)
        _global_ground[i] = 0;
    _global_ground[2] = 1;
    _global_ground[3] = -1e5;
    _forceMagnitude=0;
    _open=true;
    _oldOpen=_open;
    _towLength=60;
    _towElasticConstant=1e5;
    _towBrakeForce=100000;
    _towWeightPerM=1;
    _winchMaxSpeed=40;
    _winchRelSpeed=0;
    _winchInitialTowLength=1000;
    _winchPower=100000;
    _winchMaxForce=10000;
    _winchActualForce=0;
    _winchMaxTowLength=1000;
    _winchMinTowLength=0;
    _dist=0;
    _towEndIsConnectedToProperty=false;
    _towEndNode=0;
    _nodeIsMultiplayer=false;
    _nodeIsAiAircraft=false;
    _forceIsCalculatedByMaster=false;
    _nodeID=0;
    //_ai_MP_callsign=0;
    _height_above_ground=0;
    _winch_height_above_ground=0;
    _loPosFrac=0;
    _lowest_tow_height=0;
    _state=new State;
    _displayed_len_lower_dist_message=false;
    _last_wish=true;
    _isSlave=false;
    _mpAutoConnectPeriod=0;
    _timeToNextAutoConnectTry=0;
    _timeToNextReConnectTry=10;
    _speed_in_tow_direction=0;
    _mp_time_lag=1;
    _mp_last_reported_dist=0;
    _mp_last_reported_v=0;
    _mp_is_slave=false;
    _mp_open_last_state=false;
    _timeLagCorrectedDist=0;

    if (_node)
    {
#define TIE(x,v) _tiedProperties.Tie(_node->getNode(x, true),v)
        TIE("tow/length", SGRawValuePointer<float>(&_towLength));
        TIE("tow/elastic-constant",SGRawValuePointer<float>(&_towElasticConstant));
        TIE("tow/weight-per-m-kg-m",SGRawValuePointer<float>(&_towWeightPerM));
        TIE("tow/brake-force",SGRawValuePointer<float>(&_towBrakeForce));
        TIE("winch/max-speed-m-s",SGRawValuePointer<float>(&_winchMaxSpeed));
        TIE("winch/rel-speed",SGRawValuePointer<float>(&_winchRelSpeed));
        TIE("winch/initial-tow-length-m",SGRawValuePointer<float>(&_winchInitialTowLength));
        TIE("winch/min-tow-length-m",SGRawValuePointer<float>(&_winchMinTowLength));
        TIE("winch/max-tow-length-m",SGRawValuePointer<float>(&_winchMaxTowLength));
        TIE("winch/global-pos-x",SGRawValuePointer<double>(&_winchPos[0]));
        TIE("winch/global-pos-y",SGRawValuePointer<double>(&_winchPos[1]));
        TIE("winch/global-pos-z",SGRawValuePointer<double>(&_winchPos[2]));
        TIE("winch/max-power",SGRawValuePointer<float>(&_winchPower));
        TIE("winch/max-force",SGRawValuePointer<float>(&_winchMaxForce));
        TIE("winch/actual-force",SGRawValuePointer<float>(&_winchActualForce));
        TIE("tow/end-force-x",SGRawValuePointer<float>(&_reportTowEndForce[0]));
        TIE("tow/end-force-y",SGRawValuePointer<float>(&_reportTowEndForce[1]));
        TIE("tow/end-force-z",SGRawValuePointer<float>(&_reportTowEndForce[2]));
        TIE("force",SGRawValuePointer<float>(&_forceMagnitude));
        TIE("open",SGRawValuePointer<bool>(&_open));
        TIE("force-is-calculated-by-other",SGRawValuePointer<bool>(&_forceIsCalculatedByMaster));
        TIE("local-pos-x",SGRawValuePointer<float>(&_pos[0]));
        TIE("local-pos-y",SGRawValuePointer<float>(&_pos[1]));
        TIE("local-pos-z",SGRawValuePointer<float>(&_pos[2]));
        TIE("tow/dist",SGRawValuePointer<float>(&_dist));
        TIE("tow/dist-time-lag-corrected",SGRawValuePointer<float>(&_timeLagCorrectedDist));
        TIE("tow/connected-to-property-node",SGRawValuePointer<bool>(&_towEndIsConnectedToProperty));
        TIE("tow/connected-to-mp-node",SGRawValuePointer<bool>(&_nodeIsMultiplayer));
        TIE("tow/connected-to-ai-node",SGRawValuePointer<bool>(&_nodeIsAiAircraft));
        TIE("tow/connected-to-ai-or-mp-id",SGRawValuePointer<int>(&_nodeID));
        TIE("debug/hitch-height-above-ground",SGRawValuePointer<float>(&_height_above_ground));
        TIE("debug/tow-end-height-above-ground",SGRawValuePointer<float>(&_winch_height_above_ground));
        TIE("debug/tow-rel-lo-pos",SGRawValuePointer<float>(&_loPosFrac));
        TIE("debug/tow-lowest-pos-height",SGRawValuePointer<float>(&_lowest_tow_height));
        TIE("is-slave",SGRawValuePointer<bool>(&_isSlave));
        TIE("speed-in-tow-direction",SGRawValuePointer<float>(&_speed_in_tow_direction));
        TIE("mp-auto-connect-period",SGRawValuePointer<float>(&_mpAutoConnectPeriod));
        TIE("mp-time-lag",SGRawValuePointer<float>(&_mp_time_lag));
#undef TIE
        _node->setStringValue("tow/node","");
        _node->setStringValue("tow/connected-to-ai-or-mp-callsign");
        _node->setBoolValue("broken",false);
    }
}

Hitch::~Hitch()
{
    _tiedProperties.Untie();
    delete _state;
}

void Hitch::setPosition(float* position)
{
    int i;
    for(i=0; i<3; i++) _pos[i] = position[i];
}

void Hitch::setTowLength(float length)
{
    _towLength = length;
}

void Hitch::setOpen(bool isOpen)
{
   //test if we already processed this before
    //without this test a binded property could
    //try to close the Hitch every run
    //it will close, if we are near the end 
    //e.g. if we are flying over the parked 
    //tow-aircraft....
    if (isOpen==_last_wish) 
        return;
    _last_wish=isOpen;
    _open=isOpen;
}

void Hitch::setTowElasticConstant(float sc)
{
    _towElasticConstant=sc;
}

void Hitch::setTowBreakForce(float bf)
{
    _towBrakeForce=bf;
}

void Hitch::setWinchMaxForce(float f)
{
    _winchMaxForce=f;
}

void Hitch::setTowWeightPerM(float rw)
{
    _towWeightPerM=rw;
}

void Hitch::setWinchMaxSpeed(float mws)
{
    _winchMaxSpeed=mws;
}

void Hitch::setWinchRelSpeed(float rws)
{
    _winchRelSpeed=rws;
}

void Hitch::setWinchPosition(double *winchPosition)//in global coordinates!
{
    for (int i=0; i<3;i++)
        _winchPos[i]=winchPosition[i];
}

void Hitch::setMpAutoConnectPeriod(float dt)
{
    _mpAutoConnectPeriod=dt;
}

void Hitch::setForceIsCalculatedByOther(bool b)
{
    _forceIsCalculatedByMaster=b;
}

std::string Hitch::getConnectedPropertyNode() const
{
    if (_towEndNode)
        return _towEndNode->getDisplayName();
    else
        return std::string("");
}

void Hitch::setConnectedPropertyNode(const char *nodename)
{
    _towEndNode=fgGetNode(nodename,false);
}

void Hitch::setWinchPositionAuto(bool doit)
{
    static bool lastState=false;
    if(!_state)
        return;
    if (!doit)
    {
        lastState=false;
        return;
    }
    if(lastState)
        return;
    lastState=true;
    float lWinchPos[3];
    // The ground plane transformed to the local frame.
    float ground[4];
    _state->planeGlobalToLocal(_global_ground, ground);

    float help[3];
    //find a normalized vector pointing forward parallel to the ground
    help[0]=0;
    help[1]=1;
    help[2]=0;
    Math::cross3(ground,help,help);
    //multiplay by initial tow length;
    //reduced by 1m to be able to close the
    //hitch either if the glider slips backwards a bit
    Math::mul3((_winchInitialTowLength-1.),help,help);
    //add to the actual pos
    Math::add3(_pos,help,lWinchPos);
    //put it onto the ground plane
    Math::mul3(ground[3],ground,help);
    Math::add3(lWinchPos,help,lWinchPos);

    _state->posLocalToGlobal(lWinchPos,_winchPos);
    _towLength=_winchInitialTowLength;
    fgSetString("/sim/messages/pilot", "Connected to winch!");
    _open=false;

    _node->setBoolValue("broken",false);

    //set the dist value (if not, the hitch would open in the next calcforce run
    float delta[3];
    Math::sub3(lWinchPos,_pos,delta);
    _dist=Math::mag3(delta);
}

void Hitch::findBestAIObject(bool doit,bool running_as_autoconnect)
{
    static bool lastState=false;
    if(!_state)
        return;
    if (!running_as_autoconnect)
    {
        //if this function is binded to an input, it will be called every frame as long as the key is pressed.
        //therefore wait for a key-release before running it again.
        if (!doit)
        {
            lastState=false;
            return;
        }
        if(lastState)
            return;
        lastState=true;
    }
    double gpos[3];
    _state->posLocalToGlobal(_pos,gpos);
    double bestdist=_towLength*_towLength;//squared!
    _towEndIsConnectedToProperty=false;
    SGPropertyNode * ainode = fgGetNode("/ai/models",false);
    if(!ainode) return;
    char myCallsign[256]="***********";
    if (running_as_autoconnect)
    {
        //get own callsign
        SGPropertyNode *cs=fgGetNode("/sim/multiplay/callsign",false);
        if (cs)
        {
            strncpy(myCallsign,cs->getStringValue(),256);
            myCallsign[255]=0;
        }
        //reset tow length for search radius. Lentgh will be later copied from master 
        _towLength=_winchInitialTowLength;
    }
    bool found=false;
    vector <SGPropertyNode_ptr> nodes;//<SGPropertyNode_ptr>
    for (int i=0;i<ainode->nChildren();i++)
    {
        SGPropertyNode * n=ainode->getChild(i);
        _nodeIsMultiplayer = strncmp("multiplayer",n->getName(),11)==0;
        _nodeIsAiAircraft = strncmp("aircraft",n->getName(),8)==0;
        if (!(_nodeIsMultiplayer || _nodeIsAiAircraft))
            continue;
        if (running_as_autoconnect)
        {
            if (!_nodeIsMultiplayer)
                continue;
            if(n->getBoolValue("sim/hitches/aerotow/open",true)) continue;
            if(strncmp(myCallsign,n->getStringValue("sim/hitches/aerotow/tow/connected-to-ai-or-mp-callsign"),255)!=0)
                continue;
        }
        double pos[3];
        pos[0]=n->getDoubleValue("position/global-x",0);
        pos[1]=n->getDoubleValue("position/global-y",0);
        pos[2]=n->getDoubleValue("position/global-z",0);
        double dist=0;
        for (int j=0;j<3;j++)
            dist+=(pos[j]-gpos[j])*(pos[j]-gpos[j]);
        if (dist<bestdist)
        {
            bestdist=dist;
            _towEndNode=n;
            _towEndIsConnectedToProperty=true;
            //_node->setStringValue("tow/node",n->getPath());
            _node->setStringValue("tow/node",n->getDisplayName());
            _nodeID=n->getIntValue("id",0);
            _node->setStringValue("tow/connected-to-ai-or-mp-callsign",n->getStringValue("callsign"));
            _open=false;
            found = true;
        }
    }
    if (found)
    {
        if (!running_as_autoconnect)
        {
            std::stringstream message;
            message<<_node->getStringValue("tow/connected-to-ai-or-mp-callsign")
                    <<", I am on your hook, distance "<<Math::sqrt(bestdist)<<" meter.";
            fgSetString("/sim/messages/pilot", message.str().c_str());
        }
        else
        {
            std::stringstream message;
            message<<_node->getStringValue("tow/connected-to-ai-or-mp-callsign")
                <<": I am on your hook, distance "<<Math::sqrt(bestdist)<<" meter.";
            fgSetString("/sim/messages/ai-plane", message.str().c_str());
        }
        if (running_as_autoconnect)
            _isSlave=true;
        //set the dist value to some value below the tow lentgh (if not, the hitch
        //would open in the next calc force run
        _dist=_towLength*0.5;
        _mp_open_last_state=true;
    }
    else
        if (!running_as_autoconnect)
        {
            fgSetString("/sim/messages/atc", "Sorry, no aircraft for aerotow!");
        }
}

void Hitch::setWinchInitialTowLength(float length)
{
    _winchInitialTowLength=length;
}

void Hitch::setWinchPower(float power)
{
    _winchPower=power;
}

void Hitch::setWinchMaxTowLength(float length)
{
    _winchMaxTowLength=length;
}

void Hitch::setWinchMinTowLength(float length)
{
    _winchMinTowLength=length;
}

void Hitch::setGlobalGround(double *global_ground, float *global_vel)
{
    int i;
    for(i=0; i<4; i++) _global_ground[i] = global_ground[i];
    for(i=0; i<3; i++) _global_vel[i] = global_vel[i];
}

void Hitch::getPosition(float* out)
{
    int i;
    for(i=0; i<3; i++) out[i] = _pos[i];
}

float Hitch::getTowLength(void)
{
    return _towLength;
}

void Hitch::calcForce(Ground *g_cb, RigidBody* body, State* s)
{
    float lWinchPos[3],delta[3],deltaN[3];
    *_state=*s;
    s->posGlobalToLocal(_winchPos,lWinchPos);
    Math::sub3(lWinchPos,_pos,delta);
    if(!_towEndIsConnectedToProperty)
        _dist=Math::mag3(delta);
    else
        _dist=Math::mag3(lWinchPos); //use the aircraft center as reference for distance calculation
                                  //this position is transferred to the MP-Aircraft.
                                  //With this trick, both player in aerotow get the same length
    Math::unit3(delta,deltaN);
    float lvel[3];
    s->velGlobalToLocal(s->v,lvel);
    _speed_in_tow_direction=Math::dot3(lvel,deltaN);
    if (_towEndIsConnectedToProperty && _nodeIsMultiplayer)
    {
        float mp_delta_dist_due_to_time_lag=0.5*_mp_time_lag*(-_mp_v+_speed_in_tow_direction);
        _timeLagCorrectedDist=_dist+mp_delta_dist_due_to_time_lag;
        if(_forceIsCalculatedByMaster && !_open)
        {
            s->velGlobalToLocal(_mp_force,_force);
            return;
        }
    }
    else
        _timeLagCorrectedDist=_dist;
    if (_open)
    {
        _force[0]=_force[1]=_force[2]=0;
        return;
    }
    if(_dist>_towLength)
        if(_towLength>1e-3)
            _forceMagnitude=(_dist-_towLength)/_towLength*_towElasticConstant;
        else
            _forceMagnitude=2*_towBrakeForce;
    else
        _forceMagnitude=0;
    if(_forceMagnitude>=_towBrakeForce)
    {
        _forceMagnitude=0;
        _open=true;
        _node->setBoolValue("broken",true);
        _force[0]=_force[1]=_force[2]=0;
        _towEndForce[0]=_towEndForce[1]=_towEndForce[2]=0;
        _reportTowEndForce[0]=_reportTowEndForce[1]=_reportTowEndForce[2]=0;
        return;
    }
    Math::mul3(_forceMagnitude,deltaN,_force);
    Math::mul3(-1.,_force,_towEndForce);
    _winchActualForce=_forceMagnitude; //missing: gravity on this end and friction
    //Add the gravitiy of the rope.
    //calculate some numbers:
    float grav_force=_towWeightPerM*_towLength*9.81;
    //the length of the gravity-expanded row:
    float leng=_towLength+grav_force*_towLength/_towElasticConstant;
    // The ground plane transformed to the local frame.
    float ground[4];
    s->planeGlobalToLocal(_global_ground, ground);
        
    // The velocity of the contact patch transformed to local coordinates.
    //float glvel[3];
    //s->velGlobalToLocal(_global_vel, glvel);

    _height_above_ground = ground[3] - Math::dot3(_pos, ground);

    //the same for the winch-pos (the pos of the tow end)
    _winch_height_above_ground = ground[3] - Math::dot3(lWinchPos, ground);

    //the frac of the grav force acting on _pos:
    float grav_frac=0.5*(1+(_height_above_ground-_winch_height_above_ground)/leng);
    grav_frac=Math::clamp(grav_frac,0,1);
    float grav_frac_tow_end=1-grav_frac;
    //reduce grav_frac, if the tow has ground contact.
    if (_height_above_ground<leng) //if not, the tow can not be on ground
    {
        float fa[3],fb[3],fg[3];
        //the grav force an the hitch position:
        Math::mul3(-grav_frac*grav_force,ground,fg);
        //the total force on hitch postion:
        Math::add3(fg,_force,fa);
        //the grav force an the tow end position:
        Math::mul3(-(1-grav_frac)*grav_force,ground,fg);
        //the total force on tow end postion:
        //note: sub: _force on tow-end is negative of force on hitch postion
        Math::sub3(fg,_force,fb); 
        float fa_=Math::mag3(fa);
        float fb_=Math::mag3(fb);
        float stretchedTowLen;
        stretchedTowLen=_towLength*(1.+(fa_+fb_)/(2*_towElasticConstant));
        //the relative position of the lowest postion of the tow:
        if ((fa_+fb_)>1e-3)
            _loPosFrac=fa_/(fa_+fb_);
        else
            _loPosFrac=0.5;
        //dist to tow-end parallel to ground
        float ground_dist;
        float help[3];
        //Math::cross3(delta,ground,help);//as long as we calculate the dist without _pos, od it with lWinchpos, the dist to our center....
        Math::cross3(lWinchPos,ground,help);
        ground_dist=Math::mag3(help);
        //height of lowest tow pos (relative to _pos)
        _lowest_tow_height=_loPosFrac*Math::sqrt(Math::abs(stretchedTowLen*stretchedTowLen-ground_dist*ground_dist));
        if (_height_above_ground<_lowest_tow_height)
        {
            if (_height_above_ground>1e-3)
                grav_frac*=_height_above_ground/_lowest_tow_height;
            else
                grav_frac=0;
        }
        if (_winch_height_above_ground<(_lowest_tow_height-_height_above_ground+_winch_height_above_ground))
        {
            if (_winch_height_above_ground>1e-3)
                grav_frac_tow_end*=_winch_height_above_ground/
                    (_lowest_tow_height-_height_above_ground+_winch_height_above_ground);
            else
                grav_frac_tow_end=0;
        }
    }
    else _lowest_tow_height=_loPosFrac=-1; //for debug output
    float grav_force_v[3];
    Math::mul3(grav_frac*grav_force,ground,grav_force_v);
    Math::add3(grav_force_v,_force,_force);
    _forceMagnitude=Math::mag3(_force);
    //the same for the tow end:
    Math::mul3(grav_frac_tow_end*grav_force,ground,grav_force_v);
    Math::add3(grav_force_v,_towEndForce,_towEndForce);
    s->velLocalToGlobal(_towEndForce,_towEndForce);

    if(_forceMagnitude>=_towBrakeForce)
    {
        _forceMagnitude=0;
        _open=true;
        _node->setBoolValue("broken",true);
        _force[0]=_force[1]=_force[2]=0;
        _towEndForce[0]=_towEndForce[1]=_towEndForce[2]=0;
    }
    

}

// Computed values: total force
void Hitch::getForce(float* force, float* off)
{
    Math::set3(_force, force);
    Math::set3(_pos, off);
}

void Hitch::integrate (float dt)
{
    //check if hitch has opened or closed, if yes: message
    if (_open !=_oldOpen)
    {
        if (_oldOpen)
        {
            if (_dist>_towLength*1.00001)
            {
                std::stringstream message;
                message<<"Could not lock hitch (tow length is insufficient) on hitch "
                       <<_node->getName()<<" "<<_node->getIndex()<<"!";
                fgSetString("/sim/messages/pilot", message.str().c_str());
                _open=true;
                return;
            }
            _node->setBoolValue("broken",false);
        }
        std::stringstream message;
        if (_node->getBoolValue("broken",false)&&_open)
            message<<"Oh no, the tow is broken";
        else
            message<<(_open?"Opened hitch ":"Locked hitch ")<<_node->getName()<<" "<<_node->getIndex()<<"!";
        fgSetString("/sim/messages/pilot", message.str().c_str());
        _oldOpen=_open;
    }

    //check, if tow end should be searched in all MP-aircrafts
    if(_open && _mpAutoConnectPeriod)
    {
        _isSlave=false;
        _timeToNextAutoConnectTry-=dt;
        if ((_timeToNextAutoConnectTry>_mpAutoConnectPeriod) || (_timeToNextAutoConnectTry<0))
        {
            _timeToNextAutoConnectTry=_mpAutoConnectPeriod;
            //search for MP-Aircraft, which is towed with us
            findBestAIObject(true,true);
        }
    }
    //check, if tow end can be modified by property, if yes: update
    if(_towEndIsConnectedToProperty)
    {
        if (_node)
        {
            //_towEndNode=fgGetNode(_node->getStringValue("tow/node"), false);
            char towNode[256];
            strncpy(towNode,_node->getStringValue("tow/node"),256);
            towNode[255]=0;
            _towEndNode=fgGetNode("ai/models")->getNode(towNode, false);
            //AI and multiplayer objects seem to change node?
            //Check if we have the right one by callsign
            if (_nodeIsMultiplayer || _nodeIsAiAircraft)
            {
                char MPcallsign[256]="";
                const char *MPc;
                MPc=_node->getStringValue("tow/connected-to-ai-or-mp-callsign");
                if (MPc)
                {
                    strncpy(MPcallsign,MPc,256);
                    MPcallsign[255]=0;
                }
                if (((_towEndNode)&&(strncmp(_towEndNode->getStringValue("callsign"),MPcallsign,255)!=0))||!_towEndNode)
                {
                    _timeToNextReConnectTry-=dt;
                    if((_timeToNextReConnectTry<0)||(_timeToNextReConnectTry>10))
                    {
                        _timeToNextReConnectTry=10;
                        SGPropertyNode * ainode = fgGetNode("/ai/models",false);
                        if(ainode)
                        {
                            for (int i=0;i<ainode->nChildren();i++)
                            {
                                SGPropertyNode * n=ainode->getChild(i);
                                if(_nodeIsMultiplayer?strncmp("multiplayer",n->getName(),11)==0:strncmp("aircraft",n->getName(),8))
                                    if (strcmp(n->getStringValue("callsign"),MPcallsign)==0)//found
                                    {
                                        _towEndNode=n;
                                        //_node->setStringValue("tow/node",n->getPath());
                                        _node->setStringValue("tow/node",n->getDisplayName());
                                    }
                            }
                        }
                    }
                }
            }
            if(_towEndNode)
            {
                _winchPos[0]=_towEndNode->getDoubleValue("position/global-x",_winchPos[0]);
                _winchPos[1]=_towEndNode->getDoubleValue("position/global-y",_winchPos[1]);
                _winchPos[2]=_towEndNode->getDoubleValue("position/global-z",_winchPos[2]);
                _mp_lpos[0]=_towEndNode->getFloatValue("sim/hitches/aerotow/local-pos-x",0);
                _mp_lpos[1]=_towEndNode->getFloatValue("sim/hitches/aerotow/local-pos-y",0);
                _mp_lpos[2]=_towEndNode->getFloatValue("sim/hitches/aerotow/local-pos-z",0);
                _mp_dist=_towEndNode->getFloatValue("sim/hitches/aerotow/tow/dist");
                _mp_v=_towEndNode->getFloatValue("sim/hitches/aerotow/speed-in-tow-direction");
                _mp_force[0]=_towEndNode->getFloatValue("sim/hitches/aerotow/tow/end-force-x",0);
                _mp_force[1]=_towEndNode->getFloatValue("sim/hitches/aerotow/tow/end-force-y",0);
                _mp_force[2]=_towEndNode->getFloatValue("sim/hitches/aerotow/tow/end-force-z",0);

                if(_isSlave)
                {
#define gf(a,b) a=_towEndNode->getFloatValue(b,a)
#define gb(a,b) a=_towEndNode->getBoolValue(b,a)
                    gf(_towLength,"sim/hitches/aerotow/tow/length");
                    gf(_towElasticConstant,"sim/hitches/aerotow/tow/elastic-constant");
                    gf(_towWeightPerM,"sim/hitches/aerotow/tow/weight-per-m-kg-m");
                    gf(_towBrakeForce,"sim/hitches/aerotow/brake-force");
                    gb(_open,"sim/hitches/aerotow/open");
                    gb(_mp_is_slave,"sim/hitches/aerotow/is-slave");
#undef gf
#undef gb
                    if (_mp_is_slave) _isSlave=false; //someone should be master
                }
                else
                {
                    //check if other has opened hitch, but is neccessary, that it was closed before
                    bool mp_open=_towEndNode->getBoolValue("sim/hitches/aerotow/open",_mp_open_last_state);
                    if (mp_open != _mp_open_last_state) //state has changed
                    {
                        _mp_open_last_state=mp_open; //store that value
                        if(!_open)
                        {
                            if(mp_open)
                            {
                                _oldOpen=_open=true;
                                std::stringstream message;
                                message<<_node->getStringValue("tow/connected-to-ai-or-mp-callsign")
                                    <<": I have released the tow!";
                                fgSetString("/sim/messages/ai-plane", message.str().c_str());
                            }
                        }
                    }
                }
                //try to calculate the time lag
                if ((_mp_last_reported_dist!=_mp_dist)||(_mp_last_reported_v!=_mp_v)) //new data;
                {
                    _mp_last_reported_dist=_mp_dist;
                    _mp_last_reported_v=_mp_v;
                    float total_v=-_mp_v+_speed_in_tow_direction;//mp has opposite tow direction
                    float abs_v=Math::abs(total_v);
                    if (abs_v>0.1)
                    {
                        float actual_time_lag_guess=(_mp_dist-_dist)/total_v;
                        //check, if it sounds ok
                        if((actual_time_lag_guess>0)&&(actual_time_lag_guess<5))
                        {
                            float frac=abs_v*0.01;
                            if (frac>0.05) frac=0.05;
                            // if we are slow, the guess of the lag can be rather wrong. as faster we are
                            // the better the guess. Therefore frac is proportiona to the speed. Clamp it
                            // at 5m/s
                            _mp_time_lag=(1-frac)*_mp_time_lag+frac*actual_time_lag_guess;
                        }
                    }
                }
            }
        }
    }
    //set the _reported_tow_end_force
    Math::set3(_towEndForce,_reportTowEndForce);

    if (_open) return;
    if (_winchRelSpeed==0) return;
    float factor=1,offset=0;
    if (_winchActualForce>_winchMaxForce)
        offset=-(_winchActualForce-_winchMaxForce)/_winchMaxForce*20;
    if (_winchRelSpeed>0.01) // to avoit div by zero
    {
        float maxForcePowerLimit=_winchPower/(_winchRelSpeed*_winchMaxSpeed);
        if (_winchActualForce>maxForcePowerLimit)
            factor-=(_winchActualForce-maxForcePowerLimit)/maxForcePowerLimit;
    }
    _towLength-=dt*(factor*_winchRelSpeed+offset)*_winchMaxSpeed;
    if (_towLength<=_winchMinTowLength)
    {
        if (_winchRelSpeed<0)
            _winchRelSpeed=0;
        _towLength=_winchMinTowLength;
        return;
    }
    if (_towLength>=_winchMaxTowLength)
    {
        if (_winchRelSpeed<0)
            _winchRelSpeed=0;
        _towLength=_winchMaxTowLength;
        return;
    }
}




}; // namespace yasim
