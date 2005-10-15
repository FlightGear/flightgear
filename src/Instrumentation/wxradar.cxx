// Wx Radar background texture
//
// Written by Harald JOHNSEN, started May 2005.
//
// Copyright (C) 2005  Harald JOHNSEN - hjohnsen@evc.net
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
// Foundation, 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA
//
//

#include <plib/sg.h>
#include <plib/ssg.h>
#include <Main/fg_props.hxx>
#include <Main/globals.hxx>
#include <Cockpit/panel.hxx>
#include <Cockpit/hud.hxx>

#include <simgear/constants.h>
#include <simgear/misc/sg_path.hxx>
#include <simgear/environment/visual_enviro.hxx>
#include "instrument_mgr.hxx"
#include "od_gauge.hxx"
#include "wxradar.hxx"

// texture name to use in 2D and 3D instruments
static const char *odgauge_name = "Aircraft/Instruments/Textures/od_wxradar.rgb";

wxRadarBg::wxRadarBg ( SGPropertyNode *node) :
    name("wxRadar"),
    num(0),
    resultTexture( 0 ),
    wxEcho( 0 ),
    last_switchKnob( "off" ),
    sim_init_done ( false ),
    odg( 0 )
{
    int i;
    for ( i = 0; i < node->nChildren(); ++i ) {
        SGPropertyNode *child = node->getChild(i);
        string cname = child->getName();
        string cval = child->getStringValue();
        if ( cname == "name" ) {
            name = cval;
        } else if ( cname == "number" ) {
            num = child->getIntValue();
        } else {
            SG_LOG( SG_INSTR, SG_WARN, "Error in wxRadar config logic" );
            if ( name.length() ) {
                SG_LOG( SG_INSTR, SG_WARN, "Section = " << name );
            }
        }
    }
}

wxRadarBg::wxRadarBg ()
{
}

wxRadarBg::~wxRadarBg ()
{
    ssgDeRefDelete(resultTexture);
    ssgDeRefDelete(wxEcho);
}

void
wxRadarBg::init ()
{
    string branch;
    branch = "/instrumentation/" + name;

    _Instrument = fgGetNode(branch.c_str(), num, true );
    _serviceable_node = _Instrument->getChild("serviceable", 0, true);
    resultTexture = FGTextureManager::createTexture( odgauge_name );
    resultTexture->ref();
    SGPath tpath(globals->get_fg_root());
    tpath.append("Aircraft/Instruments/Textures/wxecho.rgb");
    // no mipmap or else alpha will mix with pixels on the border of shapes, ruining the effect
    wxEcho = new ssgTexture( tpath.c_str(), false, false, false);
    wxEcho->ref();

    _Instrument->setFloatValue("trk", 0.0);
    _Instrument->setFloatValue("tilt", 0.0);
    _Instrument->setStringValue("status","");
    // those properties are used by a radar instrument of a MFD
    // input switch = OFF | TST | STBY | ON
    // input mode = WX | WXA | MAP
    // ouput status = STBY | TEST | WX | WXA | MAP | blank
    // input lightning = true | false
    // input TRK = +/- n degrees
    // input TILT = +/- n degree
    // input autotilt = true | false
    // input range = n nm (20/40/80)
    // input display-mode = arc | rose | map | plan

    FGInstrumentMgr *imgr = (FGInstrumentMgr *) globals->get_subsystem("instrumentation");
    odg = (FGODGauge *) imgr->get_subsystem("od_gauge");
}

