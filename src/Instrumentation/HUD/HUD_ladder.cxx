// HUD_ladder.cxx -- HUD Ladder Instrument
//
// Written by Michele America, started September 1997.
//
// Copyright (C) 1997  Michele F. America  [micheleamerica#geocities:com]
// Copyright (C) 2006  Melchior FRANZ  [mfranz#aon:at]
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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <sstream>
#include <simgear/math/SGGeometry.hxx>
#include <Viewer/viewer.hxx>
#include "HUD.hxx"

// FIXME
static float get__heading() { return fgGetFloat("/orientation/heading-deg") * M_PI / 180.0; }
static float get__throttleval() { return fgGetFloat("/controls/engines/engine/throttle"); }
static float get__Vx() { return fgGetFloat("/velocities/uBody-fps"); }
static float get__Vy() { return fgGetFloat("/velocities/vBody-fps"); }
static float get__Vz() { return fgGetFloat("/velocities/wBody-fps"); }
static float get__Ax() { return fgGetFloat("/accelerations/pilot/x-accel-fps_sec"); }
static float get__Ay() { return fgGetFloat("/accelerations/pilot/y-accel-fps_sec"); }
static float get__Az() { return fgGetFloat("/accelerations/pilot/z-accel-fps_sec"); }
static float get__alpha() { return fgGetFloat("/orientation/alpha-deg"); }
static float get__beta() { return fgGetFloat("/orientation/side-slip-deg"); }
#undef ENABLE_SP_FDM


HUD::Ladder::Ladder(HUD *hud, const SGPropertyNode *n, float x, float y) :
    Item(hud, n, x, y),
    _pitch(n->getNode("pitch-input", false)),
    _roll(n->getNode("roll-input", false)),
    _width_units(int(n->getFloatValue("display-span"))),
    _div_units(int(fabs(n->getFloatValue("divisions")))),
    _scr_hole(fabs(n->getFloatValue("screen-hole")) * 0.5f),
    _zero_bar_overlength(n->getFloatValue("zero-bar-overlength", 10)),
    _dive_bar_angle(n->getBoolValue("enable-dive-bar-angle")),
    _tick_length(n->getFloatValue("tick-length")),
    _compression(n->getFloatValue("compression-factor")),
    _dynamic_origin(n->getBoolValue("enable-dynamic-origin")),
    _frl(n->getBoolValue("enable-fuselage-ref-line")),
    _target_spot(n->getBoolValue("enable-target-spot")),
    _target_markers(n->getBoolValue("enable-target-markers")),
    _velocity_vector(n->getBoolValue("enable-velocity-vector")),
    _drift_marker(n->getBoolValue("enable-drift-marker")),
    _alpha_bracket(n->getBoolValue("enable-alpha-bracket")),
    _energy_marker(n->getBoolValue("enable-energy-marker")),
    _climb_dive_marker(n->getBoolValue("enable-climb-dive-marker")),
    _glide_slope_marker(n->getBoolValue("enable-glide-slope-marker")),
    _glide_slope(n->getFloatValue("glide-slope", -4.0)),
    _energy_worm(n->getBoolValue("enable-energy-marker")),
    _waypoint_marker(n->getBoolValue("enable-waypoint-marker")),
    _zenith(n->getBoolValue("enable-zenith")),
    _nadir(n->getBoolValue("enable-nadir")),
    _hat(n->getBoolValue("enable-hat")),
    _clip_box(new ClipBox(n->getNode("clipping")))
{
    const char *t = n->getStringValue("type");
    _type = strcmp(t, "climb-dive") ? PITCH : CLIMB_DIVE;

    if (!_width_units)
        _width_units = 45;

    _vmax = _width_units / 2;
    _vmin = -_vmax;
}


HUD::Ladder::~Ladder()
{
    delete _clip_box;
}

