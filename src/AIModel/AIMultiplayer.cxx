// FGAIMultiplayer - FGAIBase-derived class creates an AI multiplayer aircraft
//
// Based on FGAIAircraft
// Written by David Culp, started October 2003.
// Also by Gregor Richards, started December 2005.
// With additions by Vivian Meazza, January 2006
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
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <simgear/math/point3d.hxx>
#include <Main/fg_props.hxx>
#include <Main/globals.hxx>
#include <Main/viewer.hxx>
#include <Scenery/scenery.hxx>
#include <Scenery/tilemgr.hxx>
#include <simgear/route/waypoint.hxx>
#include <string>
#include <math.h>
#include <time.h>
#ifdef _MSC_VER
#  include <float.h>
#  define finite _finite
#elif defined(__sun) || defined(sgi)
#  include <ieeefp.h>
#endif

SG_USING_STD(string);

#include "AIMultiplayer.hxx"
   static string tempReg;


FGAIMultiplayer::FGAIMultiplayer() : FGAIBase(otMultiplayer) {
   _time_node = fgGetNode("/sim/time/elapsed-sec", true);

   //initialise values
   speedN = speedE = rateH = rateR = rateP = 0.0;
   raw_hdg = hdg;
   raw_roll = roll;
   raw_pitch = pitch;
   raw_speed_east_deg_sec = speedE / ft_per_deg_lon;
   raw_speed_north_deg_sec = speedN / ft_per_deg_lat;
   raw_lon = damp_lon = pos.lon();
   raw_lat = damp_lat = pos.lat();
   raw_alt = damp_alt = pos.elev() / SG_FEET_TO_METER;

   //Exponentially weighted moving average time constants
   speed_north_deg_sec_constant = speed_east_deg_sec_constant = 0.1;
   alt_constant = 0.1;
   lat_constant = 0.05;
   lon_constant = 0.05;
   hdg_constant = 0.1;
   roll_constant = 0.1;
   pitch_constant = 0.1;
}


FGAIMultiplayer::~FGAIMultiplayer() {
}

bool FGAIMultiplayer::init() {
   return FGAIBase::init();
}

void FGAIMultiplayer::bind() {
    FGAIBase::bind();
    props->setStringValue("callsign", company.c_str());
    
    props->tie("controls/constants/roll",
                SGRawValuePointer<double>(&roll_constant));
    props->tie("controls/constants/pitch",
                SGRawValuePointer<double>(&pitch_constant));
    props->tie("controls/constants/hdg",
                SGRawValuePointer<double>(&hdg_constant));
    props->tie("controls/constants/altitude",
                SGRawValuePointer<double>(&alt_constant));
    /*props->tie("controls/constants/speedE",
                SGRawValuePointer<double>(&speed_east_deg_sec_constant));
    props->tie("controls/constants/speedN",
                SGRawValuePointer<double>(&speed_north_deg_sec_constant));*/
    props->tie("controls/constants/lat",
                SGRawValuePointer<double>(&lat_constant));
    props->tie("controls/constants/lon",
                SGRawValuePointer<double>(&lon_constant));
    props->tie("surface-positions/rudder-pos-norm",
                SGRawValuePointer<double>(&rudder));
    props->tie("surface-positions/elevator-pos-norm",
                SGRawValuePointer<double>(&elevator));
    props->tie("velocities/speedE-fps",
                SGRawValuePointer<double>(&speedE));


    props->setDoubleValue("sim/current-view/view-number", 1);
                   
}

void FGAIMultiplayer::setCompany(string comp) {
    company = comp;
    if (props)
        props->setStringValue("callsign", company.c_str());

}

void FGAIMultiplayer::unbind() {
    FGAIBase::unbind();
    
    props->untie("controls/constants/roll");
    props->untie("controls/constants/pitch");
    props->untie("controls/constants/hdg");
    props->untie("controls/constants/altitude");
    /*props->untie("controls/constants/speedE");
    props->untie("controls/constants/speedN");*/
    props->untie("controls/constants/lat");
    props->untie("controls/constants/lon");
    props->untie("surface-positions/rudder-pos-norm");
    props->untie("surface-positions/elevator-pos-norm");
    props->untie("velocities/speedE-fps");
}


void FGAIMultiplayer::update(double dt) {

    FGAIBase::update(dt);
    Run(dt);
    Transform();
}


