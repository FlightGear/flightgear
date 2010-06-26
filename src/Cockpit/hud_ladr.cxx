#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <simgear/constants.h>

#include "hud.hxx"
#include "panel.hxx"
#include <Main/viewer.hxx>

// FIXME
extern float get_roll(void);
extern float get_pitch(void);


HudLadder::HudLadder(const SGPropertyNode *node) :
    dual_instr_item(
            node->getIntValue("x"),
            node->getIntValue("y"),
            node->getIntValue("width"),
            node->getIntValue("height"),
            get_roll,
            get_pitch,				// FIXME getter functions from cockpit.cxx
            node->getBoolValue("working", true),
            HUDS_RIGHT),
    width_units(int(node->getFloatValue("span_units"))),
    div_units(int(fabs(node->getFloatValue("division_units")))),
    minor_div(0 /* hud.cxx: static float minor_division = 0 */),
    label_pos(node->getIntValue("lbl_pos")),
    scr_hole(node->getIntValue("screen_hole")),
    factor(node->getFloatValue("compression_factor")),
    hudladder_type(node->getStringValue("name")),
    frl(node->getBoolValue("enable_frl", false)),
    target_spot(node->getBoolValue("enable_target_spot", false)),
    velocity_vector(node->getBoolValue("enable_velocity_vector", false)),
    drift_marker(node->getBoolValue("enable_drift_marker", false)),
    alpha_bracket(node->getBoolValue("enable_alpha_bracket", false)),
    energy_marker(node->getBoolValue("enable_energy_marker", false)),
    climb_dive_marker(node->getBoolValue("enable_climb_dive_marker", false)),
    glide_slope_marker(node->getBoolValue("enable_glide_slope_marker",false)),
    glide_slope(node->getFloatValue("glide_slope", -4.0)),
    energy_worm(node->getBoolValue("enable_energy_marker", false)),
    waypoint_marker(node->getBoolValue("enable_waypoint_marker", false)),
    zenith(node->getIntValue("zenith")),
    nadir(node->getIntValue("nadir")),
    hat(node->getIntValue("hat"))
{
    // The factor assumes a base of 55 degrees per 640 pixels.
    // Invert to convert the "compression" factor to a
    // pixels-per-degree number.
    if (fgGetBool("/sim/hud/enable3d", true) && HUD_style == 1)
        factor = 640.0 / 55.0;

    SG_LOG(SG_INPUT, SG_BULK, "Done reading HudLadder instrument"
            << node->getStringValue("name", "[unnamed]"));

    if (!width_units)
        width_units = 45;

    vmax = width_units / 2;
    vmin = -vmax;
}


//
//  Draws a climb ladder in the center of the HUD
//

