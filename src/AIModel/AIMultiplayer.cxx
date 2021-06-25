// FGAIMultiplayer - FGAIBase-derived class creates an AI multiplayer aircraft
//
// Based on FGAIAircraft
// Written by David Culp, started October 2003.
// Also by Gregor Richards, started December 2005.
//
// Copyright (C) 2003  David P. Culp - davidculp2@comcast.net
// Copyright (C) 2005  Gregor Richards
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 2 of the
// License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <string>
#include <stdio.h>

#include <Aircraft/replay.hxx>
#include <Main/globals.hxx>
#include <Main/fg_props.hxx>
#include <Time/TimeManager.hxx>

#include "AIMultiplayer.hxx"

using std::string;

// #define SG_DEBUG SG_ALERT

FGAIMultiplayer::FGAIMultiplayer() :
   FGAIBase(otMultiplayer, fgGetBool("/sim/multiplay/hot", false))
{
    no_roll = false;

    mTimeOffsetSet = false;
    mAllowExtrapolation = true;
    mLagAdjustSystemSpeed = 10;
    mLastTimestamp = 0;
    lastUpdateTime = 0;
    playerLag = 0.03;
    compensateLag = 1;
    realTime = false;
    lastTime=0.0;
    lagPpsAveraged = 1.0;
    rawLag = 0.0;
    rawLagMod = 0.0;
    lagModAveraged = 0.0;
    _searchOrder = PREFER_DATA;
   
    m_simple_time_enabled       = fgGetNode("/sim/time/simple-time/enabled", true);
    m_sim_replay_replay_state   = fgGetNode("/sim/replay/replay-state", true);
    m_sim_replay_time           = fgGetNode("/sim/replay/time", true);
   
    m_simple_time_first_time = true;
    m_simple_time_compensation = 0.0;
    m_simple_time_recent_packet_time = 0;
    mLogRawSpeedMultiplayer = fgGetNode("/sim/replay/log-raw-speed-multiplayer", true);
}

FGAIMultiplayer::~FGAIMultiplayer() {
}

bool FGAIMultiplayer::init(ModelSearchOrder searchOrder)
{
    props->setStringValue("sim/model/path", model_path);
    props->setIntValue("sim/model/fallback-model-index", _getFallbackModelIndex());
    //refuel_node = fgGetNode("systems/refuel/contact", true);
    isTanker = false; // do this until this property is
                      // passed over the net

    const string& str1 = _getCallsign();
    const string str2 = "MOBIL";

    string::size_type loc1= str1.find( str2, 0 );
    if ( (loc1 != string::npos && str2 != "") ){
        // cout << " string found " << str2 << " in " << str1 << endl;
        isTanker = true;
        // cout << "isTanker " << isTanker << " " << mCallSign <<endl;
    }

    // load model
    bool result = FGAIBase::init(searchOrder);
    // propagate installation state (used by MP pilot list)
    props->setBoolValue("model-installed", _installed);
    
    m_node_simple_time_latest           = props->getNode("simple-time/latest", true);
    m_node_simple_time_offset           = props->getNode("simple-time/offset", true);
    m_node_simple_time_offset_smoothed  = props->getNode("simple-time/offset-smoothed", true);
    m_node_simple_time_compensation     = props->getNode("simple-time/compensation", true);
   
    return result;
}