void HUD::Ladder::draw(void)
{
    if (!_pitch.isValid() || !_roll.isValid())
        return;

    float roll_value = _roll.getFloatValue() * SGD_DEGREES_TO_RADIANS;
    float pitch_value = _pitch.getFloatValue();

    //**************************************************************
    glPushMatrix();
    glTranslatef(_center_x, _center_y, 0);

    // OBJECT STATIC RETICLE
    // TYPE FRL (FUSELAGE REFERENCE LINE)
    // ATTRIB - ALWAYS
    // Draw the FRL spot and line
    if (_frl) {
#define FRL_DIAMOND_SIZE 2.0
        glBegin(GL_LINE_LOOP);
        glVertex2f(-FRL_DIAMOND_SIZE, 0.0);
        glVertex2f(0.0, FRL_DIAMOND_SIZE);
        glVertex2f(FRL_DIAMOND_SIZE, 0.0);
        glVertex2f(0.0, -FRL_DIAMOND_SIZE);
        glEnd();

        glBegin(GL_LINE_STRIP);
        glVertex2f(0, FRL_DIAMOND_SIZE);
        glVertex2f(0, 8.0);
        glEnd();
#undef FRL_DIAMOND_SIZE
    }
    // TYPE WATERLINE_MARK (W shaped _    _ )		// TODO (-> HUD_misc.cxx)
    //                                \/\/

    //****************************************************************
    // TYPE TARGET_SPOT
    // Draw the target spot.
    if (_target_spot) {
#define CENTER_DIAMOND_SIZE 6.0
        glBegin(GL_LINE_LOOP);
        glVertex2f(-CENTER_DIAMOND_SIZE, 0.0);
        glVertex2f(0.0, CENTER_DIAMOND_SIZE);
        glVertex2f(CENTER_DIAMOND_SIZE, 0.0);
        glVertex2f(0.0, -CENTER_DIAMOND_SIZE);
        glEnd();
#undef CENTER_DIAMOND_SIZE
    }

    //****************************************************************
    //velocity vector reticle - computations
    float xvvr, /* yvvr, */ Vxx = 0.0, Vyy = 0.0, Vzz = 0.0;
    float Axx = 0.0, Ayy = 0.0, Azz = 0.0, total_vel = 0.0, pot_slope; //, t1;
    float up_vel, ground_vel, actslope = 0.0, psi = 0.0;
    float vel_x = 0.0, vel_y = 0.0, drift;
    float alpha;

    if (_velocity_vector) {
        drift = get__beta();
        alpha = get__alpha();

        Vxx = get__Vx();
        Vyy = get__Vy();
        Vzz = get__Vz();
        Axx = get__Ax();
        Ayy = get__Ay();
        Azz = get__Az();
        psi = get__heading();

        if (psi > 180.0)
            psi = psi - 360;

        total_vel = sqrt(Vxx * Vxx + Vyy * Vyy + Vzz * Vzz);
        ground_vel = sqrt(Vxx * Vxx + Vyy * Vyy);
        up_vel = Vzz;

        if (ground_vel < 2.0) {
            if (fabs(up_vel) < 2.0)
                actslope = 0.0;
            else
                actslope = (up_vel / fabs(up_vel)) * 90.0;

        } else {
            actslope = atan(up_vel / ground_vel) * SGD_RADIANS_TO_DEGREES;
        }

        xvvr = (-drift * (_compression / globals->get_current_view()->get_aspect_ratio()));
        // drift = ((atan2(Vyy, Vxx) * SGD_RADIANS_TO_DEGREES) - psi);
        // yvvr = (-alpha * _compression);
        // vel_y = (-alpha * cos(roll_value) + drift * sin(roll_value)) * _compression;
        // vel_x = (alpha * sin(roll_value) + drift * cos(roll_value))
        //         * (_compression / globals->get_current_view()->get_aspect_ratio());
        vel_y = -alpha * _compression;
        vel_x = -drift * (_compression / globals->get_current_view()->get_aspect_ratio());
        //  printf("%f %f %f %f\n",vel_x, vel_y, drift, psi);

        //****************************************************************
        // OBJECT MOVING RETICLE
        // TYPE - DRIFT MARKER
        // ATTRIB - ALWAYS
        // drift marker
        if (_drift_marker) {
            glBegin(GL_LINE_STRIP);
            glVertex2f((xvvr * 25 / 120) - 6, -4);
            glVertex2f(xvvr * 25 / 120, 8);
            glVertex2f((xvvr * 25 / 120) + 6, -4);
            glEnd();
        }

        //****************************************************************
        // OBJECT MOVING RETICLE
        // TYPE VELOCITY VECTOR
        // ATTRIB - ALWAYS
        // velocity vector
        draw_circle(vel_x, vel_y, 6);

        //velocity vector reticle orientation lines
        glBegin(GL_LINE_STRIP);
        glVertex2f(vel_x - 12, vel_y);
        glVertex2f(vel_x - 6, vel_y);
        glEnd();
        glBegin(GL_LINE_STRIP);
        glVertex2f(vel_x + 12, vel_y);
        glVertex2f(vel_x + 6, vel_y);
        glEnd();
        glBegin(GL_LINE_STRIP);
        glVertex2f(vel_x, vel_y + 12);
        glVertex2f(vel_x, vel_y + 6);
        glEnd();

#ifdef ENABLE_SP_FDM
        int lgear = get__iaux3();
        int ihook = get__iaux6();

        // OBJECT MOVING RETICLE
        // TYPE LINE
        // ATTRIB - ON CONDITION
        if (lgear == 1) {
            // undercarriage status
            glBegin(GL_LINE_STRIP);
            glVertex2f(vel_x + 8, vel_y);
            glVertex2f(vel_x + 8, vel_y - 4);
            glEnd();

            // OBJECT MOVING RETICLE
            // TYPE LINE
            // ATTRIB - ON CONDITION
            glBegin(GL_LINE_STRIP);
            glVertex2f(vel_x - 8, vel_y);
            glVertex2f(vel_x - 8, vel_y - 4);
            glEnd();

            // OBJECT MOVING RETICLE
            // TYPE LINE
            // ATTRIB - ON CONDITION
            glBegin(GL_LINE_STRIP);
            glVertex2f(vel_x, vel_y - 6);
            glVertex2f(vel_x, vel_y - 10);
            glEnd();
        }

        // OBJECT MOVING RETICLE
        // TYPE V
        // ATTRIB - ON CONDITION
        if (ihook == 1) {
            // arrestor hook status
            glBegin(GL_LINE_STRIP);
            glVertex2f(vel_x - 4, vel_y - 8);
            glVertex2f(vel_x, vel_y - 10);
            glVertex2f(vel_x + 4, vel_y - 8);
            glEnd();
        }
#endif
    } // if _velocity_vector

    // draw hud markers on top of each AI/MP target
    if (_target_markers) {
        SGPropertyNode *models = globals->get_props()->getNode("/ai/models", true);
        for (int i = 0; i < models->nChildren(); i++) {
            SGPropertyNode *chld = models->getChild(i);
            string name;
            name = chld->getName();
            if (name == "aircraft" || name == "multiplayer") {
                string callsign = chld->getStringValue("callsign");
                if (callsign != "") {
                    float h_deg = chld->getFloatValue("radar/h-offset");
                    float v_deg = chld->getFloatValue("radar/v-offset");
                    float pos_x = (h_deg * cos(roll_value) -
                                   v_deg * sin(roll_value)) * _compression;
                    float pos_y = (v_deg * cos(roll_value) +
                                   h_deg * sin(roll_value)) * _compression;
                    draw_circle(pos_x, pos_y, 8);
                }
            }
        }
    }

    //***************************************************************
    // OBJECT MOVING RETICLE
    // TYPE - SQUARE_BRACKET
    // ATTRIB - ON CONDITION
    // alpha bracket
#ifdef ENABLE_SP_FDM
    alpha = get__alpha();

    if (_alpha_bracket && ihook == 1) {
        glBegin(GL_LINE_STRIP);
        glVertex2f(vel_x - 20, vel_y - (16 - alpha) * _compression);
        glVertex2f(vel_x - 17, vel_y - (16 - alpha) * _compression);
        glVertex2f(vel_x - 17, vel_y - (14 - alpha) * _compression);
        glVertex2f(vel_x - 20, vel_y - (14 - alpha) * _compression);
        glEnd();

        glBegin(GL_LINE_STRIP);
        glVertex2f(vel_x + 20, vel_y - (16 - alpha) * _compression);
        glVertex2f(vel_x + 17, vel_y - (16 - alpha) * _compression);
        glVertex2f(vel_x + 17, vel_y - (14 - alpha) * _compression);
        glVertex2f(vel_x + 20, vel_y - (14 - alpha) * _compression);
        glEnd();
    }
#endif
    //printf("xvr=%f, yvr=%f, Vx=%f, Vy=%f, Vz=%f\n",xvvr, yvvr, Vx, Vy, Vz);
    //printf("Ax=%f, Ay=%f, Az=%f\n",Ax, Ay, Az);

    //****************************************************************
    // OBJECT MOVING RETICLE
    // TYPE ENERGY_MARKERS
    // ATTRIB - ALWAYS
    //energy markers - compute potential slope
    float pla = get__throttleval();
    float t2 = 0.0;

    if (_energy_marker) {
        if (total_vel < 5.0) {
//            t1 = 0;
            t2 = 0;
        } else {
//            t1 = up_vel / total_vel;
            t2 = asin((Vxx * Axx + Vyy * Ayy + Vzz * Azz) / (9.81 * total_vel));
        }
        pot_slope = ((t2 / 3) * SGD_RADIANS_TO_DEGREES) * _compression + vel_y;
        // if (pot_slope < (vel_y - 45)) pot_slope = vel_y - 45;
        // if (pot_slope > (vel_y + 45)) pot_slope = vel_y + 45;

        //energy markers
        glBegin(GL_LINE_STRIP);
        glVertex2f(vel_x - 20, pot_slope - 5);
        glVertex2f(vel_x - 15, pot_slope);
        glVertex2f(vel_x - 20, pot_slope + 5);
        glEnd();

        glBegin(GL_LINE_STRIP);
        glVertex2f(vel_x + 20, pot_slope - 5);
        glVertex2f(vel_x + 15, pot_slope);
        glVertex2f(vel_x + 20, pot_slope + 5);
        glEnd();

        if (pla > (105.0 / 131.0)) {
            glBegin(GL_LINE_STRIP);
            glVertex2f(vel_x - 24, pot_slope - 5);
            glVertex2f(vel_x - 19, pot_slope);
            glVertex2f(vel_x - 24, pot_slope + 5);
            glEnd();

            glBegin(GL_LINE_STRIP);
            glVertex2f(vel_x + 24, pot_slope - 5);
            glVertex2f(vel_x + 19, pot_slope);
            glVertex2f(vel_x + 24, pot_slope + 5);
            glEnd();
        }
    }

    //**********************************************************
    // ramp reticle
    // OBJECT STATIC RETICLE
    // TYPE LINE
    // ATTRIB - ON CONDITION
#ifdef ENABLE_SP_FDM
    int ilcanclaw = get__iaux2();

    if (_energy_worm && ilcanclaw == 1) {
        glBegin(GL_LINE_STRIP);
        glVertex2f(-15, -134);
        glVertex2f(15, -134);
        glEnd();

        // OBJECT MOVING RETICLE
        // TYPE BOX
        // ATTRIB - ON CONDITION
        glBegin(GL_LINE_STRIP);
        glVertex2f(-6, -134);
        glVertex2f(-6, t2 * SGD_RADIANS_TO_DEGREES * 4.0 - 134);
        glVertex2f(+6, t2 * SGD_RADIANS_TO_DEGREES * 4.0 - 134);
        glVertex2f(6, -134);
        glEnd();

        // OBJECT MOVING RETICLE
        // TYPE DIAMOND
        // ATTRIB - ON CONDITION
        glBegin(GL_LINE_LOOP);
        glVertex2f(-6, actslope * 4.0 - 134);
        glVertex2f(0, actslope * 4.0 -134 + 3);
        glVertex2f(6, actslope * 4.0 - 134);
        glVertex2f(0, actslope * 4.0 -134 -3);
        glEnd();
    }
#endif

    //*************************************************************
    // OBJECT MOVING RETICLE
    // TYPE DIAMOND
    // ATTRIB - ALWAYS
    // Draw the locked velocity vector.
    if (_climb_dive_marker) {
        glBegin(GL_LINE_LOOP);
        glVertex2f(-3.0, 0.0 + vel_y);
        glVertex2f(0.0, 6.0 + vel_y);
        glVertex2f(3.0, 0.0 + vel_y);
        glVertex2f(0.0, -6.0 + vel_y);
        glEnd();
    }

    //****************************************************************

    _clip_box->set();

    if (_dynamic_origin) {
        // ladder moves with alpha/beta offset projected onto horizon
        // line (so that the horizon line always aligns with the
        // actual horizon.
        _vmin = pitch_value - _width_units * 0.5f;
        _vmax = pitch_value + _width_units * 0.5f;
        {
            // the hud ladder center point should move relative to alpha/beta
            // however the horizon line should always stay on the horizon.  We
            // project the alpha/beta offset onto the horizon line to get the
            // result we want.
            
            SGVec3d d(cos(roll_value), sin(roll_value), 0.0);
            SGRayd r(SGVec3d::zeros(), d);
            SGVec3d p = r.getClosestPointTo(SGVec3d(vel_x, vel_y, 0.0));
            glTranslatef(p[0], p[1], 0);
        }
    } else {
        // ladder position is fixed relative to the center of the screen.
        _vmin = pitch_value - _width_units * 0.5f;
        _vmax = pitch_value + _width_units * 0.5f;
    }

    glRotatef(roll_value * SGD_RADIANS_TO_DEGREES, 0.0, 0.0, 1.0);

    // FRL marker not rotated - this line shifted below
    float half_span = _w * 0.5f;
    float y = 0;
    struct { float x, y; } lo, li, ri, ro, numoffs;  // left/right inner/outer

    if (_div_units) {
        _locTextList.setFont(_hud->_font_renderer);
        _locTextList.erase();
        _locLineList.erase();
        _locStippleLineList.erase();

        for (int i = int(_vmin); i < int(_vmax) + 1; i++) {
            if (i % _div_units)
                continue;

            if (_type == PITCH)
                y = float(i - pitch_value) * _compression + .5;
            else  // _type == CLIMB_DIVE
                y = float(i - actslope) * _compression + .5;

            // OBJECT LADDER MARK
            // TYPE LINE
            // ATTRIB - ON CONDITION
            // draw approach glide slope marker
#ifdef ENABLE_SP_FDM
            if (_glide_slope_marker && ihook) {
                draw_line(-half_span + 15, (_glide_slope - actslope) * _compression,
                        -half_span + hole, (_glide_slope - actslope) * _compression);
                draw_line(half_span - 15, (_glide_slope - actslope) * _compression,
                        half_span - hole, (_glide_slope - actslope) * _compression);
            }
#endif

            // draw symbols
            if (i == 90 && _zenith)
                draw_zenith(0.0, y);
            else if (i == -90 && _nadir)
                draw_nadir(0.0, y);

            if ((_zenith && i > 85) || i > 90)
                continue;
            if ((_nadir && i < -85) || i < -90)
                continue;

            lo.x = -half_span;
            ro.x = half_span;
            li.x = ri.x = 0;
            lo.y = ro.y = li.y = ri.y = y;
            numoffs.x = 4;
            numoffs.y = 0;

            if (i == 0) {
                lo.x -= _zero_bar_overlength;
                ro.x += _zero_bar_overlength;
            }

            if (_scr_hole > 0.0f) {
                li.x = -_scr_hole;
                ri.x = _scr_hole;

                if (_dive_bar_angle && i < 0) {
                    float alpha = i * SG_DEGREES_TO_RADIANS * 0.5;
                    float xoffs = (ro.x - ri.x) * cos(alpha);
                    float yoffs = (ro.x - ri.x) * sin(alpha);
                    lo.x = li.x - xoffs;
                    ro.x = ri.x + xoffs;
                    lo.y = ro.y = li.y + yoffs;
                    numoffs.x = 0;
                    numoffs.y = 4 - yoffs * 0.3;
                }
            }

            // draw bars
            if (_scr_hole) {
                draw_line(li.x, li.y, lo.x, lo.y, i < 0);
                draw_line(ri.x, ri.y, ro.x, ro.y, i < 0);
            } else {
                draw_line(lo.x, lo.y, ro.x, ro.y, i < 0);
            }

            // draw ticks
            if (_tick_length) {
                if (i < 0) {
                    draw_line(li.x, li.y, li.x, li.y + _tick_length);
                    draw_line(ri.x, ri.y, ri.x, ri.y + _tick_length);
                } else if (i > 0 || _zero_bar_overlength == 0) {
                    if (_tick_length > 0) {
                        numoffs.x = -0.3;
                        numoffs.y = -0.3;
                        draw_line(lo.x, lo.y, lo.x, lo.y - _tick_length);
                        draw_line(ro.x, ro.y, ro.x, ro.y - _tick_length);
                    } else {
                        draw_line(li.x, li.y, li.x, li.y - _tick_length);
                        draw_line(ri.x, ri.y, ri.x, ri.y - _tick_length);
                    }
                }
            }

            // draw numbers
            std::ostringstream str;
            str << i;
            // must keep this string, otherwise it will free the c_str!
            string num_str = str.str();
            const char *num = num_str.c_str();
            int valign = numoffs.y > 0 ? BOTTOM : numoffs.y < 0 ? TOP : VCENTER;
            draw_text(lo.x - numoffs.x, lo.y + numoffs.y, num,
                    valign | (numoffs.x == 0 ? CENTER : numoffs.x > 0 ? RIGHT : LEFT));
            draw_text(ro.x + numoffs.x, lo.y + numoffs.y, num,
                    valign | (numoffs.x == 0 ? CENTER : numoffs.x > 0 ? LEFT : RIGHT));

        }
        _locTextList.draw();

        glLineWidth(0.2);

        _locLineList.draw();

        glEnable(GL_LINE_STIPPLE);
        glLineStipple(1, 0x00FF);
        _locStippleLineList.draw();
        glDisable(GL_LINE_STIPPLE);
    }
    _clip_box->unset();
    glPopMatrix();
    //*************************************************************

    //*************************************************************
#ifdef ENABLE_SP_FDM
    if (_waypoint_marker) {
        //waypoint marker computation
        float fromwp_lat, towp_lat, fromwp_lon, towp_lon, dist, delx, dely, hyp, theta, brg;

        fromwp_lon = get__longitude() * SGD_DEGREES_TO_RADIANS;
        fromwp_lat = get__latitude() * SGD_DEGREES_TO_RADIANS;
        towp_lon = get__aux2() * SGD_DEGREES_TO_RADIANS;
        towp_lat = get__aux1() * SGD_DEGREES_TO_RADIANS;

        dist = acos(sin(fromwp_lat) * sin(towp_lat) + cos(fromwp_lat)
                * cos(towp_lat) * cos(fabs(fromwp_lon - towp_lon)));
        delx= towp_lat - fromwp_lat;
        dely = towp_lon - fromwp_lon;
        hyp = sqrt(pow(delx, 2) + pow(dely, 2));

        if (hyp != 0)
            theta = asin(dely / hyp);
        else
            theta = 0.0;

        brg = theta * SGD_RADIANS_TO_DEGREES;
        if (brg > 360.0)
            brg = 0.0;
        if (delx < 0)
            brg = 180 - brg;

        // {Brg  = asin(cos(towp_lat)*sin(fabs(fromwp_lon-towp_lon))/ sin(dist));
        // Brg = Brg * SGD_RADIANS_TO_DEGREES; }

        dist *= SGD_RADIANS_TO_DEGREES * 60.0 * 1852.0; //rad->deg->nm->m
        // end waypoint marker computation

        //*********************************************************
        // OBJECT MOVING RETICLE
        // TYPE ARROW
        // waypoint marker
        if (fabs(brg - psi) > 10.0) {
            glPushMatrix();
            glTranslatef(_center_x, _center_y, 0);
            glTranslatef(vel_x, vel_y, 0);
            glRotatef(brg - psi, 0.0, 0.0, -1.0);
            glBegin(GL_LINE_LOOP);
            glVertex2f(-2.5, 20.0);
            glVertex2f(-2.5, 30.0);
            glVertex2f(-5.0, 30.0);
            glVertex2f(0.0, 35.0);
            glVertex2f(5.0, 30.0);
            glVertex2f(2.5, 30.0);
            glVertex2f(2.5, 20.0);
            glEnd();
            glPopMatrix();
        }

        // waypoint marker on heading scale
        if (fabs(brg - psi) < 12.0) {
            if (!_hat) {
                glBegin(GL_LINE_LOOP);
                GLfloat x = (brg - psi) * 60 / 25;
                glVertex2f(x + 320, 240.0);
                glVertex2f(x + 326, 240.0 - 4);
                glVertex2f(x + 323, 240.0 - 4);
                glVertex2f(x + 323, 240.0 - 8);
                glVertex2f(x + 317, 240.0 - 8);
                glVertex2f(x + 317, 240.0 - 4);
                glVertex2f(x + 314, 240.0 - 4);
                glEnd();

            } else { // if (_hat)
                float x = (brg - psi) * 60 / 25 + 320, y = 240.0, r = 5.0;
                float x1, y1;

                glEnable(GL_POINT_SMOOTH);
                glBegin(GL_POINTS);

                for (int count = 0; count <= 200; count++) {
                    float temp = count * SG_PI * 3 / (200.0 * 2.0);
                    float temp1 = temp - (45.0 * SGD_DEGREES_TO_RADIANS);
                    x1 = x + r * cos(temp1);
                    y1 = y + r * sin(temp1);
                    glVertex2f(x1, y1);
                }

                glEnd();
                glDisable(GL_POINT_SMOOTH);
            }

         } //brg<12
     } // if _waypoint_marker
#endif
}//draw


