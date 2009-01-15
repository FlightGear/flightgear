//////////////////////////////////////////////////////////////////////
//
// multiplaymgr.hpp
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
#include <osg/Math>             // isNaN
#include <plib/netSocket.h>

#include <simgear/misc/stdint.hxx>
#include <simgear/timing/timestamp.hxx>
#include <simgear/debug/logstream.hxx>
#include <simgear/props/props.hxx>

#include <AIModel/AIManager.hxx>
#include <Main/fg_props.hxx>
#include "multiplaymgr.hxx"
#include "mpmessages.hxx"

using std::cerr;

#define MAX_PACKET_SIZE 1200
#define MAX_TEXT_SIZE 128

// These constants are provided so that the ident 
// command can list file versions
const char sMULTIPLAYMGR_BID[] = "$Id$";
const char sMULTIPLAYMGR_HID[] = MULTIPLAYTXMGR_HID;

// A static map of protocol property id values to property paths,
// This should be extendable dynamically for every specific aircraft ...
// For now only that static list
FGMultiplayMgr::IdPropertyList
const FGMultiplayMgr::sIdPropertyList[] = {
  {100, "surface-positions/left-aileron-pos-norm",  SGPropertyNode::FLOAT},
  {101, "surface-positions/right-aileron-pos-norm", SGPropertyNode::FLOAT},
  {102, "surface-positions/elevator-pos-norm",      SGPropertyNode::FLOAT},
  {103, "surface-positions/rudder-pos-norm",        SGPropertyNode::FLOAT},
  {104, "surface-positions/flap-pos-norm",          SGPropertyNode::FLOAT},
  {105, "surface-positions/speedbrake-pos-norm",    SGPropertyNode::FLOAT},
  {106, "gear/tailhook/position-norm",              SGPropertyNode::FLOAT},
  {107, "gear/launchbar/position-norm",             SGPropertyNode::FLOAT},
  {108, "gear/launchbar/state",                     SGPropertyNode::STRING},
  {109, "gear/launchbar/holdback-position-norm",    SGPropertyNode::FLOAT},
  {110, "canopy/position-norm",                     SGPropertyNode::FLOAT},
  {111, "surface-positions/wing-pos-norm",          SGPropertyNode::FLOAT},
  {112, "surface-positions/wing-fold-pos-norm",     SGPropertyNode::FLOAT},

  {200, "gear/gear[0]/compression-norm",           SGPropertyNode::FLOAT},
  {201, "gear/gear[0]/position-norm",              SGPropertyNode::FLOAT},
  {210, "gear/gear[1]/compression-norm",           SGPropertyNode::FLOAT},
  {211, "gear/gear[1]/position-norm",              SGPropertyNode::FLOAT},
  {220, "gear/gear[2]/compression-norm",           SGPropertyNode::FLOAT},
  {221, "gear/gear[2]/position-norm",              SGPropertyNode::FLOAT},
  {230, "gear/gear[3]/compression-norm",           SGPropertyNode::FLOAT},
  {231, "gear/gear[3]/position-norm",              SGPropertyNode::FLOAT},
  {240, "gear/gear[4]/compression-norm",           SGPropertyNode::FLOAT},
  {241, "gear/gear[4]/position-norm",              SGPropertyNode::FLOAT},

  {300, "engines/engine[0]/n1",  SGPropertyNode::FLOAT},
  {301, "engines/engine[0]/n2",  SGPropertyNode::FLOAT},
  {302, "engines/engine[0]/rpm", SGPropertyNode::FLOAT},
  {310, "engines/engine[1]/n1",  SGPropertyNode::FLOAT},
  {311, "engines/engine[1]/n2",  SGPropertyNode::FLOAT},
  {312, "engines/engine[1]/rpm", SGPropertyNode::FLOAT},
  {320, "engines/engine[2]/n1",  SGPropertyNode::FLOAT},
  {321, "engines/engine[2]/n2",  SGPropertyNode::FLOAT},
  {322, "engines/engine[2]/rpm", SGPropertyNode::FLOAT},
  {330, "engines/engine[3]/n1",  SGPropertyNode::FLOAT},
  {331, "engines/engine[3]/n2",  SGPropertyNode::FLOAT},
  {332, "engines/engine[3]/rpm", SGPropertyNode::FLOAT},
  {340, "engines/engine[4]/n1",  SGPropertyNode::FLOAT},
  {341, "engines/engine[4]/n2",  SGPropertyNode::FLOAT},
  {342, "engines/engine[4]/rpm", SGPropertyNode::FLOAT},
  {350, "engines/engine[5]/n1",  SGPropertyNode::FLOAT},
  {351, "engines/engine[5]/n2",  SGPropertyNode::FLOAT},
  {352, "engines/engine[5]/rpm", SGPropertyNode::FLOAT},
  {360, "engines/engine[6]/n1",  SGPropertyNode::FLOAT},
  {361, "engines/engine[6]/n2",  SGPropertyNode::FLOAT},
  {362, "engines/engine[6]/rpm", SGPropertyNode::FLOAT},
  {370, "engines/engine[7]/n1",  SGPropertyNode::FLOAT},
  {371, "engines/engine[7]/n2",  SGPropertyNode::FLOAT},
  {372, "engines/engine[7]/rpm", SGPropertyNode::FLOAT},
  {380, "engines/engine[8]/n1",  SGPropertyNode::FLOAT},
  {381, "engines/engine[8]/n2",  SGPropertyNode::FLOAT},
  {382, "engines/engine[8]/rpm", SGPropertyNode::FLOAT},
  {390, "engines/engine[9]/n1",  SGPropertyNode::FLOAT},
  {391, "engines/engine[9]/n2",  SGPropertyNode::FLOAT},
  {392, "engines/engine[9]/rpm", SGPropertyNode::FLOAT},

  {800, "rotors/main/rpm", SGPropertyNode::FLOAT},
  {801, "rotors/tail/rpm", SGPropertyNode::FLOAT},
  {810, "rotors/main/blade[0]/position-deg",  SGPropertyNode::FLOAT},
  {811, "rotors/main/blade[1]/position-deg",  SGPropertyNode::FLOAT},
  {812, "rotors/main/blade[2]/position-deg",  SGPropertyNode::FLOAT},
  {813, "rotors/main/blade[3]/position-deg",  SGPropertyNode::FLOAT},
  {820, "rotors/main/blade[0]/flap-deg",  SGPropertyNode::FLOAT},
  {821, "rotors/main/blade[1]/flap-deg",  SGPropertyNode::FLOAT},
  {822, "rotors/main/blade[2]/flap-deg",  SGPropertyNode::FLOAT},
  {823, "rotors/main/blade[3]/flap-deg",  SGPropertyNode::FLOAT},
  {830, "rotors/tail/blade[0]/position-deg",  SGPropertyNode::FLOAT},
  {831, "rotors/tail/blade[1]/position-deg",  SGPropertyNode::FLOAT},

  {900, "sim/hitches/aerotow/tow/length",                       SGPropertyNode::FLOAT},
  {901, "sim/hitches/aerotow/tow/elastic-constant",             SGPropertyNode::FLOAT},
  {902, "sim/hitches/aerotow/tow/weight-per-m-kg-m",            SGPropertyNode::FLOAT},
  {903, "sim/hitches/aerotow/tow/dist",                         SGPropertyNode::FLOAT},
  {904, "sim/hitches/aerotow/tow/connected-to-property-node",   SGPropertyNode::BOOL},
  {905, "sim/hitches/aerotow/tow/connected-to-ai-or-mp-callsign",   SGPropertyNode::STRING},
  {906, "sim/hitches/aerotow/tow/brake-force",                  SGPropertyNode::FLOAT},
  {907, "sim/hitches/aerotow/tow/end-force-x",                  SGPropertyNode::FLOAT},
  {908, "sim/hitches/aerotow/tow/end-force-y",                  SGPropertyNode::FLOAT},
  {909, "sim/hitches/aerotow/tow/end-force-z",                  SGPropertyNode::FLOAT},
  {930, "sim/hitches/aerotow/is-slave",                         SGPropertyNode::BOOL},
  {931, "sim/hitches/aerotow/speed-in-tow-direction",           SGPropertyNode::FLOAT},
  {932, "sim/hitches/aerotow/open",                             SGPropertyNode::BOOL},
  {933, "sim/hitches/aerotow/local-pos-x",                      SGPropertyNode::FLOAT},
  {934, "sim/hitches/aerotow/local-pos-y",                      SGPropertyNode::FLOAT},
  {935, "sim/hitches/aerotow/local-pos-z",                      SGPropertyNode::FLOAT},

  {1001, "controls/flight/slats",  SGPropertyNode::FLOAT},
  {1002, "controls/flight/speedbrake",  SGPropertyNode::FLOAT},
  {1003, "controls/flight/spoilers",  SGPropertyNode::FLOAT},
  {1004, "controls/gear/gear-down",  SGPropertyNode::FLOAT},
  {1005, "controls/lighting/nav-lights",  SGPropertyNode::FLOAT},
  {1006, "controls/armament/station[0]/jettison-all",  SGPropertyNode::BOOL},

  {1100, "sim/model/variant", SGPropertyNode::INT},
  {1101, "sim/model/livery/file", SGPropertyNode::STRING},

  {10001, "sim/multiplay/transmission-freq-hz",  SGPropertyNode::STRING},
  {10002, "sim/multiplay/chat",  SGPropertyNode::STRING},

  {10100, "sim/multiplay/generic/string[0]", SGPropertyNode::STRING},
  {10101, "sim/multiplay/generic/string[1]", SGPropertyNode::STRING},
  {10102, "sim/multiplay/generic/string[2]", SGPropertyNode::STRING},
  {10103, "sim/multiplay/generic/string[3]", SGPropertyNode::STRING},
  {10104, "sim/multiplay/generic/string[4]", SGPropertyNode::STRING},
  {10105, "sim/multiplay/generic/string[5]", SGPropertyNode::STRING},
  {10106, "sim/multiplay/generic/string[6]", SGPropertyNode::STRING},
  {10107, "sim/multiplay/generic/string[7]", SGPropertyNode::STRING},
  {10108, "sim/multiplay/generic/string[8]", SGPropertyNode::STRING},
  {10109, "sim/multiplay/generic/string[9]", SGPropertyNode::STRING},
  {10110, "sim/multiplay/generic/string[10]", SGPropertyNode::STRING},
  {10111, "sim/multiplay/generic/string[11]", SGPropertyNode::STRING},
  {10112, "sim/multiplay/generic/string[12]", SGPropertyNode::STRING},
  {10113, "sim/multiplay/generic/string[13]", SGPropertyNode::STRING},
  {10114, "sim/multiplay/generic/string[14]", SGPropertyNode::STRING},
  {10115, "sim/multiplay/generic/string[15]", SGPropertyNode::STRING},
  {10116, "sim/multiplay/generic/string[16]", SGPropertyNode::STRING},
  {10117, "sim/multiplay/generic/string[17]", SGPropertyNode::STRING},
  {10118, "sim/multiplay/generic/string[18]", SGPropertyNode::STRING},
  {10119, "sim/multiplay/generic/string[19]", SGPropertyNode::STRING},

  {10200, "sim/multiplay/generic/float[0]", SGPropertyNode::FLOAT},
  {10201, "sim/multiplay/generic/float[1]", SGPropertyNode::FLOAT},
  {10202, "sim/multiplay/generic/float[2]", SGPropertyNode::FLOAT},
  {10203, "sim/multiplay/generic/float[3]", SGPropertyNode::FLOAT},
  {10204, "sim/multiplay/generic/float[4]", SGPropertyNode::FLOAT},
  {10205, "sim/multiplay/generic/float[5]", SGPropertyNode::FLOAT},
  {10206, "sim/multiplay/generic/float[6]", SGPropertyNode::FLOAT},
  {10207, "sim/multiplay/generic/float[7]", SGPropertyNode::FLOAT},
  {10208, "sim/multiplay/generic/float[8]", SGPropertyNode::FLOAT},
  {10209, "sim/multiplay/generic/float[9]", SGPropertyNode::FLOAT},
  {10210, "sim/multiplay/generic/float[10]", SGPropertyNode::FLOAT},
  {10211, "sim/multiplay/generic/float[11]", SGPropertyNode::FLOAT},
  {10212, "sim/multiplay/generic/float[12]", SGPropertyNode::FLOAT},
  {10213, "sim/multiplay/generic/float[13]", SGPropertyNode::FLOAT},
  {10214, "sim/multiplay/generic/float[14]", SGPropertyNode::FLOAT},
  {10215, "sim/multiplay/generic/float[15]", SGPropertyNode::FLOAT},
  {10216, "sim/multiplay/generic/float[16]", SGPropertyNode::FLOAT},
  {10217, "sim/multiplay/generic/float[17]", SGPropertyNode::FLOAT},
  {10218, "sim/multiplay/generic/float[18]", SGPropertyNode::FLOAT},
  {10219, "sim/multiplay/generic/float[19]", SGPropertyNode::FLOAT},

  {10300, "sim/multiplay/generic/int[0]", SGPropertyNode::INT},
  {10301, "sim/multiplay/generic/int[1]", SGPropertyNode::INT},
  {10302, "sim/multiplay/generic/int[2]", SGPropertyNode::INT},
  {10303, "sim/multiplay/generic/int[3]", SGPropertyNode::INT},
  {10304, "sim/multiplay/generic/int[4]", SGPropertyNode::INT},
  {10305, "sim/multiplay/generic/int[5]", SGPropertyNode::INT},
  {10306, "sim/multiplay/generic/int[6]", SGPropertyNode::INT},
  {10307, "sim/multiplay/generic/int[7]", SGPropertyNode::INT},
  {10308, "sim/multiplay/generic/int[8]", SGPropertyNode::INT},
  {10309, "sim/multiplay/generic/int[9]", SGPropertyNode::INT},
  {10310, "sim/multiplay/generic/int[10]", SGPropertyNode::INT},
  {10311, "sim/multiplay/generic/int[11]", SGPropertyNode::INT},
  {10312, "sim/multiplay/generic/int[12]", SGPropertyNode::INT},
  {10313, "sim/multiplay/generic/int[13]", SGPropertyNode::INT},
  {10314, "sim/multiplay/generic/int[14]", SGPropertyNode::INT},
  {10315, "sim/multiplay/generic/int[15]", SGPropertyNode::INT},
  {10316, "sim/multiplay/generic/int[16]", SGPropertyNode::INT},
  {10317, "sim/multiplay/generic/int[17]", SGPropertyNode::INT},
  {10318, "sim/multiplay/generic/int[18]", SGPropertyNode::INT},
  {10319, "sim/multiplay/generic/int[19]", SGPropertyNode::INT},
};

