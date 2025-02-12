/*
 * SPDX-FileName: light.hxx
 * SPDX-FileComment: lighting routines
 * SPDX-FileCopyrightText: Copyright (C) 1998  Curtis L. Olson  - http://www.flightgear.org/~curt
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <simgear/props/props.hxx>
#include <simgear/structure/subsystem_mgr.hxx>
#include <simgear/props/tiedpropertylist.hxx>
#include <simgear/math/SGMath.hxx>

// Define a structure containing the global lighting parameters
class FGLight : public SGSubsystem {
private:
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

    // the rotation around our vertical axis of the sun (relative to
    // due south with positive numbers going in the counter clockwise
    // direction.)  This is the direction we'd need to face if we
    // wanted to travel towards celestial object.
    double _sun_rotation = 0.0, _moon_rotation = 0.0;

    // update all solar system bodies of interest
    void updateObjects();

    // update the position of one solar system body
    void updateBodyPos(bool sun_not_moon, double& lon, double& lat,
       SGVec4f& vec, SGVec4f& vec_inv,
       double& angle, SGPropertyNode_ptr AngleRad,
       double& rotation);

    SGPropertyNode_ptr _sunAngleRad;
    SGPropertyNode_ptr _moonAngleRad;

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
    FGLight() = default;
    virtual ~FGLight() = default;

    // Subsystem API.
    void bind() override;
    void init() override;
    void reinit() override;
    void unbind() override;
    void update(double dt) override;

    // Subsystem identification.
    static const char* staticSubsystemClassId() { return "lighting"; }


    // Sun related functions

    inline double get_sun_angle() const { return _sun_angle; }
    inline void set_sun_angle(double a) { _sun_angle = a; }

    inline double get_sun_rotation() const { return _sun_rotation; }
    inline void set_sun_rotation(double r) { _sun_rotation = r; }

    inline double get_sun_lon() const { return _sun_lon; }
    inline void set_sun_lon(double l) { _sun_lon = l; }

    inline double get_sun_lat() const { return _sun_lat; }
    inline void set_sun_lat(double l) { _sun_lat = l; }

    inline SGVec4f& sun_vec() { return _sun_vec; }
    inline SGVec4f& sun_vec_inv() { return _sun_vec_inv; }


    // Moon related functions

    inline double get_moon_angle() const { return _moon_angle; }
    inline void set_moon_angle(double a) { _moon_angle = a; }

    inline double get_moon_rotation() const { return _moon_rotation; }
    inline void set_moon_rotation(double r) { _moon_rotation = r; }

    inline double get_moon_lon() const { return _moon_lon; }
    inline void set_moon_lon(double l) { _moon_lon = l; }

    inline double get_moon_gc_lat() const { return _moon_gc_lat; }
    inline void set_moon_gc_lat(double l) { _moon_gc_lat = l; }

    inline const SGVec4f& moon_vec() const { return _moon_vec; }
    inline const SGVec4f& moon_vec_inv() const { return _moon_vec_inv; }
};
