//////////////////////////////////////////////////////////////////////
//
// multiplaymgr.hxx
//
// Written by Duncan McCreanor, started February 2003.
// duncan.mccreanor@airservicesaustralia.com
//
// Copyright (C) 2003  Airservices Australia
// Copyright (C) 2005  Oliver Schroeder
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
//
// $Id$
//  
//////////////////////////////////////////////////////////////////////

#ifndef MULTIPLAYMGR_H
#define MULTIPLAYMGR_H

#define MULTIPLAYTXMGR_HID "$Id$"

#include "mpmessages.hxx"

#include <string>
#include <vector>

#include <simgear/compiler.h>
#include <simgear/props/props.hxx>
#include <Main/globals.hxx>
#include <simgear/io/raw_socket.hxx>
#include <simgear/structure/subsystem_mgr.hxx>

#include <AIModel/AIMultiplayer.hxx>

struct FGExternalMotionInfo;

class FGMultiplayMgr : public SGSubsystem
{
public:

  struct IdPropertyList {
    unsigned id;
    const char* name;
    simgear::props::Type type;
  };
  static const IdPropertyList sIdPropertyList[];
  static const unsigned numProperties;

  static const IdPropertyList* findProperty(unsigned id);
  
  FGMultiplayMgr();
  ~FGMultiplayMgr();
  
  virtual void init(void);
  virtual void update(double dt);
  
  void Close(void);
  // transmitter
  void SendMyPosition(const FGExternalMotionData& motionInfo);
  void SendTextMessage(const string &sMsgText);
  // receiver
  
private:
  union MsgBuf;
  FGAIMultiplayer* addMultiplayer(const std::string& callsign,
                                  const std::string& modelName);
  FGAIMultiplayer* getMultiplayer(const std::string& callsign);
  void FillMsgHdr(T_MsgHdr *MsgHdr, int iMsgId, unsigned _len = 0u);
  void ProcessPosMsg(const MsgBuf& Msg, const simgear::IPAddress& SenderAddress,
                     long stamp);
  void ProcessChatMsg(const MsgBuf& Msg, const simgear::IPAddress& SenderAddress);
  bool isSane(const FGExternalMotionData& motionInfo);

  /// maps from the callsign string to the FGAIMultiplayer
  typedef std::map<std::string, SGSharedPtr<FGAIMultiplayer> > MultiPlayerMap;
  MultiPlayerMap mMultiPlayerMap;

  simgear::Socket* mSocket;
  simgear::IPAddress mServer;
  bool mHaveServer;
  bool mInitialised;
  std::string mCallsign;
};

#endif