void FGAIMultiplayer::Run(double dt) {
    
    // strangely, this is called with a dt of 0 quite often

    SG_LOG( SG_GENERAL, SG_DEBUG, "AIMultiplayer::main loop dt " << dt ) ;

    //if (dt == 0) return;
    
    //FGAIMultiplayer::dt = dt;

    //double rhr, rha; // "real" heading radius/angle

    // get the current sim elapsed time
    double time =_time_node->getDoubleValue();                        //secs
    
    dt = 0;
    
    //calulate the time difference, dt. Then use this value to extrapolate position and orientation    
    dt = time - time_stamp;
    
    SG_LOG(SG_GENERAL, SG_DEBUG, "time: "
        << time << " timestamp: " << time_stamp << " dt: " << dt << " freq Hz: " << 1/dt);
    
    // change heading/roll/pitch
    raw_hdg = hdg + rateH * dt;
    raw_roll = roll + rateR * dt;
    raw_pitch = pitch + rateP * dt;
        
    //apply lowpass filters
    hdg = (raw_hdg * hdg_constant) + (hdg * (1 - hdg_constant));
    roll = (raw_roll * roll_constant) + (roll * (1 - roll_constant));
    pitch = (raw_pitch * pitch_constant) + (pitch * (1 - pitch_constant));
    
    /*cout << "raw roll " << raw_roll <<" damp hdg " << roll << endl; 
    cout << "raw hdg" << raw_hdg <<" damp hdg " << hdg << endl;
    cout << "raw pitch " << raw_pitch <<" damp pitch " << pitch << endl;*/
    
    // sanitize HRP
    while (hdg < 0) hdg += 360;
    while (hdg >= 360) hdg -= 360;
    while (roll <= -180) roll += 360;
    while (roll > 180) roll -= 360;
    while (pitch <= -180) pitch += 360;
    while (pitch > 180) pitch -= 360;

    // calculate the new accelerations by change in the rate of heading
    /*rhr = sqrt(pow(accN,2) + pow(accE,2));
    rha = atan2(accN, accE);
    rha += rateH * dt;
    accN = sin(rha);
    accE = cos(rha);*/
    
    // calculate new speed by acceleration
    speedN += accN * dt;
    speedE += accE * dt;
    speedD += accD * dt;

    // convert speed to degrees per second
    // 1.686
    speed_north_deg_sec = speedN / ft_per_deg_lat;
    speed_east_deg_sec  = speedE / ft_per_deg_lon;
  
    // calculate new position by speed 
    raw_lat = pos.lat() + speed_north_deg_sec * dt;
    raw_lon = pos.lon() + speed_east_deg_sec * dt ;
    raw_alt = (pos.elev() / SG_FEET_TO_METER) +  (speedD * dt);
    
    //apply lowpass filters if the difference is small
    if ( fabs ( pos.lat() - raw_lat) < 0.001 ) {
        SG_LOG(SG_GENERAL, SG_DEBUG,"lat lowpass filter");
        damp_lat = (raw_lat * lat_constant) + (damp_lat * (1 - lat_constant));
    }else {
        // skip the filter
        SG_LOG(SG_GENERAL, SG_DEBUG,"lat high pass filter");
        damp_lat = raw_lat;
    }
     
    if ( fabs ( pos.lon() - raw_lon) < 0.001 ) {
        SG_LOG(SG_GENERAL, SG_DEBUG,"lon lowpass filter");
        damp_lon = (raw_lon * lon_constant) + (damp_lon * (1 - lon_constant)); 
    }else {
        // skip the filter
        SG_LOG(SG_GENERAL, SG_DEBUG,"lon high pass filter");
        damp_lon = raw_lon;
    }

    if ( fabs ( (pos.elev()/SG_FEET_TO_METER) - raw_alt) < 10 ) {
        SG_LOG(SG_GENERAL, SG_DEBUG,"alt lowpass filter");
        damp_alt = (raw_alt * alt_constant) + (damp_alt * (1 - alt_constant));
    }else {
        // skip the filter
        SG_LOG(SG_GENERAL, SG_DEBUG,"alt high pass filter");
        damp_alt = raw_alt;
    }
    
 //      cout << "raw lat" << raw_lat <<" damp lat " << damp_lat << endl;  
    //cout << "raw lon" << raw_lon <<" damp lon " << damp_lon << endl;
    //cout << "raw alt" << raw_alt <<" damp alt " << damp_alt << endl;
    
    // set new position
    pos.setlat( damp_lat );
    pos.setlon( damp_lon );
    pos.setelev( damp_alt * SG_FEET_TO_METER );

    //save the values
    time_stamp = time;
     
    //###########################//
    // do calculations for radar //
    //###########################//
    //double range_ft2 = UpdateRadar(manager);
}

void FGAIMultiplayer::setTimeStamp()
{    
    // this function sets the timestamp as the sim elapsed time 
    time_stamp = _time_node->getDoubleValue();            //secs
    
    //calculate the elapsed time since the latst update for display purposes only
    double elapsed_time = time_stamp - last_time_stamp;
    
    SG_LOG( SG_GENERAL, SG_DEBUG, " net input time s" << time_stamp << " freq Hz: " << 1/elapsed_time ) ;
        
    //save the values
    last_time_stamp = time_stamp;
}