/******************************************************************/
//  draws the zenith symbol (highest possible climb angle i.e. 90 degree climb angle)
//
void HUD::Ladder::draw_zenith(float x, float y)
{
    draw_line(x - 9.0, y, x - 3.0, y + 1.3);
    draw_line(x - 9.0, y, x - 3.0, y - 1.3);

    draw_line(x + 9.0, y, x + 3.0, y + 1.3);
    draw_line(x + 9.0, y, x + 3.0, y - 1.3);

    draw_line(x, y + 9.0, x - 1.3, y + 3.0);
    draw_line(x, y + 9.0, x + 1.3, y + 3.0);

    draw_line(x - 3.9, y + 3.9, x - 3.0, y + 1.3);
    draw_line(x - 3.9, y + 3.9, x - 1.3, y + 3.0);

    draw_line(x + 3.9, y + 3.9, x + 1.3, y + 3.0);
    draw_line(x + 3.9, y + 3.9, x + 3.0, y + 1.3);

    draw_line(x - 3.9, y - 3.9, x - 3.0, y - 1.3);
    draw_line(x - 3.9, y - 3.9, x - 1.3, y - 2.6);

    draw_line(x + 3.9, y - 3.9, x + 3.0, y - 1.3);
    draw_line(x + 3.9, y - 3.9, x + 1.3, y - 2.6);

    draw_line(x - 1.3, y - 2.6, x, y - 27.0);
    draw_line(x + 1.3, y - 2.6, x, y - 27.0);
}


