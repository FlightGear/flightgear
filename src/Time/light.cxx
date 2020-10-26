//
// light.cxx -- lighting routines
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


#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <simgear/compiler.h>

#include <cmath>

#include <simgear/constants.h>
#include <simgear/debug/logstream.hxx>
#include <simgear/math/interpolater.hxx>
#include <simgear/misc/sg_path.hxx>
#include <simgear/scene/sky/sky.hxx>
#include <simgear/screen/colors.hxx>
#include <simgear/timing/sg_time.hxx>
#include <simgear/structure/event_mgr.hxx>

#include <Main/main.hxx>
#include <Main/globals.hxx>
#include <Main/fg_props.hxx>
#include <Viewer/renderer.hxx>
#include <Viewer/view.hxx>

#include "light.hxx"
#include "bodysolver.hxx"

// initialize lighting tables
void FGLight::init () {
    SG_LOG( SG_EVENT, SG_INFO,
            "Initializing Lighting interpolation tables." );

    // build the path names of the lookup tables
    SGPath path( globals->get_fg_root() );

    // initialize ambient, diffuse and specular tables
    SGPath ambient_path = path;
    ambient_path.append( "Lighting/ambient" );
    _ambient_tbl = std::make_unique<SGInterpTable>( ambient_path );

    SGPath diffuse_path = path;
    diffuse_path.append( "Lighting/diffuse" );
    _diffuse_tbl = std::make_unique<SGInterpTable>( diffuse_path );

    SGPath specular_path = path;
    specular_path.append( "Lighting/specular" );
    _specular_tbl = std::make_unique<SGInterpTable>( specular_path );

    // initialize sky table
    SGPath sky_path = path;
    sky_path.append( "Lighting/sky" );
    _sky_tbl = std::make_unique<SGInterpTable>( sky_path );

    // update all solar system body positions of interest
    globals->get_event_mgr()->addTask("updateObjects", this,
                            &FGLight::updateObjects, 0.5 );
}


void FGLight::reinit () {
    _prev_sun_angle = -9999.0;
    _dt_total = 0;

    _ambient_tbl.reset();
    _diffuse_tbl.reset();
    _specular_tbl.reset();
    _sky_tbl.reset();

    init();

    updateObjects();
    update_sky_color();
    update_adj_fog_color();
}

void FGLight::bind () {
    SGPropertyNode *prop = globals->get_props();

    // Write Only
    tie(prop,"/rendering/scene/saturation",     SGRawValuePointer<float>(&_saturation));
    tie(prop,"/rendering/scene/scattering",     SGRawValuePointer<float>(&_scattering));
    tie(prop,"/rendering/scene/overcast",       SGRawValuePointer<float>(&_overcast));

    _sunAngleRad = prop->getNode("/sim/time/sun-angle-rad", true);
    _sunAngleRad->setDoubleValue(_sun_angle);
    _moonAngleRad = prop->getNode("/sim/time/moon-angle-rad", true);
    _moonAngleRad->setDoubleValue(_moon_angle);
    _humidity = fgGetNode("/environment/relative-humidity", true);

    // Read Only
    tie(prop,"/rendering/scene/ambient/red",    SGRawValuePointer<float>(&_scene_ambient[0]));
    tie(prop,"/rendering/scene/ambient/green",  SGRawValuePointer<float>(&_scene_ambient[1]));
    tie(prop,"/rendering/scene/ambient/blue",   SGRawValuePointer<float>(&_scene_ambient[2]));
    tie(prop,"/rendering/scene/diffuse/red",    SGRawValuePointer<float>(&_scene_diffuse[0]));
    tie(prop,"/rendering/scene/diffuse/green",  SGRawValuePointer<float>(&_scene_diffuse[1]));
    tie(prop,"/rendering/scene/diffuse/blue",   SGRawValuePointer<float>(&_scene_diffuse[2]));
    tie(prop,"/rendering/scene/specular/red",   SGRawValuePointer<float>(&_scene_specular[0]));
    tie(prop,"/rendering/scene/specular/green", SGRawValuePointer<float>(&_scene_specular[1]));
    tie(prop,"/rendering/scene/specular/blue",  SGRawValuePointer<float>(&_scene_specular[2]));
    tie(prop,"/rendering/dome/sun/red",         SGRawValuePointer<float>(&_sun_color[0]));
    tie(prop,"/rendering/dome/sun/green",       SGRawValuePointer<float>(&_sun_color[1]));
    tie(prop,"/rendering/dome/sun/blue",        SGRawValuePointer<float>(&_sun_color[2]));
    tie(prop,"/rendering/dome/sky/red",         SGRawValuePointer<float>(&_sky_color[0]));
    tie(prop,"/rendering/dome/sky/green",       SGRawValuePointer<float>(&_sky_color[1]));
    tie(prop,"/rendering/dome/sky/blue",        SGRawValuePointer<float>(&_sky_color[2]));
    tie(prop,"/rendering/dome/cloud/red",       SGRawValuePointer<float>(&_cloud_color[0]));
    tie(prop,"/rendering/dome/cloud/green",     SGRawValuePointer<float>(&_cloud_color[1]));
    tie(prop,"/rendering/dome/cloud/blue",      SGRawValuePointer<float>(&_cloud_color[2]));
    tie(prop,"/rendering/dome/fog/red",         SGRawValuePointer<float>(&_fog_color[0]));
    tie(prop,"/rendering/dome/fog/green",       SGRawValuePointer<float>(&_fog_color[1]));
    tie(prop,"/rendering/dome/fog/blue",        SGRawValuePointer<float>(&_fog_color[2]));

    // Sun vector
    tie(prop,"/ephemeris/sun/local/x", SGRawValuePointer<float>(&_sun_vec[0]));
    tie(prop,"/ephemeris/sun/local/y", SGRawValuePointer<float>(&_sun_vec[1]));
    tie(prop,"/ephemeris/sun/local/z", SGRawValuePointer<float>(&_sun_vec[2]));

    // Moon vector
    tie(prop,"/ephemeris/moon/local/x", SGRawValuePointer<float>(&_moon_vec[0]));
    tie(prop,"/ephemeris/moon/local/y", SGRawValuePointer<float>(&_moon_vec[1]));
    tie(prop,"/ephemeris/moon/local/z", SGRawValuePointer<float>(&_moon_vec[2]));

    // Properties used directly by effects
    _chromeProps[0] = prop->getNode("/rendering/scene/chrome-light/red", true);
    _chromeProps[1] = prop->getNode("/rendering/scene/chrome-light/green",
                                    true);
    _chromeProps[2] = prop->getNode("/rendering/scene/chrome-light/blue", true);
    _chromeProps[3] = prop->getNode("/rendering/scene/chrome-light/alpha",
                                    true);
    for (int i = 0; i < 4; ++i)
        _chromeProps[i]->setValue(0.0);
}

