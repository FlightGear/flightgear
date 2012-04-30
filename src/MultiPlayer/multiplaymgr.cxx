//////////////////////////////////////////////////////////////////////
//
// multiplaymgr.cxx
//
// Written by Duncan McCreanor, started February 2003.
// duncan.mccreanor@airservicesaustralia.com
//
// Copyright (C) 2003  Airservices Australia
// Copyright (C) 2005  Oliver Schroeder
// Copyright (C) 2006  Mathias Froehlich
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <iostream>
#include <algorithm>
#include <cstring>
#include <errno.h>
#include <osg/Math>             // isNaN

#include <simgear/misc/stdint.hxx>
#include <simgear/timing/timestamp.hxx>
#include <simgear/debug/logstream.hxx>
#include <simgear/props/props.hxx>

#include <AIModel/AIManager.hxx>
#include <AIModel/AIMultiplayer.hxx>
#include <Main/fg_props.hxx>
#include "multiplaymgr.hxx"
#include "mpmessages.hxx"
#include <FDM/flightProperties.hxx>

using namespace std;

#define MAX_PACKET_SIZE 1200
#define MAX_TEXT_SIZE 128

// These constants are provided so that the ident 
// command can list file versions
const char sMULTIPLAYMGR_BID[] = "$Id$";
const char sMULTIPLAYMGR_HID[] = MULTIPLAYTXMGR_HID;

struct IdPropertyList {
  unsigned id;
  const char* name;
  simgear::props::Type type;
};
 
static const IdPropertyList* findProperty(unsigned id);
  
