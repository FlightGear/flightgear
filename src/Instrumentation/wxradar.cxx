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
static const char *odgauge_name = "Aircraft/Instruments/Textures/od_wxradar.rgb";

wxRadarBg::wxRadarBg ( SGPropertyNode *node) :
    _name(node->getStringValue("name", "wxRadar")),
    _num(node->getIntValue("number", 0)),
    _last_switchKnob( "off" ),
    _sim_init_done ( false ),
    resultTexture( 0 ),
    wxEcho( 0 ),
    _odg( 0 )
{
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
    _serviceable_node = _Instrument->getChild("serviceable", 0, true);
    resultTexture = FGTextureManager::createTexture( odgauge_name );
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

    _Tacan = fgGetNode("/instrumentation/tacan", _num, true);
    _tacan_serviceable_node = _Tacan->getNode("serviceable", true);
    _tacan_distance_node    = _Tacan->getNode("indicated-distance-nm", true);
    _tacan_name_node        = _Tacan->getNode("name", true);
    _tacan_bearing_node     = _Tacan->getNode("indicated-bearing-true-deg", true);
    _tacan_in_range_node    = _Tacan->getNode("in-range", true);

    _Radar = fgGetNode("/instrumentation/radar/display-controls", _num, true);
    _radar_weather_node     = _Radar->getNode("WX", true);
    _radar_position_node    = _Radar->getNode("pos", true);
    _radar_data_node        = _Radar->getNode("data", true);
    _radar_centre_node      = _Radar->getNode("centre", true);

    _radar_centre_node->setBoolValue(false);

    _Radar = fgGetNode("/instrumentation/radar/", _num, true);
    _radar_mode_control_node = _Radar->getNode("mode-control", true);
    _radar_coverage_node = _Radar->getNode("limit-deg", true);
    _radar_ref_rng_node = _Radar->getNode("reference-range-nm", true);

    _radar_coverage_node->setFloatValue(120);
    _radar_ref_rng_node->setDoubleValue(35);

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
    string switchKnob = _Instrument->getStringValue("switch", "on");
    string modeButton = _Instrument->getStringValue("mode", "wx");
    bool drawLightning = _Instrument->getBoolValue("lightning", true);
    float range_nm = _Instrument->getFloatValue("range", 40.0);
    float range_m = range_nm * SG_NM_TO_METER;

    _user_speed_east_fps = _user_speed_east_fps_node->getDoubleValue();
    _user_speed_north_fps = _user_speed_north_fps_node->getDoubleValue();

    if ( _last_switchKnob != switchKnob ) {
        // since 3D models don't share textures with the rest of the world
        // we must locate them and replace their handle by hand
        // only do that when the instrument is turned on
        if ( _last_switchKnob == "off" )
            _odg->set_texture( odgauge_name, resultTexture->getHandle());
        _last_switchKnob = switchKnob;
    }

    FGViewer *current__view = globals->get_current_view();
    if ( current__view->getInternal() &&
        (current__view->getHeadingOffset_deg() <= 15.0 || current__view->getHeadingOffset_deg() >= 345.0) &&
        (current__view->getPitchOffset_deg() <= 15.0 || current__view->getPitchOffset_deg() >= 350.0) ) {

        // we don't update the radar echo if the pilot looks around
        // this is a copy
        _radarEchoBuffer = *sgEnviro.get_radar_echo();
        updateRadar();
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
        string display_mode = _Instrument->getStringValue("display-mode", "arc");

        // pretend we have a scan angle bigger then the FOV
        // TODO:check real fov, enlarge if < nn, and do clipping if > mm
        const float fovFactor = 1.45f;
        float view_heading = get_heading() * SG_DEGREES_TO_RADIANS;
        float range = 200.0f / range_nm;
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
        const float symbolSize = 1.0f / 8.0f ;
        float dist = 0;
        float size = 0;


        // draw the cloud radar echo

        bool drawClouds = _radar_weather_node->getBoolValue();

        if (drawClouds) {

            // we do that in 3 passes, one for each color level
            // this is to 'merge' same colors together
            glBindTexture(GL_TEXTURE_2D, wxEcho->getHandle() );
            glColor3f(1.0f, 1.0f, 1.0f);

            for (int level = 0; level <= 2 ; level++ ) {
                float col = level * symbolSize;

                for (iradarEcho = radarEcho->begin(); iradarEcho != radarEcho->end();
                        ++iradarEcho) {
                    int cloudId = (iradarEcho->cloudId) ;
                    bool upgrade = ((cloudId >> 5) & 1);
                    float lwc = iradarEcho->LWC + (upgrade ? 1.0f : 0.0f);
                    // skip ns
                    if ( iradarEcho->LWC >= 0.5 && iradarEcho->LWC <= 0.6)
                        continue;

                    if (!iradarEcho->lightning && lwc >= LWClevel[level]
                            && !iradarEcho->aircraft) {
                        dist = sgSqrt(iradarEcho->dist);
                        size = iradarEcho->radius * 2.0;

                        if ( dist - size > range_m )
                            continue;

                        dist *= range;
                        size *= range;
                        // compute the relative angle from the view direction
                        float angle = calcRelBearing(iradarEcho->bearing, view_heading);

                        // we will rotate the echo quads, this gives a better rendering
                        const float rot_x = cos (view_heading);
                        const float rot_y = sin (view_heading);

                        // and apply a fov factor to simulate a greater scan angle
                        if (angle > 0)
                            angle = angle * fovFactor + 0.25 * SG_PI;
                        else
                            angle = angle * fovFactor - 0.25 * SG_PI;

                        float x = cos( angle ) * dist;
                        float y = sin( angle ) * dist;

                        // use different shapes so the display is less boring
                        float row = symbolSize * (float) (4 + (cloudId & 3) );

                        float size_x = rot_x * size;
                        float size_y = rot_y * size;

                        glBegin(GL_QUADS);
                        glTexCoord2f( col, row);
                        glVertex2f( x - size_x, y - size_y);
                        glTexCoord2f( col+symbolSize, row);
                        glVertex2f( x + size_y, y - size_x);
                        glTexCoord2f( col+symbolSize, row+symbolSize);
                        glVertex2f( x + size_x, y + size_y);
                        glTexCoord2f( col, row+symbolSize);
                        glVertex2f( x - size_y, y + size_x);
                        glEnd(); // GL_QUADS

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
        }

        // draw lightning echos
        if ( drawLightning ) {
            float col = 3 * symbolSize;
            float row = 4 * symbolSize;
            for (iradarEcho = radarEcho->begin(); iradarEcho != radarEcho->end();
                    ++iradarEcho) {
                if ( iradarEcho->lightning ) {
                    float dist = iradarEcho->dist;
                    dist = dist * range;
                    float angle = view_heading - iradarEcho->bearing;
                    if ( angle > SG_PI )
                        angle -= 2.0*SG_PI;
                    if ( angle < - SG_PI )
                        angle += 2.0*SG_PI;
                    angle =  angle * fovFactor - SG_PI / 2.0;
                    float x = cos( angle ) * dist;
                    float y = sin( angle ) * dist;
                    glColor3f(1.0f, 1.0f, 1.0f);
                    float size = symbolSize * 0.5f;
                    glBegin( GL_QUADS );
                        glTexCoord2f( col, row);
                        glVertex2f( x - size, y - size);
                        glTexCoord2f( col+symbolSize, row);
                        glVertex2f( x + size, y - size);
                        glTexCoord2f( col+symbolSize, row+symbolSize);
                        glVertex2f( x + size, y + size);
                        glTexCoord2f( col, row+symbolSize);
                        glVertex2f( x - size, y + size);
                    glEnd();
                }
            }
        }

        // draw aircraft echoes
        bool drawAircraft = _radar_position_node->getBoolValue();

        if (drawAircraft) {
            float col = 3 * symbolSize;
            float row = 3 * symbolSize;
            glBindTexture(GL_TEXTURE_2D, wxEcho->getHandle());     // use this for different texture

            for (iradarEcho = radarEcho->begin(); iradarEcho != radarEcho->end();
                    ++iradarEcho) {

                if (!iradarEcho->aircraft)
                    continue;

                dist = iradarEcho->dist;
                dist *= range;
                // calculate relative bearing
                float angle = calcRelBearing(iradarEcho->bearing, view_heading);
                float limit = _radar_coverage_node->getFloatValue();

                if (limit > 180)
                    limit = 180;
                else if (limit < 0)
                    limit = 0;

                // if it's in coverage, draw it
                if (angle >= limit * SG_DEGREES_TO_RADIANS
                        || angle < -limit * SG_DEGREES_TO_RADIANS)
                    continue;

                size = symbolSize * iradarEcho->radius;
                float x = sin(angle) * dist;
                float y = cos(angle) * dist;
                float a_rot_x = sin(angle + (0.25 * SG_PI));
                float a_rot_y = cos(angle + (0.25 * SG_PI));
                float a_size_x = a_rot_x * size;
                float a_size_y = a_rot_y * size;

                glColor3f(1.0f, 1.0f, 1.0f);
                glBegin(GL_QUADS);
                glTexCoord2f(col, row);
                glVertex2f(x - a_size_x, y - a_size_y);
                glTexCoord2f(col + symbolSize, row);
                glVertex2f(x + a_size_y, y - a_size_x);
                glTexCoord2f(col + symbolSize, row + symbolSize);
                glVertex2f(x + a_size_x, y + a_size_y);
                glTexCoord2f(col, row + symbolSize);
                glVertex2f(x - a_size_y, y + a_size_x);
                glEnd();

                SG_LOG(SG_GENERAL, SG_DEBUG, "Radar: drawing AI"
                        << " ID " << iradarEcho->cloudId
                        << " x,y " << x << ","<< y
                        << " dist" << dist
                        << " view_heading" << view_heading / SG_DEGREES_TO_RADIANS
                        << " heading " << iradarEcho->heading / SG_DEGREES_TO_RADIANS
                        << " angle " << angle / SG_DEGREES_TO_RADIANS);
            }
        }

        // draw data
        bool drawData = _radar_data_node->getBoolValue();

        if (drawData) {
            float col = 0 * symbolSize;
            float row = 3 * symbolSize;
            glBindTexture(GL_TEXTURE_2D, wxEcho->getHandle());     // use this for different texture

            for (iradarEcho = radarEcho->begin(); iradarEcho != radarEcho->end();
                    ++iradarEcho) {

                if (!iradarEcho->aircraft)
                    continue;

                dist = iradarEcho->dist * range;
                // calculate relative bearing
                float angle = calcRelBearing(iradarEcho->bearing, view_heading);

                if (angle < 120 * SG_DEGREES_TO_RADIANS
                        && angle > -120 * SG_DEGREES_TO_RADIANS) {   // if it's in coverage, draw it
                    size = symbolSize * 500;
                    float x = sin(angle) * dist;
                    float y = cos(angle) * dist;
                    float d_rot_x = sin(iradarEcho->heading + (0.25 * SG_PI));
                    float d_rot_y = cos(iradarEcho->heading + (0.25 * SG_PI));
                    float d_size_x = d_rot_x * size;
                    float d_size_y = d_rot_y * size;

                    glColor3f(1.0f, 1.0f, 1.0f);
                    glBegin(GL_QUADS);
                        glTexCoord2f(col, row);
                        glVertex2f(x - d_size_x, y - d_size_y);
                        glTexCoord2f(col + symbolSize, row);
                        glVertex2f(x + d_size_y, y - d_size_x);
                        glTexCoord2f(col + symbolSize, row + symbolSize);
                        glVertex2f(x + d_size_x, y + d_size_y);
                        glTexCoord2f(col, row + symbolSize);
                        glVertex2f(x - d_size_y, y + d_size_x);
                    glEnd();

                    SG_LOG(SG_GENERAL, SG_DEBUG, "Radar: drawing data"
                            << " ID " << iradarEcho->cloudId
                            << " x,y " << x <<","<< y
                            << " view_heading " << view_heading / SG_DEGREES_TO_RADIANS
                            << " bearing " << angle / SG_DEGREES_TO_RADIANS
                            << " dist" << dist
                            << " heading " << iradarEcho->heading / SG_DEGREES_TO_RADIANS);
                }
            }
        }

        // draw TACAN symbol
        int mode = _radar_mode_control_node->getIntValue();
        bool inRange = _tacan_in_range_node->getBoolValue();

        if (mode == 1 && inRange) {
            float col = 1 * symbolSize;
            float row = 3 * symbolSize;
            glBindTexture(GL_TEXTURE_2D, wxEcho->getHandle());     // use this for different texture

            dist = _tacan_distance_node->getFloatValue() * SG_NM_TO_METER;
            dist *= range;
            // calculate relative bearing
            float angle = calcRelBearing(_tacan_bearing_node->getFloatValue()
                    * SG_DEGREES_TO_RADIANS, view_heading);

            // it's always in coverage, so draw it
            size = symbolSize * 500;
            float x = sin(angle) * dist;
            float y = cos(angle) * dist;

            SG_LOG(SG_GENERAL, SG_DEBUG, "Radar: drawing TACAN"
                    << " dist" << dist
                    << " view_heading " << view_heading / SG_DEGREES_TO_RADIANS
                    << " heading " << _tacan_bearing_node->getDoubleValue()
                    << " angle " << angle / SG_DEGREES_TO_RADIANS
                    << " x,y " << x <<","<< y
                    << " size " << size);

            glColor3f(1.0f, 1.0f, 1.0f);
            glBegin(GL_QUADS);
                glTexCoord2f(col, row);
                glVertex2f(x - size, y - size);
                glTexCoord2f(col + symbolSize, row);
                glVertex2f(x + size, y - size);
                glTexCoord2f(col + symbolSize, row + symbolSize);
                glVertex2f(x + size, y + size);
                glTexCoord2f(col, row + symbolSize);
                glVertex2f(x - size, y + size);
            glEnd();
        }

        //draw aircraft symbol
        float col = 2 * symbolSize;
        float row = 3 * symbolSize;

        glBindTexture(GL_TEXTURE_2D, wxEcho->getHandle());     // use this for different texture
        size = symbolSize * 500;
        view_heading = get_heading() * SG_DEGREES_TO_RADIANS;

        if (display_mode == "map")
            glTranslatef(range, range, 0.0f);

        float x = sin(view_heading);
        float y = cos(view_heading);
        float s_rot_x = sin(view_heading + (0.25 * SG_PI));
        float s_rot_y = cos(view_heading + (0.25 * SG_PI));
        float s_size_x = s_rot_x * size;
        float s_size_y = s_rot_y * size;

        glColor3f(1.0f, 1.0f, 1.0f);
        glBegin(GL_QUADS);
            glTexCoord2f(col, row);
            glVertex2f(x - s_size_x, y - s_size_y);
            glTexCoord2f(col + symbolSize, row);
            glVertex2f(x + s_size_y, y - s_size_x);
            glTexCoord2f(col + symbolSize, row + symbolSize);
            glVertex2f(x + s_size_x, y + s_size_y);
            glTexCoord2f(col, row + symbolSize);
            glVertex2f(x - s_size_y, y + s_size_x);
        glEnd();


        /*SG_LOG(SG_GENERAL, SG_DEBUG, "Radar: drawing symbol"
                << " ID " << iradarEcho->cloudId
                << " x,y " << x <<","<< y
                << " dist" << dist
                << " view_heading" << view_heading / SG_DEGREES_TO_RADIANS
                << " heading " << iradarEcho->heading / SG_DEGREES_TO_RADIANS
                << " angle " << "angle / SG_DEGREES_TO_RADIANS");*/


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

        float xOffset = 256.0f, yOffset = 180.0f;
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
    // and finally!
    glPopMatrix();
    _odg->endCapture( resultTexture->getHandle() );
}

void
wxRadarBg::updateRadar()
{
    bool ai_enabled = _ai_enabled_node->getBoolValue();

    if (!ai_enabled)
        return;

    double radius[] = {0, 1, 1.5, 1.5, 0.001, 0.1, 1.5, 2, 1.5, 1.5};
    bool isDetected = false;

    SG_LOG(SG_GENERAL, SG_DEBUG, "Radar: Loading AI submodels ");
    _radar_list = _ai->get_ai_list();

    if (_radar_list.empty()) {
        SG_LOG(SG_GENERAL, SG_DEBUG, "Radar: Unable to read AI submodel list");
        return;
    }

    SG_LOG(SG_GENERAL, SG_DEBUG, "Radar: AI submodel list size" << _radar_list.size());

    double user_alt = _user_alt_node->getDoubleValue();
    double user_lat = _user_lat_node->getDoubleValue();
    double user_lon = _user_lon_node->getDoubleValue();

    radar_list_iterator radar_list_itr = _radar_list.begin();
    radar_list_iterator end = _radar_list.end();

    while (radar_list_itr != end) {
        double range   = (*radar_list_itr)->_getRange();
        double bearing = (*radar_list_itr)->_getBearing();
        double lat     = (*radar_list_itr)->_getLatitude();
        double lon     = (*radar_list_itr)->_getLongitude();
        double alt     = (*radar_list_itr)->_getAltitude();
        double heading = (*radar_list_itr)->_getHeading();
        int id         = (*radar_list_itr)->getID();
        int type       = (*radar_list_itr)->getType();

        //range = calcRange(user_lat, user_lon, lat, lon);
        //bearing = calcBearing(user_lat, user_lon, lat, lon);
        calcRngBrg(user_lat, user_lon, lat, lon, range, bearing);

        SG_LOG(SG_GENERAL, SG_DEBUG, "Radar: AI list size" << _radar_list.size()
                << " type " << type
                << " ID " << id
                << " radar range " << range
                << " bearing " << bearing
                << " alt " << alt);

        bool isVisible = calcRadarHorizon(user_alt, alt, range);
        SG_LOG(SG_GENERAL, SG_DEBUG, "Radar: visible " << isVisible);

        if (isVisible)
            isDetected = calcMaxRange(type, range);

        //(float _heading, float _alt, float _radius, float _dist, double _LWC,
        //bool _lightning, int _cloudId, bool _aircraft)
        if (isDetected)
            _radarEchoBuffer.push_back(SGWxRadarEcho (
                    bearing * SG_DEGREES_TO_RADIANS,
                    alt,
                    radius[type] * 120,
                    range * SG_NM_TO_METER,
                    heading * SG_DEGREES_TO_RADIANS,
                    1,
                    false,
                    id,
                    true));

        ++radar_list_itr;
    }
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


    double sigma[] = {0, 1, 100, 100, 0.001, 0.1, 100, 100, 1, 1};
    double constant = _radar_ref_rng_node->getDoubleValue();

    if (constant <= 0)
        constant = 35;

    double maxrange = constant * pow(sigma[type], 0.25);

    SG_LOG(SG_GENERAL, SG_DEBUG, "Radar: max range " << maxrange);

    return maxrange >= range;
}

void
wxRadarBg::calcRngBrg(double lat, double lon, double lat2, double lon2, double &range,
                       double &bearing ) const
{
    double az2, distance;

    // calculate the bearing and range of the second pos from the first
    geo_inverse_wgs_84(lat, lon, lat2, lon2, &bearing, &az2, &distance);

    range = distance *= SG_METER_TO_NM;
}

float
wxRadarBg::calcRelBearing(float bearing, float heading)
{
    float angle = bearing - heading;

    if (angle > SG_PI)
        angle -= 2.0*SG_PI;

    if (angle < -SG_PI)
        angle += 2.0*SG_PI;

    return angle;
}

