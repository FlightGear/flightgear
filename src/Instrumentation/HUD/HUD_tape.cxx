// HUD_tape.cxx -- HUD Tape Instrument
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

#include "HUD.hxx"


HUD::Tape::Tape(HUD *hud, const SGPropertyNode *n, float x, float y) :
    Scale(hud, n, x, y),
    draw_tick_bottom(n->getBoolValue("tick-bottom", false)),
    draw_tick_top(n->getBoolValue("tick-top", false)),
    draw_tick_right(n->getBoolValue("tick-right", false)),
    draw_tick_left(n->getBoolValue("tick-left", false)),
    draw_cap_bottom(n->getBoolValue("cap-bottom", false)),
    draw_cap_top(n->getBoolValue("cap-top", false)),
    draw_cap_right(n->getBoolValue("cap-right", false)),
    draw_cap_left(n->getBoolValue("cap-left", false)),
    marker_offset(n->getFloatValue("marker-offset", 0.0)),
    pointer(n->getBoolValue("enable-pointer", true)),
    pointer_type(n->getStringValue("pointer-type")),
    tick_type(n->getStringValue("tick-type")), // 'circle' or 'line'
    tick_length(n->getStringValue("tick-length")), // for variable length
    zoom(n->getIntValue("zoom"))
{
    half_width_units = range_to_show() / 2.0;
}