// A static map of protocol property id values to property paths,
// This should be extendable dynamically for every specific aircraft ...
// For now only that static list
static const IdPropertyList sIdPropertyList[] = {
  {100, "surface-positions/left-aileron-pos-norm",  simgear::props::FLOAT},
  {101, "surface-positions/right-aileron-pos-norm", simgear::props::FLOAT},
  {102, "surface-positions/elevator-pos-norm",      simgear::props::FLOAT},
  {103, "surface-positions/rudder-pos-norm",        simgear::props::FLOAT},
  {104, "surface-positions/flap-pos-norm",          simgear::props::FLOAT},
  {105, "surface-positions/speedbrake-pos-norm",    simgear::props::FLOAT},
  {106, "gear/tailhook/position-norm",              simgear::props::FLOAT},
  {107, "gear/launchbar/position-norm",             simgear::props::FLOAT},
  {108, "gear/launchbar/state",                     simgear::props::STRING},
  {109, "gear/launchbar/holdback-position-norm",    simgear::props::FLOAT},
  {110, "canopy/position-norm",                     simgear::props::FLOAT},
  {111, "surface-positions/wing-pos-norm",          simgear::props::FLOAT},
  {112, "surface-positions/wing-fold-pos-norm",     simgear::props::FLOAT},

  {200, "gear/gear[0]/compression-norm",           simgear::props::FLOAT},
  {201, "gear/gear[0]/position-norm",              simgear::props::FLOAT},
  {210, "gear/gear[1]/compression-norm",           simgear::props::FLOAT},
  {211, "gear/gear[1]/position-norm",              simgear::props::FLOAT},
  {220, "gear/gear[2]/compression-norm",           simgear::props::FLOAT},
  {221, "gear/gear[2]/position-norm",              simgear::props::FLOAT},
  {230, "gear/gear[3]/compression-norm",           simgear::props::FLOAT},
  {231, "gear/gear[3]/position-norm",              simgear::props::FLOAT},
  {240, "gear/gear[4]/compression-norm",           simgear::props::FLOAT},
  {241, "gear/gear[4]/position-norm",              simgear::props::FLOAT},

  {300, "engines/engine[0]/n1",  simgear::props::FLOAT},
  {301, "engines/engine[0]/n2",  simgear::props::FLOAT},
  {302, "engines/engine[0]/rpm", simgear::props::FLOAT},
  {310, "engines/engine[1]/n1",  simgear::props::FLOAT},
  {311, "engines/engine[1]/n2",  simgear::props::FLOAT},
  {312, "engines/engine[1]/rpm", simgear::props::FLOAT},
  {320, "engines/engine[2]/n1",  simgear::props::FLOAT},
  {321, "engines/engine[2]/n2",  simgear::props::FLOAT},
  {322, "engines/engine[2]/rpm", simgear::props::FLOAT},
  {330, "engines/engine[3]/n1",  simgear::props::FLOAT},
  {331, "engines/engine[3]/n2",  simgear::props::FLOAT},
  {332, "engines/engine[3]/rpm", simgear::props::FLOAT},
  {340, "engines/engine[4]/n1",  simgear::props::FLOAT},
  {341, "engines/engine[4]/n2",  simgear::props::FLOAT},
  {342, "engines/engine[4]/rpm", simgear::props::FLOAT},
  {350, "engines/engine[5]/n1",  simgear::props::FLOAT},
  {351, "engines/engine[5]/n2",  simgear::props::FLOAT},
  {352, "engines/engine[5]/rpm", simgear::props::FLOAT},
  {360, "engines/engine[6]/n1",  simgear::props::FLOAT},
  {361, "engines/engine[6]/n2",  simgear::props::FLOAT},
  {362, "engines/engine[6]/rpm", simgear::props::FLOAT},
  {370, "engines/engine[7]/n1",  simgear::props::FLOAT},
  {371, "engines/engine[7]/n2",  simgear::props::FLOAT},
  {372, "engines/engine[7]/rpm", simgear::props::FLOAT},
  {380, "engines/engine[8]/n1",  simgear::props::FLOAT},
  {381, "engines/engine[8]/n2",  simgear::props::FLOAT},
  {382, "engines/engine[8]/rpm", simgear::props::FLOAT},
  {390, "engines/engine[9]/n1",  simgear::props::FLOAT},
  {391, "engines/engine[9]/n2",  simgear::props::FLOAT},
  {392, "engines/engine[9]/rpm", simgear::props::FLOAT},

  {800, "rotors/main/rpm", simgear::props::FLOAT},
  {801, "rotors/tail/rpm", simgear::props::FLOAT},
  {810, "rotors/main/blade[0]/position-deg",  simgear::props::FLOAT},
  {811, "rotors/main/blade[1]/position-deg",  simgear::props::FLOAT},
  {812, "rotors/main/blade[2]/position-deg",  simgear::props::FLOAT},
  {813, "rotors/main/blade[3]/position-deg",  simgear::props::FLOAT},
  {820, "rotors/main/blade[0]/flap-deg",  simgear::props::FLOAT},
  {821, "rotors/main/blade[1]/flap-deg",  simgear::props::FLOAT},
  {822, "rotors/main/blade[2]/flap-deg",  simgear::props::FLOAT},
  {823, "rotors/main/blade[3]/flap-deg",  simgear::props::FLOAT},
  {830, "rotors/tail/blade[0]/position-deg",  simgear::props::FLOAT},
  {831, "rotors/tail/blade[1]/position-deg",  simgear::props::FLOAT},

  {900, "sim/hitches/aerotow/tow/length",                       simgear::props::FLOAT},
  {901, "sim/hitches/aerotow/tow/elastic-constant",             simgear::props::FLOAT},
  {902, "sim/hitches/aerotow/tow/weight-per-m-kg-m",            simgear::props::FLOAT},
  {903, "sim/hitches/aerotow/tow/dist",                         simgear::props::FLOAT},
  {904, "sim/hitches/aerotow/tow/connected-to-property-node",   simgear::props::BOOL},
  {905, "sim/hitches/aerotow/tow/connected-to-ai-or-mp-callsign",   simgear::props::STRING},
  {906, "sim/hitches/aerotow/tow/brake-force",                  simgear::props::FLOAT},
  {907, "sim/hitches/aerotow/tow/end-force-x",                  simgear::props::FLOAT},
  {908, "sim/hitches/aerotow/tow/end-force-y",                  simgear::props::FLOAT},
  {909, "sim/hitches/aerotow/tow/end-force-z",                  simgear::props::FLOAT},
  {930, "sim/hitches/aerotow/is-slave",                         simgear::props::BOOL},
  {931, "sim/hitches/aerotow/speed-in-tow-direction",           simgear::props::FLOAT},
  {932, "sim/hitches/aerotow/open",                             simgear::props::BOOL},
  {933, "sim/hitches/aerotow/local-pos-x",                      simgear::props::FLOAT},
  {934, "sim/hitches/aerotow/local-pos-y",                      simgear::props::FLOAT},
  {935, "sim/hitches/aerotow/local-pos-z",                      simgear::props::FLOAT},

  {1001, "controls/flight/slats",  simgear::props::FLOAT},
  {1002, "controls/flight/speedbrake",  simgear::props::FLOAT},
  {1003, "controls/flight/spoilers",  simgear::props::FLOAT},
  {1004, "controls/gear/gear-down",  simgear::props::FLOAT},
  {1005, "controls/lighting/nav-lights",  simgear::props::FLOAT},
  {1006, "controls/armament/station[0]/jettison-all",  simgear::props::BOOL},

  {1100, "sim/model/variant", simgear::props::INT},
  {1101, "sim/model/livery/file", simgear::props::STRING},

  {1200, "environment/wildfire/data", simgear::props::STRING},
  {1201, "environment/contrail", simgear::props::INT},

  {1300, "tanker", simgear::props::INT},

  {1400, "scenery/events", simgear::props::STRING},

  {10001, "sim/multiplay/transmission-freq-hz",  simgear::props::STRING},
  {10002, "sim/multiplay/chat",  simgear::props::STRING},

  {10100, "sim/multiplay/generic/string[0]", simgear::props::STRING},
  {10101, "sim/multiplay/generic/string[1]", simgear::props::STRING},
  {10102, "sim/multiplay/generic/string[2]", simgear::props::STRING},
  {10103, "sim/multiplay/generic/string[3]", simgear::props::STRING},
  {10104, "sim/multiplay/generic/string[4]", simgear::props::STRING},
  {10105, "sim/multiplay/generic/string[5]", simgear::props::STRING},
  {10106, "sim/multiplay/generic/string[6]", simgear::props::STRING},
  {10107, "sim/multiplay/generic/string[7]", simgear::props::STRING},
  {10108, "sim/multiplay/generic/string[8]", simgear::props::STRING},
  {10109, "sim/multiplay/generic/string[9]", simgear::props::STRING},
  {10110, "sim/multiplay/generic/string[10]", simgear::props::STRING},
  {10111, "sim/multiplay/generic/string[11]", simgear::props::STRING},
  {10112, "sim/multiplay/generic/string[12]", simgear::props::STRING},
  {10113, "sim/multiplay/generic/string[13]", simgear::props::STRING},
  {10114, "sim/multiplay/generic/string[14]", simgear::props::STRING},
  {10115, "sim/multiplay/generic/string[15]", simgear::props::STRING},
  {10116, "sim/multiplay/generic/string[16]", simgear::props::STRING},
  {10117, "sim/multiplay/generic/string[17]", simgear::props::STRING},
  {10118, "sim/multiplay/generic/string[18]", simgear::props::STRING},
  {10119, "sim/multiplay/generic/string[19]", simgear::props::STRING},

  {10200, "sim/multiplay/generic/float[0]", simgear::props::FLOAT},
  {10201, "sim/multiplay/generic/float[1]", simgear::props::FLOAT},
  {10202, "sim/multiplay/generic/float[2]", simgear::props::FLOAT},
  {10203, "sim/multiplay/generic/float[3]", simgear::props::FLOAT},
  {10204, "sim/multiplay/generic/float[4]", simgear::props::FLOAT},
  {10205, "sim/multiplay/generic/float[5]", simgear::props::FLOAT},
  {10206, "sim/multiplay/generic/float[6]", simgear::props::FLOAT},
  {10207, "sim/multiplay/generic/float[7]", simgear::props::FLOAT},
  {10208, "sim/multiplay/generic/float[8]", simgear::props::FLOAT},
  {10209, "sim/multiplay/generic/float[9]", simgear::props::FLOAT},
  {10210, "sim/multiplay/generic/float[10]", simgear::props::FLOAT},
  {10211, "sim/multiplay/generic/float[11]", simgear::props::FLOAT},
  {10212, "sim/multiplay/generic/float[12]", simgear::props::FLOAT},
  {10213, "sim/multiplay/generic/float[13]", simgear::props::FLOAT},
  {10214, "sim/multiplay/generic/float[14]", simgear::props::FLOAT},
  {10215, "sim/multiplay/generic/float[15]", simgear::props::FLOAT},
  {10216, "sim/multiplay/generic/float[16]", simgear::props::FLOAT},
  {10217, "sim/multiplay/generic/float[17]", simgear::props::FLOAT},
  {10218, "sim/multiplay/generic/float[18]", simgear::props::FLOAT},
  {10219, "sim/multiplay/generic/float[19]", simgear::props::FLOAT},

  {10300, "sim/multiplay/generic/int[0]", simgear::props::INT},
  {10301, "sim/multiplay/generic/int[1]", simgear::props::INT},
  {10302, "sim/multiplay/generic/int[2]", simgear::props::INT},
  {10303, "sim/multiplay/generic/int[3]", simgear::props::INT},
  {10304, "sim/multiplay/generic/int[4]", simgear::props::INT},
  {10305, "sim/multiplay/generic/int[5]", simgear::props::INT},
  {10306, "sim/multiplay/generic/int[6]", simgear::props::INT},
  {10307, "sim/multiplay/generic/int[7]", simgear::props::INT},
  {10308, "sim/multiplay/generic/int[8]", simgear::props::INT},
  {10309, "sim/multiplay/generic/int[9]", simgear::props::INT},
  {10310, "sim/multiplay/generic/int[10]", simgear::props::INT},
  {10311, "sim/multiplay/generic/int[11]", simgear::props::INT},
  {10312, "sim/multiplay/generic/int[12]", simgear::props::INT},
  {10313, "sim/multiplay/generic/int[13]", simgear::props::INT},
  {10314, "sim/multiplay/generic/int[14]", simgear::props::INT},
  {10315, "sim/multiplay/generic/int[15]", simgear::props::INT},
  {10316, "sim/multiplay/generic/int[16]", simgear::props::INT},
  {10317, "sim/multiplay/generic/int[17]", simgear::props::INT},
  {10318, "sim/multiplay/generic/int[18]", simgear::props::INT},
  {10319, "sim/multiplay/generic/int[19]", simgear::props::INT}
};