void FGAIMultiplayer::bind() {
    FGAIBase::bind();

    //2018.1 mp-clock-mode indicates the clock mode that the client is running, so for backwards
    //       compatibility ensure this is initialized to 0 which means pre 2018.1
    props->setIntValue("sim/multiplay/mp-clock-mode", 0);

    tie("refuel/contact", SGRawValuePointer<bool>(&contact));
    tie("tanker", SGRawValuePointer<bool>(&isTanker));

    tie("controls/invisible",
        SGRawValuePointer<bool>(&invisible));
    _uBodyNode = props->getNode("velocities/uBody-fps", true);
    _vBodyNode = props->getNode("velocities/vBody-fps", true);
    _wBodyNode = props->getNode("velocities/wBody-fps", true);
    
    m_node_ai_latch = props->getNode("ai-latch", true /*create*/);
    m_node_log_multiplayer = globals->get_props()->getNode("/sim/log-multiplayer-callsign", true /*create*/);
    
#define AIMPROProp(type, name) \
SGRawValueMethods<FGAIMultiplayer, type>(*this, &FGAIMultiplayer::get##name)

#define AIMPRWProp(type, name) \
SGRawValueMethods<FGAIMultiplayer, type>(*this, \
      &FGAIMultiplayer::get##name, &FGAIMultiplayer::set##name)

    //tie("callsign", AIMPROProp(const char *, CallSign));

    tie("controls/allow-extrapolation",
        AIMPRWProp(bool, AllowExtrapolation));
    tie("controls/lag-adjust-system-speed",
        AIMPRWProp(double, LagAdjustSystemSpeed));
    tie("controls/player-lag",
        AIMPRWProp(double, playerLag));
    tie("controls/compensate-lag",
        AIMPRWProp(int, compensateLag));

#undef AIMPROProp
#undef AIMPRWProp
}


void FGAIMultiplayer::FGAIMultiplayerInterpolate(
        MotionInfo::iterator prevIt,
        MotionInfo::iterator nextIt,
        double tau,
        SGVec3d& ecPos,
        SGQuatf& ecOrient,
        SGVec3f& ecLinearVel
        )
{
    // Here we do just linear interpolation on the position
    ecPos = interpolate(tau, prevIt->second.position, nextIt->second.position);
    ecOrient = interpolate((float)tau, prevIt->second.orientation,
        nextIt->second.orientation);
    ecLinearVel = interpolate((float)tau, prevIt->second.linearVel, nextIt->second.linearVel);
    speed = norm(ecLinearVel) * SG_METER_TO_NM * 3600.0;

    if (prevIt->second.properties.size()
        == nextIt->second.properties.size())
    {
        std::vector<FGPropertyData*>::const_iterator prevPropIt;
        std::vector<FGPropertyData*>::const_iterator prevPropItEnd;
        std::vector<FGPropertyData*>::const_iterator nextPropIt;
        std::vector<FGPropertyData*>::const_iterator nextPropItEnd;
        prevPropIt = prevIt->second.properties.begin();
        prevPropItEnd = prevIt->second.properties.end();
        nextPropIt = nextIt->second.properties.begin();
        nextPropItEnd = nextIt->second.properties.end();
        while (prevPropIt != prevPropItEnd)
        {
            PropertyMap::iterator pIt = mPropertyMap.find((*prevPropIt)->id);
            //cout << " Setting property..." << (*prevPropIt)->id;

            if (pIt != mPropertyMap.end())
            {
                //cout << "Found " << pIt->second->getPath() << ":";

                float val;
                /*
                 * RJH - 2017-01-25
                 * During multiplayer operations a series of crashes were encountered that affected all players
                 * within range of each other and resulting in an exception being thrown at exactly the same moment in time
                 * (within case props::STRING: ref http://i.imgur.com/y6MBoXq.png)
                 * Investigation showed that the nextPropIt and prevPropIt were pointing to different properties
                 * which may be caused due to certain models that have overloaded mp property transmission and
                 * these craft have their properties truncated due to packet size. However the result of this
                 * will be different contents in the previous and current packets, so here we protect against
                 * this by only considering properties where the previous and next id are the same.
                 * It might be a better solution to search the previous and next lists to locate the matching id's
                 */
                if (*nextPropIt && (*nextPropIt)->id == (*prevPropIt)->id)
                {
                    switch ((*prevPropIt)->type)
                    {
                        case simgear::props::INT:
                        case simgear::props::BOOL:
                        case simgear::props::LONG:
                            // Jean Pellotier, 2018-01-02 : we don't want interpolation for integer values, they are mostly used
                            // for non linearly changing values (e.g. transponder etc ...)
                            // fixes: https://sourceforge.net/p/flightgear/codetickets/1885/
                            pIt->second->setIntValue((*nextPropIt)->int_value);
                            break;
                        case simgear::props::FLOAT:
                        case simgear::props::DOUBLE:
                            val = (1 - tau)*(*prevPropIt)->float_value +
                                tau*(*nextPropIt)->float_value;
                            //cout << "Flo: " << val << "\n";
                            pIt->second->setFloatValue(val);
                            break;
                        case simgear::props::STRING:
                        case simgear::props::UNSPECIFIED:
                            //cout << "Str: " << (*nextPropIt)->string_value << "\n";
                            pIt->second->setStringValue((*nextPropIt)->string_value);
                            break;
                        default:
                            // FIXME - currently defaults to float values
                            val = (1 - tau)*(*prevPropIt)->float_value +
                                tau*(*nextPropIt)->float_value;
                            //cout << "Unk: " << val << "\n";
                            pIt->second->setFloatValue(val);
                            break;
                    }
                }
                else
                {
                    SG_LOG(SG_AI, SG_WARN, "MP packet mismatch during lag interpolation: " << (*prevPropIt)->id << " != " << (*nextPropIt)->id << "\n");
                }
            }
            else
            {
                SG_LOG(SG_AI, SG_DEBUG, "Unable to find property: " << (*prevPropIt)->id << "\n");
            }

            ++prevPropIt;
            ++nextPropIt;
        }
    }
}

void FGAIMultiplayer::FGAIMultiplayerExtrapolate(
        MotionInfo::iterator nextIt,
        double tInterp,
        bool motion_logging,
        SGVec3d& ecPos,
        SGQuatf& ecOrient,
        SGVec3f& ecLinearVel
        )
{
    const FGExternalMotionData& motionInfo = nextIt->second;

    // The time to predict, limit to 3 seconds. But don't do this if we are
    // running motion tests, because it can mess up the results.
    //
    double t = tInterp - nextIt->first;
    if (!motion_logging)
    {
        props->setDoubleValue("lag/extrapolation-t", t);
        if (t > 3)
        {
            t = 3;
            props->setBoolValue("lag/extrapolation-out-of-range", true);
        }
        else
        {
            props->setBoolValue("lag/extrapolation-out-of-range", false);
        }
    }

    // using velocity and acceleration to guess a parabolic position...
    ecPos = motionInfo.position;
    ecOrient = motionInfo.orientation;
    ecLinearVel = motionInfo.linearVel;
    SGVec3d ecVel = toVec3d(ecOrient.backTransform(ecLinearVel));
    SGVec3f angularVel = motionInfo.angularVel;
    SGVec3d ecAcc = toVec3d(ecOrient.backTransform(motionInfo.linearAccel));

    double normVel = norm(ecVel);
    double normAngularVel = norm(angularVel);
    props->setDoubleValue("lag/norm-vel", normVel);
    props->setDoubleValue("lag/norm-angular-vel", normAngularVel);
    
    // not doing rotationnal prediction for small speed or rotation rate,
    // to avoid agitated parked plane
    
    if (( normAngularVel > 0.05 ) || ( normVel > 1.0 ))
    {
        ecOrient += t*ecOrient.derivative(angularVel);
    }

    // not using acceleration for small speed, to have better parked planes
    // note that anyway acceleration is not transmit yet by mp
    if ( normVel > 1.0 )
    {
        ecPos += t*(ecVel + 0.5*t*ecAcc);
    }
    else
    {
        ecPos += t*(ecVel);
    }

    std::vector<FGPropertyData*>::const_iterator firstPropIt;
    std::vector<FGPropertyData*>::const_iterator firstPropItEnd;
    speed = norm(ecLinearVel) * SG_METER_TO_NM * 3600.0;
    firstPropIt = motionInfo.properties.begin();
    firstPropItEnd = motionInfo.properties.end();
    while (firstPropIt != firstPropItEnd)
    {
        PropertyMap::iterator pIt = mPropertyMap.find((*firstPropIt)->id);
        //cout << " Setting property..." << (*firstPropIt)->id;

        if (pIt != mPropertyMap.end())
        {
            switch ((*firstPropIt)->type)
            {
              case simgear::props::INT:
              case simgear::props::BOOL:
              case simgear::props::LONG:
                  pIt->second->setIntValue((*firstPropIt)->int_value);
                  //cout << "Int: " << (*firstPropIt)->int_value << "\n";
                  break;
              case simgear::props::FLOAT:
              case simgear::props::DOUBLE:
                  pIt->second->setFloatValue((*firstPropIt)->float_value);
                  //cout << "Flo: " << (*firstPropIt)->float_value << "\n";
                  break;
              case simgear::props::STRING:
              case simgear::props::UNSPECIFIED:
                  pIt->second->setStringValue((*firstPropIt)->string_value);
                  //cout << "Str: " << (*firstPropIt)->string_value << "\n";
                  break;
              default:
                  // FIXME - currently defaults to float values
                  pIt->second->setFloatValue((*firstPropIt)->float_value);
                  //cout << "Unk: " << (*firstPropIt)->float_value << "\n";
                  break;
            }
        }
        else
        {
            SG_LOG(SG_AI, SG_DEBUG, "Unable to find property: " << (*firstPropIt)->id << "\n");
        }

        ++firstPropIt;
    }
}


// Populate
// /sim/replay/log-raw-speed-multiplayer-post-*-values/value[] with information
// on motion of MP and user aircraft.
//
// For use with scripts/python/recordreplay.py --test-motion-mp.
//
static void s_MotionLogging(const std::string& _callsign, double tInterp, SGVec3d ecPos, const SGGeod& pos)
{
    SGGeod pos_geod = pos;
    if (!fgGetBool("/sim/replay/replay-state-eof"))
    {
        static SGVec3d s_pos_0;
        static SGVec3d s_pos_prev;
        static double s_t_prev = -1;
        SGVec3d pos = ecPos;
        double sim_replay_time = fgGetDouble("/sim/replay/time");
        double t = fgGetBool("/sim/replay/replay-state") ? sim_replay_time : tInterp;
        if (s_t_prev == -1)
        {
            s_pos_0 = pos;
        }
        double t_dt = t - s_t_prev;
        if (s_t_prev != -1 /*&& t_dt != 0*/)
        {
            SGVec3d delta_pos = pos - s_pos_prev;
            SGVec3d delta_pos_norm = normalize(delta_pos);
            double distance0 = length(pos - s_pos_0);
            double distance = length(delta_pos);
            double speed = (t_dt) ? distance / t_dt : -999;
            if (t_dt)
            {
                SGPropertyNode* n0 = fgGetNode("/sim/replay/log-raw-speed-multiplayer-post-values", true /*create*/);
                SGPropertyNode* n;

                n = n0->addChild("value");
                n->setDoubleValue(speed);

                n = n0->addChild("description");
                char buffer[128];
                snprintf(buffer, sizeof(buffer), "s_t_prev=%f t=%f t_dt=%f distance=%f",
                        s_t_prev, t, t_dt, distance);
                n->setStringValue(buffer);
            }

            SGGeod  user_pos_geod = SGGeod::fromDegFt(
                    fgGetDouble("/position/longitude-deg"),
                    fgGetDouble("/position/latitude-deg"),
                    fgGetDouble("/position/altitude-ft")
                    );
            SGVec3d user_pos = SGVec3d::fromGeod(user_pos_geod);

            double user_to_mp_distance = SGGeodesy::distanceM(user_pos_geod, pos_geod);
            double user_to_mp_bearing = SGGeodesy::courseDeg(user_pos_geod, pos_geod);
            double user_distance0 = length(user_pos - s_pos_0);

            if (1)
            {
                fgGetNode("/sim/replay/log-raw-speed-multiplayer-post-relative-distance", true /*create*/)
                        ->addChild("value")
                        ->setDoubleValue(user_to_mp_distance)
                        ;
                fgGetNode("/sim/replay/log-raw-speed-multiplayer-post-relative-bearing", true /*create*/)
                        ->addChild("value")
                        ->setDoubleValue(user_to_mp_bearing)
                        ;
                fgGetNode("/sim/replay/log-raw-speed-multiplayer-post-absolute-distance", true /*create*/)
                        ->addChild("value")
                        ->setDoubleValue(distance0)
                        ;
                fgGetNode("/sim/replay/log-raw-speed-multiplayer-post-user-absolute-distance", true /*create*/)
                        ->addChild("value")
                        ->setDoubleValue(user_distance0)
                        ;
            }
        }
        if (t_dt)
        {
            s_t_prev = t;
            s_pos_prev = pos;
        }
    }
}

void FGAIMultiplayer::update(double dt)
{
    using namespace simgear;

    if (dt <= 0)
    {
        return;
    }
    FGAIBase::update(dt);

    // Check if we already got data
    if (mMotionInfo.empty())
    {
        return;
    }

    // motion_logging is true if we are being run by
    // scripts/python/recordreplay.py --test-motion-mp.
    //
    bool motion_logging = false;
    {
        const char* callsign = mLogRawSpeedMultiplayer->getStringValue();
        if (callsign && callsign[0] && this->_callsign == callsign)
        {
            motion_logging = true;
        }
    }
    
    double curtime;
    double tInterp;
    if (m_simple_time_enabled->getBoolValue())
    {
        // Simplified timekeeping.
        //
        if (m_sim_replay_replay_state->getBoolValue())
        {
            tInterp = m_sim_replay_time->getDoubleValue();
            SG_LOG(SG_GENERAL, SG_BULK, "tInterp=" << std::fixed << std::setprecision(6) << tInterp);
        }
        else
        {
            tInterp = globals->get_subsystem<TimeManager>()->getMPProtocolClockSec();
        }
        curtime = tInterp;
    }
    else
    {
        // Get the last available time
        MotionInfo::reverse_iterator motioninfo_back = mMotionInfo.rbegin();
        const double curentPkgTime = motioninfo_back->first;

        // The current simulation time we need to update for,
        // note that the simulation time is updated before calling all the
        // update methods. Thus motioninfo_back contains the time intervals *end* time
        // 2018: notice this time is specifically used for mp protocol
        curtime = globals->get_subsystem<TimeManager>()->getMPProtocolClockSec();

        // Dynamically optimize the time offset between the feeder and the client
        // Well, 'dynamically' means that the dynamic of that update must be very
        // slow. You would otherwise notice huge jumps in the multiplayer models.
        // The reason is that we want to avoid huge extrapolation times since
        // extrapolation is highly error prone. For that we need something
        // approaching the average latency of the packets. This first order lag
        // component will provide this. We just take the error of the currently
        // requested time to the most recent available packet. This is the
        // target we want to reach in average.
        double lag = motioninfo_back->second.lag;

        rawLag = curentPkgTime - curtime;
        realTime = false; //default behaviour

        if (!mTimeOffsetSet)
        {
            mTimeOffsetSet = true;
            mTimeOffset = curentPkgTime - curtime - lag;
            lastTime = curentPkgTime;
            lagModAveraged = remainder((curentPkgTime - curtime), 3600.0);
            props->setDoubleValue("lag/pps-averaged", lagPpsAveraged);
            props->setDoubleValue("lag/lag-mod-averaged", lagModAveraged);
        }
        else
        {
            if ((curentPkgTime - lastTime) != 0)
            {
                lagPpsAveraged = 0.99 * lagPpsAveraged + 0.01 * fabs( 1 / (lastTime - curentPkgTime));
                lastTime = curentPkgTime;
                rawLagMod = remainder(rawLag, 3600.0);
                lagModAveraged = lagModAveraged *0.99 + 0.01 * rawLagMod;
                props->setDoubleValue("lag/pps-averaged", lagPpsAveraged);
                props->setDoubleValue("lag/lag-mod-averaged", lagModAveraged);
            }

            double offset = 0.0;

            //spectator mode, more late to be in the interpolation zone
            if (compensateLag == 3)
            {
                offset = curentPkgTime - curtime - lag + playerLag;
                // old behaviour
            }
            else if (compensateLag == 1)
            {
                offset = curentPkgTime - curtime - lag;

                // using the prediction mode to display the mpaircraft in the futur/past with given playerlag value
                //currently compensatelag = 2
            }
            else if (fabs(lagModAveraged) < 0.3)
            {
                mTimeOffset = (round(rawLag/3600))*3600; //real time mode if close enough
                realTime = true;
            }
            else
            {
                offset = curentPkgTime - curtime + 0.48*lag + playerLag;
            }

            if (!realTime)
            {
                if ((!mAllowExtrapolation && offset + lag < mTimeOffset) || (offset - 10 > mTimeOffset))
                {
                    mTimeOffset = offset;
                    SG_LOG(SG_AI, SG_DEBUG, "Resetting time offset adjust system to "
                           "avoid extrapolation: time offset = " << mTimeOffset);
                }
                else
                {
                    // the error of the offset, respectively the negative error to avoid
                    // a minus later ...
                    double err = offset - mTimeOffset;
                    // limit errors leading to shorter lag values somehow, that is late
                    // arriving packets will pessimize the overall lag much more than
                    // early packets will shorten the overall lag
                    double sysSpeed;
                    //trying to slow the rudderlag phenomenon thus using more the prediction system
                    //if we are off by less than 1.5s, do a little correction, and bigger step above 1.5s
                    if (fabs(err) < 1.5)
                    {
                        if (err < 0)
                        {
                            sysSpeed = mLagAdjustSystemSpeed*err*0.01;
                        }
                        else
                        {
                            sysSpeed = SGMiscd::min(0.5*err*err, 0.05);
                        }
                    }
                    else
                    {
                        if (err < 0)
                        {
                            // Ok, we have some very late packets and nothing newer increase the
                            // lag by the given speedadjust
                            sysSpeed = mLagAdjustSystemSpeed*err;
                        }
                        else
                        {
                            // We have a too pessimistic display delay shorten that a small bit
                            sysSpeed = SGMiscd::min(0.1*err*err, 0.5);
                        }
                    }

                    // simple euler integration for that first order system including some
                    // overshooting guard to prevent to aggressive system speeds
                    // (stiff systems) to explode the systems state
                    double systemIncrement = dt*sysSpeed;
                    if (fabs(err) < fabs(systemIncrement))
                    {
                        systemIncrement = err;
                    }
                    mTimeOffset += systemIncrement;

                    SG_LOG(SG_AI, SG_DEBUG, "Offset adjust system: time offset = "
                         << mTimeOffset << ", expected longitudinal position error due to "
                         " current adjustment of the offset: "
                         << fabs(norm(motioninfo_back->second.linearVel)*systemIncrement));
                }
            }
        }

        // Compute the time in the feeders time scale which fits the current time
        // we need to
        tInterp = curtime + mTimeOffset;
    }

    SGVec3d ecPos;
    SGQuatf ecOrient;
    SGVec3f ecLinearVel;

    MotionInfo::iterator nextIt = mMotionInfo.upper_bound(tInterp);
    MotionInfo::iterator prevIt = nextIt;
    
    if (nextIt != mMotionInfo.end() && nextIt->first >= tInterp)
    {
        // Ok, we need a time prevous to the last available packet,
        // that is good ...
        // the case tInterp = curentPkgTime need to be in the interpolation, to avoid a bug zeroing the position

        double tau = 0;
        if (nextIt == mMotionInfo.end())    // Not sure this can happen.
        {
            /*
            * RJH: 2017-02-16 another exception thrown here when running under debug (and hence huge frame delays)
            * the value of nextIt was already end(); which I think means that we cannot run the entire next section of code.
            */
            // Leave prevIt and nextIt pointing at same item.
            --nextIt;
            --prevIt;
        }
        else if (nextIt == mMotionInfo.begin())
        {
            // Leave prevIt and nextIt pointing at same item.
            SG_LOG(SG_GENERAL, SG_DEBUG, "Only one frame for interpolation: " << _callsign);
        }
        else
        {
            --prevIt;
            // Interpolation coefficient is between 0 and 1
            double intervalStart = prevIt->first;
            double intervalEnd = nextIt->first;

            double intervalLen = intervalEnd - intervalStart;
            if (intervalLen != 0.0)
            {
                tau = (tInterp - intervalStart) / intervalLen;
            }
        }
        
        FGAIMultiplayerInterpolate(prevIt, nextIt, tau, ecPos, ecOrient, ecLinearVel);
    }
    else
    {
        // Ok, we need to predict the future, so, take the best data we can have
        // and do some eom computation to guess that for now.
        --nextIt;
        --prevIt;   // so mMotionInfo.erase() does the right thing below.
        FGAIMultiplayerExtrapolate(nextIt, tInterp, motion_logging, ecPos, ecOrient, ecLinearVel);
    }

    // Remove any motion information before <prevIt> - we will not need this in
    // the future.
    //
    mMotionInfo.erase(mMotionInfo.begin(), prevIt);
    
    // extract the position
    pos = SGGeod::fromCart(ecPos);
    double recent_alt_ft = altitude_ft;
    altitude_ft = pos.getElevationFt();

    // expose a valid vertical speed
    if (lastUpdateTime != 0)
    {
        double dT = curtime - lastUpdateTime;
        double Weighting=1;
        if (dt < 1.0)
        {
            Weighting = dt;
        }
        // simple smoothing over 1 second
        vs_fps = (1.0-Weighting)*vs_fps +  Weighting * (altitude_ft - recent_alt_ft) / dT * 60;
    }
    lastUpdateTime = curtime;

    // The quaternion rotating from the earth centered frame to the
    // horizontal local frame
    SGQuatf qEc2Hl = SGQuatf::fromLonLatRad((float)pos.getLongitudeRad(), (float)pos.getLatitudeRad());
    // The orientation wrt the horizontal local frame
    SGQuatf hlOr = conj(qEc2Hl)*ecOrient;
    float hDeg, pDeg, rDeg;
    hlOr.getEulerDeg(hDeg, pDeg, rDeg);
    hdg = hDeg;
    roll = rDeg;
    pitch = pDeg;

    // expose velocities/u,v,wbody-fps in the mp tree
    _uBodyNode->setValue(ecLinearVel[0] * SG_METER_TO_FEET);
    _vBodyNode->setValue(ecLinearVel[1] * SG_METER_TO_FEET);
    _wBodyNode->setValue(ecLinearVel[2] * SG_METER_TO_FEET);
    
    if (ecLinearVel[0] == 0) {
        // MP packets for carriers have zero ecLinearVel, but do specify
        // velocities/speed-kts.
        double speed_kts = props->getDoubleValue("velocities/speed-kts");
        double speed_fps = speed_kts * SG_KT_TO_FPS;
        _uBodyNode->setDoubleValue(speed_fps);
    }
    
    std::string ai_latch = m_node_ai_latch->getStringValue();
    if (ai_latch != m_ai_latch) {
        SG_LOG(SG_AI, SG_ALERT, "latching _callsign=" << _callsign << " to mp " << ai_latch);
        m_ai_latch = ai_latch;
        SGPropertyNode* mp_node = globals->get_props()->getNode(ai_latch, false /*create*/);
        m_node_ai_latch_latitude = mp_node ? mp_node->getNode("position/latitude-deg", true /*create*/) : nullptr;
        m_node_ai_latch_longitude = mp_node ? mp_node->getNode("position/longitude-deg", true /*create*/) : nullptr;
        m_node_ai_latch_altitude = mp_node ? mp_node->getNode("position/altitude-ft", true /*create*/) : nullptr;
        m_node_ai_latch_heading = mp_node ? mp_node->getNode("orientation/true-heading-deg", true /*create*/) : nullptr;
        m_node_ai_latch_pitch = mp_node ? mp_node->getNode("orientation/pitch-deg", true /*create*/) : nullptr;
        m_node_ai_latch_roll = mp_node ? mp_node->getNode("orientation/roll-deg", true /*create*/) : nullptr;
        m_node_ai_latch_ubody_fps = mp_node ? mp_node->getNode("velocities/uBody-fps", true /*create*/) : nullptr;
        m_node_ai_latch_vbody_fps = mp_node ? mp_node->getNode("velocities/vBody-fps", true /*create*/) : nullptr;
        m_node_ai_latch_wbody_fps = mp_node ? mp_node->getNode("velocities/wBody-fps", true /*create*/) : nullptr;
        m_node_ai_latch_speed_kts = mp_node ? mp_node->getNode("velocities/speed-kts", true /*create*/) : nullptr;
    }
    if (ai_latch != "") {
        SGPropertyNode* mp_node = globals->get_props()->getNode(ai_latch, true /*create*/);
        assert(m_node_ai_latch_latitude->getPath() == mp_node->getNode("position/latitude-deg")->getPath());
        m_node_ai_latch_latitude->setDoubleValue(pos.getLatitudeDeg());
        m_node_ai_latch_longitude->setDoubleValue(pos.getLongitudeDeg());
        m_node_ai_latch_altitude->setDoubleValue(pos.getElevationFt());
        m_node_ai_latch_heading->setDoubleValue(hDeg);
        m_node_ai_latch_pitch->setDoubleValue(pitch);
        m_node_ai_latch_roll->setDoubleValue(roll);
        m_node_ai_latch_ubody_fps->setDoubleValue(_uBodyNode->getDoubleValue());
        m_node_ai_latch_vbody_fps->setDoubleValue(_vBodyNode->getDoubleValue());
        m_node_ai_latch_wbody_fps->setDoubleValue(_wBodyNode->getDoubleValue());
        
        // /ai/models/carrier[]/velocities/speed-kts seems to be used
        // to calculate friction between carrier deck and our aircraft
        // undercarriage when brakes are on, so we set it here from
        // /ai/models/multiplayer[]/velocities/uBody-fps.
        //
        // [We could possibly use
        // /ai/models/multiplayer[]/velocities/true-airspeed-kt instead, but
        // seems nicer to use uBody.]
        //
        // [todo: use rate of change of heading to calculating
        // movement/rotation of the ship under the aircraft, so aircraft
        // doesn't slip when carrier changes course. Also would be good to
        // handle vbody and wbody?]
        //
        m_node_ai_latch_speed_kts->setDoubleValue(_uBodyNode->getDoubleValue() * SG_FPS_TO_KT);
    }
    assert(m_node_ai_latch == props->getNode("ai-latch"));

    //SG_LOG(SG_AI, SG_DEBUG, "Multiplayer position and orientation: " << ecPos << ", " << hlOr);

    if (motion_logging)
    {
        s_MotionLogging(_callsign, tInterp, ecPos, pos);
    }
    
    //###########################//
    // do calculations for radar //
    //###########################//
    double range_ft2 = UpdateRadar(manager);

    //************************************//
    // Tanker code                        //
    //************************************//

    if ( isTanker)
    {
        //cout << "IS tanker ";
        if (range_ft2 < 250.0 * 250.0 && y_shift > 0.0 && elevation > 0.0)
        {
            // refuel_node->setBoolValue(true);
            //cout << "in contact"  << endl;
            contact = true;
        }
        else
        {
            // refuel_node->setBoolValue(false);
            //cout << "not in contact"  << endl;
            contact = false;
        }
    }
    else
    {
        //cout << "NOT tanker " << endl;
        contact = false;
    }

    Transform();
}

void
FGAIMultiplayer::addMotionInfo(FGExternalMotionData& motionInfo,
                               long stamp)
{
    mLastTimestamp = stamp;

    if (m_simple_time_enabled->getBoolValue()) {
        // Update simple-time stats and set m_simple_time_compensation.
        //
        double t = m_sim_replay_replay_state->getBoolValue()
                ? m_sim_replay_time->getDoubleValue()
                : globals->get_subsystem<TimeManager>()->getMPProtocolClockSec()
                ;
    
        m_simple_time_offset = motionInfo.time - t;
        if (m_simple_time_first_time)
        {
            m_simple_time_first_time = false;
            m_simple_time_offset_smoothed = m_simple_time_offset;
        }
        double e = 0.01;
        m_simple_time_offset_smoothed = (1-e) * m_simple_time_offset_smoothed
                + e * m_simple_time_offset;

        if (m_simple_time_offset_smoothed < -2.0 || m_simple_time_offset_smoothed > 1)
        {
            // If the sender is using simple-time mode and their clock is
            // synchronised to ours (e.g. both using NTP), <time_offset> will
            // be a negative value due to the network delay plus any difference
            // in the remote and local UTC time.
            //
            // Thus we could expect m_time_offset_max around -0.2.
            //
            // If m_time_offset_max is very different from zero, this indicates
            // that the sender is not putting UTC time into MP packets. E.g.
            // they are using simple-time mode but their system clock is not
            // set correctly; or they are not using simple-time mode so are
            // sending their FDM time or a time that has been synchronised with
            // other MP aircraft. Another possibility is that our UTC clock is
            // incorrect.
            //
            // We need a time that can be consistently compared with our UTC
            // tInterp. So we set m_time_compensation to something to be
            // added to all times received in MP packets from _callsign. We
            // use compensated time for keys in the the mMotionInfo[]
            // map, thus code should generally use these key values, not
            // mMotionInfo[].time.
            //
            m_simple_time_compensation = -m_simple_time_offset_smoothed;
            
            // m_simple_time_offset_smoothed will usually be too big to be
            // useful here.
            //
            props->setDoubleValue("lag/lag-mod-averaged", 0);
        }
        else
        {
            m_simple_time_compensation = 0;
            props->setDoubleValue("lag/lag-mod-averaged", m_simple_time_offset_smoothed);
        }
        
        m_node_simple_time_latest->setDoubleValue(motionInfo.time);
        m_node_simple_time_offset->setDoubleValue(m_simple_time_offset);
        m_node_simple_time_offset_smoothed->setDoubleValue(m_simple_time_offset_smoothed);
        m_node_simple_time_compensation->setDoubleValue(m_simple_time_compensation);

        double t_key = motionInfo.time + m_simple_time_compensation;
        if (m_simple_time_recent_packet_time)
        {
            double dt = t_key - m_simple_time_recent_packet_time;
            if (dt != 0)
            {
                double e = 0.05;
                lagPpsAveraged = (1-e) * lagPpsAveraged + e * (1/dt);
                props->setDoubleValue("lag/pps-averaged", lagPpsAveraged);
            }
        }
        m_simple_time_recent_packet_time = t_key;
        
        // We use compensated time <t_key> as key in mMotionInfo.
        // m_time_compensation is set to non-zero if packets seem to have
        // wildly different times from us, if simple-time mode is enabled.
        //
        // So most code with an <iterator> into mMotionInfo that needs to
        // use the MP packet's time, will actuall use iterator->first, not
        // iterator->second.time..
        //
        mMotionInfo[t_key] = motionInfo;
    }
    else
    {
        mMotionInfo[motionInfo.time] = motionInfo;
    }

    // We just copied the property (pointer) list - they are ours now. Clear the
    // properties list in given/returned object, so former owner won't deallocate them.
    motionInfo.properties.clear();
  
    {
        // Gather data on multiplayer speed, used by scripts/python/recordreplay.py.
        //
        if (m_node_log_multiplayer->getStringValue() == this->_callsign)
        {
            static SGVec3d pos_prev;
            static double t_prev = 0;
            
            double distance = length(motionInfo.position - pos_prev);
            double dt = motionInfo.time - t_prev;
            double speed = distance / dt;
            
            double linear_vel = norm(motionInfo.linearVel);
            
            SGPropertyNode* item = fgGetNode("/sim/log-multiplayer", true /*create*/)->addChild("mppacket");
            item->setDoubleValue("distance", distance);
            item->setDoubleValue("speed", speed);
            item->setDoubleValue("dt", dt);
            item->setDoubleValue("linear_vel", linear_vel);
            item->setDoubleValue("t", motionInfo.time );
            
            pos_prev = motionInfo.position;
            t_prev = motionInfo.time;
        }
    }
    
}

void
FGAIMultiplayer::setDoubleProperty(const std::string& prop, double val)
{
  SGPropertyNode* pNode = props->getChild(prop.c_str(), true);
  pNode->setDoubleValue(val);
}

void FGAIMultiplayer::clearMotionInfo()
{
    mMotionInfo.clear();
    mLastTimestamp = 0;
}
