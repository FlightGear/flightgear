// FGAIBase.hxx - abstract base class for AI objects
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
class FGAIFlightPlan;


typedef struct {
   string callsign;

   // can be aircraft, ship, storm, thermal or ballistic
   string m_type;
   string m_class;
   string path;
   string flightplan;

   FGAIFlightPlan *fp;

   double repeat;             // in seconds
   double latitude;           // used if no flightplan defined
   double longitude;          // used if no flightplan defined
   double altitude;           // used if no flightplan defined
   double speed;              // used if no flightplan defined
   double heading;            // used if no flightplan defined
   double roll;               // used if no flightplan defined
   double azimuth;            // used by ballistic objects
   double elevation;          // used by ballistic objects
   double rudder;             // used by ship objects
   double strength;           // used by thermal objects
   double diameter;           // used by thermal objects
   double eda;                // used by ballistic objects
   double life;               // life span in seconds
   double buoyancy;           // acceleration in ft per sec2
   double wind_from_east;     // in feet per second
   double wind_from_north;    // in feet per second
   double cd;                 // coefficient of drag
   bool wind;                 // if true, model reacts to parent wind
   double mass;               // in slugs
} FGAIModelEntity;


class FGAIBase {

public:

    FGAIBase();
    virtual ~FGAIBase();
    virtual void update(double dt);
    inline Point3D GetPos() { return(pos); }

    enum object_type { otNull = 0, otAircraft, otShip, otBallistic,
                       otRocket, otStorm, otThermal, 
                       MAX_OBJECTS };	// Needs to be last!!!

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

    void* getID();
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

    double ft_per_deg_lon;
    double ft_per_deg_lat;

    // these describe the model's desired state
    double tgt_heading;  // target heading, degrees true
    double tgt_altitude; // target altitude, *feet* above sea level
    double tgt_speed;    // target speed, KTAS
    double tgt_roll;
    double tgt_pitch;
    double tgt_yaw;
    double tgt_vs;

    // these describe radar information for the user
    bool in_range;       // true if in range of the radar, otherwise false
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
    bool invisible;
    bool no_roll;
    double life;
    FGAIFlightPlan *fp;

    void Transform();

    double UpdateRadar(FGAIManager* manager);

    string _type_str;
    object_type _otype;
    int index;

public:

    object_type getType();
    bool isa( object_type otype );

    double _getVS_fps() const;
    void _setVS_fps( double _vs );

    double _getAltitude() const;
    void _setAltitude( double _alt );

    void _setLongitude( double longitude );
    void _setLatitude ( double latitude );

    double _getLongitude() const;
    double _getLatitude () const;

    double _getBearing() const;
    double _getElevation() const;
    double _getRdot() const;
    double _getH_offset() const;
    double _getV_offset() const;
    double _getX_shift() const;
    double _getY_shift() const;
    double _getRotation() const;

    double rho;
    double T;                             // temperature, degs farenheit
    double p;                             // pressure lbs/sq ft
	double a;                             // speed of sound at altitude (ft/s)
	double Mach;                          // Mach number
	
    static const double e;
    static const double lbs_to_slugs;

    int _getID() const;

    inline double _getRange() { return range; };

    static bool _isNight();
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
  no_roll = false;
}

inline void FGAIBase::setLongitude( double longitude ) {
    pos.setlon( longitude );
}
inline void FGAIBase::setLatitude ( double latitude ) {
    pos.setlat( latitude );
}

inline void FGAIBase::setDie( bool die ) { delete_me = die; }
inline bool FGAIBase::getDie() { return delete_me; }

inline FGAIBase::object_type FGAIBase::getType() { return _otype; }

inline void* FGAIBase::getID() { return this; }

#endif	// _FG_AIBASE_HXX