const unsigned int numProperties = (sizeof(sIdPropertyList)
                                 / sizeof(sIdPropertyList[0]));

// Look up a property ID using binary search.
namespace
{
  struct ComparePropertyId
  {
    bool operator()(const IdPropertyList& lhs,
                    const IdPropertyList& rhs)
    {
      return lhs.id < rhs.id;
    }
    bool operator()(const IdPropertyList& lhs,
                    unsigned id)
    {
      return lhs.id < id;
    }
    bool operator()(unsigned id,
                    const IdPropertyList& rhs)
    {
      return id < rhs.id;
    }
  };    
}

const IdPropertyList* findProperty(unsigned id)
{
  std::pair<const IdPropertyList*, const IdPropertyList*> result
    = std::equal_range(sIdPropertyList, sIdPropertyList + numProperties, id,
                       ComparePropertyId());
  if (result.first == result.second) {
    return 0;
  } else {
    return result.first;
  }
}

namespace
{
  bool verifyProperties(const xdr_data_t* data, const xdr_data_t* end)
  {
    using namespace simgear;
    const xdr_data_t* xdr = data;
    while (xdr < end) {
      unsigned id = XDR_decode_uint32(*xdr);
      const IdPropertyList* plist = findProperty(id);
    
      if (plist) {
        xdr++;
        // How we decode the remainder of the property depends on the type
        switch (plist->type) {
        case props::INT:
        case props::BOOL:
        case props::LONG:
          xdr++;
          break;
        case props::FLOAT:
        case props::DOUBLE:
          {
            float val = XDR_decode_float(*xdr);
            if (osg::isNaN(val))
              return false;
            xdr++;
            break;
          }
        case props::STRING:
        case props::UNSPECIFIED:
          {
            // String is complicated. It consists of
            // The length of the string
            // The string itself
            // Padding to the nearest 4-bytes.
            // XXX Yes, each byte is padded out to a word! Too late
            // to change...
            uint32_t length = XDR_decode_uint32(*xdr);
            xdr++;
            // Old versions truncated the string but left the length
            // unadjusted.
            if (length > MAX_TEXT_SIZE)
              length = MAX_TEXT_SIZE;
            xdr += length;
            // Now handle the padding
            while ((length % 4) != 0)
              {
                xdr++;
                length++;
                //cout << "0";
              }
          }
          break;
        default:
          // cerr << "Unknown Prop type " << id << " " << type << "\n";
          xdr++;
          break;
        }            
      }
      else {
        // give up; this is a malformed property list.
        return false;
      }
    }
    return true;
  }
}

class MPPropertyListener : public SGPropertyChangeListener
{
public:
  MPPropertyListener(FGMultiplayMgr* mp) :
    _multiplay(mp)
  {
  }

  virtual void childAdded(SGPropertyNode*, SGPropertyNode*)
  {
    _multiplay->setPropertiesChanged();
  }

private:
  FGMultiplayMgr* _multiplay;
};

//////////////////////////////////////////////////////////////////////
//
//  MultiplayMgr constructor
//
//////////////////////////////////////////////////////////////////////
FGMultiplayMgr::FGMultiplayMgr() 
{
  mInitialised   = false;
  mHaveServer    = false;
  mListener = NULL;
} // FGMultiplayMgr::FGMultiplayMgr()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
//
//  MultiplayMgr destructor
//
//////////////////////////////////////////////////////////////////////
FGMultiplayMgr::~FGMultiplayMgr() 
{
  
} // FGMultiplayMgr::~FGMultiplayMgr()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
//
//  Initialise object
//
//////////////////////////////////////////////////////////////////////
void
FGMultiplayMgr::init (void) 
{
  //////////////////////////////////////////////////
  //  Initialise object if not already done
  //////////////////////////////////////////////////
  if (mInitialised) {
    SG_LOG(SG_NETWORK, SG_WARN, "FGMultiplayMgr::init - already initialised");
    return;
  }

  fgSetBool("/sim/multiplay/online", false);

  //////////////////////////////////////////////////
  //  Set members from property values
  //////////////////////////////////////////////////
  short rxPort = fgGetInt("/sim/multiplay/rxport");
  string rxAddress = fgGetString("/sim/multiplay/rxhost");
  short txPort = fgGetInt("/sim/multiplay/txport", 5000);
  string txAddress = fgGetString("/sim/multiplay/txhost");
  
  int hz = fgGetInt("/sim/multiplay/tx-rate-hz", 10);
  if (hz < 1) {
    hz = 10;
  }
  
  mDt = 1.0 / hz;
  mTimeUntilSend = 0.0;
  
  mCallsign = fgGetString("/sim/multiplay/callsign");
  if ((!txAddress.empty()) && (txAddress!="0")) {
    mServer.set(txAddress.c_str(), txPort);
    if (strncmp (mServer.getHost(), "0.0.0.0", 8) == 0) {
      mHaveServer = false;
      SG_LOG(SG_NETWORK, SG_ALERT,
        "Cannot enable multiplayer mode: resolving MP server address '"
        << txAddress << "' failed.");
      return;
    } else {
      SG_LOG(SG_NETWORK, SG_INFO, "FGMultiplayMgr - have server");
      mHaveServer = true;
    }
    if (rxPort <= 0)
      rxPort = txPort;
  } else {
    SG_LOG(SG_NETWORK, SG_INFO, "FGMultiplayMgr - multiplayer mode disabled (no MP server specificed).");
    return;
  }

  if (rxPort <= 0) {
    SG_LOG(SG_NETWORK, SG_ALERT,
      "Cannot enable multiplayer mode: No receiver port specified.");
    return;
  }
  if (mCallsign.empty())
    mCallsign = "JohnDoe"; // FIXME: use getpwuid
  SG_LOG(SG_NETWORK,SG_INFO,"FGMultiplayMgr::init-txaddress= "<<txAddress);
  SG_LOG(SG_NETWORK,SG_INFO,"FGMultiplayMgr::init-txport= "<<txPort );
  SG_LOG(SG_NETWORK,SG_INFO,"FGMultiplayMgr::init-rxaddress="<<rxAddress );
  SG_LOG(SG_NETWORK,SG_INFO,"FGMultiplayMgr::init-rxport= "<<rxPort);
  SG_LOG(SG_NETWORK,SG_INFO,"FGMultiplayMgr::init-callsign= "<<mCallsign);
  
  mSocket.reset(new simgear::Socket());
  if (!mSocket->open(false)) {
    SG_LOG( SG_NETWORK, SG_ALERT,
            "Cannot enable multiplayer mode: creating data socket failed." );
    return;
  }
  mSocket->setBlocking(false);
  if (mSocket->bind(rxAddress.c_str(), rxPort) != 0) {
    SG_LOG( SG_NETWORK, SG_ALERT,
            "Cannot enable multiplayer mode: binding receive socket failed. "
            << strerror(errno) << "(errno " << errno << ")");
    return;
  }
  
  mPropertiesChanged = true;
  mListener = new MPPropertyListener(this);
  globals->get_props()->addChangeListener(mListener, false);
  
  fgSetBool("/sim/multiplay/online", true);
  mInitialised = true;

  SG_LOG(SG_NETWORK, SG_ALERT, "Multiplayer mode active!");

  if (!fgGetBool("/sim/ai/enabled"))
  {
      // multiplayer depends on AI module
      fgSetBool("/sim/ai/enabled", true);
  }
} // FGMultiplayMgr::init()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
//
//  Closes and deletes the local player object. Closes
//  and deletes the tx socket. Resets the object state to unitialised.
//
//////////////////////////////////////////////////////////////////////
void
FGMultiplayMgr::shutdown (void) 
{
  fgSetBool("/sim/multiplay/online", false);
  
  if (mSocket.get()) {
    mSocket->close();
    mSocket.reset(); 
  }
  
  MultiPlayerMap::iterator it = mMultiPlayerMap.begin(),
    end = mMultiPlayerMap.end();
  for (; it != end; ++it) {
    it->second->setDie(true);
  }
  mMultiPlayerMap.clear();
  
  if (mListener) {
    globals->get_props()->removeChangeListener(mListener);
    delete mListener;
    mListener = NULL;
  }
  
  mInitialised = false;
} // FGMultiplayMgr::Close(void)
//////////////////////////////////////////////////////////////////////

