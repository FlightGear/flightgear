// FGAIMultiplayer - AIBase derived class creates an AI multiplayer aircraft
//
// Written by David Culp, started October 2003.
//
// Copyright (C) 2003  David P. Culp - davidculp2@comcast.net
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

#ifndef _FG_AIMultiplayer_HXX
#define _FG_AIMultiplayer_HXX

#include <map>
#include <string>

#include <MultiPlayer/mpmessages.hxx>
#include "AIBase.hxx"

class FGAIMultiplayer : public FGAIBase {
public:
  FGAIMultiplayer();
  virtual ~FGAIMultiplayer();

  bool init(ModelSearchOrder searchOrder) override;
  void bind() override;
  void update(double dt) override;

  void addMotionInfo(FGExternalMotionData& motionInfo, long stamp);
  void setDoubleProperty(const std::string& prop, double val);

  long getLastTimestamp(void) const
  {
    return mLastTimestamp;
  }

  void setAllowExtrapolation(bool allowExtrapolation)
  {
    mAllowExtrapolation = allowExtrapolation;
  
  }
  bool getAllowExtrapolation(void) const
  {
    return mAllowExtrapolation;
  }
  
  void setLagAdjustSystemSpeed(double lagAdjustSystemSpeed)
  {
    if (lagAdjustSystemSpeed < 0)
      lagAdjustSystemSpeed = 0;
    mLagAdjustSystemSpeed = lagAdjustSystemSpeed;
  }
  
  double getLagAdjustSystemSpeed(void) const
  {
    return mLagAdjustSystemSpeed;
  }

  void addPropertyId(unsigned id, const char* name)
  {
    mPropertyMap[id] = props->getNode(name, true);
  }

  double getplayerLag(void) const
  {
    return playerLag;
  }

  void setplayerLag(double mplayerLag)
  {
    playerLag = mplayerLag;
  }

  int getcompensateLag(void) const
  {
    return compensateLag;
  }

  void setcompensateLag(int mcompensateLag)
  {
    compensateLag = mcompensateLag;
  }

  SGPropertyNode* getPropertyRoot()
  {
    return props;
  }
  
  void clearMotionInfo();

  const char* getTypeString(void) const override
  {
    return "multiplayer";
  }

private:

  // Automatic sorting of motion data according to its timestamp
  typedef std::map<double,FGExternalMotionData> MotionInfo;
  MotionInfo mMotionInfo;

  // Map between the property id's from the multiplayers network packets
  // and the property nodes
  typedef std::map<unsigned, SGSharedPtr<SGPropertyNode> > PropertyMap;
  PropertyMap mPropertyMap;
  
  // Calculates position, orientation and velocity using interpolation between
  // *prevIt and *nextIt, specifically (1-tau)*(*prevIt) + tau*(*nextIt).
  //
  // Cannot call this method 'interpolate' because that would hide the name in
  // OSG.
  //
  void FGAIMultiplayerInterpolate(
        MotionInfo::iterator prevIt,
        MotionInfo::iterator nextIt,
        double tau,
        SGVec3d& ecPos,
        SGQuatf& ecOrient,
        SGVec3f& ecLinearVel
        );

  bool mTimeOffsetSet;
  bool realTime;
  int compensateLag;
  double playerLag;
  double mTimeOffset;
  double lastUpdateTime;
  double lastTime;
  double lagPpsAveraged;
  double rawLag, rawLagMod, lagModAveraged;

  /// Properties which are for now exposed for testing
  bool mAllowExtrapolation;
  double mLagAdjustSystemSpeed;

  long mLastTimestamp;

  // Properties for tankers
  SGPropertyNode_ptr refuel_node;
  bool isTanker;
  bool contact;          // set if this tanker is within fuelling range
  
  // velocities/u,v,wbody-fps 
  SGPropertyNode_ptr _uBodyNode;
  SGPropertyNode_ptr _vBodyNode;
  SGPropertyNode_ptr _wBodyNode;
  
  // Things for simple-time.
  //
  SGPropertyNode_ptr m_simple_time_enabled;
  
  SGPropertyNode_ptr m_sim_replay_replay_state;
  SGPropertyNode_ptr m_sim_replay_time;
  
  bool      m_simple_time_first_time;
  double    m_simple_time_offset;
  double    m_simple_time_offset_smoothed;
  double    m_simple_time_compensation;
  double    m_simple_time_recent_packet_time;
  
  SGPropertyNode_ptr    m_node_simple_time_latest;
  SGPropertyNode_ptr    m_node_simple_time_offset;
  SGPropertyNode_ptr    m_node_simple_time_offset_smoothed;
  SGPropertyNode_ptr    m_node_simple_time_compensation;
  
  // For use with scripts/python/recordreplay.py --test-motion-mp.
  SGPropertyNode_ptr mLogRawSpeedMultiplayer;
};

#endif  // _FG_AIMultiplayer_HXX
