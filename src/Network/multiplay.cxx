// multiplay.cxx -- protocol object for multiplay in Flightgear
//
// Written by Diarmuid Tyson, started February 2003.
// diarmuid.tyson@airservicesaustralia.com
//
// With addtions by Vivian Meazza, January 2006
//
// Copyright (C) 2003  Airservices Australia
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
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <simgear/compiler.h>

#include STL_STRING

#include <iostream>
#include <map>
#include <string>

#include <simgear/debug/logstream.hxx>
#include <simgear/scene/model/placement.hxx>
#include <simgear/scene/model/placementtrans.hxx>

#include <Scenery/scenery.hxx>

#include "multiplay.hxx"

SG_USING_STD(string);


// These constants are provided so that the ident command can list file versions.
const char sFG_MULTIPLAY_BID[] = "$Id$";
const char sFG_MULTIPLAY_HID[] = FG_MULTIPLAY_HID;


/******************************************************************
* Name: FGMultiplay
* Description: Constructor.  Initialises the protocol and stores
* host and port information.
******************************************************************/
FGMultiplay::FGMultiplay (const string &dir, const int rate, const string &host, const int port) {

    last_time = 0;
    last_speedN = last_speedE = last_speedD = 0;
    calcaccN = calcaccE = calcaccD = 0;
    set_hz(rate);

    set_direction(dir);

    if (get_direction() == SG_IO_IN) {

        fgSetInt("/sim/multiplay/rxport", port);
        fgSetString("/sim/multiplay/rxhost", host.c_str());

    } else if (get_direction() == SG_IO_OUT) {

        fgSetInt("/sim/multiplay/txport", port);
        fgSetString("/sim/multiplay/txhost", host.c_str());

    }

    lat_n = fgGetNode("/position/latitude-deg", true);
    lon_n = fgGetNode("/position/longitude-deg", true);
    alt_n = fgGetNode("/position/altitude-ft", true);
    heading_n = fgGetNode("/orientation/heading-deg", true);
    roll_n = fgGetNode("/orientation/roll-deg", true);
    pitch_n = fgGetNode("/orientation/pitch-deg", true);
    speedN_n = fgGetNode("/velocities/speed-north-fps", true);
    speedE_n = fgGetNode("/velocities/speed-east-fps", true);
    speedD_n = fgGetNode("/velocities/speed-down-fps", true);
    left_aileron_n = fgGetNode("/surface-positions/left-aileron-pos-norm", true);
    right_aileron_n = fgGetNode("/surface-positions/right-aileron-pos-norm", true);
    elevator_n = fgGetNode("/surface-positions/elevator-pos-norm", true);
    rudder_n = fgGetNode("/surface-positions/rudder-pos-norm", true);
    /*rpms_n[0] = fgGetNode("/engines/engine/rpm", true);
    rpms_n[1] = fgGetNode("/engines/engine[1]/rpm", true);
    rpms_n[2] = fgGetNode("/engines/engine[2]/rpm", true);
    rpms_n[3] = fgGetNode("/engines/engine[3]/rpm", true);
    rpms_n[4] = fgGetNode("/engines/engine[4]/rpm", true);
    rpms_n[5] = fgGetNode("/engines/engine[5]/rpm", true);*/
    rateH_n = fgGetNode("/orientation/yaw-rate-degps", true);
    rateR_n = fgGetNode("/orientation/roll-rate-degps", true);
    rateP_n = fgGetNode("/orientation/pitch-rate-degps", true);

    SGPropertyNode_ptr n = fgGetNode("/controls/flight/slats",true);
    _node_cache *c = new _node_cache( n->getDoubleValue(), n );
    props["controls/flight/slats"] = c;

    n = fgGetNode("/controls/flight/speedbrake", true);
    c = new _node_cache( n->getDoubleValue(), n );
    props["controls/flight/speedbrake"] = c;

    n = fgGetNode("/controls/flight/spoilers", true);
    c = new _node_cache( n->getDoubleValue(), n );
    props["controls/flight/spoilers"] = c;

    n = fgGetNode("/controls/gear/gear-down", true);
    c = new _node_cache( n->getDoubleValue(), n );
    props["controls/gear/gear-down"] = c;

    n = fgGetNode("/controls/lighting/nav-lights", true);
    c = new _node_cache( n->getDoubleValue(), n );
    props["controls/lighting/nav-lights"] = c;

    n = fgGetNode("/surface-positions/flap-pos-norm", true);
    c = new _node_cache( n->getDoubleValue(), n );
    props["surface-positions/flap-pos-norm"] = c;

    n = fgGetNode("/surface-positions/speedbrake-pos-norm", true);
    c = new _node_cache( n->getDoubleValue(), n );
    props["surface-positions/speedbrake-pos-norm"] = c;

    for (int i = 0; i < 6; i++)
    {
        char *s = new char[32];

        snprintf(s, 32, "engines/engine[%i]/n1", i);
        n = fgGetNode(s, true);
        c = new _node_cache( n->getDoubleValue(), n );
        props["s"] = c;

        snprintf(s, 32, "engines/engine[%i]/n2", i);
        n = fgGetNode(s, true);
        c = new _node_cache( n->getDoubleValue(), n );
        props[s] = c;

        snprintf(s, 32, "engines/engine[%i]/rpm", i);
        n = fgGetNode(s, true);
        c = new _node_cache( n->getDoubleValue(), n );
        props[s] = c;

        delete [] s;
    }

    for (int j = 0; j < 5; j++)
    {
        char *s = new char[32];
    
        snprintf(s, 32, "gear/gear[%i]/compression-norm", j);
        n = fgGetNode(s, true);
        c = new _node_cache( n->getDoubleValue(), n );
        props["s"] = c;

        snprintf(s, 32, "gear/gear[%i]/position-norm", j);
        n = fgGetNode(s, true);
        c = new _node_cache( n->getDoubleValue(), n );
        props[s] = c;
#if 0
        snprintf(s, 32, "gear/gear[%i]/rollspeed-ms", j);
        n = fgGetNode(s, true);
        c = new _node_cache( n->getDoubleValue(), n );
        props[s] = c;
#endif
        delete [] s;
    }

    n = fgGetNode("gear/tailhook/position-norm", true);
    c = new _node_cache( n->getDoubleValue(), n );
    props["gear/tailhook/position-norm"] = c;
}