void
FGMultiplayMgr::reinit()
{
  shutdown();
  init();
}

//////////////////////////////////////////////////////////////////////
//
//  Description: Sends the position data for the local position.
//
//////////////////////////////////////////////////////////////////////

/**
 * The buffer that holds a multi-player message, suitably aligned.
 */
union FGMultiplayMgr::MsgBuf
{
    MsgBuf()
    {
        memset(&Msg, 0, sizeof(Msg));
    }

    T_MsgHdr* msgHdr()
    {
        return &Header;
    }

    const T_MsgHdr* msgHdr() const
    {
        return reinterpret_cast<const T_MsgHdr*>(&Header);
    }

    T_PositionMsg* posMsg()
    {
        return reinterpret_cast<T_PositionMsg*>(Msg + sizeof(T_MsgHdr));
    }

    const T_PositionMsg* posMsg() const
    {
        return reinterpret_cast<const T_PositionMsg*>(Msg + sizeof(T_MsgHdr));
    }

    xdr_data_t* properties()
    {
        return reinterpret_cast<xdr_data_t*>(Msg + sizeof(T_MsgHdr)
                                             + sizeof(T_PositionMsg));
    }

    const xdr_data_t* properties() const
    {
        return reinterpret_cast<const xdr_data_t*>(Msg + sizeof(T_MsgHdr)
                                                   + sizeof(T_PositionMsg));
    }
    /**
     * The end of the properties buffer.
     */
    xdr_data_t* propsEnd()
    {
        return reinterpret_cast<xdr_data_t*>(Msg + MAX_PACKET_SIZE);
    };

    const xdr_data_t* propsEnd() const
    {
        return reinterpret_cast<const xdr_data_t*>(Msg + MAX_PACKET_SIZE);
    };
    /**
     * The end of properties actually in the buffer. This assumes that
     * the message header is valid.
     */
    xdr_data_t* propsRecvdEnd()
    {
        return reinterpret_cast<xdr_data_t*>(Msg + Header.MsgLen);
    }

    const xdr_data_t* propsRecvdEnd() const
    {
        return reinterpret_cast<const xdr_data_t*>(Msg + Header.MsgLen);
    }
    
    xdr_data2_t double_val;
    char Msg[MAX_PACKET_SIZE];
    T_MsgHdr Header;
};

bool
FGMultiplayMgr::isSane(const FGExternalMotionData& motionInfo)
{
    // check for corrupted data (NaNs)
    bool isCorrupted = false;
    isCorrupted |= ((osg::isNaN(motionInfo.time           )) ||
                    (osg::isNaN(motionInfo.lag            )) ||
                    (osg::isNaN(motionInfo.orientation(3) )));
    for (unsigned i = 0; (i < 3)&&(!isCorrupted); ++i)
    {
        isCorrupted |= ((osg::isNaN(motionInfo.position(i)      ))||
                        (osg::isNaN(motionInfo.orientation(i)   ))||
                        (osg::isNaN(motionInfo.linearVel(i))    )||
                        (osg::isNaN(motionInfo.angularVel(i))   )||
                        (osg::isNaN(motionInfo.linearAccel(i))  )||
                        (osg::isNaN(motionInfo.angularAccel(i)) ));
    }
    return !isCorrupted;
}

