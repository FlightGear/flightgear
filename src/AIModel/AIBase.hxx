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

#include <simgear/constants.h>
#include <simgear/math/point3d.hxx>
#include <simgear/scene/model/placement.hxx>
#include <string>

SG_USING_STD(string);

class FGAIBase {

public:

    FGAIBase();
    virtual ~FGAIBase();
    virtual void update(double dt);
    inline Point3D GetPos() { return(pos); }

    virtual bool init();
    virtual void bind();
    virtual void unbind();

    void setPath( const char* model );
    void setSpeed( double speed_KTAS );
    void setAltitude( double altitude_ft );
    void setHeading( double heading );
    void setLatitude( double latitude );
    void setLongitude( double longitude );

    void setDie( bool die );
    bool getDie();

protected:

    SGPropertyNode *props;

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

    static FGAIBase *_self;
    const char *_type_str;

private:

    static void _setLongitude( double longitude );
    static void _setLatitude ( double latitude );
    static double _getLongitude();
    static double _getLatitude ();

};


inline void FGAIBase::setPath( const char* model ) {
  model_path.append(model);
}

inline void FGAIBase::setSpeed( double speed_KTAS ) {
  speed = tgt_speed = speed_KTAS;
}

inline void FGAIBase::setAltitude( double altitude_ft ) {
  altitude = tgt_altitude = altitude_ft;
  pos.setelev(altitude * SG_FEET_TO_METER);
}

inline void FGAIBase::setHeading( double heading ) {
  hdg = tgt_heading = heading;
}

inline void FGAIBase::setLongitude( double longitude ) {
    pos.setlon( longitude );
}

inline void FGAIBase::setLatitude ( double latitude ) {
    pos.setlat( latitude );
}

inline void FGAIBase::setDie( bool die ) { delete_me = die; }
inline bool FGAIBase::getDie() { return delete_me; }

#endif  // _FG_AIBASE_HXX

