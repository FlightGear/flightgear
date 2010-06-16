//////////////////////////////////////////////////////////////////////
//
// multiplaymgr.hpp
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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <string>
using std::string;
#include <vector>
using std::vector;

#include <simgear/compiler.h>
#include <simgear/props/props.hxx>
#include <plib/netSocket.h>
#include <Main/globals.hxx>

#include <AIModel/AIMultiplayer.hxx>

struct FGExternalMotionInfo;

class FGMultiplayMgr 
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
  bool init(void);
  void Close(void);
  // transmitter
  void SendMyPosition(const FGExternalMotionData& motionInfo);
  void SendTextMessage(const string &sMsgText);
  // receiver
  void Update(void);
  
private:
  union MsgBuf;
  FGAIMultiplayer* addMultiplayer(const std::string& callsign,
                                  const std::string& modelName);
  FGAIMultiplayer* getMultiplayer(const std::string& callsign);
  void FillMsgHdr(T_MsgHdr *MsgHdr, int iMsgId, unsigned _len = 0u);
  void ProcessPosMsg(const MsgBuf& Msg, const netAddress& SenderAddress,
                     long stamp);
  void ProcessChatMsg(const MsgBuf& Msg, const netAddress& SenderAddress);

  /// maps from the callsign string to the FGAIMultiplayer
  typedef std::map<std::string, SGSharedPtr<FGAIMultiplayer> > MultiPlayerMap;
  MultiPlayerMap mMultiPlayerMap;

  netSocket* mSocket;
  netAddress mServer;
  bool mHaveServer;
  bool mInitialised;
  string mCallsign;
};

#endif