/******************************************************************
* Name: ~FGMultiplay
* Description: Destructor.
******************************************************************/
FGMultiplay::~FGMultiplay () {
    props.clear();
}


/******************************************************************
* Name: open
* Description: Enables the protocol.
******************************************************************/
bool FGMultiplay::open() {

    if ( is_enabled() ) {
    SG_LOG( SG_IO, SG_ALERT, "This shouldn't happen, but the channel "
        << "is already in use, ignoring" );
    return false;
    }

    set_enabled(true);

    return is_enabled();
}


/******************************************************************
* Name: process
* Description: Prompts the multiplayer mgr to either send
* or receive data over the network
******************************************************************/
bool FGMultiplay::process() {

  if (get_direction() == SG_IO_IN) {

    globals->get_multiplayer_mgr()->ProcessData();

  } else if (get_direction() == SG_IO_OUT) {

    double accN, accE, accD;
    string fdm = fgGetString("/sim/flight-model");

    if(fdm == "jsb"){
        calcAcc(speedN_n->getDoubleValue(),
                speedE_n->getDoubleValue(),
                speedD_n->getDoubleValue());
        accN = calcaccN;
        accE = calcaccE;
        accD = calcaccD;
    }else{
        SG_LOG(SG_GENERAL, SG_DEBUG," not doing acc calc" << fdm);
        accN = fgGetDouble("/accelerations/ned/north-accel-fps_sec");
        accE = fgGetDouble("/accelerations/ned/east-accel-fps_sec");
        accD = fgGetDouble("/accelerations/ned/down-accel-fps_sec");
    }

    globals->get_multiplayer_mgr()->SendMyPosition(
                                     lat_n->getDoubleValue(),
                                     lon_n->getDoubleValue(),
                                     alt_n->getDoubleValue(),
                                     heading_n->getDoubleValue(),
                                     roll_n->getDoubleValue(),
                                     pitch_n->getDoubleValue(),
                                     speedN_n->getDoubleValue(),
                                     speedE_n->getDoubleValue(),
                                     speedD_n->getDoubleValue(),
                                     left_aileron_n->getDoubleValue(),
                                     right_aileron_n->getDoubleValue(),
                                     elevator_n->getDoubleValue(),
                                     rudder_n->getDoubleValue(),
                                     rateH_n->getDoubleValue(),
                                     rateR_n->getDoubleValue(),
                                     rateP_n->getDoubleValue(),
                                     accN, accE, accD);
    
    // check for changes
    for (propit = props.begin(); propit != props.end(); propit++)
    {
      double val = propit->second->val;
      double curr_val = propit->second->node->getDoubleValue();
      if (curr_val < val * 0.99 || curr_val > val * 1.01 )
      {
        SGPropertyNode::Type type = propit->second->node->getType();
        propit->second->val = val = curr_val;
        globals->get_multiplayer_mgr()->SendPropMessage(propit->first, type, val);
        //cout << "Prop " << propit->first <<" type " << type << " val " << val << endl;
      } else {
          //  cout << "no change" << endl;
      }
    }

    // send all properties when necessary
    // FGMultiplayMgr::getSendAllProps();
    bool send_all = globals->get_multiplayer_mgr()->getSendAllProps();
    //cout << "send_all in " << send;
    if (send_all){
        SG_LOG( SG_NETWORK, SG_ALERT,
          "FGMultiplay::sending ALL property messages" );
          for (propit = props.begin(); propit != props.end(); propit++) {
              SGPropertyNode::Type type = propit->second->node->getType();
              double val = propit->second->val;
              globals->get_multiplayer_mgr()->SendPropMessage(propit->first, type, val);
          }
        send_all = false;
        globals->get_multiplayer_mgr()->setSendAllProps(send_all);
    }
    //cout << " send_all out " << s << endl;
  }

    return true;
}


