// FGAIBase - abstract base class for AI objects
// Written by David Culp, started Nov 2003, based on
// David Luff's FGAIEntity class.
// - davidculp2@comcast.net
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

#ifndef _FG_AIBASE_HXX
#define _FG_AIBASE_HXX

#include <simgear/math/point3d.hxx>
#include <simgear/scene/model/placement.hxx>
#include <string>

SG_USING_STD(string);

class FGAIBase {

public:

    virtual ~FGAIBase();
    virtual void update(double dt);
    inline Point3D GetPos() { return(pos); }
    virtual bool init();

    void setPath( const char* model );
    void setSpeed( double speed_KTAS );
    void setAltitude( double altitude_ft );
    void setLongitude( double longitude );
    void setLatitude( double latitude );
    void setHeading( double heading );
    void setDie( bool die );
    inline bool getDie() { return delete_me; }

protected:

    Point3D pos;	// WGS84 lat & lon in degrees, elev above sea-level in meters
    double hdg;		// True heading in degrees
    double roll;	// degrees, left is negative
    double pitch;	// degrees, nose-down is negative
    double speed;       // knots true airspeed
    double altitude;    // meters above sea level
    double vs;           // vertical speed, feet per minute   

    double tgt_heading;  // target heading, degrees true
    double tgt_altitude; // target altitude, *feet* above sea level
    double tgt_speed;    // target speed, KTAS
    double tgt_roll;
    double tgt_pitch;
    double tgt_yaw;
    double tgt_vs;


    string model_path;	   //Path to the 3D model
    SGModelPlacement aip;
    bool delete_me;

    void Transform();
};

#endif  // _FG_AIBASE_HXX

