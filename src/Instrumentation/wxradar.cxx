// Wx Radar background texture
//
// Written by Harald JOHNSEN, started May 2005.
// With major amendments by Vivian MEAZZA May 2007
//
//
// Copyright (C) 2005  Harald JOHNSEN
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
//

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <plib/sg.h>

#include <simgear/constants.h>
#include <simgear/misc/sg_path.hxx>
#include <simgear/environment/visual_enviro.hxx>
#include <simgear/structure/exception.hxx>
#include <simgear/misc/sg_path.hxx>
#include <simgear/math/sg_geodesy.hxx>

#include <Main/fg_props.hxx>
#include <Main/globals.hxx>
#include <Cockpit/panel.hxx>
#include <Cockpit/hud.hxx>
#include <AIModel/AIBase.hxx>
#include <AIModel/AIManager.hxx>
#include <AIModel/AIBallistic.hxx>

#include "instrument_mgr.hxx"
#include "od_gauge.hxx"
#include "wxradar.hxx"


// texture name to use in 2D and 3D instruments
static const char *ODGAUGE_NAME = "Aircraft/Instruments/Textures/od_wxradar.rgb";
static const float UNIT = 1.0f / 8.0f;


wxRadarBg::wxRadarBg ( SGPropertyNode *node) :
    _name(node->getStringValue("name", "radar")),
    _num(node->getIntValue("number", 0)),
    _interval(node->getDoubleValue("update-interval-sec", 1.0)),
    _time( 0.0 ),
    _last_switchKnob( "off" ),
    _sim_init_done ( false ),
    resultTexture( 0 ),
    wxEcho( 0 ),
    _odg( 0 )
{
    const char *tacan_source = node->getStringValue("tacan-source",
            "/instrumentation/tacan");
    _Tacan = fgGetNode(tacan_source, true);
}


wxRadarBg::~wxRadarBg ()
{
}


void
wxRadarBg::init ()
{
    string branch;
    branch = "/instrumentation/" + _name;

    _Instrument = fgGetNode(branch.c_str(), _num, true );
    _serviceable_node = _Instrument->getNode("serviceable", true);
    resultTexture = FGTextureManager::createTexture( ODGAUGE_NAME );
    SGPath tpath(globals->get_fg_root());
    tpath.append("Aircraft/Instruments/Textures/wxecho.rgb");
    // no mipmap or else alpha will mix with pixels on the border of shapes, ruining the effect
    wxEcho = new ssgTexture( tpath.c_str(), false, false, false);

    _Instrument->setFloatValue("trk", 0.0);
    _Instrument->setFloatValue("tilt", 0.0);
    _Instrument->setStringValue("status","");
    // those properties are used by a radar instrument of a MFD
    // input switch = OFF | TST | STBY | ON
    // input mode = WX | WXA | MAP
    // output status = STBY | TEST | WX | WXA | MAP | blank
    // input lightning = true | false
    // input TRK = +/- n degrees
    // input TILT = +/- n degree
    // input autotilt = true | false
    // input range = n nm (20/40/80)
    // input display-mode = arc | rose | map | plan

    FGInstrumentMgr *imgr = (FGInstrumentMgr *) globals->get_subsystem("instrumentation");
    _odg = (FGODGauge *) imgr->get_subsystem("od_gauge");

    _ai = (FGAIManager*)globals->get_subsystem("ai_model");

    _user_lat_node = fgGetNode("/position/latitude-deg", true);
    _user_lon_node = fgGetNode("/position/longitude-deg", true);
    _user_alt_node = fgGetNode("/position/altitude-ft", true);

    _user_speed_east_fps_node   = fgGetNode("/velocities/speed-east-fps", true);
    _user_speed_north_fps_node  = fgGetNode("/velocities/speed-north-fps", true);

    _tacan_serviceable_node = _Tacan->getNode("serviceable", true);
    _tacan_distance_node    = _Tacan->getNode("indicated-distance-nm", true);
    _tacan_name_node        = _Tacan->getNode("name", true);
    _tacan_bearing_node     = _Tacan->getNode("indicated-bearing-true-deg", true);
    _tacan_in_range_node    = _Tacan->getNode("in-range", true);

    _radar_mode_control_node = _Instrument->getNode("mode-control", true);
    _radar_coverage_node = _Instrument->getNode("limit-deg", true);
    _radar_ref_rng_node = _Instrument->getNode("reference-range-nm", true);
    _radar_coverage_node->setFloatValue(120);
    _radar_ref_rng_node->setDoubleValue(35);

    SGPropertyNode *n = _Instrument->getNode("display-controls", true);
    _radar_weather_node     = n->getNode("WX", true);
    _radar_position_node    = n->getNode("pos", true);
    _radar_data_node        = n->getNode("data", true);
    _radar_centre_node      = n->getNode("centre", true);

    _radar_centre_node->setBoolValue(false);

    _ai_enabled_node = fgGetNode("/sim/ai/enabled", true);

    _x_displacement = 0;
    _y_displacement = 0;
    _x_sym_displacement = 0;
    _y_sym_displacement = 0;

}


