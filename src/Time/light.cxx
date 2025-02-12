/*
 * SPDX-FileName: light.cxx
 * SPDX-FileComment: lighting routines
 * SPDX-FileCopyrightText: Copyright (C) 1998  Curtis L. Olson  - http://www.flightgear.org/~curt
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <cmath>

#include <simgear/constants.h>
#include <simgear/debug/logstream.hxx>
#include <simgear/timing/sg_time.hxx>
#include <simgear/structure/event_mgr.hxx>

#include <Main/globals.hxx>
#include <Main/fg_props.hxx>

#include "light.hxx"
#include "bodysolver.hxx"

void FGLight::init()
{
    // update all solar system body positions of interest
    globals->get_event_mgr()->addTask("updateObjects",
        [this](){ this->updateObjects(); }, 0.5 );
}

void FGLight::reinit()
{
    init();
    updateObjects();
}

void FGLight::bind()
{
    SGPropertyNode *prop = globals->get_props();

    _sunAngleRad = prop->getNode("/sim/time/sun-angle-rad", true);
    _sunAngleRad->setDoubleValue(_sun_angle);
    _moonAngleRad = prop->getNode("/sim/time/moon-angle-rad", true);
    _moonAngleRad->setDoubleValue(_moon_angle);

    // Sun vector
    tie(prop,"/ephemeris/sun/local/x", SGRawValuePointer<float>(&_sun_vec[0]));
    tie(prop,"/ephemeris/sun/local/y", SGRawValuePointer<float>(&_sun_vec[1]));
    tie(prop,"/ephemeris/sun/local/z", SGRawValuePointer<float>(&_sun_vec[2]));

    // Moon vector
    tie(prop,"/ephemeris/moon/local/x", SGRawValuePointer<float>(&_moon_vec[0]));
    tie(prop,"/ephemeris/moon/local/y", SGRawValuePointer<float>(&_moon_vec[1]));
    tie(prop,"/ephemeris/moon/local/z", SGRawValuePointer<float>(&_moon_vec[2]));
}

void FGLight::unbind()
{
    _tiedProperties.Untie();
    _sunAngleRad.reset();
    _moonAngleRad.reset();
}

void FGLight::update(double dt)
{
    SG_UNUSED(dt);
}

void FGLight::updateObjects()
{
    // update the sun position
    bool sun_not_moon = true;
    updateBodyPos(sun_not_moon, _sun_lon, _sun_lat,
                  _sun_vec, _sun_vec_inv,
                  _sun_angle, _sunAngleRad,
                  _sun_rotation);

    // update the moon position
    sun_not_moon = false;
    updateBodyPos(sun_not_moon, _moon_lon, _moon_gc_lat,
                  _moon_vec, _moon_vec_inv,
                  _moon_angle, _moonAngleRad,
                  _moon_rotation);
}

void FGLight::updateBodyPos(bool sun_not_moon, double& lon, double& lat,
                            SGVec4f& vec, SGVec4f& vec_inv,
                            double& angle, SGPropertyNode_ptr AngleRad,
                            double& rotation)
{
    SGTime *t = globals->get_time_params();

    // returns lon and lat based on GST
    fgBodyPositionGST(t->getGst(), lon, lat, sun_not_moon);

    // It might seem that gc_lat needs to be converted to geodetic
    // latitude here, but it doesn't. The body latitude is the latitude
    // of the point on the earth where the up vector has the same
    // angle from geocentric Z as the body direction. But geodetic
    // latitude is defined as 90 - angle of up vector from Z!
    SGVec3d bodypos = SGVec3d::fromGeoc(SGGeoc::fromRadM(lon, lat,
                                                         SGGeodesy::EQURAD));

    // update the body vector
    vec = SGVec4f(toVec3f(normalize(bodypos)), 0);
    vec_inv = - vec;

    // calculate the body's relative angle to local up
    SGQuatd hlOr =  SGQuatd::fromLonLat( globals->get_view_position() );
    SGVec3d world_up = hlOr.backTransform( -SGVec3d::e3() );
    // cout << "nup = " << nup[0] << "," << nup[1] << ","
    //      << nup[2] << endl;
    // cout << "nbody = " << nbody[0] << "," << nbody[1] << ","
    //      << nbody[2] << endl;

    SGVec3d nbody = normalize(bodypos);
    SGVec3d nup = normalize(world_up);
    angle = acos( dot( nup, nbody ) );

    double signedPI = (angle < 0.0) ? -SGD_PI : SGD_PI;
    angle = fmod(angle+signedPI, SGD_2PI) - signedPI;

    // Get direction to the body in the local frame.
    SGVec3d local_vec = hlOr.transform(nbody);

    // Angle from South.
    // atan2(y,x) returns the angle between the positive X-axis
    // and the vector with the origin at 0, going through (x,y)
    // Since the local frame coordinates have x-positive pointing Nord and
    // y-positive pointing East we need to negate local_vec.x()
    // rotation is positive counterclockwise from South (body in the East)
    // and negative clockwise from South (body in the West)
    rotation = atan2(local_vec.y(), -local_vec.x());

    // cout << "  Sky needs to rotate = " << rotation << " rads = "
    //      << rotation * SGD_RADIANS_TO_DEGREES << " degrees." << endl;

    AngleRad->setDoubleValue(angle);
}


// Register the subsystem.
SGSubsystemMgr::Registrant<FGLight> registrantFGLight(
    SGSubsystemMgr::DISPLAY);