void FGLight::unbind () {
    _tiedProperties.Untie();

    for (int i = 0; i < 4; ++i)
        _chromeProps[i] = SGPropertyNode_ptr();
    _sunAngleRad = SGPropertyNode_ptr();
    _moonAngleRad.reset();
    _humidity = SGPropertyNode_ptr();
}


// update lighting parameters based on current sun position
void FGLight::update( double dt )
{
    update_adj_fog_color();

    if (_prev_sun_angle != _sun_angle) {
        _prev_sun_angle = _sun_angle;
        update_sky_color();
    }
}

void FGLight::update_sky_color () {
    const SGVec4f base_sky_color( 0.31, 0.43, 0.69, 1.0 );
    const SGVec4f base_fog_color( 0.63, 0.72, 0.88, 1.0 );

    // calculate lighting parameters based on sun's relative angle to
    // local up
    float av = _humidity->getFloatValue() * 45;
    float visibility_log = log(av)/11.0;
    float visibility_inv = (45000.0 - av)/45000.0;

    float deg = _sun_angle * SGD_RADIANS_TO_DEGREES;

    if (_saturation < 0.0) _saturation = 0.0;
    else if (_saturation > 1.0) _saturation = 1.0;
    if (_scattering < 0.0) _scattering = 0.0;
    else if (_scattering > 1.0) _scattering = 1.0;
    if (_overcast < 0.0) _overcast = 0.0;
    else if (_overcast > 1.0) _overcast = 1.0;

    float ambient = _ambient_tbl->interpolate( deg ) + visibility_inv/10;
    float diffuse = _diffuse_tbl->interpolate( deg );
    float specular = _specular_tbl->interpolate( deg ) * visibility_log;
    float sky_brightness = _sky_tbl->interpolate( deg );

    ambient *= _saturation;
    diffuse *= _saturation;
    specular *= _saturation;
    sky_brightness *= _saturation;

    // sky_brightness = 0.15;  // used to force a dark sky (when testing)

    /** fog color */
    float sqr_sky_brightness = sky_brightness * sky_brightness * _scattering;
    _fog_color = base_fog_color * sqr_sky_brightness;
    _fog_color[3] = base_fog_color[3];
    gamma_correct_rgb( _fog_color.data() );

    /** sky color */
    static const SGVec4f one_vec( 1.0f, 1.0f, 1.0f, 1.0f);
    SGVec4f overcast_color = (one_vec - base_sky_color) * _overcast;
    _sky_color = (base_sky_color + overcast_color) * sky_brightness;
    _sky_color[3] = base_sky_color[3];
    gamma_correct_rgb( _sky_color.data() );

    /** cloud color */
    _cloud_color = base_fog_color * sky_brightness;

    /** adjust the cloud colors for sunrise/sunset effects (darken them) */
    if (_sun_angle > 1.0) {
       float sun2 = 1.0 / sqrt(_sun_angle);
       _cloud_color *= sun2;
    }
    _cloud_color[3] = base_fog_color[3];
    gamma_correct_rgb( _cloud_color.data() );

    /** ambient light */
    _scene_ambient = _fog_color  * ambient;
    _scene_ambient[3] = _fog_color[3];
    gamma_correct_rgb( _scene_ambient.data() );

    /** diffuse light */
    SGSky* thesky = globals->get_renderer()->getSky();
    SGVec4f color = thesky->get_scene_color();
    _scene_diffuse = color * diffuse;
    _scene_diffuse[3] = color[3];
    gamma_correct_rgb( _scene_diffuse.data() );

    SGVec4f chrome = _scene_ambient * .4f + _scene_diffuse;
    chrome[3] = 1.0f;
    if (chrome != _scene_chrome) {
        _scene_chrome = chrome;
        for (int i = 0; i < 4; ++i)
            _chromeProps[i]->setValue(static_cast<double>(_scene_chrome[i]));
    }

    /** specular light */
    _sun_color = thesky->get_sun_color();
    _scene_specular = _sun_color * specular;
    _scene_specular[3] = _sun_color[3];
    gamma_correct_rgb( _scene_specular.data() );
}