void
FGMultiplayMgr::SendMyPosition(const FGExternalMotionData& motionInfo)
{
  if ((! mInitialised) || (! mHaveServer))
        return;

  if (! mHaveServer) {
      SG_LOG( SG_NETWORK, SG_DEBUG, "FGMultiplayMgr::SendMyPosition - no server");
      return;
  }

  if (!isSane(motionInfo))
  {
      // Current local data is invalid (NaN), so stop MP transmission.
      // => Be nice to older FG versions (no NaN checks) and don't waste bandwidth.
      SG_LOG(SG_NETWORK, SG_ALERT, "FGMultiplayMgr::SendMyPosition - "
              << "Local data is invalid (NaN). Data not transmitted.");
      return;
  }

  static MsgBuf msgBuf;
  static unsigned msgLen = 0;
  T_PositionMsg* PosMsg = msgBuf.posMsg();

  strncpy(PosMsg->Model, fgGetString("/sim/model/path"), MAX_MODEL_NAME_LEN);
  PosMsg->Model[MAX_MODEL_NAME_LEN - 1] = '\0';
  if (fgGetBool("/sim/freeze/replay-state", true)&&
      fgGetBool("/sim/multiplay/freeze-on-replay",true))
  {
      // do not send position updates during replay
      for (unsigned i = 0 ; i < 3; ++i)
      {
          // no movement during replay
          PosMsg->linearVel[i] = XDR_encode_float (0.0);
          PosMsg->angularVel[i] = XDR_encode_float (0.0);
          PosMsg->linearAccel[i] = XDR_encode_float (0.0);
          PosMsg->angularAccel[i] = XDR_encode_float (0.0);
      }
      // all other data remains unchanged (resend last state) 
  }
  else
  {
      PosMsg->time = XDR_encode_double (motionInfo.time);
      PosMsg->lag = XDR_encode_double (motionInfo.lag);
      for (unsigned i = 0 ; i < 3; ++i)
        PosMsg->position[i] = XDR_encode_double (motionInfo.position(i));
      SGVec3f angleAxis;
      motionInfo.orientation.getAngleAxis(angleAxis);
      for (unsigned i = 0 ; i < 3; ++i)
        PosMsg->orientation[i] = XDR_encode_float (angleAxis(i));
      for (unsigned i = 0 ; i < 3; ++i)
        PosMsg->linearVel[i] = XDR_encode_float (motionInfo.linearVel(i));
      for (unsigned i = 0 ; i < 3; ++i)
        PosMsg->angularVel[i] = XDR_encode_float (motionInfo.angularVel(i));
      for (unsigned i = 0 ; i < 3; ++i)
        PosMsg->linearAccel[i] = XDR_encode_float (motionInfo.linearAccel(i));
      for (unsigned i = 0 ; i < 3; ++i)
        PosMsg->angularAccel[i] = XDR_encode_float (motionInfo.angularAccel(i));

      xdr_data_t* ptr = msgBuf.properties();
      std::vector<FGPropertyData*>::const_iterator it;
      it = motionInfo.properties.begin();
      //cout << "OUTPUT PROPERTIES\n";
      xdr_data_t* msgEnd = msgBuf.propsEnd();
      while (it != motionInfo.properties.end() && ptr + 2 < msgEnd) {
        
        // First element is the ID. Write it out when we know we have room for
        // the whole property.
        xdr_data_t id =  XDR_encode_uint32((*it)->id);
        // The actual data representation depends on the type
        switch ((*it)->type) {
          case simgear::props::INT:
          case simgear::props::BOOL:
          case simgear::props::LONG:
            *ptr++ = id;
            *ptr++ = XDR_encode_uint32((*it)->int_value);
            //cout << "Prop:" << (*it)->id << " " << (*it)->type << " "<< (*it)->int_value << "\n";
            break;
          case simgear::props::FLOAT:
          case simgear::props::DOUBLE:
            *ptr++ = id;
            *ptr++ = XDR_encode_float((*it)->float_value);
            //cout << "Prop:" << (*it)->id << " " << (*it)->type << " "<< (*it)->float_value << "\n";
            break;
          case simgear::props::STRING:
          case simgear::props::UNSPECIFIED:
            {
              // String is complicated. It consists of
              // The length of the string
              // The string itself
              // Padding to the nearest 4-bytes.        
              const char* lcharptr = (*it)->string_value;
              
              if (lcharptr != 0)
              {
                // Add the length         
                ////cout << "String length: " << strlen(lcharptr) << "\n";
                uint32_t len = strlen(lcharptr);
                if (len > MAX_TEXT_SIZE)
                  len = MAX_TEXT_SIZE;
                // XXX This should not be using 4 bytes per character!
                // If there's not enough room for this property, drop it
                // on the floor.
                if (ptr + 2 + ((len + 3) & ~3) > msgEnd)
                    goto escape;
                //cout << "String length unint32: " << len << "\n";
                *ptr++ = id;
                *ptr++ = XDR_encode_uint32(len);
                if (len != 0)
                {
                  // Now the text itself
                  // XXX This should not be using 4 bytes per character!
                  int lcount = 0;
                  while ((*lcharptr != '\0') && (lcount < MAX_TEXT_SIZE)) 
                  {
                    *ptr++ = XDR_encode_int8(*lcharptr);
                    lcharptr++;
                    lcount++;          
                  }
    
                  //cout << "Prop:" << (*it)->id << " " << (*it)->type << " " << len << " " << (*it)->string_value;
    
                  // Now pad if required
                  while ((lcount % 4) != 0)
                  {
                    *ptr++ = XDR_encode_int8(0);
                    lcount++;          
                    //cout << "0";
                  }
                  
                  //cout << "\n";
                }
              }
              else
              {
                // Nothing to encode
                *ptr++ = id;
                *ptr++ = XDR_encode_uint32(0);
                //cout << "Prop:" << (*it)->id << " " << (*it)->type << " 0\n";
              }
            }
            break;
            
          default:
            //cout << " Unknown Type: " << (*it)->type << "\n";
            *ptr++ = id;
            *ptr++ = XDR_encode_float((*it)->float_value);;
            //cout << "Prop:" << (*it)->id << " " << (*it)->type << " "<< (*it)->float_value << "\n";
            break;
        }
            
        ++it;
      }
  escape:
      msgLen = reinterpret_cast<char*>(ptr) - msgBuf.Msg;
      FillMsgHdr(msgBuf.msgHdr(), POS_DATA_ID, msgLen);
  }
  if (msgLen>0)
      mSocket->sendto(msgBuf.Msg, msgLen, 0, &mServer);
  SG_LOG(SG_NETWORK, SG_DEBUG, "FGMultiplayMgr::SendMyPosition");
} // FGMultiplayMgr::SendMyPosition()

