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

#include <simgear/compiler.h>

#include <simgear/props/props.hxx>
#include <simgear/structure/subsystem_mgr.hxx>
#include <simgear/props/tiedpropertylist.hxx>
#include <simgear/math/interpolater.hxx>


// Define a structure containing the global lighting parameters
class FGLight : public SGSubsystem
{
private:
    /*
     * Lighting look up tables (based on sun angle with local horizon)
     */
    std::unique_ptr<SGInterpTable> _ambient_tbl, _diffuse_tbl, _specular_tbl;
    std::unique_ptr<SGInterpTable> _sky_tbl;

    /**
     * position of the sun and moon in various forms
     */

    // in geocentric coordinates
    double _sun_lon = 0.0, _sun_lat = 0.0;
    double _moon_lon = 0.0, _moon_gc_lat = 0.0;

    // (in view coordinates)
    SGVec4f _sun_vec = {0, 0, 0, 0};
    SGVec4f _moon_vec = {0, 0, 0, 0};


    // inverse (in view coordinates)
    SGVec4f _sun_vec_inv = {0, 0, 0, 0};
    SGVec4f _moon_vec_inv = {0, 0, 0, 0};

    // the angle between the celestial object and the local horizontal
    // (in radians)
    double _sun_angle = 0.0 , _moon_angle = 0.0;
    double _prev_sun_angle = 0.0;

    // the rotation around our vertical axis of the sun (relative to
    // due south with positive numbers going in the counter clockwise
    // direction.)  This is the direction we'd need to face if we
    // wanted to travel towards celestial object.
    double _sun_rotation = 0.0, _moon_rotation = 0.0;

    /**
     * Derived lighting values
     */

    // ambient, diffuse and specular component
    SGVec4f _scene_ambient = {0, 0, 0, 0};
    SGVec4f _scene_diffuse = {0, 0, 0, 0};
    SGVec4f _scene_specular = {0, 0, 0, 0};
    SGVec4f _scene_chrome = {0, 0, 0, 0};

    // clear sky, fog and cloud color
    SGVec4f _sun_color = {1, 1, 1, 0};
    SGVec4f _sky_color = {0, 0, 0, 0};
    SGVec4f _fog_color = {0, 0, 0, 0};
    SGVec4f _cloud_color = {0, 0, 0, 0};

    // clear sky and fog color adjusted for sunset effects
    SGVec4f _adj_fog_color = {0, 0, 0, 0};
    SGVec4f _adj_sky_color = {0, 0, 0, 0};

    // input parameters affected by the weather system
    float _saturation = 1.0f;
    float _scattering = 0.8f;
    float _overcast = 0.0f;

    double _dt_total = 0.0;

    void update_sky_color ();
    void update_adj_fog_color ();

    // update all solar system bodies of interest
    void updateObjects();

    // update the position of one solar system body
    void updateBodyPos(bool sun_not_moon, double& lon, double& lat,
       SGVec4f& vec, SGVec4f& vec_inv,
       double& angle, SGPropertyNode_ptr AngleRad,
       double& rotation);

    // properties for chrome light; not a tie because I want to fire
    // property listeners when the values change.
    SGPropertyNode_ptr _chromeProps[4];

    SGPropertyNode_ptr _sunAngleRad;
    SGPropertyNode_ptr _moonAngleRad;

    SGPropertyNode_ptr _humidity;

    simgear::TiedPropertyList _tiedProperties;

    /**
     * Tied-properties helper, record nodes which are tied for easy un-tie-ing
     */
    template <typename T>
    void tie(SGPropertyNode* aNode, const char* aRelPath, const SGRawValue<T>& aRawValue)
    {
        _tiedProperties.Tie(aNode->getNode(aRelPath, true), aRawValue);
    }

public:
    FGLight () = default;
    virtual ~FGLight () = default;

    // Subsystem API.
    void bind() override;
    void init() override;
    void reinit() override;
    void unbind() override;
    void update(double dt) override;

    // Subsystem identification.
    static const char* staticSubsystemClassId() { return "lighting"; }

    // Color related functions

    inline const SGVec4f& scene_ambient () const { return _scene_ambient; }
    inline const SGVec4f& scene_diffuse () const { return _scene_diffuse; }
    inline const SGVec4f& scene_specular () const { return _scene_specular; }
    inline const SGVec4f& scene_chrome () const { return _scene_chrome; }

    inline const SGVec4f& sky_color () const { return _sky_color; }
    inline const SGVec4f& cloud_color () const { return _cloud_color; }
    inline const SGVec4f& adj_fog_color () const { return _adj_fog_color; }
    inline const SGVec4f& adj_sky_color () const { return _adj_sky_color; }

    // Sun related functions

    inline double get_sun_angle () const { return _sun_angle; }
    inline void set_sun_angle (double a) { _sun_angle = a; }

    inline double get_sun_rotation () const { return _sun_rotation; }
    inline void set_sun_rotation (double r) { _sun_rotation = r; }

    inline double get_sun_lon () const { return _sun_lon; }
    inline void set_sun_lon (double l) { _sun_lon = l; }

    inline double get_sun_lat () const { return _sun_lat; }
    inline void set_sun_lat (double l) { _sun_lat = l; }

    inline SGVec4f& sun_vec () { return _sun_vec; }
    inline SGVec4f& sun_vec_inv () { return _sun_vec_inv; }


    // Moon related functions

    inline double get_moon_angle () const { return _moon_angle; }
    inline void set_moon_angle (double a) { _moon_angle = a; }

    inline double get_moon_rotation () const { return _moon_rotation; }
    inline void set_moon_rotation (double r) { _moon_rotation = r; }

    inline double get_moon_lon () const { return _moon_lon; }
    inline void set_moon_lon (double l) { _moon_lon = l; }

    inline double get_moon_gc_lat () const { return _moon_gc_lat; }
    inline void set_moon_gc_lat (double l) { _moon_gc_lat = l; }

    inline const SGVec4f& moon_vec () const { return _moon_vec; }
    inline const SGVec4f& moon_vec_inv () const { return _moon_vec_inv; }
};

#endif // _LIGHT_HXX

