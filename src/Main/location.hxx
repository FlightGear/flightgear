// location.hxx -- class for determining model location in the flightgear world.
//
// Written by Jim Wilson, David Megginson, started April 2002.
//
// Copyright (C) 2002  Jim Wilson, David Megginson
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
// $Id$


#ifndef _LOCATION_HXX
#define _LOCATION_HXX


#ifndef __cplusplus                                                          
# error This library requires C++
#endif                                   

#include <simgear/compiler.h>
#include <simgear/constants.h>

#include <plib/sg.h>		// plib include

#include "fgfs.hxx"


// Define a structure containing view information
class FGLocation
{

public:

    // Constructor
    FGLocation( void );

    // Destructor
    virtual ~FGLocation( void );

    //////////////////////////////////////////////////////////////////////
    // Part 1: standard FGSubsystem implementation.
    //////////////////////////////////////////////////////////////////////

    virtual void init ();
    virtual void bind ();
    virtual void unbind ();
    void update (int dt);


    //////////////////////////////////////////////////////////////////////
    // Part 2: user settings.
    //////////////////////////////////////////////////////////////////////

    // Geodetic position of model...
    virtual double getLongitude_deg () const { return _lon_deg; }
    virtual double getLatitude_deg () const { return _lat_deg; }
    virtual double getAltitudeASL_ft () const { return _alt_ft; }
    virtual void setPosition (double lon_deg, double lat_deg, double alt_ft);


    // Reference orientation rotations...
    //   These are rotations that represent the plane attitude effect on
    //   the view (in Pilot view).  IE The view frustrum rotates as the plane
    //   turns, pitches, and rolls.
    //   In model view (lookat/chaseview) these end up changing the angle that
    //   the eye is looking at the ojbect (ie the model).
    //   FIXME: the FGModel class should have its own version of these so that
    //   it can generate it's own model rotations.
    virtual double getRoll_deg () const { return _roll_deg; }
    virtual double getPitch_deg () const {return _pitch_deg; }
    virtual double getHeading_deg () const {return _heading_deg; }
    virtual void setOrientation (double roll_deg, double pitch_deg, double heading_deg);


    //////////////////////////////////////////////////////////////////////
    // Part 3: output vectors and matrices in FlightGear coordinates.
    //////////////////////////////////////////////////////////////////////

    // Vectors and positions...

    // Get zero view_pos
    virtual float * get_view_pos() { return _relative_view_pos; }
    // Get the absolute view position in fgfs coordinates.
    virtual double * get_absolute_view_pos ();
    // Get zero elev
    virtual float * get_zero_elev() { return _zero_elev_view_pos; }
    // Get world up vector
    virtual float *get_world_up() { return _world_up; }
    // Get the relative (to scenery center) view position in fgfs coordinates.
    virtual float * getRelativeViewPos ();
    // Get the absolute zero-elevation view position in fgfs coordinates.
    virtual float * getZeroElevViewPos ();
    // Get surface east vector
    virtual float *get_surface_east() {	return _surface_east; }
    // Get surface south vector
    virtual float *get_surface_south() { return _surface_south; }

    // Matrices...
    virtual const sgMat4 &getTransformMatrix() { if ( _dirty ) { recalc(); }	return TRANS; }
    virtual const sgMat4 &getCachedTransformMatrix() { return TRANS; }
    virtual const sgMat4 &getUpMatrix()  { if ( _dirty ) { recalc(); }	return UP; }
    virtual const sgMat4 &getCachedUpMatrix()  { return UP; }


private:

    //////////////////////////////////////////////////////////////////
    // private data                                                 //
    //////////////////////////////////////////////////////////////////

    // flag forcing a recalc of derived view parameters
    bool _dirty;

    mutable sgdVec3 _absolute_view_pos;
    mutable sgVec3 _relative_view_pos;
    mutable sgVec3 _zero_elev_view_pos;

    double _lon_deg;
    double _lat_deg;
    double _alt_ft;

    double _roll_deg;
    double _pitch_deg;
    double _heading_deg;

    // surface vector heading south
    sgVec3 _surface_south;

    // surface vector heading east (used to unambiguously align sky
    // with sun)
    sgVec3 _surface_east;

    // world up vector (normal to the plane tangent to the earth's
    // surface at the spot we are directly above
    sgVec3 _world_up;

    // sg versions of our friendly matrices
    sgMat4 TRANS, UP;

    //////////////////////////////////////////////////////////////////
    // private functions                                            //
    //////////////////////////////////////////////////////////////////

    void recalc ();
    void recalcPosition (double lon_deg, double lat_deg, double alt_ft) const;

    inline void set_dirty() { _dirty = true; }
    inline void set_clean() { _dirty = false; }

};


#endif // _LOCATION_HXX