void
wxRadarBg::update (double delta_time_sec)
{
    if ( ! _sim_init_done ) {
        if ( ! fgGetBool("sim/sceneryloaded", false) )
            return;
        _sim_init_done = true;
    }
    if ( !_odg || ! _serviceable_node->getBoolValue() ) {
        _Instrument->setStringValue("status","");
        return;
    }
    _time += delta_time_sec;
    if (_time < _interval)
        return;

    _time = 0.0;

    string switchKnob = _Instrument->getStringValue("switch", "on");
    string display_mode = _Instrument->getStringValue("display-mode", "arc");

    if ( _last_switchKnob != switchKnob ) {
        // since 3D models don't share textures with the rest of the world
        // we must locate them and replace their handle by hand
        // only do that when the instrument is turned on
        if ( _last_switchKnob == "off" )
            _odg->set_texture( ODGAUGE_NAME, resultTexture->getHandle());
        _last_switchKnob = switchKnob;
    }

    _odg->beginCapture(256);
    _odg->Clear();

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glPushMatrix();
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glBindTexture(GL_TEXTURE_2D, 0);


    if ( switchKnob == "off" ) {
        _Instrument->setStringValue("status","");
    } else if ( switchKnob == "stby" ) {
        _Instrument->setStringValue("status","STBY");
    } else if ( switchKnob == "tst" ) {
        _Instrument->setStringValue("status","TST");
        // find something interesting to do...
    } else {
        update_weather(display_mode, delta_time_sec);
        update_aircraft();
        update_tacan();
        update_heading_marker(display_mode);


        // erase what is out of sight of antenna
        /*
            |\     /|
            | \   / |
            |  \ /  |
            ---------
            |       |
            |       |
            ---------
        */

        float xOffset = 256.0f;
        float yOffset = 180.0f;
        SG_LOG(SG_GENERAL, SG_DEBUG, "Radar: display mode " << display_mode);

        if ( display_mode == "arc" ) {
            glDisable(GL_BLEND);
            glColor4f(1.0f, 0.0f, 0.0f, 0.01f);
            glBegin( GL_QUADS );
            glTexCoord2f( 0.5f, 0.25f);
            glVertex2f(-xOffset, 0.0 + yOffset);
            glTexCoord2f( 1.0f, 0.25f);
            glVertex2f(xOffset, 0.0 + yOffset);
            glTexCoord2f( 1.0f, 0.5f);
            glVertex2f(xOffset, 256.0 + yOffset);
            glTexCoord2f( 0.5f, 0.5f);
            glVertex2f(-xOffset, 256.0 + yOffset);
            glEnd();

            glColor4f(0.0f, 0.0f, 0.0f, 0.0f);
            //glColor4f(0.0f, 1.0f, 0.0f, 1.0f);
            glDisable(GL_ALPHA_TEST);
            glBindTexture(GL_TEXTURE_2D, 0);

            glBegin( GL_TRIANGLES );
            glVertex2f(0.0, 0.0);
            glVertex2f(-256.0, 0.0);
            glVertex2f(-256.0, 256.0 * tan(30*SG_DEGREES_TO_RADIANS));

            glVertex2f(0.0, 0.0);
            glVertex2f(256.0, 0.0);
            glVertex2f(256.0, 256.0 * tan(30*SG_DEGREES_TO_RADIANS));

            glVertex2f(-256, 0.0);
            glVertex2f(256.0, 0.0);
            glVertex2f(-256.0, -256.0);

            glVertex2f(256, 0.0);
            glVertex2f(256.0, -256.0);
            glVertex2f(-256.0, -256.0);
            glEnd();

        } else {
            xOffset = 240.0f;
            yOffset = 40.0f;
        }

        // DEBUG only
        /*glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
        glBegin( GL_LINES );
        glVertex2f(0.0, 0.0);
        glVertex2f(-256.0, 256.0);
        glVertex2f(0.0, 0.0);
        glVertex2f(256.0, 256.0);
        glEnd();
        glRotatef(90, xOffset, yOffset, 0.0f);*/

        glEnable(GL_BLEND);
        glEnable(GL_ALPHA_TEST);

    }
    glPopMatrix();
    _odg->endCapture( resultTexture->getHandle() );
}