void
wxRadarBg::update (double delta_time_sec)
{
    if( ! sim_init_done ) {
        if( ! fgGetBool("sim/sceneryloaded", false) )
            return;
        sim_init_done = true;
    }
    if ( !odg || ! _serviceable_node->getBoolValue() ) {
        _Instrument->setStringValue("status","");
        return;
    }
    string switchKnob = _Instrument->getStringValue("switch", "on");
    string modeButton = _Instrument->getStringValue("mode", "wx");
    bool drawLightning = _Instrument->getBoolValue("lightning", true);
    float range_nm = _Instrument->getFloatValue("range", 40.0);
    float range_m = range_nm * SG_NM_TO_METER;

    if( last_switchKnob != switchKnob ) {
        // since 3D models don't share textures with the rest of the world
        // we must locate them and replace their handle by hand
        // only do that when the instrument is turned on
        if( last_switchKnob == "off" )
            odg->set_texture( odgauge_name, resultTexture->getHandle());
        last_switchKnob = switchKnob;
    }
    odg->beginCapture(256);
    odg->Clear();
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glPushMatrix();
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        glBindTexture(GL_TEXTURE_2D, 0);
        if( switchKnob == "off" ) {
            _Instrument->setStringValue("status","");
        } else if( switchKnob == "stby" ) {
            _Instrument->setStringValue("status","STBY");
        } else if( switchKnob == "tst" ) {
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
            if( display_mode == "arc" ) {
                glTranslatef(0.0f, -180.0f, 0.0f);
                range = 2*180.0f / range_nm;
            } else if( display_mode == "map" ) {
//                float view_heading = get_track() * SG_DEGREES_TO_RADIANS;
            } else if( display_mode == "plan" ) {
                // no sense I presume
                float view_heading = 0.0;
            } else {
                // rose
            }
            range /= SG_NM_TO_METER;
            // we will rotate the echo quads, this gives a better rendering
            const float rot_x = cos ( view_heading );
            const float rot_y = sin ( view_heading );

            list_of_SGWxRadarEcho *radarEcho = sgEnviro.get_radar_echo();
            list_of_SGWxRadarEcho::iterator iradarEcho;
            const float LWClevel[] = { 0.1f, 0.5f, 2.1f };
            const float symbolSize = 1.0f / 8.0f ;
            // draw the radar echo, we do that in 3 passes, one for each color level
            // this is to 'merge' same colors together
            glBindTexture(GL_TEXTURE_2D, wxEcho->getHandle() );
            glColor3f(1.0f, 1.0f, 1.0f);
            glBegin( GL_QUADS );

            for(int level = 0; level <= 2 ; level++ ) {
                float col = level * symbolSize;
                for(iradarEcho = radarEcho->begin() ; iradarEcho != radarEcho->end() ; iradarEcho++ ) {
                    int cloudId = (iradarEcho->cloudId) ;
                    bool upgrade = ((cloudId >> 5) & 1);
                    float lwc = iradarEcho->LWC + (upgrade ? 1.0f : 0.0f);
                    // skip ns
                    if( iradarEcho->LWC >= 0.5 && iradarEcho->LWC <= 0.6)
                        continue;
                    if( (! iradarEcho->lightning) && ( lwc >= LWClevel[level]) ) {
                        float dist = sgSqrt( iradarEcho->dist );
                        float size = iradarEcho->radius * 2.0;
                        if( dist - size > range_m )
                            continue;
                        dist = dist * range;
                        size = size * range;
                        // compute the relative angle from the view direction
                        float angle = ( view_heading + iradarEcho->heading );
                        if( angle > SG_PI )
                            angle -= 2.0*SG_PI;
                        if( angle < - SG_PI )
                            angle += 2.0*SG_PI;
                        // and apply a fov factor to simulate a greater scan angle
                        angle =  angle * fovFactor + SG_PI / 2.0;
                        float x = cos( angle ) * dist;
                        float y = sin( angle ) * dist;
                        // use different shapes so the display is less boring
                        float row = symbolSize * (float) (4 + (cloudId & 3) );
                        float size_x = rot_x * size;
                        float size_y = rot_y * size;
                        glTexCoord2f( col, row);
                        glVertex2f( x - size_x, y - size_y);
                        glTexCoord2f( col+symbolSize, row);
                        glVertex2f( x + size_y, y - size_x);
                        glTexCoord2f( col+symbolSize, row+symbolSize);
                        glVertex2f( x + size_x, y + size_y);
                        glTexCoord2f( col, row+symbolSize);
                        glVertex2f( x - size_y, y + size_x);
                    }
                }
            }
            glEnd(); // GL_QUADS

            // draw lightning echos
            if( drawLightning ) {
                float col = 3 * symbolSize;
                float row = 4 * symbolSize;
                for(iradarEcho = radarEcho->begin() ; iradarEcho != radarEcho->end() ; iradarEcho++ ) {
                    if( iradarEcho->lightning ) {
                        float dist = iradarEcho->dist;
                        dist = dist * range;
                        float angle = (view_heading - iradarEcho->heading);
                        if( angle > SG_PI )
                            angle -= 2.0*SG_PI;
                        if( angle < - SG_PI )
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
            float yOffset = 180.0f, xOffset = 256.0f;
            if( display_mode != "arc" ) {
                yOffset = 40.0f;
                xOffset = 240.0f;
            }
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
//            glColor4f(0.0f, 1.0f, 0.0f, 1.0f);
            glDisable(GL_ALPHA_TEST);
            glBindTexture(GL_TEXTURE_2D, 0);

            glBegin( GL_TRIANGLES );
                glVertex2f(0.0, 0.0);
                glVertex2f(-256.0, 0.0);
                glVertex2f(-256.0, 256.0);

                glVertex2f(0.0, 0.0);
                glVertex2f(256.0, 0.0);
                glVertex2f(256.0, 256.0);

                glVertex2f(-256, 0.0);
                glVertex2f(256.0, 0.0);
                glVertex2f(-256.0, -256.0);

                glVertex2f(256, 0.0);
                glVertex2f(256.0, -256.0);
                glVertex2f(-256.0, -256.0);
            glEnd();

            // DEBUG only
/*            glColor4f(1.0f, 0.0f, 0.0f, 1.0f);
            glBegin( GL_LINES );
                glVertex2f(0.0, 0.0);
                glVertex2f(-256.0, 256.0);
                glVertex2f(0.0, 0.0);
                glVertex2f(256.0, 256.0);
            glEnd();*/

            glEnable(GL_BLEND);
            glEnable(GL_ALPHA_TEST);
        }
    glPopMatrix();
    odg->endCapture( resultTexture->getHandle() );
}