void HUD::Tape::draw(void) //  (HUD_scale * pscale)
{
    if (!_input.isValid())
        return;

    float vmin = 0.0, vmax = 0.0;
    float marker_xs;
    float marker_xe;
    float marker_ys;
    float marker_ye;
    float text_x = 0.0, text_y = 0.0;
    int lenstr;
    float height, width;
    int i, last;
    const int BUFSIZE = 80;
    char buf[BUFSIZE];
    bool condition;
    int disp_val = 0;
    int oddtype, k; //odd or even values for ticks

    Point mid_scr = get_centroid();
    float cur_value = _input.getFloatValue();

    if ((int)_input.max() & 1)
        oddtype = 1; //draw ticks at odd values
    else
        oddtype = 0; //draw ticks at even values

    Rect scrn_rect = get_location();

    height = scrn_rect.top + scrn_rect.bottom;
    width = scrn_rect.left + scrn_rect.right;


    // was: if (type != "gauge") { ... until end
    // if its not explicitly a gauge default to tape
    if (pointer) {
        if (pointer_type == "moving") {
            vmin = _input.min();
            vmax = _input.max();

        } else {
            // default to fixed
            vmin = cur_value - half_width_units; // width units == needle travel
            vmax = cur_value + half_width_units; // or picture unit span.
            text_x = mid_scr.x;
            text_y = mid_scr.y;
        }

    } else {
        vmin = cur_value - half_width_units; // width units == needle travel
        vmax = cur_value + half_width_units; // or picture unit span.
        text_x = mid_scr.x;
        text_y = mid_scr.y;
    }

    // Draw the basic markings for the scale...

    if (option_vert()) { // Vertical scale
        // Bottom tick bar
        if (draw_tick_bottom)
            draw_line(scrn_rect.left, scrn_rect.top, width, scrn_rect.top);

        // Top tick bar
        if (draw_tick_top)
            draw_line(scrn_rect.left, height, width, height);

        marker_xs = scrn_rect.left;  // x start
        marker_xe = width;           // x extent
        marker_ye = height;

        //    glBegin(GL_LINES);

        // Bottom tick bar
        //    glVertex2f(marker_xs, scrn_rect.top);
        //    glVertex2f(marker_xe, scrn_rect.top);

        // Top tick bar
        //    glVertex2f(marker_xs, marker_ye);
        //    glVertex2f(marker_xe, marker_ye);
        //    glEnd();


        // We do not use else in the following so that combining the
        // two options produces a "caged" display with double
        // carrots. The same is done for horizontal card indicators.

        // begin vertical/left
        //First draw capping lines and pointers
        if (option_left()) {    // Calculate x marker offset

            if (draw_cap_right) {
                // Cap right side
                draw_line(marker_xe, scrn_rect.top, marker_xe, marker_ye);
            }

            marker_xs  = marker_xe - scrn_rect.right / 3;   // Adjust tick xs

            // draw_line(marker_xs, mid_scr.y,
            //              marker_xe, mid_scr.y + scrn_rect.right / 6);
            // draw_line(marker_xs, mid_scr.y,
            //              marker_xe, mid_scr.y - scrn_rect.right / 6);

            // draw pointer
            if (pointer) {
                if (pointer_type == "moving") {
                    if (zoom == 0) {
                        //Code for Moving Type Pointer
                        float ycentre, ypoint, xpoint;
                        float range, wth;
                        if (cur_value > _input.max())
                            cur_value = _input.max();
                        if (cur_value < _input.min())
                            cur_value = _input.min();

                        if (_input.min() >= 0.0)
                            ycentre = scrn_rect.top;
                        else if (_input.max() + _input.min() == 0.0)
                            ycentre = mid_scr.y;
                        else if (oddtype == 1)
                            ycentre = scrn_rect.top + (1.0 - _input.min()) * scrn_rect.bottom
                                    / (_input.max() - _input.min());
                        else
                            ycentre = scrn_rect.top + _input.min() * scrn_rect.bottom
                                    / (_input.max() - _input.min());

                        range = scrn_rect.bottom;
                        wth = scrn_rect.left + scrn_rect.right;

                        if (oddtype == 1)
                            ypoint = ycentre + ((cur_value - 1.0) * range / val_span);
                        else
                            ypoint = ycentre + (cur_value * range / val_span);

                        xpoint = wth + marker_offset;
                        draw_line(xpoint, ycentre, xpoint, ypoint);
                        draw_line(xpoint, ypoint, xpoint - marker_offset, ypoint);
                        draw_line(xpoint - marker_offset, ypoint, xpoint - 5.0, ypoint + 5.0);
                        draw_line(xpoint - marker_offset, ypoint, xpoint - 5.0, ypoint - 5.0);
                    } //zoom=0

                } else {
                    // default to fixed
                    fixed(marker_offset + marker_xe, text_y + scrn_rect.right / 6,
                            marker_offset + marker_xs, text_y, marker_offset + marker_xe,
                            text_y - scrn_rect.right / 6);
                }//end pointer type
            } //if pointer
        } //end vertical/left

        // begin vertical/right
        //First draw capping lines and pointers
        if (option_right()) {  // We'll default this for now.
            if (draw_cap_left) {
                // Cap left side
                draw_line(scrn_rect.left, scrn_rect.top, scrn_rect.left, marker_ye);
            } //endif cap_left

            marker_xe = scrn_rect.left + scrn_rect.right / 3;     // Adjust tick xe
            // Indicator carrot
            // draw_line(scrn_rect.left, mid_scr.y +  scrn_rect.right / 6,
            //              marker_xe, mid_scr.y);
            // draw_line(scrn_rect.left, mid_scr.y -  scrn_rect.right / 6,
            //              marker_xe, mid_scr.y);

            // draw pointer
            if (pointer) {
                if (pointer_type == "moving") {
                    if (zoom == 0) {
                        //type-fixed & zoom=1, behaviour to be defined
                        // Code for Moving Type Pointer
                        float ycentre, ypoint, xpoint;
                        float range;

                        if (cur_value > _input.max())
                            cur_value = _input.max();
                        if (cur_value < _input.min())
                            cur_value = _input.min();

                        if (_input.min() >= 0.0)
                            ycentre = scrn_rect.top;
                        else if (_input.max() + _input.min() == 0.0)
                            ycentre = mid_scr.y;
                        else if (oddtype == 1)
                            ycentre = scrn_rect.top + (1.0 - _input.min()) * scrn_rect.bottom / (_input.max() - _input.min());
                        else
                            ycentre = scrn_rect.top + _input.min() * scrn_rect.bottom / (_input.max() - _input.min());

                        range = scrn_rect.bottom;

                        if (oddtype == 1)
                            ypoint = ycentre + ((cur_value - 1.0) * range / val_span);
                        else
                            ypoint = ycentre + (cur_value * range / val_span);

                        xpoint = scrn_rect.left - marker_offset;
                        draw_line(xpoint, ycentre, xpoint, ypoint);
                        draw_line(xpoint, ypoint, xpoint + marker_offset, ypoint);
                        draw_line(xpoint + marker_offset, ypoint, xpoint + 5.0, ypoint + 5.0);
                        draw_line(xpoint + marker_offset, ypoint, xpoint + 5.0, ypoint - 5.0);
                    }

                } else {
                    // default to fixed
                    fixed(-marker_offset + scrn_rect.left, text_y +  scrn_rect.right / 6,
                            -marker_offset + marker_xe, text_y, -marker_offset + scrn_rect.left,
                            text_y - scrn_rect.right / 6);
                }
            } //if pointer
        }  //end vertical/right

        // At this point marker x_start and x_end values are transposed.
        // To keep this from confusing things they are now interchanged.
        if (option_both()) {
            marker_ye = marker_xs;
            marker_xs = marker_xe;
            marker_xe = marker_ye;
        }

        // Work through from bottom to top of scale. Calculating where to put
        // minor and major ticks.

        // draw scale or tape

//        last = float_to_int(vmax)+1;
//        i = float_to_int(vmin);
        last = (int)vmax + 1; // N
        i = (int)vmin; // N

        if (zoom == 1) {
            zoomed_scale((int)vmin, (int)vmax);
        } else {
            for (; i < last; i++) {
                condition = true;
                if (!modulo() && i < _input.min())
                    condition = false;

                if (condition) {  // Show a tick if necessary
                    // Calculate the location of this tick
                    marker_ys = scrn_rect.top + ((i - vmin) * factor()/*+.5f*/);
                    // marker_ys = scrn_rect.top + (int)((i - vmin) * factor() + .5);
                    // Block calculation artifact from drawing ticks below min coordinate.
                    // Calculation here accounts for text height.

                    if ((marker_ys < (scrn_rect.top + 4))
                            || (marker_ys > (height - 4))) {
                        // Magic numbers!!!
                        continue;
                    }

                    if (oddtype == 1)
                        k = i + 1; //enable ticks at odd values
                    else
                        k = i;

                    // Minor ticks
                    if (_minor_divs) {
                        // if ((i % _minor_divs) == 0) {
                        if (!(k % (int)_minor_divs)) {
                            if (((marker_ys - 5) > scrn_rect.top)
                                    && ((marker_ys + 5) < (height))) {

                                //vertical/left OR vertical/right
                                if (option_both()) {
                                    if (tick_type == "line") {
                                        if (tick_length == "variable") {
                                            draw_line(scrn_rect.left, marker_ys,
                                                    marker_xs, marker_ys);
                                            draw_line(marker_xe, marker_ys,
                                                    width, marker_ys);
                                        } else {
                                            draw_line(scrn_rect.left, marker_ys,
                                                    marker_xs, marker_ys);
                                            draw_line(marker_xe, marker_ys,
                                                    width, marker_ys);
                                        }

                                    } else if (tick_type == "circle") {
                                        circle(scrn_rect.left,(float)marker_ys, 3.0);

                                    } else {
                                        // if neither line nor circle draw default as line
                                        draw_line(scrn_rect.left, marker_ys,
                                                marker_xs, marker_ys);
                                        draw_line(marker_xe, marker_ys,
                                                width, marker_ys);
                                    }
                                    // glBegin(GL_LINES);
                                    // glVertex2f(scrn_rect.left, marker_ys);
                                    // glVertex2f(marker_xs,      marker_ys);
                                    // glVertex2f(marker_xe,      marker_ys);
                                    // glVertex2f(scrn_rect.left + scrn_rect.right,  marker_ys);
                                    // glEnd();
                                    // anything other than option_both

                                } else {
                                    if (option_left()) {
                                        if (tick_type == "line") {
                                            if (tick_length == "variable") {
                                                draw_line(marker_xs + 4, marker_ys,
                                                        marker_xe, marker_ys);
                                            } else {
                                                draw_line(marker_xs, marker_ys,
                                                        marker_xe, marker_ys);
                                            }
                                        } else if (tick_type == "circle") {
                                            circle((float)marker_xs + 4, (float)marker_ys, 3.0);

                                        } else {
                                            draw_line(marker_xs + 4, marker_ys,
                                                    marker_xe, marker_ys);
                                        }

                                    }  else {
                                        if (tick_type == "line") {
                                            if (tick_length == "variable") {
                                                draw_line(marker_xs, marker_ys,
                                                        marker_xe - 4, marker_ys);
                                            } else {
                                                draw_line(marker_xs, marker_ys,
                                                        marker_xe, marker_ys);
                                            }

                                        } else if (tick_type == "circle") {
                                            circle((float)marker_xe - 4, (float)marker_ys, 3.0);
                                        } else {
                                            draw_line(marker_xs, marker_ys,
                                                    marker_xe - 4, marker_ys);
                                        }
                                    }
                                } //end huds both
                            }
                        } //end draw minor ticks
                    }  //end minor ticks

                    // Major ticks
                    if (_major_divs) {
                        if (!(k % (int)_major_divs)) {

                            if (modulo()) {
                                disp_val = i % (int) modulo(); // ?????????
                                if (disp_val < 0) {
                                    while (disp_val < 0)
                                        disp_val += modulo();
                                }
                            } else {
                                disp_val = i;
                            }

// FIXME what nonsense is this?!?
                            lenstr = snprintf(buf, BUFSIZE, "%d", int(disp_val * _input.factor()/*+.5*/));   // was data_scaling ... makes no sense at all
                            // (int)(disp_val  * data_scaling() +.5));
                            /* if (((marker_ys - 8) > scrn_rect.top) &&
                               ((marker_ys + 8) < (height))){ */
                            // option_both
                            if (option_both()) {
                                // draw_line(scrn_rect.left, marker_ys,
                                //              marker_xs,      marker_ys);
                                // draw_line(marker_xs, marker_ys,
                                //              scrn_rect.left + scrn_rect.right,
                                //              marker_ys);
                                if (tick_type == "line") {
                                    glBegin(GL_LINE_STRIP);
                                    glVertex2f(scrn_rect.left, marker_ys);
                                    glVertex2f(marker_xs, marker_ys);
                                    glVertex2f(width, marker_ys);
                                    glEnd();

                                } else if (tick_type == "circle") {
                                    circle(scrn_rect.left, (float)marker_ys, 5.0);

                                } else {
                                    glBegin(GL_LINE_STRIP);
                                    glVertex2f(scrn_rect.left, marker_ys);
                                    glVertex2f(marker_xs, marker_ys);
                                    glVertex2f(width, marker_ys);
                                    glEnd();
                                }

                                if (!option_notext())
                                    draw_text(marker_xs + 2, marker_ys, buf, 0);

                            } else {
                                /* Changes are made to draw a circle when tick_type="circle" */
                                // anything other than option_both
                                if (tick_type == "line")
                                    draw_line(marker_xs, marker_ys, marker_xe, marker_ys);
                                else if (tick_type == "circle")
                                    circle((float)marker_xs + 4, (float)marker_ys, 5.0);
                                else
                                    draw_line(marker_xs, marker_ys, marker_xe, marker_ys);

                                if (!option_notext()) {
                                    if (option_left()) {
                                        draw_text(marker_xs - 8 * lenstr - 2,
                                                marker_ys - 4, buf, 0);
                                    } else {
                                        draw_text(marker_xe + 3 * lenstr,
                                                marker_ys - 4, buf, 0);
                                    } //End if option_left
                                } //End if !option_notext
                            }  //End if huds-both
                        }  // End if draw major ticks
                    }   // End if major ticks
                }  // End condition
            }  // End for
        }  //end of zoom
        // End if VERTICAL SCALE TYPE (tape loop yet to be closed)

    } else {
        // Horizontal scale by default
        // left tick bar
        if (draw_tick_left)
            draw_line(scrn_rect.left, scrn_rect.top, scrn_rect.left, height);

        // right tick bar
        if (draw_tick_right)
            draw_line(width, scrn_rect.top, width, height);

        marker_ys = scrn_rect.top;    // Starting point for
        marker_ye = height;           // tick y location calcs
        marker_xe = width;
        marker_xs = scrn_rect.left + ((cur_value - vmin) * factor() /*+ .5f*/);

        //    glBegin(GL_LINES);
        // left tick bar
        //    glVertex2f(scrn_rect.left, scrn_rect.top);
        //    glVertex2f(scrn_rect.left, marker_ye);

        // right tick bar
        //    glVertex2f(marker_xe, scrn_rect.top);
        //    glVertex2f(marker_xe, marker_ye);
        //    glEnd();

        if (option_top()) {
            // Bottom box line
            if (draw_cap_bottom)
                draw_line(scrn_rect.left, scrn_rect.top, width, scrn_rect.top);

            // Tick point adjust
            marker_ye  = scrn_rect.top + scrn_rect.bottom / 2;
            // Bottom arrow
            // draw_line(mid_scr.x, marker_ye,
            //              mid_scr.x - scrn_rect.bottom / 4, scrn_rect.top);
            // draw_line(mid_scr.x, marker_ye,
            //              mid_scr.x + scrn_rect.bottom / 4, scrn_rect.top);
            // draw pointer
            if (pointer) {
                if (pointer_type == "moving") {
                    if (zoom == 0) {
                        //Code for Moving Type Pointer
                        // static float xcentre, xpoint, ypoint;
                        // static int range;
                        if (cur_value > _input.max())
                            cur_value = _input.max();
                        if (cur_value < _input.min())
                            cur_value = _input.min();

                        float xcentre = mid_scr.x;
                        float range = scrn_rect.right;
                        float xpoint = xcentre + (cur_value * range / val_span);
                        float ypoint = scrn_rect.top - marker_offset;
                        draw_line(xcentre, ypoint, xpoint, ypoint);
                        draw_line(xpoint, ypoint, xpoint, ypoint + marker_offset);
                        draw_line(xpoint, ypoint + marker_offset, xpoint + 5.0, ypoint + 5.0);
                        draw_line(xpoint, ypoint + marker_offset, xpoint - 5.0, ypoint + 5.0);
                    }

                } else {
                    //default to fixed
                    fixed(marker_xs - scrn_rect.bottom / 4, scrn_rect.top, marker_xs,
                            marker_ye, marker_xs + scrn_rect.bottom / 4, scrn_rect.top);
                }
            }  //if pointer
        } //End Horizontal scale/top

        if (option_bottom()) {
            // Top box line
            if (draw_cap_top)
                draw_line(scrn_rect.left, height, width, height);

            // Tick point adjust
            marker_ys = height - scrn_rect.bottom / 2;
            // Top arrow
            //      draw_line(mid_scr.x + scrn_rect.bottom / 4,
            //                   scrn_rect.top + scrn_rect.bottom,
            //                   mid_scr.x, marker_ys);
            //      draw_line(mid_scr.x - scrn_rect.bottom / 4,
            //                   scrn_rect.top + scrn_rect.bottom,
            //                   mid_scr.x , marker_ys);

            // draw pointer
            if (pointer) {
                if (pointer_type == "moving") {
                    if (zoom == 0) {
                        //Code for Moving Type Pointer
                        // static float xcentre, xpoint, ypoint;
                        // static int range, hgt;
                        if (cur_value > _input.max())
                            cur_value = _input.max();
                        if (cur_value < _input.min())
                            cur_value = _input.min();

                        float xcentre = mid_scr.x ;
                        float range = scrn_rect.right;
                        float hgt = scrn_rect.top + scrn_rect.bottom;
                        float xpoint = xcentre + (cur_value * range / val_span);
                        float ypoint = hgt + marker_offset;
                        draw_line(xcentre, ypoint, xpoint, ypoint);
                        draw_line(xpoint, ypoint, xpoint, ypoint - marker_offset);
                        draw_line(xpoint, ypoint - marker_offset, xpoint + 5.0, ypoint - 5.0);
                        draw_line(xpoint, ypoint - marker_offset, xpoint - 5.0, ypoint - 5.0);
                    }
                } else {
                    fixed(marker_xs + scrn_rect.bottom / 4, height, marker_xs, marker_ys,
                            marker_xs - scrn_rect.bottom / 4, height);
                }
            } //if pointer
        }  //end horizontal scale bottom


        if (zoom == 1) {
            zoomed_scale((int)vmin,(int)vmax);
        } else {
            //default to zoom=0
            last = (int)vmax + 1;
            i = (int)vmin;
            for (; i < last; i++) {
                // for (i = (int)vmin; i <= (int)vmax; i++)     {
                // printf("<*> i = %d\n", i);
                condition = true;
                if (!modulo() && i < _input.min())
                    condition = false;

                // printf("<**> i = %d\n", i);
                if (condition) {
                    // marker_xs = scrn_rect.left + (int)((i - vmin) * factor() + .5);
                    marker_xs = scrn_rect.left + (((i - vmin) * factor()/*+ .5f*/));

                    if (oddtype == 1)
                        k = i + 1; //enable ticks at odd values
                    else
                        k = i;

                    if (_minor_divs) {
                        //          if ((i % (int)_minor_divs) == 0) {
                        //draw minor ticks
                        if (!(k % (int)_minor_divs)) {
                            // draw in ticks only if they aren't too close to the edge.
                            if (((marker_xs - 5) > scrn_rect.left)
                                    && ((marker_xs + 5)< (scrn_rect.left + scrn_rect.right))) {

                                if (option_both()) {
                                    if (tick_length == "variable") {
                                        draw_line(marker_xs, scrn_rect.top,
                                                marker_xs, marker_ys - 4);
                                        draw_line(marker_xs, marker_ye + 4,
                                                marker_xs, height);
                                    } else {
                                        draw_line(marker_xs, scrn_rect.top,
                                                marker_xs, marker_ys);
                                        draw_line(marker_xs, marker_ye,
                                                marker_xs, height);
                                    }
                                    // glBegin(GL_LINES);
                                    // glVertex2f(marker_xs, scrn_rect.top);
                                    // glVertex2f(marker_xs, marker_ys - 4);
                                    // glVertex2f(marker_xs, marker_ye + 4);
                                    // glVertex2f(marker_xs, scrn_rect.top + scrn_rect.bottom);
                                    // glEnd();

                                } else {
                                    if (option_top()) {
                                        //draw minor ticks
                                        if (tick_length == "variable")
                                            draw_line(marker_xs, marker_ys, marker_xs, marker_ye - 4);
                                        else
                                            draw_line(marker_xs, marker_ys, marker_xs, marker_ye);

                                    } else if (tick_length == "variable") {
                                        draw_line(marker_xs, marker_ys + 4, marker_xs, marker_ye);
                                    } else {
                                        draw_line(marker_xs, marker_ys, marker_xs, marker_ye);
                                    }
                                }
                            }
                        } //end draw minor ticks
                    } //end minor ticks

                    //major ticks
                    if (_major_divs) {
                        // printf("i = %d\n", i);
                        // if ((i % (int)_major_divs)==0) {
                        //     draw major ticks

                        if (!(k % (int)_major_divs)) {
                            if (modulo()) {
                                disp_val = i % (int) modulo(); // ?????????
                                if (disp_val < 0) {
                                    while (disp_val<0)
                                        disp_val += modulo();
                                }
                            } else {
                                disp_val = i;
                            }
                            // printf("disp_val = %d\n", disp_val);
                            // printf("%d\n", (int)(disp_val  * (double)data_scaling() + 0.5));
                            lenstr = snprintf(buf, BUFSIZE, "%d",
                                    // (int)(disp_val  * data_scaling() +.5));
                                    int(disp_val * _input.factor() /*+.5*/));  // was data_scaling() ... makes no sense at all

                            // Draw major ticks and text only if far enough from the edge.
                            if (((marker_xs - 10)> scrn_rect.left)
                                    && ((marker_xs + 10) < (scrn_rect.left + scrn_rect.right))) {
                                if (option_both()) {
                                    // draw_line(marker_xs, scrn_rect.top,
                                    //              marker_xs, marker_ys);
                                    // draw_line(marker_xs, marker_ye,
                                    //              marker_xs, scrn_rect.top + scrn_rect.bottom);
                                    glBegin(GL_LINE_STRIP);
                                    glVertex2f(marker_xs, scrn_rect.top);
                                    glVertex2f(marker_xs, marker_ye);
                                    glVertex2f(marker_xs, height);
                                    glEnd();

                                    if (!option_notext()) {
                                        draw_text(marker_xs - 4 * lenstr,
                                                marker_ys + 4, buf, 0);
                                    }
                                } else {
                                    draw_line(marker_xs, marker_ys, marker_xs, marker_ye);

                                    if (!option_notext()) {
                                        if (option_top()) {
                                            draw_text(marker_xs - 4 * lenstr,
                                                    height - 10, buf, 0);

                                        }  else  {
                                            draw_text(marker_xs - 4 * lenstr,
                                                    scrn_rect.top, buf, 0);
                                        }
                                    }
                                }
                            }
                        }  //end draw major ticks
                    } //endif major ticks
                }   //end condition
            } //end for
        }  //end zoom
    } //end horizontal/vertical scale
} //draw