/******************************************************************
* Name: close
* Description:  Closes the multiplayer mgrs to stop any further
* network processing
******************************************************************/
bool FGMultiplay::close() {

  if (get_direction() == SG_IO_IN) {

    globals->get_multiplayer_mgr()->Close();

  } else if (get_direction() == SG_IO_OUT) {

//    globals->get_multiplayer_mgr()->Close();

  }

    return true;
}

/******************************************************************
 * Name: CalcAcc
 * Description: Calculate accelerations given speedN, speedE, speedD
 ******************************************************************/
void FGMultiplay::calcAcc(double speedN, double speedE, double speedD)
{
    double time, dt;                        //secs
    /*double accN, accE, accD;    */            //fps2

    dt = 0;

    time = fgGetDouble("/sim/time/elapsed-sec");

    dt = time-last_time;
    
    SG_LOG(SG_GENERAL, SG_DEBUG," doing acc calc"
    <<"time: "<< time << " last " << last_time << " dt " << dt );
                                
    //calculate the accelerations  
    calcaccN = (speedN - last_speedN)/dt;
    calcaccE = (speedE - last_speedE)/dt;
    calcaccD = (speedD - last_speedD)/dt;

    //set the properties
    /*fgSetDouble("/accelerations/ned/north-accel-fps_sec",accN);
    fgSetDouble("/accelerations/ned/east-accel-fps_sec",accE);
    fgSetDouble("/accelerations/ned/down-accel-fps_sec",accN);*/
    
    //save the values
    last_time = time;
    last_speedN = speedN;
    last_speedE = speedE;
    last_speedD = speedD;
    
}// end calcAcc