// calculate fog color adjusted for sunrise/sunset effects
void FGLight::update_adj_fog_color () {

//    double pitch = globals->get_current_view()->getPitch_deg()
//                     * SGD_DEGREES_TO_RADIANS;
//    double pitch_offset = globals->get_current_view()-> getPitchOffset_deg()
//                     * SGD_DEGREES_TO_RADIANS;
    double heading = globals->get_current_view()->getHeading_deg()
                     * SGD_DEGREES_TO_RADIANS;
    double heading_offset = globals->get_current_view()->getHeadingOffset_deg()
                            * SGD_DEGREES_TO_RADIANS;

    // set fog color (we'll try to match the sunset color in the
    // direction we are looking

    // Do some sanity checking ...
    if ( _sun_rotation < -2.0 * SGD_2PI || _sun_rotation > 2.0 * SGD_2PI ) {
        SG_LOG( SG_EVENT, SG_ALERT, "Sun rotation bad = " << _sun_rotation );
        return;
    }

    if ( heading < -2.0 * SGD_2PI || heading > 2.0 * SGD_2PI ) {
        SG_LOG( SG_EVENT, SG_ALERT, "Heading rotation bad = " << heading );
        return;
    }

    if ( heading_offset < -2.0 * SGD_2PI || heading_offset > 2.0 * SGD_2PI ) {
        SG_LOG( SG_EVENT, SG_ALERT, "Heading offset bad = " << heading_offset );
        return;
    }

    static float gamma = system_gamma;

    // first determine the difference between our view angle and local
    // direction to the sun
    //double vert_rotation = pitch + pitch_offset;

    // revert to unmodified values before using them.
    //
    SGSky* thesky = globals->get_renderer()->getSky();
    SGVec4f color = thesky->get_scene_color();

    gamma_restore_rgb( _fog_color.data(), gamma );
    gamma_restore_rgb( _sky_color.data(), gamma );

    // Calculate the fog color in the direction of the sun for
    // sunrise/sunset effects.
    //
    _sun_color[0] = color[0]*color[0]*color[0];
    _sun_color[1] = color[1]*color[1]*color[1];
    _sun_color[2] = color[2]*color[2];

    // interpolate between the sunrise/sunset color and the color
    // at the opposite direction of this effect. Take in account
    // the current visibility.
    //
    float av = thesky->get_visibility();
    if (av > 45000) av = 45000;

    float avf = 0.87 - (45000 - av) / 83333.33;
    float sif = 0.5 - cos(_sun_angle*2)/2;

    if (sif < 1e-3)
       sif = 1e-3;

    // determine horizontal angle between current view direction and sun
    // since _sun_rotation is relative to South, and heading is in the local frame
    // we need to account for the 180 degrees offset and differing signs
    // hence the negation and SGD_PI adjustment.
    double hor_rotation = -_sun_rotation - SGD_PI - heading + heading_offset;
    if (hor_rotation < 0 )
        hor_rotation = fmod(hor_rotation, SGD_2PI) + SGD_2PI;
    else
        hor_rotation = fmod(hor_rotation, SGD_2PI);

    float rf1 = fabs((hor_rotation - SGD_PI) / SGD_PI); // 0.0 .. 1.0
    float rf2 = avf * pow(rf1*rf1, 1/sif) * 1.0639 * _saturation * _scattering;
    float rf3 = 1.0 - rf2;

    gamma = system_gamma * (0.9 - sif*avf);
    _adj_fog_color = rf3 * _fog_color + rf2 * _sun_color;
    _adj_fog_color[3] = 0;
    gamma_correct_rgb( _adj_fog_color.data(), gamma);

    // make sure the colors have their original value before they are being
    // used by the rest of the program.
    //
    gamma_correct_rgb( _fog_color.data(), gamma );
    gamma_correct_rgb( _sky_color.data(), gamma );
}

// update all solar system bodies of interest
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

// update the position of one solar system body
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