void HUD::Tape::circle(float x, float y, float size)
{
    glEnable(GL_POINT_SMOOTH);
    glPointSize(size);

    glBegin(GL_POINTS);
    glVertex2f(x, y);
    glEnd();

    glPointSize(1.0);
    glDisable(GL_POINT_SMOOTH);
}


void HUD::Tape::fixed(float x1, float y1, float x2, float y2, float x3, float y3)
{
    glBegin(GL_LINE_STRIP);
    glVertex2f(x1, y1);
    glVertex2f(x2, y2);
    glVertex2f(x3, y3);
    glEnd();
}


void HUD::Tape::zoomed_scale(int first, int last)
{
    Point mid_scr = get_centroid();
    Rect scrn_rect = get_location();
    const int BUFSIZE = 80;
    char buf[BUFSIZE];
    int data[80];

    float x, y, w, h, bottom;
    float cur_value = _input.getFloatValue();
    if (cur_value > _input.max())
        cur_value = _input.max();
    if (cur_value < _input.min())
        cur_value = _input.min();

    int a = 0;

    while (first <= last) {
        if ((first % (int)_major_divs) == 0) {
            data[a] = first;
            a++ ;
        }
        first++;
    }
    int centre = a / 2;

    if (option_vert()) {
        x = scrn_rect.left;
        y = scrn_rect.top;
        w = scrn_rect.left + scrn_rect.right;
        h = scrn_rect.top + scrn_rect.bottom;
        bottom = scrn_rect.bottom;

        float xstart, yfirst, ycentre, ysecond;

        float hgt = bottom * 20.0 / 100.0;  // 60% of height should be zoomed
        yfirst = mid_scr.y - hgt;
        ycentre = mid_scr.y;
        ysecond = mid_scr.y + hgt;
        float range = hgt * 2;

        int i;
        float factor = range / 10.0;

        float hgt1 = bottom * 30.0 / 100.0;
        int  incrs = ((int)val_span - (_major_divs * 2)) / _major_divs ;
        int  incr = incrs / 2;
        float factors = hgt1 / incr;

        // begin
        //this is for moving type pointer
        static float ycent, ypoint, xpoint;
        static float wth;

        ycent = mid_scr.y;
        wth = scrn_rect.left + scrn_rect.right;

        if (cur_value <= data[centre + 1])
            if (cur_value > data[centre]) {
                ypoint = ycent + ((cur_value - data[centre]) * hgt / _major_divs);
            }

        if (cur_value >= data[centre - 1])
            if (cur_value <= data[centre]) {
                ypoint = ycent - ((data[centre] - cur_value) * hgt / _major_divs);
            }

        if (cur_value < data[centre - 1])
            if (cur_value >= _input.min()) {
                float diff  = _input.min() - data[centre - 1];
                float diff1 = cur_value - data[centre - 1];
                float val = (diff1 * hgt1) / diff;

                ypoint = ycent - hgt - val;
            }

        if (cur_value > data[centre + 1])
            if (cur_value <= _input.max()) {
                float diff  = _input.max() - data[centre + 1];
                float diff1 = cur_value - data[centre + 1];
                float val = (diff1 * hgt1) / diff;

                ypoint = ycent + hgt + val;
            }

        if (option_left()) {
            xstart = w;

            draw_line(xstart, ycentre, xstart - 5.0, ycentre); //centre tick

            snprintf(buf, BUFSIZE, "%3.0f\n", (float)(data[centre] * _input.factor())); // was data_scaling() ... makes not sense at all

            if (!option_notext())
                draw_text(x, ycentre, buf, 0);

            for (i = 1; i < 5; i++) {
                yfirst += factor;
                ycentre += factor;
                circle(xstart - 2.5, yfirst, 3.0);
                circle(xstart - 2.5, ycentre, 3.0);
            }

            yfirst = mid_scr.y - hgt;

            for (i = 0; i <= incr; i++) {
                draw_line(xstart, yfirst, xstart - 5.0, yfirst);
                draw_line(xstart, ysecond, xstart - 5.0, ysecond);

                snprintf(buf, BUFSIZE, "%3.0f\n", (float)(data[centre - i - 1] * _input.factor()));  // was data_scaling() ... makes no sense at all

                if (!option_notext())
                    draw_text(x, yfirst, buf, 0);

                snprintf(buf, BUFSIZE, "%3.0f\n", (float)(data[centre + i + 1] * _input.factor())); // was data_scaling() ... makes no sense at all

                if (!option_notext())
                    draw_text(x, ysecond, buf, 0);

                yfirst -= factors;
                ysecond += factors;

            }

            //to draw moving type pointer for left option
            //begin
            xpoint = wth + 10.0;

            if (pointer_type == "moving") {
                draw_line(xpoint, ycent, xpoint, ypoint);
                draw_line(xpoint, ypoint, xpoint - 10.0, ypoint);
                draw_line(xpoint - 10.0, ypoint, xpoint - 5.0, ypoint + 5.0);
                draw_line(xpoint - 10.0, ypoint, xpoint - 5.0, ypoint - 5.0);
            }
            //end

        } else {
            //option_right
            xstart = (x + w) / 2;

            draw_line(xstart, ycentre, xstart + 5.0, ycentre); //centre tick

            snprintf(buf, BUFSIZE, "%3.0f\n", (float)(data[centre] * _input.factor())); // was data_scaling() ... makes no sense at all

            if (!option_notext())
                draw_text(w, ycentre, buf, 0);

            for (i = 1; i < 5; i++) {
                yfirst += factor;
                ycentre += factor;
                circle(xstart + 2.5, yfirst, 3.0);
                circle(xstart + 2.5, ycentre, 3.0);
            }

            yfirst = mid_scr.y - hgt;

            for (i = 0; i <= incr; i++) {
                draw_line(xstart, yfirst, xstart + 5.0, yfirst);
                draw_line(xstart, ysecond, xstart + 5.0, ysecond);

                snprintf(buf, BUFSIZE, "%3.0f\n", (float)(data[centre - i - 1] * _input.factor())); // was data_scaling() ... makes no sense at all

                if (!option_notext())
                    draw_text(w, yfirst, buf, 0);

                snprintf(buf, BUFSIZE, "%3.0f\n", (float)(data[centre + i + 1] * _input.factor()));

                if (!option_notext())
                    draw_text(w, ysecond, buf, 0);

                yfirst -= factors;
                ysecond += factors;

            }

            // to draw moving type pointer for right option
            //begin
            xpoint = scrn_rect.left;

            if (pointer_type == "moving") {
                draw_line(xpoint, ycent, xpoint, ypoint);
                draw_line(xpoint, ypoint, xpoint + 10.0, ypoint);
                draw_line(xpoint + 10.0, ypoint, xpoint + 5.0, ypoint + 5.0);
                draw_line(xpoint + 10.0, ypoint, xpoint + 5.0, ypoint - 5.0);
            }
            //end
        }//end option_right /left
        //end of vertical scale

    } else {
        //horizontal scale
        x = scrn_rect.left;
        y = scrn_rect.top;
        w = scrn_rect.left + scrn_rect.right;
        h = scrn_rect.top + scrn_rect.bottom;
        bottom = scrn_rect.right;

        float ystart, xfirst, xcentre, xsecond;

        float hgt = bottom * 20.0 / 100.0;  // 60% of height should be zoomed
        xfirst = mid_scr.x - hgt;
        xcentre = mid_scr.x;
        xsecond = mid_scr.x + hgt;
        float range = hgt * 2;

        int i;
        float factor = range / 10.0;

        float hgt1 = bottom * 30.0 / 100.0;
        int  incrs = ((int)val_span - (_major_divs * 2)) / _major_divs ;
        int  incr = incrs / 2;
        float factors = hgt1 / incr;


        //Code for Moving Type Pointer
        //begin
        static float xcent, xpoint, ypoint;

        xcent = mid_scr.x;

        if (cur_value <= data[centre + 1])
            if (cur_value > data[centre]) {
                xpoint = xcent + ((cur_value - data[centre]) * hgt / _major_divs);
            }

        if (cur_value >= data[centre - 1])
            if (cur_value <= data[centre]) {
                xpoint = xcent - ((data[centre] - cur_value) * hgt / _major_divs);
            }

        if (cur_value < data[centre - 1])
            if (cur_value >= _input.min()) {
                float diff = _input.min() - data[centre - 1];
                float diff1 = cur_value - data[centre - 1];
                float val = (diff1 * hgt1) / diff;

                xpoint = xcent - hgt - val;
            }


        if (cur_value > data[centre + 1])
            if (cur_value <= _input.max()) {
                float diff = _input.max() - data[centre + 1];
                float diff1 = cur_value - data[centre + 1];
                float val = (diff1 * hgt1) / diff;

                xpoint = xcent + hgt + val;
            }

        //end
        if (option_top()) {
            ystart = h;
            draw_line(xcentre, ystart, xcentre, ystart - 5.0); //centre tick

            snprintf(buf, BUFSIZE, "%3.0f\n", (float)(data[centre] * _input.factor()));  // was data_scaling() ... makes no sense at all

            if (!option_notext())
                draw_text(xcentre - 10.0, y, buf, 0);

            for (i = 1; i < 5; i++) {
                xfirst += factor;
                xcentre += factor;
                circle(xfirst, ystart - 2.5, 3.0);
                circle(xcentre, ystart - 2.5, 3.0);
            }

            xfirst = mid_scr.x - hgt;

            for (i = 0; i <= incr; i++) {
                draw_line(xfirst, ystart, xfirst,  ystart - 5.0);
                draw_line(xsecond, ystart, xsecond, ystart - 5.0);

                snprintf(buf, BUFSIZE, "%3.0f\n", (float)(data[centre - i - 1] * _input.factor())); // was data_scaling() ... makes no sense at all

                if (!option_notext())
                    draw_text(xfirst - 10.0, y, buf, 0);

                snprintf(buf, BUFSIZE, "%3.0f\n", (float)(data[centre + i + 1] * _input.factor())); // was data_scaling() ... makes no sense at all

                if (!option_notext())
                    draw_text(xsecond - 10.0, y, buf, 0);


                xfirst -= factors;
                xsecond += factors;
            }
            //this is for moving pointer for top option
            //begin

            ypoint = scrn_rect.top + scrn_rect.bottom + 10.0;

            if (pointer_type == "moving") {
                draw_line(xcent, ypoint, xpoint, ypoint);
                draw_line(xpoint, ypoint, xpoint, ypoint - 10.0);
                draw_line(xpoint, ypoint - 10.0, xpoint + 5.0, ypoint - 5.0);
                draw_line(xpoint, ypoint - 10.0, xpoint - 5.0, ypoint - 5.0);
            }
            //end of top option

        } else {
            //else option_bottom
            ystart = (y + h) / 2;

            //draw_line(xstart, yfirst,  xstart - 5.0, yfirst);
            draw_line(xcentre, ystart, xcentre, ystart + 5.0); //centre tick

            snprintf(buf, BUFSIZE, "%3.0f\n", (float)(data[centre] * _input.factor())); // was data_scaling() ... makes no sense at all

            if (!option_notext())
                draw_text(xcentre - 10.0, h, buf, 0);

            for (i = 1; i < 5; i++) {
                xfirst += factor;
                xcentre += factor;
                circle(xfirst, ystart + 2.5, 3.0);
                circle(xcentre, ystart + 2.5, 3.0);
            }

            xfirst = mid_scr.x - hgt;

            for (i = 0; i <= incr; i++) {
                draw_line(xfirst, ystart, xfirst, ystart + 5.0);
                draw_line(xsecond, ystart, xsecond, ystart + 5.0);

                snprintf(buf, BUFSIZE, "%3.0f\n", (float)(data[centre - i - 1] * _input.factor())); // was data_scaling() ... makes no sense at all

                if (!option_notext())
                    draw_text(xfirst - 10.0, h, buf, 0);

                snprintf(buf, BUFSIZE, "%3.0f\n", (float)(data[centre + i + 1] * _input.factor())); // was data_scaling() ... makes no sense at all

                if (!option_notext())
                    draw_text(xsecond - 10.0, h, buf, 0);

                xfirst -= factors;
                xsecond   += factors;
            }
            //this is for movimg pointer for bottom option
            //begin

            ypoint = scrn_rect.top - 10.0;
            if (pointer_type == "moving") {
                draw_line(xcent, ypoint, xpoint, ypoint);
                draw_line(xpoint, ypoint, xpoint, ypoint + 10.0);
                draw_line(xpoint, ypoint + 10.0, xpoint + 5.0, ypoint + 5.0);
                draw_line(xpoint, ypoint + 10.0, xpoint - 5.0, ypoint + 5.0);
            }
        }//end hud_top or hud_bottom
    }  //end of horizontal/vertical scales
}//end draw