void
wxRadarBg::update_weather(const string& display_mode, double delta_time_sec)
{
    string modeButton = _Instrument->getStringValue("mode", "wx");
    bool drawLightning = _Instrument->getBoolValue("lightning", true);
    float range_nm = _Instrument->getFloatValue("range", 40.0);  // FIXME share
    float range_m = range_nm * SG_NM_TO_METER;
    float range = 200.0f / range_nm;

    _user_speed_east_fps = _user_speed_east_fps_node->getDoubleValue();
    _user_speed_north_fps = _user_speed_north_fps_node->getDoubleValue();

    _radarEchoBuffer = *sgEnviro.get_radar_echo();

    // pretend we have a scan angle bigger then the FOV
    // TODO:check real fov, enlarge if < nn, and do clipping if > mm
    const float fovFactor = 1.45f;
    float view_heading = get_heading() * SG_DEGREES_TO_RADIANS;
    _Instrument->setStringValue("status", modeButton.c_str());

    if ( display_mode == "arc" ) {
        glTranslatef(0.0f, -180.0f, 0.0f);
        range = 2*180.0f / range_nm;

    } else if ( display_mode == "map" ) {
        view_heading = 0;
        _x_displacement += range * _user_speed_east_fps * SG_FPS_TO_KT * delta_time_sec / (60*60);
        _y_displacement += range * _user_speed_north_fps * SG_FPS_TO_KT * delta_time_sec / (60*60);

        bool centre = _radar_centre_node->getBoolValue();

        if (centre)
            _x_displacement = _y_displacement = 0;

        SG_LOG(SG_GENERAL, SG_DEBUG, "Radar: displacement "
                << _x_displacement <<", "<<_y_displacement
                << " _user_speed_east_fps * SG_FPS_TO_KT "
                << _user_speed_east_fps * SG_FPS_TO_KT
                << " _user_speed_north_fps * SG_FPS_TO_KT "
                << _user_speed_north_fps * SG_FPS_TO_KT
                << " dt " << delta_time_sec);

        glTranslatef(_x_displacement, _y_displacement, 0.0f);

    } else if ( display_mode == "plan" ) {
        // no sense I presume
        view_heading = 0;
    } else {
        // rose
        view_heading = 0;
    }

    range /= SG_NM_TO_METER;

    list_of_SGWxRadarEcho *radarEcho = &_radarEchoBuffer;
    list_of_SGWxRadarEcho::iterator iradarEcho;
    const float LWClevel[] = { 0.1f, 0.5f, 2.1f };
    float dist = 0;
    float size = 0;


    // draw the cloud radar echo

    bool drawClouds = _radar_weather_node->getBoolValue();

    if (drawClouds) {
        // we do that in 3 passes, one for each color level
        // this is to 'merge' same colors together
        glBindTexture(GL_TEXTURE_2D, wxEcho->getHandle() );
        glColor3f(1.0f, 1.0f, 1.0f);
        glBegin(GL_QUADS);
        for (int level = 0; level <= 2 ; level++ ) {
            float col = level * UNIT;

            for (iradarEcho = radarEcho->begin(); iradarEcho != radarEcho->end();
                    ++iradarEcho) {
                int cloudId = (iradarEcho->cloudId);
                bool upgrade = ((cloudId >> 5) & 1);
                float lwc = iradarEcho->LWC + (upgrade ? 1.0f : 0.0f);
                // skip ns
                if ( iradarEcho->LWC >= 0.5 && iradarEcho->LWC <= 0.6)
                    continue;

                if (!iradarEcho->lightning && lwc >= LWClevel[level]) {
                    dist = sgSqrt(iradarEcho->dist);
                    size = iradarEcho->radius * 2.0;

                    if ( dist - size > range_m )
                        continue;

                    dist *= range;
                    size *= range;
                    // compute the relative angle from the view direction
                    float angle = calcRelBearing(iradarEcho->heading, view_heading);

                    // we will rotate the echo quads, this gives a better rendering
                    const float rot_x = cos (view_heading);
                    const float rot_y = sin (view_heading);

                    // and apply a fov factor to simulate a greater scan angle
                    if (angle > 0)
                        angle = angle * fovFactor + SGD_PI_4;
                    else
                        angle = angle * fovFactor - SGD_PI_4;

                    float x = cos( angle ) * dist;
                    float y = sin( angle ) * dist;

                    // use different shapes so the display is less boring
                    float row = UNIT * (float) (4 + (cloudId & 3) );

                    float size_x = rot_x * size;
                    float size_y = rot_y * size;

                    glTexCoord2f(col, row);
                    glVertex2f(x - size_x, y - size_y);
                    glTexCoord2f(col+UNIT, row);
                    glVertex2f(x + size_y, y - size_x);
                    glTexCoord2f(col+UNIT, row+UNIT);
                    glVertex2f(x + size_x, y + size_y);
                    glTexCoord2f(col, row+UNIT);
                    glVertex2f(x - size_y, y + size_x);

                    SG_LOG(SG_GENERAL, SG_DEBUG, "Radar: drawing clouds"
                            << " ID " << iradarEcho->cloudId
                            << " x,y " << x <<","<< y
                            << " dist" << dist
                            << " view_heading" << view_heading / SG_DEGREES_TO_RADIANS
                            << " heading " << iradarEcho->heading / SG_DEGREES_TO_RADIANS
                            << " angle " << angle / SG_DEGREES_TO_RADIANS);
                }
            }
        }
        glEnd(); // GL_QUADS
    }

    // draw lightning echos
    if ( drawLightning ) {
        glColor3f(1.0f, 1.0f, 1.0f);
        glBegin( GL_QUADS );
        float col = 3 * UNIT;
        float row = 4 * UNIT;
        for (iradarEcho = radarEcho->begin(); iradarEcho != radarEcho->end();
                ++iradarEcho) {
            if ( iradarEcho->lightning ) {
                float dist = iradarEcho->dist;
                dist = dist * range;
                float angle = view_heading - iradarEcho->heading;
                if ( angle > SG_PI )
                    angle -= 2.0*SG_PI;
                if ( angle < - SG_PI )
                    angle += 2.0*SG_PI;
                angle =  angle * fovFactor - SG_PI / 2.0;
                float x = cos( angle ) * dist;
                float y = sin( angle ) * dist;
                float size = UNIT * 0.5f;

                    glTexCoord2f(col, row);
                    glVertex2f(x - size, y - size);
                    glTexCoord2f(col + UNIT, row);
                    glVertex2f(x + size, y - size);
                    glTexCoord2f(col + UNIT, row + UNIT);
                    glVertex2f(x + size, y + size);
                    glTexCoord2f(col, row + UNIT);
                    glVertex2f(x - size, y + size);
            }
        }
        glEnd();
    }
}


