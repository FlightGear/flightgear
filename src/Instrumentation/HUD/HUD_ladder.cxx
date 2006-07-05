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

#include <Main/viewer.hxx>
#include "HUD.hxx"


// FIXME
float get__heading() { return fgGetFloat("/orientation/heading-deg") * M_PI / 180.0; }
float get__throttleval() { return fgGetFloat("/controls/engines/engine/throttle"); }
float get__aoa() { return fgGetFloat("/sim/frame-rate"); }					// FIXME
float get__Vx() { return fgGetFloat("/velocities/uBody-fps"); }
float get__Vy() { return fgGetFloat("/velocities/vBody-fps"); }
float get__Vz() { return fgGetFloat("/velocities/wBody-fps"); }
float get__Ax() { return fgGetFloat("/acclerations/pilot/x-accel-fps_sec"); }
float get__Ay() { return fgGetFloat("/acclerations/pilot/y-accel-fps_sec"); }
float get__Az() { return fgGetFloat("/acclerations/pilot/z-accel-fps_sec"); }
#undef ENABLE_SP_FMDS


HUD::Ladder::Ladder(HUD *hud, const SGPropertyNode *n, float x, float y) :
    Item(hud, n, x, y),
    _pitch(n->getNode("pitch-input", false)),
    _roll(n->getNode("roll-input", false)),
    _width_units(int(n->getFloatValue("display-span"))),
    div_units(int(fabs(n->getFloatValue("divisions")))),
    label_pos(n->getIntValue("lbl-pos")),
    _scr_hole(n->getIntValue("screen-hole")),
    _compression(n->getFloatValue("compression-factor")),
    _frl(n->getBoolValue("enable-fuselage-ref-line", false)),
    _target_spot(n->getBoolValue("enable-target-spot", false)),
    _velocity_vector(n->getBoolValue("enable-velocity-vector", false)),
    _drift_marker(n->getBoolValue("enable-drift-marker", false)),
    _alpha_bracket(n->getBoolValue("enable-alpha-bracket", false)),
    _energy_marker(n->getBoolValue("enable-energy-marker", false)),
    _climb_dive_marker(n->getBoolValue("enable-climb-dive-marker", false)),
    _glide_slope_marker(n->getBoolValue("enable-glide-slope-marker",false)),
    _glide_slope(n->getFloatValue("glide-slope", -4.0)),
    _energy_worm(n->getBoolValue("enable-energy-marker", false)),
    _waypoint_marker(n->getBoolValue("enable-waypoint-marker", false)),
    _zenith(n->getIntValue("zenith")),
    _nadir(n->getIntValue("nadir")),
    _hat(n->getIntValue("hat"))
{
    const char *t = n->getStringValue("type");
    _type = strcmp(t, "climb-dive") ? PITCH : CLIMB_DIVE;

    if (!_width_units)
        _width_units = 45;

    _vmax = _width_units / 2;
    _vmin = -_vmax;
}


