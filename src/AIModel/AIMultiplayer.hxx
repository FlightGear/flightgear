// FGAIMultiplayer - AIBase derived class creates an AI multiplayer aircraft
//
// Written by David Culp, started October 2003.
// With additions by Vivian Meazza
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
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

#ifndef _FG_AIMultiplayer_HXX
#define _FG_AIMultiplayer_HXX

#include "AIManager.hxx"
#include "AIBase.hxx"

//#include <Traffic/SchedFlight.hxx>
//#include <Traffic/Schedule.hxx>

#include <string>
SG_USING_STD(string);


class FGAIMultiplayer : public FGAIBase {

    public:
    FGAIMultiplayer(FGAIManager* mgr);
    ~FGAIMultiplayer();
    
    bool init();
    virtual void bind();
    virtual void unbind();
    void update(double dt);
    

    void setSpeedN(double sn);
    void setSpeedE(double se);
    void setSpeedD(double sd);
    void setAccN(double an);
    void setAccE(double ae);
    void setAccD(double ad);
    void setRateH(double rh);
    void setRateR(double rr);
    void setRateP(double rp);
    void setRudder( double r ) { rudder = r;}   
    void setElevator( double e ) { elevator = e; }
    void setLeftAileron( double la ) { left_aileron = la; } 
    void setRightAileron( double ra ) { right_aileron = ra; }   
    void setTimeStamp();
    
    inline SGPropertyNode *FGAIMultiplayer::getProps() { return props; }
        
    void setAcType(string ac) { acType = ac; };
    void setCompany(string comp);

    double dt; 
    double speedN, speedE, speedD;
    double rateH, rateR, rateP;
    double raw_hdg , raw_roll , raw_pitch ;
    double raw_speed_north_deg_sec, raw_speed_east_deg_sec;
    double raw_lat, damp_lat, lat_constant;
    double raw_lon, damp_lon, lon_constant;
    double raw_alt, damp_alt, alt_constant;
    double hdg_constant, roll_constant, pitch_constant;
    double speed_north_deg_sec_constant, speed_east_deg_sec_constant;
    double speed_north_deg_sec, speed_east_deg_sec;
    double accN, accE, accD;
    double rudder, elevator, left_aileron, right_aileron;
    double time_stamp, last_time_stamp;

    SGPropertyNode_ptr _time_node;
        
    void Run(double dt);
    inline double sign(double x) { return (x < 0.0) ? -1.0 : 1.0; }
  
    string acType;
    string company;
};

inline void FGAIMultiplayer::setSpeedN(double sn) { speedN = sn; }
inline void FGAIMultiplayer::setSpeedE(double se) { speedE = se; }
inline void FGAIMultiplayer::setSpeedD(double sd) { speedD = sd; }
inline void FGAIMultiplayer::setAccN(double an) { accN = an; }
inline void FGAIMultiplayer::setAccE(double ae) { accE = ae; }
inline void FGAIMultiplayer::setAccD(double ad) { accD = ad; }
inline void FGAIMultiplayer::setRateH(double rh) { rateH = rh; }
inline void FGAIMultiplayer::setRateR(double rr) { rateR = rr; }
inline void FGAIMultiplayer::setRateP(double rp) { rateP = rp; }

#endif  // _FG_AIMultiplayer_HXX