void
wxRadarBg::update_aircraft()
{
    if (!_ai_enabled_node->getBoolValue())
        return;

    // A list of pointers to AI objects
    typedef list <SGSharedPtr<FGAIBase> > radar_list_type;
    typedef radar_list_type::iterator radar_list_iterator;
    typedef radar_list_type::const_iterator radar_list_const_iterator;

    radar_list_type _radar_list;
    _radar_list = _ai->get_ai_list();

    SG_LOG(SG_GENERAL, SG_DEBUG, "Radar: AI submodel list size" << _radar_list.size());
    if (_radar_list.empty())
        return;


    SG_LOG(SG_GENERAL, SG_DEBUG, "Radar: Loading AI submodels ");
    const double echo_radii[] = {0, 1, 1.5, 1.5, 0.001, 0.1, 1.5, 2, 1.5, 1.5};

    float range_nm = _Instrument->getFloatValue("range", 40.0);  // FIXME share
    float range_m = range_nm * SG_NM_TO_METER;
    float scale = 200.0f / range_m;
    float view_heading = get_heading() * SG_DEGREES_TO_RADIANS;

    double user_lat = _user_lat_node->getDoubleValue();
    double user_lon = _user_lon_node->getDoubleValue();
    double user_alt = _user_alt_node->getDoubleValue();

    radar_list_iterator radar_list_itr = _radar_list.begin();
    radar_list_iterator end = _radar_list.end();

    for (; radar_list_itr != end; ++radar_list_itr) {
        FGAIBase *ac = *radar_list_itr;
        int type       = ac->getType();
        double lat     = ac->_getLatitude();
        double lon     = ac->_getLongitude();
        double alt     = ac->_getAltitude();
        double heading = ac->_getHeading();

        double range, bearing;
        calcRangeBearing(user_lat, user_lon, lat, lon, range, bearing);

        SG_LOG(SG_GENERAL, SG_DEBUG,
                "Radar: ID=" << ac->getID() << "(" << _radar_list.size() << ")"
                << " type=" << type
                << " view_heading=" << view_heading * SG_RADIANS_TO_DEGREES
                << " heading=" << heading
                << " range=" << range
                << " bearing=" << bearing
                << " alt=" << alt);

        bool isVisible = calcRadarHorizon(user_alt, alt, range);
        SG_LOG(SG_GENERAL, SG_DEBUG, "Radar: visible " << isVisible);
        if (!isVisible)
            continue;

        bool isDetected = calcMaxRange(type, range);
        if (!isDetected)
            continue;

        bearing *= SG_DEGREES_TO_RADIANS;
        heading *= SG_DEGREES_TO_RADIANS;
        range *= SG_NM_TO_METER;

        float radius = range * scale;

        // draw aircraft echoes
        if (_radar_position_node->getBoolValue()) {
            glBindTexture(GL_TEXTURE_2D, wxEcho->getHandle());  // different texture
            glColor3f(1.0f, 1.0f, 1.0f);
            glBegin(GL_QUADS);

            float col = 3 * UNIT;    // EEEW FIXME
            float row = 3 * UNIT;
            float limit = _radar_coverage_node->getFloatValue();
            float angle = calcRelBearing(bearing, view_heading);

            if (limit > 180)
                limit = 180;
            else if (limit < 0)
                limit = 0;

            if (angle < limit * SG_DEGREES_TO_RADIANS
                    && angle > -limit * SG_DEGREES_TO_RADIANS) { // in coverage?

                float echo_radius = echo_radii[type] * 120;
                float size = echo_radius * UNIT;
                float x = sin(bearing) * radius;
                float y = cos(bearing) * radius;
                float a_rot_x = sin(angle + SGD_PI_4);
                float a_rot_y = cos(angle + SGD_PI_4);
                float a_size_x = a_rot_x * size;
                float a_size_y = a_rot_y * size;

                glTexCoord2f(col, row);
                glVertex2f(x - a_size_x, y - a_size_y);
                glTexCoord2f(col + UNIT, row);
                glVertex2f(x + a_size_y, y - a_size_x);
                glTexCoord2f(col + UNIT, row + UNIT);
                glVertex2f(x + a_size_x, y + a_size_y);
                glTexCoord2f(col, row + UNIT);
                glVertex2f(x - a_size_y, y + a_size_x);

                SG_LOG(SG_GENERAL, SG_DEBUG, "Radar:    drawing AI"
                        << " x=" << x << " y=" << y
                        << " radius=" << radius
                        << " angle=" << angle * SG_RADIANS_TO_DEGREES);
            }
            glEnd();
        }

        // draw aircraft symbols (data mode)
        if (_radar_data_node->getBoolValue()) {
            glBindTexture(GL_TEXTURE_2D, wxEcho->getHandle());  // different texture
            glColor3f(1.0f, 1.0f, 1.0f);
            glBegin(GL_QUADS);

            float col = 0 * UNIT;
            float row = 3 * UNIT;
            float angle = calcRelBearing(bearing, view_heading);

            if (angle < 120 * SG_DEGREES_TO_RADIANS
                    && angle > -120 * SG_DEGREES_TO_RADIANS) { // in coverage?

                float size = 500 * UNIT;
                float x = sin(bearing) * radius;
                float y = cos(bearing) * radius;
                float d_rot_x = sin(heading + SGD_PI_4);
                float d_rot_y = cos(heading + SGD_PI_4);
                float d_size_x = d_rot_x * size;
                float d_size_y = d_rot_y * size;

                glTexCoord2f(col, row);
                glVertex2f(x - d_size_x, y - d_size_y);
                glTexCoord2f(col + UNIT, row);
                glVertex2f(x + d_size_y, y - d_size_x);
                glTexCoord2f(col + UNIT, row + UNIT);
                glVertex2f(x + d_size_x, y + d_size_y);
                glTexCoord2f(col, row + UNIT);
                glVertex2f(x - d_size_y, y + d_size_x);

                SG_LOG(SG_GENERAL, SG_DEBUG, "Radar:    drawing data"
                        << " x=" << x <<" y="<< y
                        << " bearing=" << angle * SG_RADIANS_TO_DEGREES
                        << " radius=" << radius);
            }
            glEnd();
        }
    }
}


