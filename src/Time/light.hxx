// light.hxx -- lighting routines
//
// Written by Curtis Olson, started April 1998.
//
// Copyright (C) 1998  Curtis L. Olson  - http://www.flightgear.org/~curt
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


#ifndef _LIGHT_HXX
#define _LIGHT_HXX


#ifndef __cplusplus                                                          
# error This library requires C++
#endif                                   


#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#ifdef HAVE_WINDOWS_H
#  include <windows.h>
#endif

#include <simgear/compiler.h>

#include SG_GL_H

#include <plib/sg.h>			// plib include

#include <simgear/structure/subsystem_mgr.hxx>
#include <simgear/math/interpolater.hxx>
#include <simgear/math/point3d.hxx>


// Define a structure containing the global lighting parameters
class FGLight : public SGSubsystem
{

private:

    /*
     * Lighting look up tables (based on sun angle with local horizon)
     */
    SGInterpTable *_ambient_tbl, *_diffuse_tbl, *_specular_tbl;
    SGInterpTable *_sky_tbl;

    /**
     * position of the sun and moon in various forms
     */

    // in geocentric coordinates
    double _sun_lon, _sun_gc_lat;
    double _moon_lon, _moon_gc_lat;

    // in cartesian coordiantes
    Point3D _sunpos, _moonpos;

    // (in view coordinates)
    sgVec4 _sun_vec, _moon_vec;

    // inverse (in view coordinates)
    sgVec4 _sun_vec_inv, _moon_vec_inv;

    // the angle between the celestial object and the local horizontal
    // (in radians)
    double _sun_angle, _moon_angle;
    double _prev_sun_angle;

    // the rotation around our vertical axis of the sun (relative to
    // due south with positive numbers going in the counter clockwise
    // direction.)  This is the direction we'd need to face if we
    // wanted to travel towards celestial object.
    double _sun_rotation, _moon_rotation;

    /**
     * Derived lighting values
     */

    // ambient, diffuse and specular component
    GLfloat _scene_ambient[4];
    GLfloat _scene_diffuse[4];
    GLfloat _scene_specular[4];

    // clear sky, fog and cloud color
    GLfloat _sky_color[4];
    GLfloat _fog_color[4];
    GLfloat _cloud_color[4];

    // clear sky and fog color adjusted for sunset effects
    GLfloat _adj_fog_color[4];
    GLfloat _adj_sky_color[4];

    double _dt_total;

    void update_sky_color ();
    void update_adj_fog_color ();

public:

    FGLight ();
    virtual ~FGLight ();

    virtual void init ();
    virtual void reinit ();
    virtual void bind ();
    virtual void unbind ();
    virtual void update ( double dt );


    // Color related functions

    inline float *scene_ambient () const { return (float *)_scene_ambient; }
    inline float *scene_diffuse () const { return (float *)_scene_diffuse; }
    inline float *scene_specular () const { return (float *)_scene_specular; }

    inline float *sky_color () const { return (float *)_sky_color; }
    inline float *cloud_color () const { return (float *)_cloud_color; }
    inline float *adj_fog_color () const { return (float *)_adj_fog_color; }


    // Sun related functions

    inline double get_sun_angle () const { return _sun_angle; }
    inline void set_sun_angle (double a) { _sun_angle = a; }

    inline double get_sun_rotation () const { return _sun_rotation; }
    inline void set_sun_rotation (double r) { _sun_rotation = r; }

    inline double get_sun_lon () const { return _sun_lon; }
    inline void set_sun_lon (double l) { _sun_lon = l; }

    inline double get_sun_gc_lat () const { return _sun_gc_lat; }
    inline void set_sun_gc_lat (double l) { _sun_gc_lat = l; }

    inline const Point3D& get_sunpos () const { return _sunpos; }
    inline void set_sunpos (const Point3D& p) { _sunpos = p; }

    inline float *sun_vec () const { return (float *)_sun_vec; }
    inline float *sun_vec_inv () const { return (float *)_sun_vec_inv; }


    // Moon related functions

    inline double get_moon_angle () const { return _moon_angle; }
    inline void set_moon_angle (double a) { _moon_angle = a; }

    inline double get_moon_rotation () const { return _moon_rotation; }
    inline void set_moon_rotation (double r) { _moon_rotation = r; }

    inline double get_moon_lon () const { return _moon_lon; }
    inline void set_moon_lon (double l) { _moon_lon = l; }

    inline double get_moon_gc_lat () const { return _moon_gc_lat; }
    inline void set_moon_gc_lat (double l) { _moon_gc_lat = l; }

    inline const Point3D& get_moonpos () const { return _moonpos; }
    inline void set_moonpos (const Point3D& p) { _moonpos = p; }

    inline float *moon_vec () const { return (float *)_moon_vec; }
    inline float *moon_vec_inv () const { return (float *)_moon_vec_inv; }
};

#endif // _LIGHT_HXX