//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
//
//  Name: SendTextMessage
//  Description: Sends a message to the player. The message must
//  contain a valid and correctly filled out header and optional
//  message body.
//
//////////////////////////////////////////////////////////////////////
void
FGMultiplayMgr::SendTextMessage(const string &MsgText)
{
  if (!mInitialised || !mHaveServer)
    return;

  T_MsgHdr MsgHdr;
  FillMsgHdr(&MsgHdr, CHAT_MSG_ID);
  //////////////////////////////////////////////////
  // Divide the text string into blocks that fit
  // in the message and send the blocks.
  //////////////////////////////////////////////////
  unsigned iNextBlockPosition = 0;
  T_ChatMsg ChatMsg;
  
  char Msg[sizeof(T_MsgHdr) + sizeof(T_ChatMsg)];
  while (iNextBlockPosition < MsgText.length()) {
    strncpy (ChatMsg.Text, 
             MsgText.substr(iNextBlockPosition, MAX_CHAT_MSG_LEN - 1).c_str(),
             MAX_CHAT_MSG_LEN);
    ChatMsg.Text[MAX_CHAT_MSG_LEN - 1] = '\0';
    memcpy (Msg, &MsgHdr, sizeof(T_MsgHdr));
    memcpy (Msg + sizeof(T_MsgHdr), &ChatMsg, sizeof(T_ChatMsg));
    mSocket->sendto (Msg, sizeof(T_MsgHdr) + sizeof(T_ChatMsg), 0, &mServer);
    iNextBlockPosition += MAX_CHAT_MSG_LEN - 1;

  }
  
  
} // FGMultiplayMgr::SendTextMessage ()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
//
//  Name: ProcessData
//  Description: Processes data waiting at the receive socket. The
//  processing ends when there is no more data at the socket.
//  
//////////////////////////////////////////////////////////////////////
void
FGMultiplayMgr::update(double dt) 
{
  if (!mInitialised)
    return;

  /// Just for expiry
  long stamp = SGTimeStamp::now().getSeconds();

  //////////////////////////////////////////////////
  //  Send if required
  //////////////////////////////////////////////////
  mTimeUntilSend -= dt;
  if (mTimeUntilSend <= 0.0) {
    Send();
  }

  //////////////////////////////////////////////////
  //  Read the receive socket and process any data
  //////////////////////////////////////////////////
  ssize_t bytes;
  do {
    MsgBuf msgBuf;
    //////////////////////////////////////////////////
    //  Although the recv call asks for 
    //  MAX_PACKET_SIZE of data, the number of bytes
    //  returned will only be that of the next
    //  packet waiting to be processed.
    //////////////////////////////////////////////////
    simgear::IPAddress SenderAddress;
    int RecvStatus = mSocket->recvfrom(msgBuf.Msg, sizeof(msgBuf.Msg), 0,
                              &SenderAddress);
    //////////////////////////////////////////////////
    //  no Data received
    //////////////////////////////////////////////////
    if (RecvStatus == 0)
        break;

    // socket error reported?
    // errno isn't thread-safe - so only check its value when
    // socket return status < 0 really indicates a failure.
    if ((RecvStatus < 0)&&
        ((errno == EAGAIN) || (errno == 0))) // MSVC output "NoError" otherwise
    {
        // ignore "normal" errors
        break;
    }

    if (RecvStatus<0)
    {
        SG_LOG(SG_NETWORK, SG_DEBUG, "FGMultiplayMgr::MP_ProcessData - Unable to receive data. "
               << strerror(errno) << "(errno " << errno << ")");
        break;
    }

    // status is positive: bytes received
    bytes = (ssize_t) RecvStatus;
    if (bytes <= static_cast<ssize_t>(sizeof(T_MsgHdr))) {
      SG_LOG( SG_NETWORK, SG_DEBUG, "FGMultiplayMgr::MP_ProcessData - "
              << "received message with insufficient data" );
      break;
    }
    //////////////////////////////////////////////////
    //  Read header
    //////////////////////////////////////////////////
    T_MsgHdr* MsgHdr = msgBuf.msgHdr();
    MsgHdr->Magic       = XDR_decode_uint32 (MsgHdr->Magic);
    MsgHdr->Version     = XDR_decode_uint32 (MsgHdr->Version);
    MsgHdr->MsgId       = XDR_decode_uint32 (MsgHdr->MsgId);
    MsgHdr->MsgLen      = XDR_decode_uint32 (MsgHdr->MsgLen);
    MsgHdr->ReplyPort   = XDR_decode_uint32 (MsgHdr->ReplyPort);
    MsgHdr->Callsign[MAX_CALLSIGN_LEN -1] = '\0';
    if (MsgHdr->Magic != MSG_MAGIC) {
      SG_LOG( SG_NETWORK, SG_DEBUG, "FGMultiplayMgr::MP_ProcessData - "
              << "message has invalid magic number!" );
      break;
    }
    if (MsgHdr->Version != PROTO_VER) {
      SG_LOG( SG_NETWORK, SG_DEBUG, "FGMultiplayMgr::MP_ProcessData - "
              << "message has invalid protocol number!" );
      break;
    }
    if (static_cast<ssize_t>(MsgHdr->MsgLen) != bytes) {
      SG_LOG(SG_NETWORK, SG_DEBUG, "FGMultiplayMgr::MP_ProcessData - "
             << "message from " << MsgHdr->Callsign << " has invalid length!");
      break;
    }
    //////////////////////////////////////////////////
    //  Process messages
    //////////////////////////////////////////////////
    switch (MsgHdr->MsgId) {
    case CHAT_MSG_ID:
      ProcessChatMsg(msgBuf, SenderAddress);
      break;
    case POS_DATA_ID:
      ProcessPosMsg(msgBuf, SenderAddress, stamp);
      break;
    case UNUSABLE_POS_DATA_ID:
    case OLD_OLD_POS_DATA_ID:
    case OLD_PROP_MSG_ID:
    case OLD_POS_DATA_ID:
      break;
    default:
      SG_LOG( SG_NETWORK, SG_DEBUG, "FGMultiplayMgr::MP_ProcessData - "
              << "Unknown message Id received: " << MsgHdr->MsgId );
      break;
    }
  } while (bytes > 0);

  // check for expiry
  MultiPlayerMap::iterator it = mMultiPlayerMap.begin();
  while (it != mMultiPlayerMap.end()) {
    if (it->second->getLastTimestamp() + 10 < stamp) {
      std::string name = it->first;
      it->second->setDie(true);
      mMultiPlayerMap.erase(it);
      it = mMultiPlayerMap.upper_bound(name);
    } else
      ++it;
  }
} // FGMultiplayMgr::ProcessData(void)
//////////////////////////////////////////////////////////////////////