void
wxRadarBg::update_tacan()
{
    // draw TACAN symbol
    int mode = _radar_mode_control_node->getIntValue();
    bool inRange = _tacan_in_range_node->getBoolValue();

    if (mode != 1 || !inRange)
        return;

    float range_nm = _Instrument->getFloatValue("range", 40.0);  // FIXME share
    float range = 200.0f / range_nm;
    range /= SG_NM_TO_METER;
    float view_heading = get_heading() * SG_DEGREES_TO_RADIANS;

    float col = 1 * UNIT;
    float row = 3 * UNIT;
    glBindTexture(GL_TEXTURE_2D, wxEcho->getHandle());     // use this for different texture

    float dist = _tacan_distance_node->getFloatValue() * SG_NM_TO_METER;
    dist *= range;
    // calculate relative bearing
    float angle = calcRelBearing(_tacan_bearing_node->getFloatValue()
            * SG_DEGREES_TO_RADIANS, view_heading);

    // it's always in coverage, so draw it
    float size = UNIT * 500;
    float x = sin(angle) * dist;
    float y = cos(angle) * dist;

    SG_LOG(SG_GENERAL, SG_DEBUG, "Radar:     drawing TACAN"
            << " dist=" << dist
            << " view_heading=" << view_heading * SG_RADIANS_TO_DEGREES
            << " heading=" << _tacan_bearing_node->getDoubleValue()
            << " angle=" << angle * SG_RADIANS_TO_DEGREES
            << " x=" << x << " y="<< y
            << " size=" << size);

    glColor3f(1.0f, 1.0f, 1.0f);
    glBegin(GL_QUADS);
        glTexCoord2f(col, row);
        glVertex2f(x - size, y - size);
        glTexCoord2f(col + UNIT, row);
        glVertex2f(x + size, y - size);
        glTexCoord2f(col + UNIT, row + UNIT);
        glVertex2f(x + size, y + size);
        glTexCoord2f(col, row + UNIT);
        glVertex2f(x - size, y + size);
    glEnd();
}