void HUD::Ladder::draw(void)
{
    if (!_pitch.isValid() || !_roll.isValid())
        return;

    float x_ini, x_ini2;
    float x_end, x_end2;
    float y = 0;
    int count;
    float cosine, sine, xvvr, yvvr, Vxx = 0.0, Vyy = 0.0, Vzz = 0.0;
    float up_vel, ground_vel, actslope = 0.0;
    float Axx = 0.0, Ayy = 0.0, Azz = 0.0, total_vel = 0.0, pot_slope, t1;
    float t2 = 0.0, psi = 0.0, alpha, pla;
    float vel_x = 0.0, vel_y = 0.0, drift;
    bool pitch_ladder = false;
    bool climb_dive_ladder = false;
    bool clip_plane = false;

    GLdouble eqn_top[4] = {0.0, -1.0, 0.0, 0.0};
    GLdouble eqn_left[4] = {-1.0, 0.0, 0.0, 100.0};
    GLdouble eqn_right[4] = {1.0, 0.0, 0.0, 100.0};

    Point centroid = get_centroid();
    Rect box = get_location();

    float half_span = box.right / 2.0;
    float roll_value = _roll.getFloatValue() * SGD_DEGREES_TO_RADIANS;		// FIXME rad/deg conversion
    alpha = get__aoa();
    pla = get__throttleval();

#ifdef ENABLE_SP_FMDS
    int lgear, wown, wowm, ilcanclaw, ihook;
    ilcanclaw = get__iaux2();
    lgear = get__iaux3();
    wown = get__iaux4();
    wowm = get__iaux5();
    ihook = get__iaux6();
#endif
    float pitch_value = _pitch.getFloatValue();

    if (_type == CLIMB_DIVE) {
        pitch_ladder = false;
        climb_dive_ladder = true;
        clip_plane = true;

    } else { // _type == PITCH
        pitch_ladder = true;
        climb_dive_ladder = false;
        clip_plane = false;
    }

    //**************************************************************
    glPushMatrix();
    // define (0, 0) as center of screen
    glTranslatef(centroid.x, centroid.y, 0);

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
    // TYPE WATERLINE_MARK (W shaped _    _ )
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
    if (_velocity_vector) {
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

        xvvr = (((atan2(Vyy, Vxx) * SGD_RADIANS_TO_DEGREES) - psi)
                * (_compression / globals->get_current_view()->get_aspect_ratio()));
        drift = ((atan2(Vyy, Vxx) * SGD_RADIANS_TO_DEGREES) - psi);
        yvvr = ((actslope - pitch_value) * _compression);
        vel_y = ((actslope - pitch_value) * cos(roll_value) + drift * sin(roll_value)) * _compression;
        vel_x = (-(actslope - pitch_value) * sin(roll_value) + drift * cos(roll_value))
                * (_compression / globals->get_current_view()->get_aspect_ratio());
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
        // Clipping coordinates for ladder to be input from xml file
        // Clip hud ladder
        if (clip_plane) {
            glClipPlane(GL_CLIP_PLANE0, eqn_top);
            glEnable(GL_CLIP_PLANE0);
            glClipPlane(GL_CLIP_PLANE1, eqn_left);
            glEnable(GL_CLIP_PLANE1);
            glClipPlane(GL_CLIP_PLANE2, eqn_right);
            glEnable(GL_CLIP_PLANE2);
            // glScissor(-100,-240, 200, 240);
            // glEnable(GL_SCISSOR_TEST);
        }

        //****************************************************************
        // OBJECT MOVING RETICLE
        // TYPE VELOCITY VECTOR
        // ATTRIB - ALWAYS
        // velocity vector
        glBegin(GL_LINE_LOOP);  // Use polygon to approximate a circle
        for (count = 0; count < 50; count++) {
            cosine = 6 * cos(count * SGD_2PI / 50.0);
            sine =   6 * sin(count * SGD_2PI / 50.0);
            glVertex2f(cosine + vel_x, sine + vel_y);
        }
        glEnd();

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

#ifdef ENABLE_SP_FMDS
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


    //***************************************************************
    // OBJECT MOVING RETICLE
    // TYPE - SQUARE_BRACKET
    // ATTRIB - ON CONDITION
    // alpha bracket
#ifdef ENABLE_SP_FMDS
    if (_alpha_bracket && ihook == 1) {
        glBegin(GL_LINE_STRIP);
        glVertex2f(vel_x - 20 , vel_y - (16 - alpha) * _compression);
        glVertex2f(vel_x - 17, vel_y - (16 - alpha) * _compression);
        glVertex2f(vel_x - 17, vel_y - (14 - alpha) * _compression);
        glVertex2f(vel_x - 20, vel_y - (14 - alpha) * _compression);
        glEnd();

        glBegin(GL_LINE_STRIP);
        glVertex2f(vel_x + 20 , vel_y - (16 - alpha) * _compression);
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
    if (_energy_marker) {
        if (total_vel < 5.0) {
            t1 = 0;
            t2 = 0;
        } else {
            t1 = up_vel / total_vel;
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
#ifdef ENABLE_SP_FMDS
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

    if (climb_dive_ladder) { // CONFORMAL_HUD
        _vmin = pitch_value - _width_units;
        _vmax = pitch_value + _width_units;
        glTranslatef(vel_x, vel_y, 0);

    } else { // pitch_ladder - Default Hud
        _vmin = pitch_value - _width_units * 0.5f;
        _vmax = pitch_value + _width_units * 0.5f;
    }

    glRotatef(roll_value * SGD_RADIANS_TO_DEGREES, 0.0, 0.0, 1.0);
    // FRL marker not rotated - this line shifted below

    if (div_units) {
        const int BUFSIZE = 8;
        char buf[BUFSIZE];
        float label_length;
        float label_height;
        float left;
        float right;
        float bot;
        float top;
        float text_offset = 4.0f;
        float zero_offset = 0.0;

        if (climb_dive_ladder)
            zero_offset = 50.0f; // horizon line is wider by this much (hard coded ??)
        else
            zero_offset = 10.0f;

        fntFont *font = _hud->_font_renderer->getFont();			// FIXME
        float pointsize = _hud->_font_renderer->getPointSize();
        float italic = _hud->_font_renderer->getSlant();

        _locTextList.setFont(_hud->_font_renderer);
        _locTextList.erase();
        _locLineList.erase();
        _locStippleLineList.erase();

        int last = int(_vmax) + 1;
        int i = int(_vmin);

        if (!_scr_hole) {
            x_end = half_span;

            for (; i < last; i++) {
                y = (i - pitch_value) * _compression + .5f;

                if (!(i % div_units)) {           //  At integral multiple of div
                    snprintf(buf, BUFSIZE, "%d", i);
                    font->getBBox(buf, pointsize, italic, &left, &right, &bot, &top);
                    label_length = right - left;
                    label_length += text_offset;
                    label_height = (top - bot) / 2.0f;

                    x_ini = -half_span;

                    if (i >= 0) {
                        // Make zero point wider on left
                        if (i == 0)
                            x_ini -= zero_offset;

                        // Zero or above draw solid lines
                        draw_line(x_ini, y, x_end, y);

                        if (i == 90 && _zenith == 1)
                            draw_zenith(x_ini, x_end, y);
                    } else {
                        // Below zero draw dashed lines.
                        draw_stipple_line(x_ini, y, x_end, y);

                        if (i == -90 && _nadir ==1)
                            draw_nadir(x_ini, x_end, y);
                    }

                    // Calculate the position of the left text and write it.
                    draw_text(x_ini - label_length, y - label_height, buf);
                    draw_text(x_end + text_offset, y - label_height, buf);
                }
            }

        } else { // if (_scr_hole)
            // Draw ladder with space in the middle of the lines
            float hole = _scr_hole / 2.0f;

            x_end = -half_span + hole;
            x_ini2 = half_span - hole;

            for (; i < last; i++) {
                if (_type == PITCH)
                    y = float(i - pitch_value) * _compression + .5;
                else // _type == CLIMB_DIVE
                    y = float(i - actslope) * _compression + .5;

                if (!(i % div_units)) {  //  At integral multiple of div
                    snprintf(buf, BUFSIZE, "%d", i);
                    font->getBBox(buf, pointsize, italic, &left, &right, &bot, &top);
                    label_length = right - left;
                    label_length += text_offset;
                    label_height = (top - bot) / 2.0f;
                    // printf("l %f r %f b %f t %f\n",left, right, bot, top);

                    // Start by calculating the points and drawing the
                    // left side lines.
                    x_ini = -half_span;
                    x_end2 = half_span;

                    if (i >= 0) {
                        // Make zero point wider on left
                        if (i == 0) {
                            x_ini -= zero_offset;
                            x_end2 += zero_offset;
                        }
                        //draw climb bar vertical lines
                        if (climb_dive_ladder) {
                            // Zero or above draw solid lines
                            draw_line(x_end, y - 5.0, x_end, y);
                            draw_line(x_ini2, y - 5.0, x_ini2, y);
                        }
                        // draw pitch / climb bar
                        draw_line(x_ini, y, x_end, y);
                        draw_line(x_ini2, y, x_end2, y);

                        if (i == 90 && _zenith == 1)
                            draw_zenith(x_ini2, x_end, y);

                    } else { // i < 0
                        // draw dive bar vertical lines
                        if (climb_dive_ladder) {
                            draw_line(x_end, y + 5.0, x_end, y);
                            draw_line(x_ini2, y + 5.0, x_ini2, y);
                        }

                        // draw pitch / dive bars
                        draw_stipple_line(x_ini, y, x_end, y);
                        draw_stipple_line(x_ini2, y, x_end2, y);

                        if (i == -90 && _nadir == 1)
                            draw_nadir(x_ini2, x_end, y);
                    }

                    // Now calculate the location of the left side label using
                    draw_text(x_ini - label_length, y - label_height, buf);
                    draw_text(x_end2 + text_offset, y - label_height, buf);
                }
            }

            // OBJECT LADDER MARK
            // TYPE LINE
            // ATTRIB - ON CONDITION
            // draw appraoch glide slope marker
#ifdef ENABLE_SP_FMDS
            if (_glide_slope_marker && ihook) {
                draw_line(-half_span + 15, (_glide_slope - actslope) * _compression,
                        -half_span + hole, (_glide_slope - actslope) * _compression);
                draw_line(half_span - 15, (_glide_slope - actslope) * _compression,
                        half_span - hole, (_glide_slope - actslope) * _compression);
            }
#endif
        }
        _locTextList.draw();

        glLineWidth(0.2);

        _locLineList.draw();

        glEnable(GL_LINE_STIPPLE);
        glLineStipple(1, 0x00FF);
        _locStippleLineList.draw();
        glDisable(GL_LINE_STIPPLE);
    }
    glDisable(GL_CLIP_PLANE0);
    glDisable(GL_CLIP_PLANE1);
    glDisable(GL_CLIP_PLANE2);
    //  glDisable(GL_SCISSOR_TEST);
    glPopMatrix();
    //*************************************************************

    //*************************************************************
#ifdef ENABLE_SP_FMDS
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
        if (fabs(brg-psi) > 10.0) {
            glPushMatrix();
            glTranslatef(centroid.x, centroid.y, 0);
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
        if (fabs(brg-psi) < 12.0) {
            if (_hat == 0) {
                glBegin(GL_LINE_LOOP);
                glVertex2f(((brg - psi) * 60 / 25) + 320, 240.0);
                glVertex2f(((brg - psi) * 60 / 25) + 326, 240.0 - 4);
                glVertex2f(((brg - psi) * 60 / 25) + 323, 240.0 - 4);
                glVertex2f(((brg - psi) * 60 / 25) + 323, 240.0 - 8);
                glVertex2f(((brg - psi) * 60 / 25) + 317, 240.0 - 8);
                glVertex2f(((brg - psi) * 60 / 25) + 317, 240.0 - 4);
                glVertex2f(((brg - psi) * 60 / 25) + 314, 240.0 - 4);
                glEnd();

            } else { //if _hat=0
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
            } //_hat=0

         } //brg<12
     } // if _waypoint_marker
#endif
}//draw


/******************************************************************/
//  draws the zenith symbol  for highest possible climb angle (i.e. 90 degree climb angle)
//
void HUD::Ladder::draw_zenith(float xfirst, float xlast, float yvalue)
{
    float xcentre = (xfirst + xlast) / 2.0;
    float ycentre = yvalue;

    draw_line(xcentre - 9.0, ycentre, xcentre - 3.0, ycentre + 1.3);
    draw_line(xcentre - 9.0, ycentre, xcentre - 3.0, ycentre - 1.3);

    draw_line(xcentre + 9.0, ycentre, xcentre + 3.0, ycentre + 1.3);
    draw_line(xcentre + 9.0, ycentre, xcentre + 3.0, ycentre - 1.3);

    draw_line(xcentre, ycentre + 9.0, xcentre - 1.3, ycentre + 3.0);
    draw_line(xcentre, ycentre + 9.0, xcentre + 1.3, ycentre + 3.0);

    draw_line(xcentre - 3.9, ycentre + 3.9, xcentre - 3.0, ycentre + 1.3);
    draw_line(xcentre - 3.9, ycentre + 3.9, xcentre - 1.3, ycentre + 3.0);

    draw_line(xcentre + 3.9, ycentre + 3.9, xcentre + 1.3, ycentre+3.0);
    draw_line(xcentre + 3.9, ycentre + 3.9, xcentre + 3.0, ycentre+1.3);

    draw_line(xcentre - 3.9, ycentre - 3.9, xcentre - 3.0, ycentre-1.3);
    draw_line(xcentre - 3.9, ycentre - 3.9, xcentre - 1.3, ycentre-2.6);

    draw_line(xcentre + 3.9, ycentre - 3.9, xcentre + 3.0, ycentre-1.3);
    draw_line(xcentre + 3.9, ycentre - 3.9, xcentre + 1.3, ycentre-2.6);

    draw_line(xcentre - 1.3, ycentre - 2.6, xcentre, ycentre - 27.0);
    draw_line(xcentre + 1.3, ycentre - 2.6, xcentre, ycentre - 27.0);
}


//  draws the nadir symbol  for lowest possible dive angle (i.e. 90 degree dive angle)
//
void HUD::Ladder::draw_nadir(float xfirst, float xlast, float yvalue)
{
    float xcentre = (xfirst + xlast) / 2.0;
    float ycentre = yvalue;

    float r = 7.5;
    float x1, y1, x2, y2;

    // to draw a circle
    float xcent1, xcent2, ycent1, ycent2;
    xcent1 = xcentre + r;
    ycent1 = ycentre;

    for (int count = 1; count <= 400; count++) {
        float temp = count * 2 * SG_PI / 400.0;
        xcent2 = xcentre + r * cos(temp);
        ycent2 = ycentre + r * sin(temp);

        draw_line(xcent1, ycent1, xcent2, ycent2);

        xcent1 = xcent2;
        ycent1 = ycent2;
    }

    xcent2 = xcentre + r;
    ycent2 = ycentre;

    Item::draw_line(xcent1, ycent1, xcent2, ycent2); //to connect last point to first point
    //end circle

    //to draw a line above the circle
    draw_line(xcentre, ycentre + 7.5, xcentre, ycentre + 22.5);

    //line in the middle of circle
    draw_line(xcentre - 7.5, ycentre, xcentre + 7.5, ycentre);

    float theta = asin(2.5 / 7.5);
    float theta1 = asin(5.0 / 7.5);

    x1 = xcentre + r * cos(theta);
    y1 = ycentre + 2.5;
    x2 = xcentre + r * cos((180.0 * SGD_DEGREES_TO_RADIANS) - theta);
    y2 = ycentre + 2.5;
    draw_line(x1, y1, x2, y2);

    x1 = xcentre + r * cos(theta1);
    y1 = ycentre + 5.0;
    x2 = xcentre + r * cos((180.0 * SGD_DEGREES_TO_RADIANS) - theta1);
    y2 = ycentre + 5.0;
    draw_line(x1, y1, x2, y2);

    x1 = xcentre + r * cos((180.0 * SGD_DEGREES_TO_RADIANS) + theta);
    y1 = ycentre - 2.5;
    x2 = xcentre + r * cos((360.0 * SGD_DEGREES_TO_RADIANS) - theta);
    y2 = ycentre - 2.5;
    draw_line(x1, y1, x2, y2);

    x1 = xcentre + r * cos((180.0 * SGD_DEGREES_TO_RADIANS) + theta1);
    y1 = ycentre - 5.0;
    x2 = xcentre + r * cos((360.0 * SGD_DEGREES_TO_RADIANS) - theta1);
    y2 = ycentre - 5.0;
    draw_line(x1, y1, x2, y2);
}