void
FGMultiplayMgr::Send()
{
  using namespace simgear;
  
  findProperties();
    
  // smooth the send rate, by adjusting based on the 'remainder' time, which
  // is how -ve mTimeUntilSend is. Watch for large values and ignore them,
  // however.
    if ((mTimeUntilSend < 0.0) && (fabs(mTimeUntilSend) < mDt)) {
      mTimeUntilSend = mDt + mTimeUntilSend;
    } else {
      mTimeUntilSend = mDt;
    }

    double sim_time = globals->get_sim_time_sec();
//    static double lastTime = 0.0;
    
   // SG_LOG(SG_GENERAL, SG_INFO, "actual dt=" << sim_time - lastTime);
//    lastTime = sim_time;
    
    FlightProperties ifce;

    // put together a motion info struct, you will get that later
    // from FGInterface directly ...
    FGExternalMotionData motionInfo;

    // The current simulation time we need to update for,
    // note that the simulation time is updated before calling all the
    // update methods. Thus it contains the time intervals *end* time.
    // The FDM is already run, so the states belong to that time.
    motionInfo.time = sim_time;
    motionInfo.lag = mDt;

    // These are for now converted from lat/lon/alt and euler angles.
    // But this should change in FGInterface ...
    double lon = ifce.get_Longitude();
    double lat = ifce.get_Latitude();
    // first the aprioriate structure for the geodetic one
    SGGeod geod = SGGeod::fromRadFt(lon, lat, ifce.get_Altitude());
    // Convert to cartesion coordinate
    motionInfo.position = SGVec3d::fromGeod(geod);
    
    // The quaternion rotating from the earth centered frame to the
    // horizontal local frame
    SGQuatf qEc2Hl = SGQuatf::fromLonLatRad((float)lon, (float)lat);
    // The orientation wrt the horizontal local frame
    float heading = ifce.get_Psi();
    float pitch = ifce.get_Theta();
    float roll = ifce.get_Phi();
    SGQuatf hlOr = SGQuatf::fromYawPitchRoll(heading, pitch, roll);
    // The orientation of the vehicle wrt the earth centered frame
    motionInfo.orientation = qEc2Hl*hlOr;

    if (!globals->get_subsystem("flight")->is_suspended()) {
      // velocities
      motionInfo.linearVel = SG_FEET_TO_METER*SGVec3f(ifce.get_uBody(),
                                                      ifce.get_vBody(),
                                                      ifce.get_wBody());
      motionInfo.angularVel = SGVec3f(ifce.get_P_body(),
                                      ifce.get_Q_body(),
                                      ifce.get_R_body());
      
      // accels, set that to zero for now.
      // Angular accelerations are missing from the interface anyway,
      // linear accelerations are screwed up at least for JSBSim.
//  motionInfo.linearAccel = SG_FEET_TO_METER*SGVec3f(ifce.get_U_dot_body(),
//                                                    ifce.get_V_dot_body(),
//                                                    ifce.get_W_dot_body());
      motionInfo.linearAccel = SGVec3f::zeros();
      motionInfo.angularAccel = SGVec3f::zeros();
    } else {
      // if the interface is suspendend, prevent the client from
      // wild extrapolations
      motionInfo.linearVel = SGVec3f::zeros();
      motionInfo.angularVel = SGVec3f::zeros();
      motionInfo.linearAccel = SGVec3f::zeros();
      motionInfo.angularAccel = SGVec3f::zeros();
    }

    // now send the properties
    PropertyMap::iterator it;
    for (it = mPropertyMap.begin(); it != mPropertyMap.end(); ++it) {
      FGPropertyData* pData = new FGPropertyData;
      pData->id = it->first;
      pData->type = it->second->getType();
      
      switch (pData->type) {
        case props::INT:
        case props::LONG:
        case props::BOOL:
          pData->int_value = it->second->getIntValue();
          break;
        case props::FLOAT:
        case props::DOUBLE:
          pData->float_value = it->second->getFloatValue();
          break;
        case props::STRING:
        case props::UNSPECIFIED:
          {
            // FIXME: We assume unspecified are strings for the moment.

            const char* cstr = it->second->getStringValue();
            int len = strlen(cstr);
            
            if (len > 0)
            {            
              pData->string_value = new char[len + 1];
              strcpy(pData->string_value, cstr);
            }
            else
            {
              // Size 0 - ignore
              pData->string_value = 0;            
            }

            //cout << " Sending property " << pData->id << " " << pData->type << " " <<  pData->string_value << "\n";
            break;        
          }
        default:
          // FIXME Currently default to a float. 
          //cout << "Unknown type when iterating through props: " << pData->type << "\n";
          pData->float_value = it->second->getFloatValue();
          break;
      }
      
      motionInfo.properties.push_back(pData);
    }

    SendMyPosition(motionInfo);
}