const unsigned
FGMultiplayMgr::numProperties = (sizeof(FGMultiplayMgr::sIdPropertyList)
                                 / sizeof(FGMultiplayMgr::sIdPropertyList[0]));

// Look up a property ID using binary search.
namespace
{
  struct ComparePropertyId
  {
    bool operator()(const FGMultiplayMgr::IdPropertyList& lhs,
                    const FGMultiplayMgr::IdPropertyList& rhs)
    {
      return lhs.id < rhs.id;
    }
    bool operator()(const FGMultiplayMgr::IdPropertyList& lhs,
                    unsigned id)
    {
      return lhs.id < id;
    }
    bool operator()(unsigned id,
                    const FGMultiplayMgr::IdPropertyList& rhs)
    {
      return id < rhs.id;
    }
  };
    
}
const FGMultiplayMgr::IdPropertyList* FGMultiplayMgr::findProperty(unsigned id)
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
    const xdr_data_t* xdr = data;
    while (xdr < end) {
      unsigned id = XDR_decode_uint32(*xdr);
      const FGMultiplayMgr::IdPropertyList* plist
        = FGMultiplayMgr::findProperty(id);
    
      if (plist) {
        xdr++;
        // How we decode the remainder of the property depends on the type
        switch (plist->type) {
        case SGPropertyNode::INT:        
        case SGPropertyNode::BOOL:
        case SGPropertyNode::LONG:
          xdr++;
          break;
        case SGPropertyNode::FLOAT:
        case SGPropertyNode::DOUBLE:
          {
            float val = XDR_decode_float(*xdr);
            if (osg::isNaN(val))
              return false;
            xdr++;
            break;
          }
        case SGPropertyNode::STRING:
        case SGPropertyNode::UNSPECIFIED:
          {
            // String is complicated. It consists of
            // The length of the string
            // The string itself
            // Padding to the nearest 4-bytes.
            // XXX Yes, each byte is padded out to a word! Too late
            // to change...
            uint32_t length = XDR_decode_uint32(*xdr);
            xdr++;
            if ((length > 0) && (length < MAX_TEXT_SIZE)) {
              xdr += length;
              // Now handle the padding
              while ((length % 4) != 0)
                {
                  xdr++;
                  length++;
                  //cout << "0";
                }
            } else {
              // The string appears to be invalid; bail.
              return false;
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
//////////////////////////////////////////////////////////////////////
//
//  MultiplayMgr constructor
//
//////////////////////////////////////////////////////////////////////
FGMultiplayMgr::FGMultiplayMgr() 
{
  mSocket        = 0;
  mInitialised   = false;
  mHaveServer    = false;
} // FGMultiplayMgr::FGMultiplayMgr()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
//
//  MultiplayMgr destructor
//
//////////////////////////////////////////////////////////////////////
FGMultiplayMgr::~FGMultiplayMgr() 
{
  Close();
} // FGMultiplayMgr::~FGMultiplayMgr()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
//
//  Initialise object
//
//////////////////////////////////////////////////////////////////////
bool
FGMultiplayMgr::init (void) 
{
  //////////////////////////////////////////////////
  //  Initialise object if not already done
  //////////////////////////////////////////////////
  if (mInitialised) {
    SG_LOG(SG_NETWORK, SG_WARN, "FGMultiplayMgr::init - already initialised");
    return false;
  }
  //////////////////////////////////////////////////
  //  Set members from property values
  //////////////////////////////////////////////////
  short rxPort = fgGetInt("/sim/multiplay/rxport");
  string rxAddress = fgGetString("/sim/multiplay/rxhost");
  short txPort = fgGetInt("/sim/multiplay/txport");
  string txAddress = fgGetString("/sim/multiplay/txhost");
  mCallsign = fgGetString("/sim/multiplay/callsign");
  if (txPort > 0 && !txAddress.empty()) {
    mServer.set(txAddress.c_str(), txPort);
    if (strncmp (mServer.getHost(), "0.0.0.0", 8) == 0) {
      mHaveServer = false;
      SG_LOG(SG_NETWORK, SG_ALERT,
        "FGMultiplayMgr - could not resolve '"
        << txAddress << "', Multiplayermode disabled");
    } else {
      mHaveServer = true;
    }
    if (rxPort <= 0)
      rxPort = txPort;
  }
  if (rxPort <= 0) {
    SG_LOG(SG_NETWORK, SG_ALERT,
      "FGMultiplayMgr - No receiver port, Multiplayermode disabled");
    return (false);
  }
  if (mCallsign.empty())
    mCallsign = "JohnDoe"; // FIXME: use getpwuid
  SG_LOG(SG_NETWORK,SG_INFO,"FGMultiplayMgr::init-txaddress= "<<txAddress);
  SG_LOG(SG_NETWORK,SG_INFO,"FGMultiplayMgr::init-txport= "<<txPort );
  SG_LOG(SG_NETWORK,SG_INFO,"FGMultiplayMgr::init-rxaddress="<<rxAddress );
  SG_LOG(SG_NETWORK,SG_INFO,"FGMultiplayMgr::init-rxport= "<<rxPort);
  SG_LOG(SG_NETWORK,SG_INFO,"FGMultiplayMgr::init-callsign= "<<mCallsign);
  Close(); // Should Init be called twice, close Socket first
           // A memory leak was reported here by valgrind
  mSocket = new netSocket();
  if (!mSocket->open(false)) {
    SG_LOG( SG_NETWORK, SG_ALERT,
            "FGMultiplayMgr::init - Failed to create data socket" );
    return false;
  }
  mSocket->setBlocking(false);
  if (mSocket->bind(rxAddress.c_str(), rxPort) != 0) {
    perror("bind");
    SG_LOG( SG_NETWORK, SG_ALERT,
            "FGMultiplayMgr::Open - Failed to bind receive socket" );
    return false;
  }
  mInitialised = true;
  return true;
} // FGMultiplayMgr::init()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
//
//  Closes and deletes the local player object. Closes
//  and deletes the tx socket. Resets the object state to unitialised.
//
//////////////////////////////////////////////////////////////////////
void
FGMultiplayMgr::Close (void) 
{
  mMultiPlayerMap.clear();

  if (mSocket) {
    mSocket->close();
    delete mSocket;
    mSocket = 0;
  }
  mInitialised = false;
} // FGMultiplayMgr::Close(void)
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
//
//  Description: Sends the position data for the local position.
//
//////////////////////////////////////////////////////////////////////
void
FGMultiplayMgr::SendMyPosition(const FGExternalMotionData& motionInfo)
{
  if ((! mInitialised) || (! mHaveServer))
        return;
  if (! mHaveServer) {
    SG_LOG( SG_NETWORK, SG_ALERT, "FGMultiplayMgr::SendMyPosition - no server");
    return;
  }

  T_PositionMsg PosMsg;

  memset(&PosMsg, 0, sizeof(PosMsg));
  strncpy(PosMsg.Model, fgGetString("/sim/model/path"), MAX_MODEL_NAME_LEN);
  PosMsg.Model[MAX_MODEL_NAME_LEN - 1] = '\0';
  
  PosMsg.time = XDR_encode_double (motionInfo.time);
  PosMsg.lag = XDR_encode_double (motionInfo.lag);
  for (unsigned i = 0 ; i < 3; ++i)
    PosMsg.position[i] = XDR_encode_double (motionInfo.position(i));
  SGVec3f angleAxis;
  motionInfo.orientation.getAngleAxis(angleAxis);
  for (unsigned i = 0 ; i < 3; ++i)
    PosMsg.orientation[i] = XDR_encode_float (angleAxis(i));
  for (unsigned i = 0 ; i < 3; ++i)
    PosMsg.linearVel[i] = XDR_encode_float (motionInfo.linearVel(i));
  for (unsigned i = 0 ; i < 3; ++i)
    PosMsg.angularVel[i] = XDR_encode_float (motionInfo.angularVel(i));
  for (unsigned i = 0 ; i < 3; ++i)
    PosMsg.linearAccel[i] = XDR_encode_float (motionInfo.linearAccel(i));
  for (unsigned i = 0 ; i < 3; ++i)
    PosMsg.angularAccel[i] = XDR_encode_float (motionInfo.angularAccel(i));

  char Msg[MAX_PACKET_SIZE];
  memcpy(Msg + sizeof(T_MsgHdr), &PosMsg, sizeof(T_PositionMsg));
  
  char* ptr = Msg + sizeof(T_MsgHdr) + sizeof(T_PositionMsg);
  std::vector<FGPropertyData*>::const_iterator it;
  it = motionInfo.properties.begin();
  //cout << "OUTPUT PROPERTIES\n";
  while (it != motionInfo.properties.end()
         && ptr + 2 * sizeof(xdr_data_t) < (Msg + MAX_PACKET_SIZE)) {
             
    // First elements is the ID
    xdr_data_t xdr = XDR_encode_uint32((*it)->id);
    memcpy(ptr, &xdr, sizeof(xdr_data_t));
    ptr += sizeof(xdr_data_t);
    
    // The actual data representation depends on the type
    switch ((*it)->type) {
      case SGPropertyNode::INT:        
      case SGPropertyNode::BOOL:        
      case SGPropertyNode::LONG:        
        xdr = XDR_encode_uint32((*it)->int_value);
        memcpy(ptr, &xdr, sizeof(xdr_data_t));
        ptr += sizeof(xdr_data_t);
        //cout << "Prop:" << (*it)->id << " " << (*it)->type << " "<< (*it)->int_value << "\n";
        break;
      case SGPropertyNode::FLOAT:
      case SGPropertyNode::DOUBLE:
        xdr = XDR_encode_float((*it)->float_value);;
        memcpy(ptr, &xdr, sizeof(xdr_data_t));
        ptr += sizeof(xdr_data_t);
        //cout << "Prop:" << (*it)->id << " " << (*it)->type << " "<< (*it)->float_value << "\n";
        break;
      case SGPropertyNode::STRING:
      case SGPropertyNode::UNSPECIFIED:
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
            // XXX This should not be using 4 bytes per character!
            if (ptr + (1 + len + (4 - len % 4)) * sizeof (xdr_data_t)
                >= (Msg + MAX_PACKET_SIZE))
                goto escape;
            //cout << "String length unint32: " << len << "\n";
            xdr = XDR_encode_uint32(len);
            memcpy(ptr, &xdr, sizeof(xdr_data_t));
            ptr += sizeof(xdr_data_t);
            
            if (len != 0)
            {

              // Now the text itself
              // XXX This should not be using 4 bytes per character!
              int lcount = 0;
              while ((*lcharptr != '\0') && (lcount < MAX_TEXT_SIZE)) 
              {
                xdr = XDR_encode_int8(*lcharptr);
                memcpy(ptr, &xdr, sizeof(xdr_data_t));
                ptr += sizeof(xdr_data_t);
                lcharptr++;
                lcount++;          
              }

              //cout << "Prop:" << (*it)->id << " " << (*it)->type << " " << len << " " << (*it)->string_value;

              // Now pad if required
              while ((lcount % 4) != 0)
              {
                xdr = XDR_encode_int8(0);
                memcpy(ptr, &xdr, sizeof(xdr_data_t));
                ptr += sizeof(xdr_data_t);
                lcount++;          
                //cout << "0";
              }
              
              //cout << "\n";
            }
          }
          else
          {
            // Nothing to encode
            xdr = XDR_encode_uint32(0);
            memcpy(ptr, &xdr, sizeof(xdr_data_t));
            ptr += sizeof(xdr_data_t);
            //cout << "Prop:" << (*it)->id << " " << (*it)->type << " 0\n";
          }
           
        }
        break;
        
      default:
        //cout << " Unknown Type: " << (*it)->type << "\n";
        xdr = XDR_encode_float((*it)->float_value);;
        memcpy(ptr, &xdr, sizeof(xdr_data_t));
        ptr += sizeof(xdr_data_t);
        //cout << "Prop:" << (*it)->id << " " << (*it)->type << " "<< (*it)->float_value << "\n";
        break;
    }
        
    ++it;
  }
escape:
  
  T_MsgHdr MsgHdr;
  FillMsgHdr(&MsgHdr, POS_DATA_ID, ptr - Msg);
  memcpy(Msg, &MsgHdr, sizeof(T_MsgHdr));

  mSocket->sendto(Msg, ptr - Msg, 0, &mServer);
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
FGMultiplayMgr::Update(void) 
{
  if (!mInitialised)
    return;

  /// Just for expiry
  SGTimeStamp timestamper;
  timestamper.stamp();
  long stamp = timestamper.get_seconds();

  //////////////////////////////////////////////////
  //  Read the receive socket and process any data
  //////////////////////////////////////////////////
  int bytes;
  do {
    char Msg[MAX_PACKET_SIZE];
    //////////////////////////////////////////////////
    //  Although the recv call asks for 
    //  MAX_PACKET_SIZE of data, the number of bytes
    //  returned will only be that of the next
    //  packet waiting to be processed.
    //////////////////////////////////////////////////
    netAddress SenderAddress;
    bytes = mSocket->recvfrom(Msg, sizeof(Msg), 0, &SenderAddress);
    //////////////////////////////////////////////////
    //  no Data received
    //////////////////////////////////////////////////
    if (bytes <= 0) {
      if (errno != EAGAIN && errno != 0) // MSVC output "NoError" otherwise
        perror("FGMultiplayMgr::MP_ProcessData");
      break;
    }
    if (bytes <= sizeof(T_MsgHdr)) {
      SG_LOG( SG_NETWORK, SG_ALERT, "FGMultiplayMgr::MP_ProcessData - "
              << "received message with insufficient data" );
      break;
    }
    //////////////////////////////////////////////////
    //  Read header
    //////////////////////////////////////////////////
    T_MsgHdr* MsgHdr = (T_MsgHdr *)Msg;
    MsgHdr->Magic       = XDR_decode_uint32 (MsgHdr->Magic);
    MsgHdr->Version     = XDR_decode_uint32 (MsgHdr->Version);
    MsgHdr->MsgId       = XDR_decode_uint32 (MsgHdr->MsgId);
    MsgHdr->MsgLen      = XDR_decode_uint32 (MsgHdr->MsgLen);
    MsgHdr->ReplyPort   = XDR_decode_uint32 (MsgHdr->ReplyPort);
    if (MsgHdr->Magic != MSG_MAGIC) {
      SG_LOG( SG_NETWORK, SG_ALERT, "FGMultiplayMgr::MP_ProcessData - "
              << "message has invalid magic number!" );
      break;
    }
    if (MsgHdr->Version != PROTO_VER) {
      SG_LOG( SG_NETWORK, SG_ALERT, "FGMultiplayMgr::MP_ProcessData - "
              << "message has invalid protocoll number!" );
      break;
    }
    if (MsgHdr->MsgLen != bytes) {
      SG_LOG( SG_NETWORK, SG_ALERT, "FGMultiplayMgr::MP_ProcessData - "
              << "message has invalid length!" );
      break;
    }
    //////////////////////////////////////////////////
    //  Process messages
    //////////////////////////////////////////////////
    switch (MsgHdr->MsgId) {
    case CHAT_MSG_ID:
      ProcessChatMsg(Msg, SenderAddress);
      break;
    case POS_DATA_ID:
      ProcessPosMsg(Msg, SenderAddress, bytes, stamp);
      break;
    case UNUSABLE_POS_DATA_ID:
    case OLD_OLD_POS_DATA_ID:
    case OLD_PROP_MSG_ID:
    case OLD_POS_DATA_ID:
      break;
    default:
      SG_LOG( SG_NETWORK, SG_ALERT, "FGMultiplayMgr::MP_ProcessData - "
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

//////////////////////////////////////////////////////////////////////
//
//  handle a position message
//
//////////////////////////////////////////////////////////////////////
void
FGMultiplayMgr::ProcessPosMsg(const char *Msg, netAddress & SenderAddress,
                              unsigned len, long stamp)
{
  T_MsgHdr* MsgHdr = (T_MsgHdr *)Msg;
  if (MsgHdr->MsgLen < sizeof(T_MsgHdr) + sizeof(T_PositionMsg)) {
    SG_LOG( SG_NETWORK, SG_ALERT, "FGMultiplayMgr::MP_ProcessData - "
            << "Position message received with insufficient data" );
    return;
  }
  T_PositionMsg* PosMsg = (T_PositionMsg *)(Msg + sizeof(T_MsgHdr));
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


  //cout << "INPUT MESSAGE\n";
  xdr_data_t* xdr = (xdr_data_t*) 
                   (Msg + sizeof(T_MsgHdr) + sizeof(T_PositionMsg));
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
  if (PosMsg->pad != 0) {
    if (verifyProperties(&PosMsg->pad,
                         reinterpret_cast<const xdr_data_t*>(Msg + len)))
      xdr = &PosMsg->pad;
    else if (!verifyProperties(xdr,
                               reinterpret_cast<const xdr_data_t*>(Msg + len)))
      goto noprops;
  }
  while ((char*)xdr < Msg + len) {
    FGPropertyData* pData = new FGPropertyData;
    SGPropertyNode::Type type = SGPropertyNode::UNSPECIFIED;
    
    // First element is always the ID
    pData->id = XDR_decode_uint32(*xdr);
    //cout << pData->id << " ";
    xdr++;
    
    // Check the ID actually exists and get the type
    const IdPropertyList* plist = findProperty(pData->id);
    
    if (plist)
    {
      pData->type = plist->type;
      // How we decode the remainder of the property depends on the type
      switch (pData->type) {
        case SGPropertyNode::INT:        
        case SGPropertyNode::BOOL:
        case SGPropertyNode::LONG:        
          pData->int_value = XDR_decode_uint32(*xdr);
          xdr++;
          //cout << pData->int_value << "\n";
          break;
        case SGPropertyNode::FLOAT:
        case SGPropertyNode::DOUBLE:
          pData->float_value = XDR_decode_float(*xdr);
          xdr++;
          //cout << pData->float_value << "\n";
          break;
        case SGPropertyNode::STRING:
        case SGPropertyNode::UNSPECIFIED:
          {
            // String is complicated. It consists of
            // The length of the string
            // The string itself
            // Padding to the nearest 4-bytes.    
            uint32_t length = XDR_decode_uint32(*xdr);
            xdr++;
            //cout << length << " ";

            if ((length > 0) && (length < MAX_TEXT_SIZE))
            {
              pData->string_value = new char[length + 1];
              //cout << " String: ";

              for (int i = 0; i < length; i++)
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
            }
            else
            {
              pData->string_value = new char[1];
              pData->string_value[0] = '\0';
            }

            //cout << "\n";
          }
          break;

        default:
          pData->float_value = XDR_decode_float(*xdr);
          cerr << "Unknown Prop type " << pData->id << " " << pData->type << "\n";
          xdr++;
          break;
      }            

      motionInfo.properties.push_back(pData);
    }
    else
    {
      // We failed to find the property. We'll try the next packet immediately.
      SG_LOG(SG_NETWORK, SG_WARN, "FGMultiplayMgr::ProcessPosMsg - "
             << "found unknown property id" << pData->id); 
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
//  FIXME: display chat message withi flightgear
//
//////////////////////////////////////////////////////////////////////
void
FGMultiplayMgr::ProcessChatMsg(const char *Msg, netAddress& SenderAddress)
{
  T_MsgHdr* MsgHdr = (T_MsgHdr *)Msg;
  if (MsgHdr->MsgLen < sizeof(T_MsgHdr) + 1) {
    SG_LOG( SG_NETWORK, SG_ALERT, "FGMultiplayMgr::MP_ProcessData - "
            << "Chat message received with insufficient data" );
    return;
  }
  
  char *MsgBuf = new char[MsgHdr->MsgLen - sizeof(T_MsgHdr)];
  strncpy(MsgBuf, ((T_ChatMsg *)(Msg + sizeof(T_MsgHdr)))->Text,
          MsgHdr->MsgLen - sizeof(T_MsgHdr));
  MsgBuf[MsgHdr->MsgLen - sizeof(T_MsgHdr) - 1] = '\0';
  
  T_ChatMsg* ChatMsg = (T_ChatMsg *)(Msg + sizeof(T_MsgHdr));
  SG_LOG (SG_NETWORK, SG_ALERT, "Chat [" << MsgHdr->Callsign << "]"
           << " " << MsgBuf);

  delete [] MsgBuf;
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

  FGAIManager *aiMgr = (FGAIManager*)globals->get_subsystem("ai_model");
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