void
wxRadarBg::update_heading_marker(const string& display_mode)
{
    float range_nm = _Instrument->getFloatValue("range", 40.0);  // FIXME share
    float range = 200.0f / range_nm;
    range /= SG_NM_TO_METER;

    float col = 2 * UNIT;
    float row = 3 * UNIT;

    glBindTexture(GL_TEXTURE_2D, wxEcho->getHandle());     // use this for different texture
    float size = UNIT * 500;
    float view_heading = get_heading() * SG_DEGREES_TO_RADIANS;

    if (display_mode == "map")
        glTranslatef(range, range, 0.0f);

    float x = sin(view_heading);
    float y = cos(view_heading);
    float s_rot_x = sin(view_heading + SGD_PI_4);
    float s_rot_y = cos(view_heading + SGD_PI_4);
    float s_size_x = s_rot_x * size;
    float s_size_y = s_rot_y * size;

    glColor3f(1.0f, 1.0f, 1.0f);
    glBegin(GL_QUADS);
        glTexCoord2f(col, row);
        glVertex2f(x - s_size_x, y - s_size_y);
        glTexCoord2f(col + UNIT, row);
        glVertex2f(x + s_size_y, y - s_size_x);
        glTexCoord2f(col + UNIT, row + UNIT);
        glVertex2f(x + s_size_x, y + s_size_y);
        glTexCoord2f(col, row + UNIT);
        glVertex2f(x - s_size_y, y + s_size_x);
    glEnd();

    //SG_LOG(SG_GENERAL, SG_DEBUG, "Radar:   drawing heading marker"
    //        << " x,y " << x <<","<< y
    //        << " dist" << dist
    //        << " view_heading" << view_heading * SG_RADIANS_TO_DEGREES
    //        << " heading " << iradarEcho->heading * SG_RADIANS_TO_DEGREES
    //        << " angle " << angle * SG_RADIANS_TO_DEGREES);
}