void HudLadder::draw(void)
{

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

    POINT centroid = get_centroid();
    RECT box = get_location();

    float half_span  = box.right / 2.0;
    float roll_value = current_ch2();
    alpha = get_aoa();
    pla = get_throttleval();

#ifdef ENABLE_SP_FDM
    int lgear, wown, wowm, ilcanclaw, ihook;
    ilcanclaw = fgGetInt("/fdm-ada/iaux2", 0);
    lgear = fgGetInt("/fdm-ada/iaux3", 0);
    wown = fgGetInt("/fdm-ada/iaux4", 0);
    wowm = fgGetInt("/fdm-ada/iaux5", 0);;
    ihook = fgGetInt("/fdm-ada/iaux6", 0);
#endif
    float pitch_value = current_ch1() * SGD_RADIANS_TO_DEGREES;

    if (hudladder_type == "Climb/Dive Ladder") {
        pitch_ladder = false;
        climb_dive_ladder = true;
        clip_plane = true;

    } else { // hudladder_type == "Pitch Ladder"
        pitch_ladder = true;
        climb_dive_ladder = false;
        clip_plane = false;
    }

    //**************************************************************
    glPushMatrix();
    // define (0, 0) as center of screen
    glTranslatef(centroid.x, centroid.y, 0);

    // OBJECT STATIC RETICLE
    // TYPE FRL
    // ATTRIB - ALWAYS
    // Draw the FRL spot and line
    if (frl) {
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
    //                                    \/\/

    //****************************************************************
    // TYPE TARGET_SPOT
    // Draw the target spot.
    if (target_spot) {
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
    if (velocity_vector) {
        Vxx = fgGetDouble("/velocities/north-relground-fps", 0.0);
        Vyy = fgGetDouble("/velocities/east-relground-fps", 0.0);
        Vzz = fgGetDouble("/velocities/down-relground-fps", 0.0);
        Axx = fgGetDouble("/accelerations/ned/north-accel-fps_sec", 0.0);
        Ayy = fgGetDouble("/accelerations/ned/east-accel-fps_sec", 0.0);
        Azz = fgGetDouble("/accelerations/ned/down-accel-fps_sec", 0.0);
        psi = get_heading();

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
                * (factor / globals->get_current_view()->get_aspect_ratio()));
        drift = ((atan2(Vyy, Vxx) * SGD_RADIANS_TO_DEGREES) - psi);
        yvvr = ((actslope - pitch_value) * factor);
        vel_y = ((actslope - pitch_value) * cos(roll_value) + drift * sin(roll_value)) * factor;
        vel_x = (-(actslope - pitch_value) * sin(roll_value) + drift * cos(roll_value))
                * (factor/globals->get_current_view()->get_aspect_ratio());
        //  printf("%f %f %f %f\n",vel_x, vel_y, drift, psi);

        //****************************************************************
        // OBJECT MOVING RETICLE
        // TYPE - DRIFT MARKER
        // ATTRIB - ALWAYS
        // drift marker
        if (drift_marker) {
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

#ifdef ENABLE_SP_FDM
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
    } // if velocity_vector


    //***************************************************************
    // OBJECT MOVING RETICLE
    // TYPE - SQUARE_BRACKET
    // ATTRIB - ON CONDITION
    // alpha bracket
#ifdef ENABLE_SP_FDM
    if (alpha_bracket && ihook == 1) {
        glBegin(GL_LINE_STRIP);
        glVertex2f(vel_x - 20 , vel_y - (16 - alpha) * factor);
        glVertex2f(vel_x - 17, vel_y - (16 - alpha) * factor);
        glVertex2f(vel_x - 17, vel_y - (14 - alpha) * factor);
        glVertex2f(vel_x - 20, vel_y - (14 - alpha) * factor);
        glEnd();

        glBegin(GL_LINE_STRIP);
        glVertex2f(vel_x + 20 , vel_y - (16 - alpha) * factor);
        glVertex2f(vel_x + 17, vel_y - (16 - alpha) * factor);
        glVertex2f(vel_x + 17, vel_y - (14 - alpha) * factor);
        glVertex2f(vel_x + 20, vel_y - (14 - alpha) * factor);
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
    if (energy_marker) {
        if (total_vel < 5.0) {
            t1 = 0;
            t2 = 0;
        } else {
            t1 = up_vel / total_vel;
            t2 = asin((Vxx * Axx + Vyy * Ayy + Vzz * Azz) / (9.81 * total_vel));
        }
        pot_slope = ((t2 / 3) * SGD_RADIANS_TO_DEGREES) * factor + vel_y;
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
    if (energy_worm && ilcanclaw == 1) {
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
    if (climb_dive_marker) {
        glBegin(GL_LINE_LOOP);
        glVertex2f(-3.0, 0.0 + vel_y);
        glVertex2f(0.0, 6.0 + vel_y);
        glVertex2f(3.0, 0.0 + vel_y);
        glVertex2f(0.0, -6.0 + vel_y);
        glEnd();
    }

    //****************************************************************

    if (climb_dive_ladder) { // CONFORMAL_HUD
        vmin = pitch_value - (float)width_units;
        vmax = pitch_value + (float)width_units;
        glTranslatef(vel_x, vel_y, 0);

    } else { // pitch_ladder - Default Hud
        vmin = pitch_value - (float)width_units * 0.5f;
        vmax = pitch_value + (float)width_units * 0.5f;
    }

    glRotatef(roll_value * SGD_RADIANS_TO_DEGREES, 0.0, 0.0, 1.0);
    // FRL marker not rotated - this line shifted below

    if (div_units) {
        char  TextLadder[8];
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

        fntFont *font = HUDtext->getFont();			// FIXME
        float pointsize = HUDtext->getPointSize();
        float italic = HUDtext->getSlant();

        TextList.setFont(HUDtext);
        TextList.erase();
        LineList.erase();
        StippleLineList.erase();

        int last = float_to_int(vmax) + 1;
        int i = float_to_int(vmin);

        if (!scr_hole) {
            x_end = half_span;

            for (; i<last; i++) {
                y = (((float)(i - pitch_value) * factor) + .5f);

                if (!(i % div_units)) {           //  At integral multiple of div
                    sprintf(TextLadder, "%d", i);
                    font->getBBox(TextLadder, pointsize, italic,
                            &left, &right, &bot, &top);
                    label_length = right - left;
                    label_length += text_offset;
                    label_height = (top - bot) / 2.0f;

                    x_ini = -half_span;

                    if (i >= 0) {
                        // Make zero point wider on left
                        if (i == 0)
                            x_ini -= zero_offset;

                        // Zero or above draw solid lines
                        Line(x_ini, y, x_end, y);

                        if (i == 90 && zenith == 1)
                            drawZenith(x_ini, x_end, y);
                    } else {
                        // Below zero draw dashed lines.
                        StippleLine(x_ini, y, x_end, y);

                        if (i == -90 && nadir ==1)
                            drawNadir(x_ini, x_end, y);
                    }

                    // Calculate the position of the left text and write it.
                    Text(x_ini-label_length, y-label_height, TextLadder);
                    Text(x_end+text_offset, y-label_height, TextLadder);
                }
            }

        } else { // if (scr_hole)
            // Draw ladder with space in the middle of the lines
            float hole = (float)((scr_hole) / 2.0f);

            x_end = -half_span + hole;
            x_ini2 = half_span - hole;

            for (; i < last; i++) {
                if (hudladder_type == "Pitch Ladder")
                    y = (((float)(i - pitch_value) * factor) + .5);
                else if (hudladder_type == "Climb/Dive Ladder")
                    y = (((float)(i - actslope) * factor) + .5);

                if (!(i % div_units)) {  //  At integral multiple of div
                    sprintf(TextLadder, "%d", i);
                    font->getBBox(TextLadder, pointsize, italic,
                            &left, &right, &bot, &top);
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
                            Line(x_end, y - 5.0, x_end, y);
                            Line(x_ini2, y - 5.0, x_ini2, y);
                        }
                        // draw pitch / climb bar
                        Line(x_ini, y, x_end, y);
                        Line(x_ini2, y, x_end2, y);

                        if (i == 90 && zenith == 1)
                            drawZenith(x_ini2, x_end, y);

                    } else { // i < 0
                        // draw dive bar vertical lines
                        if (climb_dive_ladder) {
                            Line(x_end, y + 5.0, x_end, y);
                            Line(x_ini2, y + 5.0, x_ini2, y);
                        }

                        // draw pitch / dive bars
                        StippleLine(x_ini, y, x_end, y);
                        StippleLine(x_ini2, y, x_end2, y);

                        if (i == -90 && nadir == 1)
                            drawNadir(x_ini2, x_end, y);
                    }

                    // Now calculate the location of the left side label using
                    Text(x_ini-label_length, y - label_height, TextLadder);
                    Text(x_end2+text_offset, y - label_height, TextLadder);
                }
            }

            // OBJECT LADDER MARK
            // TYPE LINE
            // ATTRIB - ON CONDITION
            // draw appraoch glide slope marker
#ifdef ENABLE_SP_FDM
            if (glide_slope_marker && ihook) {
                Line(-half_span + 15, (glide_slope-actslope) * factor,
                        -half_span + hole, (glide_slope - actslope) * factor);
                Line(half_span - 15, (glide_slope-actslope) * factor,
                        half_span - hole, (glide_slope - actslope) * factor);
            }
#endif
        }
        TextList.draw();

        glLineWidth(0.2);

        LineList.draw();

        glEnable(GL_LINE_STIPPLE);
        glLineStipple(1, 0x00FF);
        StippleLineList.draw();
        glDisable(GL_LINE_STIPPLE);
    }
    glDisable(GL_CLIP_PLANE0);
    glDisable(GL_CLIP_PLANE1);
    glDisable(GL_CLIP_PLANE2);
    //  glDisable(GL_SCISSOR_TEST);
    glPopMatrix();
    //*************************************************************

    //*************************************************************
#ifdef ENABLE_SP_FDM
    if (waypoint_marker) {
        //waypoint marker computation
        float fromwp_lat, towp_lat, fromwp_lon, towp_lon, dist, delx, dely, hyp, theta, brg;

        fromwp_lon = get_longitude() * SGD_DEGREES_TO_RADIANS;
        fromwp_lat = get_latitude() * SGD_DEGREES_TO_RADIANS;
        towp_lon = fgGetDouble("/fdm-ada/ship-lon", 0.0) * SGD_DEGREES_TO_RADIANS;
        towp_lat = fgGetDouble("/fdm-ada/ship-lat", 0.0) * SGD_DEGREES_TO_RADIANS;

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
            if (hat == 0) {
                glBegin(GL_LINE_LOOP);
                glVertex2f(((brg - psi) * 60 / 25) + 320, 240.0);
                glVertex2f(((brg - psi) * 60 / 25) + 326, 240.0 - 4);
                glVertex2f(((brg - psi) * 60 / 25) + 323, 240.0 - 4);
                glVertex2f(((brg - psi) * 60 / 25) + 323, 240.0 - 8);
                glVertex2f(((brg - psi) * 60 / 25) + 317, 240.0 - 8);
                glVertex2f(((brg - psi) * 60 / 25) + 317, 240.0 - 4);
                glVertex2f(((brg - psi) * 60 / 25) + 314, 240.0 - 4);
                glEnd();

            } else { //if hat=0
                float x = (brg - psi) * 60 / 25 + 320, y = 240.0, r = 5.0;
                float x1, y1;

                glEnable(GL_POINT_SMOOTH);
                glBegin(GL_POINTS);

                for (int count = 0; count <= 200; count++) {
                    float temp = count * 3.142 * 3 / (200.0 * 2.0);
                    float temp1 = temp - (45.0 * SGD_DEGREES_TO_RADIANS);
                    x1 = x + r * cos(temp1);
                    y1 = y + r * sin(temp1);
                    glVertex2f(x1, y1);
                }

                glEnd();
                glDisable(GL_POINT_SMOOTH);
            } //hat=0

         } //brg<12
     } // if waypoint_marker
#endif
}//draw


/******************************************************************/
//  draws the zenith symbol  for highest possible climb angle (i.e. 90 degree climb angle)
//
void HudLadder::drawZenith(float xfirst, float xlast, float yvalue)
{
    float xcentre = (xfirst + xlast) / 2.0;
    float ycentre = yvalue;

    Line(xcentre - 9.0, ycentre, xcentre - 3.0, ycentre + 1.3);
    Line(xcentre - 9.0, ycentre, xcentre - 3.0, ycentre - 1.3);

    Line(xcentre + 9.0, ycentre, xcentre + 3.0, ycentre + 1.3);
    Line(xcentre + 9.0, ycentre, xcentre + 3.0, ycentre - 1.3);

    Line(xcentre, ycentre + 9.0, xcentre - 1.3, ycentre + 3.0);
    Line(xcentre, ycentre + 9.0, xcentre + 1.3, ycentre + 3.0);

    Line(xcentre - 3.9, ycentre + 3.9, xcentre - 3.0, ycentre + 1.3);
    Line(xcentre - 3.9, ycentre + 3.9, xcentre - 1.3, ycentre + 3.0);

    Line(xcentre + 3.9, ycentre + 3.9, xcentre + 1.3, ycentre+3.0);
    Line(xcentre + 3.9, ycentre + 3.9, xcentre + 3.0, ycentre+1.3);

    Line(xcentre - 3.9, ycentre - 3.9, xcentre - 3.0, ycentre-1.3);
    Line(xcentre - 3.9, ycentre - 3.9, xcentre - 1.3, ycentre-2.6);

    Line(xcentre + 3.9, ycentre - 3.9, xcentre + 3.0, ycentre-1.3);
    Line(xcentre + 3.9, ycentre - 3.9, xcentre + 1.3, ycentre-2.6);

    Line(xcentre - 1.3, ycentre - 2.6, xcentre, ycentre - 27.0);
    Line(xcentre + 1.3, ycentre - 2.6, xcentre, ycentre - 27.0);
}


//  draws the nadir symbol  for lowest possible dive angle (i.e. 90 degree dive angle)
//
void HudLadder::drawNadir(float xfirst, float xlast, float yvalue)
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
        float temp = count * 2 * 3.142 / 400.0;
        xcent2 = xcentre + r * cos(temp);
        ycent2 = ycentre + r * sin(temp);

        Line(xcent1, ycent1, xcent2, ycent2);

        xcent1 = xcent2;
        ycent1 = ycent2;
    }

    xcent2 = xcentre + r;
    ycent2 = ycentre;

    drawOneLine(xcent1, ycent1, xcent2, ycent2); //to connect last point to first point
    //end circle

    //to draw a line above the circle
    Line(xcentre, ycentre + 7.5, xcentre, ycentre + 22.5);

    //line in the middle of circle
    Line(xcentre - 7.5, ycentre, xcentre + 7.5, ycentre);

    float theta = asin (2.5 / 7.5);
    float theta1 = asin(5.0 / 7.5);

    x1 = xcentre + r * cos(theta);
    y1 = ycentre + 2.5;
    x2 = xcentre + r * cos((180.0 * SGD_DEGREES_TO_RADIANS) - theta);
    y2 = ycentre + 2.5;
    Line(x1, y1, x2, y2);

    x1 = xcentre + r * cos(theta1);
    y1 = ycentre + 5.0;
    x2 = xcentre + r * cos((180.0 * SGD_DEGREES_TO_RADIANS) - theta1);
    y2 = ycentre + 5.0;
    Line(x1, y1, x2, y2);

    x1 = xcentre + r * cos((180.0 * SGD_DEGREES_TO_RADIANS) + theta);
    y1 = ycentre - 2.5;
    x2 = xcentre + r * cos((360.0 * SGD_DEGREES_TO_RADIANS) - theta);
    y2 = ycentre - 2.5;
    Line(x1, y1, x2, y2);

    x1 = xcentre + r * cos((180.0 * SGD_DEGREES_TO_RADIANS) + theta1);
    y1 = ycentre - 5.0;
    x2 = xcentre + r * cos((360.0 * SGD_DEGREES_TO_RADIANS) - theta1);
    y2 = ycentre - 5.0;
    Line(x1, y1, x2, y2);
}


