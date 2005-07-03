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
#include <list>

#include <simgear/constants.h>
#include <simgear/math/point3d.hxx>
#include <simgear/scene/model/placement.hxx>
#include <simgear/misc/sg_path.hxx>

#include <Main/fg_props.hxx>

SG_USING_STD(string);
SG_USING_STD(list);

class FGAIManager;
class FGAIFlightPlan;

struct ParkPosition {
  ParkPosition(const ParkPosition& pp)
    : name(pp.name), offset(pp.offset), heading_deg(pp.heading_deg)
  {}
  ParkPosition(const string& n, const Point3D& off = Point3D(), double heading = 0)
    : name(n), offset(off), heading_deg(heading)
  {}
  string name;
  Point3D offset;
  double heading_deg;
};

typedef struct {
   string callsign;

   // can be aircraft, ship, storm, thermal, static or ballistic
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
   double strength;           // used by thermal and storm objects
   double diameter;           // used by thermal and storm objects
   double height_msl;         // used by thermal and storm objects
   double eda;                // used by ballistic objects
   double life;               // life span in seconds
   double buoyancy;           // acceleration in ft per sec2
   double wind_from_east;     // in feet per second
   double wind_from_north;    // in feet per second
   double cd;                 // coefficient of drag
   bool wind;                 // if true, model reacts to parent wind
   double mass;               // in slugs
   bool aero_stabilised;      // if true, ballistic object aligns with trajectory
   list<string> solid_objects;    // List of solid object names
   list<string> wire_objects;     // List of wire object names
   list<string> catapult_objects; // List of catapult object names
   list<ParkPosition> ppositions; // List of positions on a carrier where an aircraft can start.
   Point3D flols_offset;      // used by carrier objects, in meters
   double radius;             // used by ship objects, in feet
   string name;               // used by carrier objects
   string pennant_number;     // used by carrier objects
   string acType;             // used by aircraft objects
   string company;            // used by aircraft objects
} FGAIModelEntity;


class FGAIBase {

public:

    FGAIBase();
    virtual ~FGAIBase();
    virtual void update(double dt);
    inline Point3D GetPos() { return(pos); }

    enum object_type { otNull = 0, otAircraft, otShip, otCarrier, otBallistic,
                       otRocket, otStorm, otThermal, otStatic,
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
    void setRadius ( double radius );
    void setXoffset( double x_offset );
    void setYoffset( double y_offset );
    void setZoffset( double z_offset );


    void* getID();
    void setDie( bool die );
    bool getDie();

    Point3D getCartPosAt(const Point3D& off) const;
    Point3D getGeocPosAt(const Point3D& off) const;

protected:

    SGPropertyNode_ptr props;
    FGAIManager* manager;

    // these describe the model's actual state
    Point3D pos;	// WGS84 lat & lon in degrees, elev above sea-level in meters
    double hdg;		// True heading in degrees
    double roll;	// degrees, left is negative
    double pitch;	// degrees, nose-down is negative
    double speed;       // knots true airspeed
    double altitude;    // meters above sea level
    double vs;          // vertical speed, feet per minute  
    double turn_radius_ft; // turn radius ft at 15 kts rudder angle 15 degrees

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
    void CalculateMach();
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
  ssgBranch * load3DModel(const string& fg_root, 
			  const string &path,
			  SGPropertyNode *prop_root, 
			  double sim_time_sec);

    static bool _isNight();
};


inline void FGAIBase::setPath( const char* model ) {
  model_path.append(model);
}

inline void FGAIBase::setSpeed( double speed_KTAS ) {
  speed = tgt_speed = speed_KTAS;
}

inline void FGAIBase::setRadius( double radius ) {
  turn_radius_ft = radius;
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

