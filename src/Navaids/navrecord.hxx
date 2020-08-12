// navrecord.hxx -- generic vor/dme/ndb class
//
// Written by Curtis Olson, started May 2004.
//
// Copyright (C) 2004  Curtis L. Olson - http://www.flightgear.org/~curt
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


#ifndef _FG_NAVRECORD_HXX
#define _FG_NAVRECORD_HXX

#include <iosfwd>

#include "navaids_fwd.hxx"
#include "positioned.hxx"
#include <Airports/airports_fwd.hxx>

#include <simgear/props/propsfwd.hxx>
#include <simgear/timing/timestamp.hxx>

const double FG_NAV_DEFAULT_RANGE = 50; // nm
const double FG_LOC_DEFAULT_RANGE = 18; // nm
const double FG_DME_DEFAULT_RANGE = 50; // nm
const double FG_TACAN_DEFAULT_RANGE = 250; // nm
const double FG_NAV_MAX_RANGE = 300;    // nm

class FGNavRecord : public FGPositioned
{

    int freq;
    int range;
    double multiuse;            // can be slaved variation of VOR
                                // (degrees) or localizer heading
                                // (degrees) or dme bias (nm)

    std::string mName;          // verbose name in nav database
    PositionedID mRunway;       // associated runway, if there is one
    PositionedID mColocated;    // Colocated DME at a navaid (ILS, VOR, TACAN, NDB)

  protected:
    mutable bool serviceable;   // for failure modeling

  public:
    static bool isType(FGPositioned::Type ty)
    { return (ty >= FGPositioned::NDB) && (ty <= FGPositioned::DME); }

    FGNavRecord( PositionedID aGuid,
                 Type type,
                 const std::string& ident,
                 const std::string& name,
                 const SGGeod& aPos,
                 int freq,
                 int range,
                 double multiuse,
                 PositionedID aRunway );

    inline double get_lon() const { return longitude(); } // degrees
    inline double get_lat() const { return latitude(); } // degrees
    inline double get_elev_ft() const { return elevation(); }

    inline int get_freq() const { return freq; }
    inline int get_range() const { return range; }
    inline double get_multiuse() const { return multiuse; }
    inline void set_multiuse( double m ) { multiuse = m; }
    inline const char *get_ident() const { return ident().c_str(); }

    inline bool get_serviceable() const { return serviceable; }
    inline const char *get_trans_ident() const { return get_ident(); }

    virtual const std::string& name() const
    { return mName; }

    /**
    * Retrieve the runway this navaid is associated with (for ILS/LOC/GS)
    */
    FGRunwayRef runway() const;

    /**
    * return the localizer width, in degrees
    * computation is based up ICAO stdandard width at the runway threshold
    * see implementation for further details.
    */
    double localizerWidth() const;

    /**
     * extract the glide slope angle, in degrees, from the multiuse field
     * Return 0.0 for non-GS navaids (including an ILS or LOC - you need
     * to call this on the paired GS record.
     */
    double glideSlopeAngleDeg() const;
    
    void bindToNode(SGPropertyNode* nd) const;
    void unbindFromNode(SGPropertyNode* nd) const;

    void setColocatedDME(PositionedID other);
    
    bool hasDME() const;

    PositionedID colocatedDME() const;

    bool isVORTAC() const;

    void updateFromXML(const SGGeod& geod, double heading);
};

/**
 * A mobile navaid, aka. a navaid which can change its position (eg. a mobile
 * TACAN)
 */
class FGMobileNavRecord:
  public FGNavRecord
{
  public:
      static bool isType(FGPositioned::Type ty)
      {
          return (ty == MOBILE_TACAN);
      }

    FGMobileNavRecord( PositionedID aGuid,
                       Type type,
                       const std::string& ident,
                       const std::string& name,
                       const SGGeod& aPos,
                       int freq,
                       int range,
                       double multiuse,
                       PositionedID aRunway );

    virtual const SGGeod& geod() const;
    virtual const SGVec3d& cart() const;

    void updateVehicle();
    void updatePos();

    void clearVehicle();

protected:
    SGTimeStamp _last_vehicle_update;
    SGPropertyNode_ptr _vehicle_node;
    double _initial_elevation_ft; // Elevation as given in the config file
};

class FGTACANRecord : public SGReferenced {

    std::string channel;
    int freq;

public:

     FGTACANRecord(void);
    inline ~FGTACANRecord(void) {}

    inline const std::string& get_channel() const { return channel; }
    inline int get_freq() const { return freq; }
    friend std::istream& operator>> ( std::istream&, FGTACANRecord& );
    };

#endif // _FG_NAVRECORD_HXX
