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

#include <string>

#include <simgear/constants.h>
#include <simgear/math/point3d.hxx>
#include <simgear/scene/model/placement.hxx>

#include <Main/fg_props.hxx>

SG_USING_STD(string);

class FGAIManager;

class FGAIBase {

public:

    FGAIBase();
    virtual ~FGAIBase();
    virtual void update(double dt);
    inline Point3D GetPos() { return(pos); }

    enum object_type { otNull, otAircraft, otShip, otBallistic,
                       otRocket, otStorm, otThermal };

    virtual bool init();
    virtual void bind();
    virtual void unbind();

    void setPath( const char* model );
    void setSpeed( double speed_KTAS );
    void setAltitude( double altitude_ft );
    void setHeading( double heading );
    void setLatitude( double latitude );
    void setLongitude( double longitude );
    void setBank( double bank );

    void setID( int ID );
    int  getID();
    void setDie( bool die );
    bool getDie();

protected:

    SGPropertyNode *props;
    FGAIManager* manager;

    // these describe the model's actual state
    Point3D pos;	// WGS84 lat & lon in degrees, elev above sea-level in meters
    double hdg;		// True heading in degrees
    double roll;	// degrees, left is negative
    double pitch;	// degrees, nose-down is negative
    double speed;       // knots true airspeed
    double altitude;    // meters above sea level
    double vs;          // vertical speed, feet per minute   

    // these describe the model's desired state
    double tgt_heading;  // target heading, degrees true
    double tgt_altitude; // target altitude, *feet* above sea level
    double tgt_speed;    // target speed, KTAS
    double tgt_roll;
    double tgt_pitch;
    double tgt_yaw;
    double tgt_vs;

    // these describe radar information for the user
    double bearing;      // true bearing from user to this model
    double elevation;    // elevation in degrees from user to this model
    double range;        // range from user to this model, nm
    double rdot;         // range rate, in knots
    double horiz_offset; // look left/right from user to me, deg
    double vert_offset;  // look up/down from user to me, deg
    double x_shift;      // value used by radar display instrument
    double y_shift;      // value used by radar display instrument
    double rotation;     // value used by radar display instrument


    string model_path;	   //Path to the 3D model
    ssgBranch * model;     //The 3D model object
    SGModelPlacement aip;
    bool delete_me;
    int id;
    bool invisible;
    bool no_roll;

    void Transform();

    string _type_str;
    object_type _otype;
    int index;

public:

    object_type getType();
    bool isa( object_type otype );

    static double _getVS_fps(void *p);
    static void _setVS_fps( double _vs, void *p );

    static double _getAltitude(void *p);
    static void _setAltitude( double _alt, void *p );

    static void _setLongitude( double longitude, void *p );
    static void _setLatitude ( double latitude, void *p );

    static double _getLongitude(void *p);
    static double _getLatitude (void *p);

    static double _getBearing(void *p);
    static double _getElevation(void *p);
    inline double _getRange() { return range; };
    static double _getRdot(void *p);
    static double _getH_offset(void *p);
    static double _getV_offset(void *p);
    static double _getX_shift(void *p);
    static double _getY_shift(void *p);
    static double _getRotation(void *p);

    static bool _isNight();

private:
    FGAIBase *self;
};


inline void FGAIBase::setPath( const char* model ) {
  model_path.append(model);
}

inline void FGAIBase::setSpeed( double speed_KTAS ) {
  speed = tgt_speed = speed_KTAS;
}

inline void FGAIBase::setHeading( double heading ) {
  hdg = tgt_heading = heading;
}

inline void FGAIBase::setAltitude( double altitude_ft ) {
    altitude = tgt_altitude = altitude_ft;
    pos.setelev(altitude * SG_FEET_TO_METER);
}

inline void FGAIBase::setBank( double bank ) {
  roll = tgt_roll = bank;
}

inline void FGAIBase::setLongitude( double longitude ) {
    pos.setlon( longitude );
}
inline void FGAIBase::setLatitude ( double latitude ) {
    pos.setlat( latitude );
}

inline void FGAIBase::setDie( bool die ) { delete_me = die; }
inline bool FGAIBase::getDie() { return delete_me; }

inline void FGAIBase::setID( int ID ) { id = ID; }
inline int  FGAIBase::getID() { return id; }

inline FGAIBase::object_type FGAIBase::getType() { return _otype; }

#endif  // _FG_AIBASE_HXX