//////////////////////////////////////////////////////////////////////
//
//  handle a position message
//
//////////////////////////////////////////////////////////////////////
void
FGMultiplayMgr::ProcessPosMsg(const FGMultiplayMgr::MsgBuf& Msg,
                              const simgear::IPAddress& SenderAddress, long stamp)
{
  const T_MsgHdr* MsgHdr = Msg.msgHdr();
  if (MsgHdr->MsgLen < sizeof(T_MsgHdr) + sizeof(T_PositionMsg)) {
    SG_LOG( SG_NETWORK, SG_DEBUG, "FGMultiplayMgr::MP_ProcessData - "
            << "Position message received with insufficient data" );
    return;
  }
  const T_PositionMsg* PosMsg = Msg.posMsg();
  FGExternalMotionData motionInfo;
  motionInfo.time = XDR_decode_double(PosMsg->time);
  motionInfo.lag = XDR_decode_double(PosMsg->lag);
  for (unsigned i = 0; i < 3; ++i)
    motionInfo.position(i) = XDR_decode_double(PosMsg->position[i]);
  SGVec3f angleAxis;
  for (unsigned i = 0; i < 3; ++i)
    angleAxis(i) = XDR_decode_float(PosMsg->orientation[i]);
  motionInfo.orientation = SGQuatf::fromAngleAxis(angleAxis);
  for (unsigned i = 0; i < 3; ++i)
    motionInfo.linearVel(i) = XDR_decode_float(PosMsg->linearVel[i]);
  for (unsigned i = 0; i < 3; ++i)
    motionInfo.angularVel(i) = XDR_decode_float(PosMsg->angularVel[i]);
  for (unsigned i = 0; i < 3; ++i)
    motionInfo.linearAccel(i) = XDR_decode_float(PosMsg->linearAccel[i]);
  for (unsigned i = 0; i < 3; ++i)
    motionInfo.angularAccel(i) = XDR_decode_float(PosMsg->angularAccel[i]);

  // sanity check: do not allow injection of corrupted data (NaNs)
  if (!isSane(motionInfo))
  {
      // drop this message, keep old position until receiving valid data
      SG_LOG(SG_NETWORK, SG_DEBUG, "FGMultiplayMgr::ProcessPosMsg - "
              << "Position message with invalid data (NaN) received from "
              << MsgHdr->Callsign);
      return;
  }

  //cout << "INPUT MESSAGE\n";

  // There was a bug in 1.9.0 and before: T_PositionMsg was 196 bytes
  // on 32 bit architectures and 200 bytes on 64 bit, and this
  // structure is put directly on the wire. By looking at the padding,
  // we can sort through the mess, mostly:
  // If padding is 0 (which is not a valid property type), then the
  // message was produced by a new client or an old 64 bit client that
  // happened to have 0 on the stack;
  // Else if the property list starting with the padding word is
  // well-formed, then the client is probably an old 32 bit client and
  // we'll go with that;
  // Else it is an old 64-bit client and properties start after the
  // padding.
  // There is a chance that we could be fooled by garbage in the
  // padding looking like a valid property, so verifyProperties() is
  // strict about the validity of the property values.
  const xdr_data_t* xdr = Msg.properties();
  if (PosMsg->pad != 0) {
    if (verifyProperties(&PosMsg->pad, Msg.propsRecvdEnd()))
      xdr = &PosMsg->pad;
    else if (!verifyProperties(xdr, Msg.propsRecvdEnd()))
      goto noprops;
  }
  while (xdr < Msg.propsRecvdEnd()) {
    // simgear::props::Type type = simgear::props::UNSPECIFIED;
    
    // First element is always the ID
    unsigned id = XDR_decode_uint32(*xdr);
    //cout << pData->id << " ";
    xdr++;
    
    // Check the ID actually exists and get the type
    const IdPropertyList* plist = findProperty(id);
    
    if (plist)
    {
      FGPropertyData* pData = new FGPropertyData;
      pData->id = id;
      pData->type = plist->type;
      // How we decode the remainder of the property depends on the type
      switch (pData->type) {
        case simgear::props::INT:
        case simgear::props::BOOL:
        case simgear::props::LONG:
          pData->int_value = XDR_decode_uint32(*xdr);
          xdr++;
          //cout << pData->int_value << "\n";
          break;
        case simgear::props::FLOAT:
        case simgear::props::DOUBLE:
          pData->float_value = XDR_decode_float(*xdr);
          xdr++;
          //cout << pData->float_value << "\n";
          break;
        case simgear::props::STRING:
        case simgear::props::UNSPECIFIED:
          {
            // String is complicated. It consists of
            // The length of the string
            // The string itself
            // Padding to the nearest 4-bytes.    
            uint32_t length = XDR_decode_uint32(*xdr);
            xdr++;
            //cout << length << " ";
            // Old versions truncated the string but left the length unadjusted.
            if (length > MAX_TEXT_SIZE)
              length = MAX_TEXT_SIZE;
            pData->string_value = new char[length + 1];
            //cout << " String: ";
            for (unsigned i = 0; i < length; i++)
              {
                pData->string_value[i] = (char) XDR_decode_int8(*xdr);
                xdr++;
                //cout << pData->string_value[i];
              }

            pData->string_value[length] = '\0';

            // Now handle the padding
            while ((length % 4) != 0)
              {
                xdr++;
                length++;
                //cout << "0";
              }
            //cout << "\n";
          }
          break;

        default:
          pData->float_value = XDR_decode_float(*xdr);
          SG_LOG(SG_NETWORK, SG_DEBUG, "Unknown Prop type " << pData->id << " " << pData->type);
          xdr++;
          break;
      }

      motionInfo.properties.push_back(pData);
    }
    else
    {
      // We failed to find the property. We'll try the next packet immediately.
      SG_LOG(SG_NETWORK, SG_DEBUG, "FGMultiplayMgr::ProcessPosMsg - "
             "message from " << MsgHdr->Callsign << " has unknown property id "
             << id); 
    }
  }
 noprops:
  FGAIMultiplayer* mp = getMultiplayer(MsgHdr->Callsign);
  if (!mp)
    mp = addMultiplayer(MsgHdr->Callsign, PosMsg->Model);
  mp->addMotionInfo(motionInfo, stamp);
} // FGMultiplayMgr::ProcessPosMsg()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
//
//  handle a chat message
//  FIXME: display chat message within flightgear
//
//////////////////////////////////////////////////////////////////////
void
FGMultiplayMgr::ProcessChatMsg(const MsgBuf& Msg,
                               const simgear::IPAddress& SenderAddress)
{
  const T_MsgHdr* MsgHdr = Msg.msgHdr();
  if (MsgHdr->MsgLen < sizeof(T_MsgHdr) + 1) {
    SG_LOG( SG_NETWORK, SG_DEBUG, "FGMultiplayMgr::MP_ProcessData - "
            << "Chat message received with insufficient data" );
    return;
  }
  
  char *chatStr = new char[MsgHdr->MsgLen - sizeof(T_MsgHdr)];
  const T_ChatMsg* ChatMsg
      = reinterpret_cast<const T_ChatMsg *>(Msg.Msg + sizeof(T_MsgHdr));
  strncpy(chatStr, ChatMsg->Text,
          MsgHdr->MsgLen - sizeof(T_MsgHdr));
  chatStr[MsgHdr->MsgLen - sizeof(T_MsgHdr) - 1] = '\0';
  
  SG_LOG (SG_NETWORK, SG_WARN, "Chat [" << MsgHdr->Callsign << "]"
           << " " << chatStr);

  delete [] chatStr;
} // FGMultiplayMgr::ProcessChatMsg ()
//////////////////////////////////////////////////////////////////////

void
FGMultiplayMgr::FillMsgHdr(T_MsgHdr *MsgHdr, int MsgId, unsigned _len)
{
  uint32_t len;
  switch (MsgId) {
  case CHAT_MSG_ID:
    len = sizeof(T_MsgHdr) + sizeof(T_ChatMsg);
    break;
  case POS_DATA_ID:
    len = _len;
    break;
  default:
    len = sizeof(T_MsgHdr);
    break;
  }
  MsgHdr->Magic           = XDR_encode_uint32(MSG_MAGIC);
  MsgHdr->Version         = XDR_encode_uint32(PROTO_VER);
  MsgHdr->MsgId           = XDR_encode_uint32(MsgId);
  MsgHdr->MsgLen          = XDR_encode_uint32(len);
  MsgHdr->ReplyAddress    = 0; // Are obsolete, keep them for the server for
  MsgHdr->ReplyPort       = 0; // now
  strncpy(MsgHdr->Callsign, mCallsign.c_str(), MAX_CALLSIGN_LEN);
  MsgHdr->Callsign[MAX_CALLSIGN_LEN - 1] = '\0';
}

FGAIMultiplayer*
FGMultiplayMgr::addMultiplayer(const std::string& callsign,
                               const std::string& modelName)
{
  if (0 < mMultiPlayerMap.count(callsign))
    return mMultiPlayerMap[callsign].get();

  FGAIMultiplayer* mp = new FGAIMultiplayer;
  mp->setPath(modelName.c_str());
  mp->setCallSign(callsign);
  mMultiPlayerMap[callsign] = mp;

  FGAIManager *aiMgr = (FGAIManager*)globals->get_subsystem("ai-model");
  if (aiMgr) {
    aiMgr->attach(mp);

    /// FIXME: that must follow the attach ATM ...
    for (unsigned i = 0; i < numProperties; ++i)
      mp->addPropertyId(sIdPropertyList[i].id, sIdPropertyList[i].name);
  }

  return mp;
}

FGAIMultiplayer*
FGMultiplayMgr::getMultiplayer(const std::string& callsign)
{
  if (0 < mMultiPlayerMap.count(callsign))
    return mMultiPlayerMap[callsign].get();
  else
    return 0;
}

void
FGMultiplayMgr::findProperties()
{
  if (!mPropertiesChanged) {
    return;
  }
  
  mPropertiesChanged = false;
  
  for (unsigned i = 0; i < numProperties; ++i) {
      const char* name = sIdPropertyList[i].name;
      SGPropertyNode* pNode = globals->get_props()->getNode(name);
      if (!pNode) {
        continue;
      }
      
      int id = sIdPropertyList[i].id;
      if (mPropertyMap.find(id) != mPropertyMap.end()) {
        continue; // already activated
      }
      
      mPropertyMap[id] = pNode;
      SG_LOG(SG_NETWORK, SG_DEBUG, "activating MP property:" << pNode->getPath());
    }
}