bool
wxRadarBg::calcRadarHorizon(double user_alt, double alt, double range)
{
    // Radar Horizon  = 1.23(ht^1/2 + hr^1/2),
    //don't allow negative altitudes (an approximation - yes altitudes can be negative)

    if (user_alt < 0)
        user_alt = 0;

    if (alt < 0)
        alt = 0;

    double radarhorizon = 1.23 * (sqrt(alt) + sqrt(user_alt));
    SG_LOG(SG_GENERAL, SG_DEBUG, "Radar: horizon " << radarhorizon);
    return radarhorizon >= range;
}


bool
wxRadarBg::calcMaxRange(int type, double range)
{
    //The Radar Equation:
    //
    // MaxRange^4 = (TxPower * AntGain^2 * lambda^2 * sigma)/((constant) * MDS)
    //
    // Where (constant) = (4*pi)3 and MDS is the Minimum Detectable Signal power.
    //
    // For a given radar we can assume that the only variable is sigma,
    // the target radar cross section.
    //
    // Here, we will use a normalised rcs (sigma) for a standard taget and assume that this
    // will provide a maximum range of 35nm;
    //
    // TODO - make the maximum range adjustable at runtime

    const double sigma[] = {0, 1, 100, 100, 0.001, 0.1, 100, 100, 1, 1};
    double constant = _radar_ref_rng_node->getDoubleValue();

    if (constant <= 0)
        constant = 35;

    double maxrange = constant * pow(sigma[type], 0.25);
    SG_LOG(SG_GENERAL, SG_DEBUG, "Radar: max range " << maxrange);
    return maxrange >= range;
}


void
wxRadarBg::calcRangeBearing(double lat, double lon, double lat2, double lon2,
        double &range, double &bearing ) const
{
    // calculate the bearing and range of the second pos from the first
    double az2, distance;
    geo_inverse_wgs_84(lat, lon, lat2, lon2, &bearing, &az2, &distance);
    range = distance *= SG_METER_TO_NM;
}


float
wxRadarBg::calcRelBearing(float bearing, float heading)
{
    float angle = bearing - heading;

    if (angle >= SG_PI)
        angle -= 2.0 * SG_PI;

    if (angle < -SG_PI)
        angle += 2.0 * SG_PI;

    return angle;
}