//  draws the nadir symbol (lowest possible dive angle i.e. 90 degree dive angle))
//
void HUD::Ladder::draw_nadir(float x, float y)
{
    const float R = 7.5;

    draw_circle(x, y, R);
    draw_line(x, y + R, x, y + 22.5); // line above the circle
    draw_line(x - R, y, x + R, y);    // line at middle of circle

    float theta = asin(2.5 / R);
    float theta1 = asin(5.0 / R);
    float x1, y1, x2, y2;

    x1 = x + R * cos(theta);
    y1 = y + 2.5;
    x2 = x + R * cos((180.0 * SGD_DEGREES_TO_RADIANS) - theta);
    y2 = y + 2.5;
    draw_line(x1, y1, x2, y2);

    x1 = x + R * cos(theta1);
    y1 = y + 5.0;
    x2 = x + R * cos((180.0 * SGD_DEGREES_TO_RADIANS) - theta1);
    y2 = y + 5.0;
    draw_line(x1, y1, x2, y2);

    x1 = x + R * cos((180.0 * SGD_DEGREES_TO_RADIANS) + theta);
    y1 = y - 2.5;
    x2 = x + R * cos((360.0 * SGD_DEGREES_TO_RADIANS) - theta);
    y2 = y - 2.5;
    draw_line(x1, y1, x2, y2);

    x1 = x + R * cos((180.0 * SGD_DEGREES_TO_RADIANS) + theta1);
    y1 = y - 5.0;
    x2 = x + R * cos((360.0 * SGD_DEGREES_TO_RADIANS) - theta1);
    y2 = y - 5.0;
    draw_line(x1, y1, x2, y2);
}


