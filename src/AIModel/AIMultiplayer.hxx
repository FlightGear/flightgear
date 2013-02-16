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

  virtual bool init(bool search_in_AI_path=false);
  virtual void bind();
  virtual void update(double dt);

  void addMotionInfo(FGExternalMotionData& motionInfo, long stamp);
  void setDoubleProperty(const std::string& prop, double val);

  long getLastTimestamp(void) const
  { return mLastTimestamp; }

  void setAllowExtrapolation(bool allowExtrapolation)
  { mAllowExtrapolation = allowExtrapolation; }
  bool getAllowExtrapolation(void) const
  { return mAllowExtrapolation; }
  void setLagAdjustSystemSpeed(double lagAdjustSystemSpeed)
  {
    if (lagAdjustSystemSpeed < 0)
      lagAdjustSystemSpeed = 0;
    mLagAdjustSystemSpeed = lagAdjustSystemSpeed;
  }
  double getLagAdjustSystemSpeed(void) const
  { return mLagAdjustSystemSpeed; }

  void addPropertyId(unsigned id, const char* name)
  { mPropertyMap[id] = props->getNode(name, true); }

  SGPropertyNode* getPropertyRoot()
  { return props; }

  virtual const char* getTypeString(void) const { return "multiplayer"; }

private:

  // Automatic sorting of motion data according to its timestamp
  typedef std::map<double,FGExternalMotionData> MotionInfo;
  MotionInfo mMotionInfo;

  // Map between the property id's from the multiplayers network packets
  // and the property nodes
  typedef std::map<unsigned, SGSharedPtr<SGPropertyNode> > PropertyMap;
  PropertyMap mPropertyMap;

  double mTimeOffset;
  bool mTimeOffsetSet;

  double lastUpdateTime;

  /// Properties which are for now exposed for testing
  bool mAllowExtrapolation;
  double mLagAdjustSystemSpeed;

  long mLastTimestamp;

  // Properties for tankers
  SGPropertyNode_ptr refuel_node;
  bool isTanker;
  bool contact;          // set if this tanker is within fuelling range
};

#endif  // _FG_AIMultiplayer_HXX
