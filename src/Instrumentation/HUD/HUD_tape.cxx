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
    _draw_tick_bottom(n->getBoolValue("tick-bottom", false)),
    _draw_tick_top(n->getBoolValue("tick-top", false)),
    _draw_tick_right(n->getBoolValue("tick-right", false)),
    _draw_tick_left(n->getBoolValue("tick-left", false)),
    _draw_cap_bottom(n->getBoolValue("cap-bottom", false)),
    _draw_cap_top(n->getBoolValue("cap-top", false)),
    _draw_cap_right(n->getBoolValue("cap-right", false)),
    _draw_cap_left(n->getBoolValue("cap-left", false)),
    _marker_offset(n->getFloatValue("marker-offset", 0.0)),
    _pointer(n->getBoolValue("enable-pointer", true)),
    _zoom(n->getIntValue("zoom"))
{
    _half_width_units = range_to_show() / 2.0;

    const char *s;
    s = n->getStringValue("pointer-type");
    _pointer_type = strcmp(s, "moving") ? FIXED : MOVING;    // "fixed", "moving"

    s = n->getStringValue("tick-type");
    _tick_type = strcmp(s, "circle") ? LINE : CIRCLE;        // "circle", "line"

    s = n->getStringValue("tick-length");                    // "variable", "constant"
    _tick_length = strcmp(s, "constant") ? VARIABLE : CONSTANT;
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
    const int BUFSIZE = 80;
    char buf[BUFSIZE];
    int oddtype;
//    int k; //odd or even values for ticks		// FIXME odd scale

    Point mid_scr = get_centroid();
    float cur_value = _input.getFloatValue();

    if (int(floor(_input.max() + 0.5)) & 1)
        oddtype = 1; //draw ticks at odd values
    else
        oddtype = 0; //draw ticks at even values

    height = _y + _h;			// FIXME huh?
    width = _x + _w;


    if (_pointer) {
        if (_pointer_type == MOVING) {
            vmin = _input.min();
            vmax = _input.max();

        } else { // FIXED
            vmin = cur_value - _half_width_units; // width units == needle travel
            vmax = cur_value + _half_width_units; // or picture unit span.
            text_x = mid_scr.x;
            text_y = mid_scr.y;
        }

    } else {
        vmin = cur_value - _half_width_units; // width units == needle travel
        vmax = cur_value + _half_width_units; // or picture unit span.
        text_x = mid_scr.x;
        text_y = mid_scr.y;
    }


///////////////////////////////////////////////////////////////////////////////
// VERTICAL SCALE
///////////////////////////////////////////////////////////////////////////////

    // Draw the basic markings for the scale...

    if (option_vert()) { // Vertical scale
        // Bottom tick bar
        if (_draw_tick_bottom)
            draw_line(_x, _y, width, _y);

        // Top tick bar
        if (_draw_tick_top)
            draw_line(_x, height, width, height);

        marker_xs = _x;  // x start
        marker_xe = width;           // x extent
        marker_ye = height;

        //    glBegin(GL_LINES);

        // Bottom tick bar
        //    glVertex2f(marker_xs, _y);
        //    glVertex2f(marker_xe, _y);

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

            if (_draw_cap_right)
                draw_line(marker_xe, _y, marker_xe, marker_ye);

            marker_xs  = marker_xe - _w / 3 + 0.5;   // Adjust tick xs

            // draw_line(marker_xs, mid_scr.y, marker_xe, mid_scr.y + _w / 6);
            // draw_line(marker_xs, mid_scr.y, marker_xe, mid_scr.y - _w / 6);

            // draw pointer
            if (_pointer) {
                if (_pointer_type == MOVING) {
                    if (!_zoom) {
                        //Code for Moving Type Pointer
                        float ycentre, ypoint, xpoint;
                        float range, wth;

                        if (_input.min() >= 0.0)
                            ycentre = _y;
                        else if (_input.max() + _input.min() == 0.0)
                            ycentre = mid_scr.y;
                        else if (oddtype)
                            ycentre = _y + (1.0 - _input.min()) * _h
                                    / (_input.max() - _input.min());
                        else
                            ycentre = _y + _input.min() * _h
                                    / (_input.max() - _input.min());

                        range = _h;
                        wth = _x + _w;

                        if (oddtype)
                            ypoint = ycentre + ((cur_value - 1.0) * range / _val_span);
                        else
                            ypoint = ycentre + (cur_value * range / _val_span);

                        xpoint = wth + _marker_offset;
                        draw_line(xpoint, ycentre, xpoint, ypoint);
                        draw_line(xpoint, ypoint, xpoint - _marker_offset, ypoint);
                        draw_line(xpoint - _marker_offset, ypoint, xpoint - 5.0, ypoint + 5.0);
                        draw_line(xpoint - _marker_offset, ypoint, xpoint - 5.0, ypoint - 5.0);
                    } // !_zoom

                } else {
                    // default to fixed
                    fixed(_marker_offset + marker_xe, text_y + _w / 6,
                            _marker_offset + marker_xs, text_y, _marker_offset + marker_xe,
                            text_y - _w / 6);
                } // end pointer type
            } // if pointer
        } // end vertical/left


        // begin vertical/right
        // First draw capping lines and pointers
        if (option_right()) {

            if (_draw_cap_left)
                draw_line(_x, _y, _x, marker_ye);

            marker_xe = _x + _w / 3 - 0.5;     // Adjust tick xe
            // Indicator carrot
            // draw_line(_x, mid_scr.y +  _w / 6, marker_xe, mid_scr.y);
            // draw_line(_x, mid_scr.y -  _w / 6, marker_xe, mid_scr.y);

            // draw pointer
            if (_pointer) {
                if (_pointer_type == MOVING) {
                    if (!_zoom) {
                        // type-fixed & _zoom=1, behaviour to be defined
                        // Code for Moving Type Pointer
                        float ycentre, ypoint, xpoint;
                        float range;

                        if (_input.min() >= 0.0)
                            ycentre = _y;
                        else if (_input.max() + _input.min() == 0.0)
                            ycentre = mid_scr.y;
                        else if (oddtype)
                            ycentre = _y + (1.0 - _input.min()) * _h / (_input.max() - _input.min());
                        else
                            ycentre = _y + _input.min() * _h / (_input.max() - _input.min());

                        range = _h;

                        if (oddtype)
                            ypoint = ycentre + ((cur_value - 1.0) * range / _val_span);
                        else
                            ypoint = ycentre + (cur_value * range / _val_span);

                        xpoint = _x - _marker_offset;
                        draw_line(xpoint, ycentre, xpoint, ypoint);
                        draw_line(xpoint, ypoint, xpoint + _marker_offset, ypoint);
                        draw_line(xpoint + _marker_offset, ypoint, xpoint + 5.0, ypoint + 5.0);
                        draw_line(xpoint + _marker_offset, ypoint, xpoint + 5.0, ypoint - 5.0);
                    }

                } else {
                    // default to fixed
                    fixed(-_marker_offset + _x, text_y +  _w / 6,
                            -_marker_offset + marker_xe, text_y, -_marker_offset + _x,
                            text_y - _w / 6);
                }
            } // if pointer
        } // end vertical/right


        // At this point marker x_start and x_end values are transposed.
        // To keep this from confusing things they are now swapped.
        if (option_both())
            marker_ye = marker_xs, marker_xs = marker_xe, marker_xe = marker_ye;



        // Work through from bottom to top of scale. Calculating where to put
        // minor and major ticks.

        // draw scale or tape
        if (_zoom) {
            zoomed_scale((int)vmin, (int)vmax);
        } else {

//#############################################################################

            int div_ratio;
            if (_minor_divs != 0.0f)
                div_ratio = int(_major_divs / _minor_divs + 0.5f);
            else
                div_ratio = 0, _minor_divs = _major_divs;			// FIXME move that into Scale/Constructor ?

            float vstart = floorf(vmin / _major_divs) * _major_divs;

            // FIXME consider oddtype
            for (int i = 0; ; i++) {
                float v = vstart + i * _minor_divs;

                if (!modulo() && (v < _input.min() || v > _input.max()))
                    continue;

                float y = _y + (v - vmin) * factor();

                if (y < _y + 4)
                    continue;
                if (y > height - 4)
                    break;

                if (div_ratio && i % div_ratio) { // minor div
                    if (option_both()) {
                        if (_tick_type == LINE) {
                            if (_tick_length == VARIABLE) {
                                draw_line(_x, y, marker_xs, y);
                                draw_line(marker_xe, y, width, y);
                            } else {
                                draw_line(_x, y, marker_xs, y);
                                draw_line(marker_xe, y, width, y);
                            }

                        } else if (_tick_type == CIRCLE) {
                            draw_bullet(_x, y, 3.0);

                        } else {
                            // if neither line nor circle draw default as line
                            draw_line(_x, y, marker_xs, y);
                            draw_line(marker_xe, y, width, y);
                        }
                        // glBegin(GL_LINES);
                        // glVertex2f(_x, y);
                        // glVertex2f(marker_xs,      y);
                        // glVertex2f(marker_xe,      y);
                        // glVertex2f(_x + _w,  y);
                        // glEnd();
                        // anything other than huds_both

                    } else {
                        if (option_left()) {
                            if (_tick_type == LINE) {
                                if (_tick_length == VARIABLE) {
                                    draw_line(marker_xs + 4, y, marker_xe, y);
                                } else {
                                    draw_line(marker_xs, y, marker_xe, y);
                                }
                            } else if (_tick_type == CIRCLE) {
                                draw_bullet(marker_xs + 4, y, 3.0);
                            } else {
                                draw_line(marker_xs + 4, y, marker_xe, y);
                            }

                        }  else {
                            if (_tick_type == LINE) {
                                if (_tick_length == VARIABLE) {
                                    draw_line(marker_xs, y, marker_xe - 4, y);
                                } else {
                                    draw_line(marker_xs, y, marker_xe, y);
                                }

                            } else if (_tick_type == CIRCLE) {
                                draw_bullet(marker_xe - 4, y, 3.0);
                            } else {
                                draw_line(marker_xs, y, marker_xe - 4, y);
                            }
                        }
                    } // end huds both

                } else { // major div
                    lenstr = snprintf(buf, BUFSIZE, "%d", int(v));

                    if (option_both()) {
                        // draw_line(_x, y, marker_xs, y);
                        // draw_line(marker_xs, y, _x + _w, y);
                        if (_tick_type == LINE) {
                            glBegin(GL_LINE_STRIP);
                            glVertex2f(_x, y);
                            glVertex2f(marker_xs, y);
                            glVertex2f(width, y);
                            glEnd();

                        } else if (_tick_type == CIRCLE) {
                            draw_bullet(_x, y, 5.0);

                        } else {
                            glBegin(GL_LINE_STRIP);
                            glVertex2f(_x, y);
                            glVertex2f(marker_xs, y);
                            glVertex2f(width, y);
                            glEnd();
                        }

                        if (!option_notext())
                            draw_text(marker_xs + 2, y, buf, 0);

                    } else {
                        /* Changes are made to draw a circle when tick_type=CIRCLE */
                        // anything other than option_both
                        if (_tick_type == LINE)
                            draw_line(marker_xs, y, marker_xe, y);
                        else if (_tick_type == CIRCLE)
                            draw_bullet(marker_xs + 4, y, 5.0);
                        else
                            draw_line(marker_xs, y, marker_xe, y);

                        if (!option_notext()) {
                            if (option_left())
                                draw_text(marker_xs - 8 * lenstr - 2, y - 4, buf, 0);
                            else
                                draw_text(marker_xe + 3 * lenstr, y - 4, buf, 0);
                        }
                    } // End if huds-both
                }
            }
        } // end of zoom


///////////////////////////////////////////////////////////////////////////////
// HORIZONTAL SCALE
///////////////////////////////////////////////////////////////////////////////


    } else {
        // left tick bar
        if (_draw_tick_left)
            draw_line(_x, _y, _x, height);

        // right tick bar
        if (_draw_tick_right)
            draw_line(width, _y, width, height);

        marker_ys = _y;    // Starting point for
        marker_ye = height;           // tick y location calcs
        marker_xe = width;
        marker_xs = _x + ((cur_value - vmin) * factor());

        //    glBegin(GL_LINES);
        // left tick bar
        //    glVertex2f(_x, _y);
        //    glVertex2f(_x, marker_ye);

        // right tick bar
        //    glVertex2f(marker_xe, _y);
        //    glVertex2f(marker_xe, marker_ye);
        //    glEnd();

        if (option_top()) {
            if (_draw_cap_bottom)
                draw_line(_x, _y, width, _y);

            // Tick point adjust
            marker_ye  = _y + _h / 2;
            // Bottom arrow
            // draw_line(mid_scr.x, marker_ye, mid_scr.x - _h / 4, _y);
            // draw_line(mid_scr.x, marker_ye, mid_scr.x + _h / 4, _y);
            // draw pointer
            if (_pointer) {
                if (_pointer_type == MOVING) {
                    if (!_zoom) {
                        //Code for Moving Type Pointer

                        float xcentre = mid_scr.x;
                        float range = _w;
                        float xpoint = xcentre + (cur_value * range / _val_span);
                        float ypoint = _y - _marker_offset;
                        draw_line(xcentre, ypoint, xpoint, ypoint);
                        draw_line(xpoint, ypoint, xpoint, ypoint + _marker_offset);
                        draw_line(xpoint, ypoint + _marker_offset, xpoint + 5.0, ypoint + 5.0);
                        draw_line(xpoint, ypoint + _marker_offset, xpoint - 5.0, ypoint + 5.0);
                    }

                } else {
                    //default to fixed
                    fixed(marker_xs - _h / 4, _y, marker_xs,
                            marker_ye, marker_xs + _h / 4, _y);
                }
            }
        } // End Horizontal scale/top

        if (option_bottom()) {
            if (_draw_cap_top)
                draw_line(_x, height, width, height);

            // Tick point adjust
            marker_ys = height - _h / 2;
            // Top arrow
            // draw_line(mid_scr.x + _h / 4, _y + _h, mid_scr.x, marker_ys);
            // draw_line(mid_scr.x - _h / 4, _y + _h, mid_scr.x , marker_ys);

            // draw pointer
            if (_pointer) {
                if (_pointer_type == MOVING) {
                    if (!_zoom) {
                        //Code for Moving Type Pointer

                        float xcentre = mid_scr.x ;
                        float range = _w;
                        float hgt = _y + _h;
                        float xpoint = xcentre + (cur_value * range / _val_span);
                        float ypoint = hgt + _marker_offset;
                        draw_line(xcentre, ypoint, xpoint, ypoint);
                        draw_line(xpoint, ypoint, xpoint, ypoint - _marker_offset);
                        draw_line(xpoint, ypoint - _marker_offset, xpoint + 5.0, ypoint - 5.0);
                        draw_line(xpoint, ypoint - _marker_offset, xpoint - 5.0, ypoint - 5.0);
                    }
                } else {
                    fixed(marker_xs + _h / 4, height, marker_xs, marker_ys,
                            marker_xs - _h / 4, height);
                }
            }
        } //end horizontal scale bottom


        if (_zoom) {
            zoomed_scale((int)vmin,(int)vmax);
        } else {
            int div_ratio;					// FIXME abstract that out of hor/vert
            if (_minor_divs != 0.0f)
                div_ratio = int(_major_divs / _minor_divs + 0.5f);
            else
                div_ratio = 0, _minor_divs = _major_divs;			// FIXME move that into Scale/Constructor ?

            float vstart = floorf(vmin / _major_divs) * _major_divs;

            // FIXME consider oddtype
            for (int i = 0; ; i++) {
                float v = vstart + i * _minor_divs;

                if (!modulo() && (v < _input.min() || v > _input.max()))
                    continue;

                float x = _x + (v - vmin) * factor();

                if (x < _x + 4)
                    continue;
                if (x > width - 4)
                    break;

                if (div_ratio && i % div_ratio) { // minor div
                    if (option_both()) {
                        if (_tick_length == VARIABLE) {
                            draw_line(x, _y, x, marker_ys - 4);
                            draw_line(x, marker_ye + 4, x, height);
                        } else {
                            draw_line(x, _y, x, marker_ys);
                            draw_line(x, marker_ye, x, height);
                        }
                        // glBegin(GL_LINES);
                        // glVertex2f(x, _y);
                        // glVertex2f(x, marker_ys - 4);
                        // glVertex2f(x, marker_ye + 4);
                        // glVertex2f(x, _y + _h);
                        // glEnd();

                    } else {
                        if (option_top()) {
                            // draw minor ticks
                            if (_tick_length == VARIABLE)
                                draw_line(x, marker_ys, x, marker_ye - 4);
                            else
                                draw_line(x, marker_ys, x, marker_ye);

                        } else if (_tick_length == VARIABLE) {
                            draw_line(x, marker_ys + 4, x, marker_ye);
                        } else {
                            draw_line(x, marker_ys, x, marker_ye);
                        }
                    }

                } else { // major divs
                    lenstr = snprintf(buf, BUFSIZE, "%d", int(v));

                    // Draw major ticks and text only if far enough from the edge.			// FIXME
                    if (x < _x + 10 || x + 10 > _x + _w)
                        continue;

                    if (option_both()) {
                        // draw_line(x, _y,
                        //              x, marker_ys);
                        // draw_line(x, marker_ye,
                        //              x, _y + _h);
                        glBegin(GL_LINE_STRIP);
                        glVertex2f(x, _y);
                        glVertex2f(x, marker_ye);
                        glVertex2f(x, height);
                        glEnd();

                        if (!option_notext())
                            draw_text(x - 4 * lenstr, marker_ys + 4, buf, 0);

                    } else {
                        draw_line(x, marker_ys, x, marker_ye);

                        if (!option_notext()) {
                            if (option_top())
                                draw_text(x - 4 * lenstr, height - 10, buf, 0);
                            else
                                draw_text(x - 4 * lenstr, _y, buf, 0);
                        }
                    }
                }
            } // end for
        } // end zoom
    } // end horizontal/vertical scale
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
    const int BUFSIZE = 80;
    char buf[BUFSIZE];
    int data[80];

    float x, y, w, h, bottom;
    float cur_value = _input.getFloatValue();
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
        x = _x;
        y = _y;
        w = _x + _w;
        h = _y + _h;
        bottom = _h;

        float xstart, yfirst, ycentre, ysecond;

        float hgt = bottom * 20.0 / 100.0;  // 60% of height should be zoomed
        yfirst = mid_scr.y - hgt;
        ycentre = mid_scr.y;
        ysecond = mid_scr.y + hgt;
        float range = hgt * 2;

        int i;
        float factor = range / 10.0;

        float hgt1 = bottom * 30.0 / 100.0;
        int  incrs = int((_val_span - _major_divs * 2.0) / _major_divs);
        int  incr = incrs / 2;
        float factors = hgt1 / incr;

        // begin
        //this is for moving type pointer
        static float ycent, ypoint, xpoint;					// FIXME really static?
        static float wth;

        ycent = mid_scr.y;
        wth = _x + _w;

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

            snprintf(buf, BUFSIZE, "%3.0f\n", float(data[centre] * _input.factor())); // was data_scaling() ... makes not sense at all

            if (!option_notext())
                draw_text(x, ycentre, buf, 0);

            for (i = 1; i < 5; i++) {
                yfirst += factor;
                ycentre += factor;
                draw_bullet(xstart - 2.5, yfirst, 3.0);
                draw_bullet(xstart - 2.5, ycentre, 3.0);
            }

            yfirst = mid_scr.y - hgt;

            for (i = 0; i <= incr; i++) {
                draw_line(xstart, yfirst, xstart - 5.0, yfirst);
                draw_line(xstart, ysecond, xstart - 5.0, ysecond);

                snprintf(buf, BUFSIZE, "%3.0f\n", float(data[centre - i - 1] * _input.factor()));  // was data_scaling() ... makes no sense at all

                if (!option_notext())
                    draw_text(x, yfirst, buf, 0);

                snprintf(buf, BUFSIZE, "%3.0f\n", float(data[centre + i + 1] * _input.factor())); // was data_scaling() ... makes no sense at all

                if (!option_notext())
                    draw_text(x, ysecond, buf, 0);

                yfirst -= factors;
                ysecond += factors;

            }

            //to draw moving type pointer for left option
            //begin
            xpoint = wth + 10.0;

            if (_pointer_type == MOVING) {
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

            snprintf(buf, BUFSIZE, "%3.0f\n", float(data[centre] * _input.factor())); // was data_scaling() ... makes no sense at all

            if (!option_notext())
                draw_text(w, ycentre, buf, 0);

            for (i = 1; i < 5; i++) {
                yfirst += factor;
                ycentre += factor;
                draw_bullet(xstart + 2.5, yfirst, 3.0);
                draw_bullet(xstart + 2.5, ycentre, 3.0);
            }

            yfirst = mid_scr.y - hgt;

            for (i = 0; i <= incr; i++) {
                draw_line(xstart, yfirst, xstart + 5.0, yfirst);
                draw_line(xstart, ysecond, xstart + 5.0, ysecond);

                snprintf(buf, BUFSIZE, "%3.0f\n", float(data[centre - i - 1] * _input.factor())); // was data_scaling() ... makes no sense at all

                if (!option_notext())
                    draw_text(w, yfirst, buf, 0);

                snprintf(buf, BUFSIZE, "%3.0f\n", float(data[centre + i + 1] * _input.factor()));

                if (!option_notext())
                    draw_text(w, ysecond, buf, 0);

                yfirst -= factors;
                ysecond += factors;

            }

            // to draw moving type pointer for right option
            //begin
            xpoint = _x;

            if (_pointer_type == MOVING) {
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
        x = _x;
        y = _y;
        w = _x + _w;
        h = _y + _h;
        bottom = _w;

        float ystart, xfirst, xcentre, xsecond;

        float hgt = bottom * 20.0 / 100.0;  // 60% of height should be zoomed
        xfirst = mid_scr.x - hgt;
        xcentre = mid_scr.x;
        xsecond = mid_scr.x + hgt;
        float range = hgt * 2;

        int i;
        float factor = range / 10.0;

        float hgt1 = bottom * 30.0 / 100.0;
        int  incrs = int((_val_span - _major_divs * 2.0) / _major_divs);
        int  incr = incrs / 2;
        float factors = hgt1 / incr;


        //Code for Moving Type Pointer
        //begin
        static float xcent, xpoint, ypoint;				// FIXME really static?

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

            snprintf(buf, BUFSIZE, "%3.0f\n", float(data[centre] * _input.factor()));  // was data_scaling() ... makes no sense at all

            if (!option_notext())
                draw_text(xcentre - 10.0, y, buf, 0);

            for (i = 1; i < 5; i++) {
                xfirst += factor;
                xcentre += factor;
                draw_bullet(xfirst, ystart - 2.5, 3.0);
                draw_bullet(xcentre, ystart - 2.5, 3.0);
            }

            xfirst = mid_scr.x - hgt;

            for (i = 0; i <= incr; i++) {
                draw_line(xfirst, ystart, xfirst,  ystart - 5.0);
                draw_line(xsecond, ystart, xsecond, ystart - 5.0);

                snprintf(buf, BUFSIZE, "%3.0f\n", float(data[centre - i - 1] * _input.factor())); // was data_scaling() ... makes no sense at all

                if (!option_notext())
                    draw_text(xfirst - 10.0, y, buf, 0);

                snprintf(buf, BUFSIZE, "%3.0f\n", float(data[centre + i + 1] * _input.factor())); // was data_scaling() ... makes no sense at all

                if (!option_notext())
                    draw_text(xsecond - 10.0, y, buf, 0);


                xfirst -= factors;
                xsecond += factors;
            }
            //this is for moving pointer for top option
            //begin

            ypoint = _y + _h + 10.0;

            if (_pointer_type == MOVING) {
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

            snprintf(buf, BUFSIZE, "%3.0f\n", float(data[centre] * _input.factor())); // was data_scaling() ... makes no sense at all

            if (!option_notext())
                draw_text(xcentre - 10.0, h, buf, 0);

            for (i = 1; i < 5; i++) {
                xfirst += factor;
                xcentre += factor;
                draw_bullet(xfirst, ystart + 2.5, 3.0);
                draw_bullet(xcentre, ystart + 2.5, 3.0);
            }

            xfirst = mid_scr.x - hgt;

            for (i = 0; i <= incr; i++) {
                draw_line(xfirst, ystart, xfirst, ystart + 5.0);
                draw_line(xsecond, ystart, xsecond, ystart + 5.0);

                snprintf(buf, BUFSIZE, "%3.0f\n", float(data[centre - i - 1] * _input.factor())); // was data_scaling() ... makes no sense at all

                if (!option_notext())
                    draw_text(xfirst - 10.0, h, buf, 0);

                snprintf(buf, BUFSIZE, "%3.0f\n", float(data[centre + i + 1] * _input.factor())); // was data_scaling() ... makes no sense at all

                if (!option_notext())
                    draw_text(xsecond - 10.0, h, buf, 0);

                xfirst -= factors;
                xsecond   += factors;
            }
            //this is for movimg pointer for bottom option
            //begin

            ypoint = _y - 10.0;
            if (_pointer_type == MOVING) {
                draw_line(xcent, ypoint, xpoint, ypoint);
                draw_line(xpoint, ypoint, xpoint, ypoint + 10.0);
                draw_line(xpoint, ypoint + 10.0, xpoint + 5.0, ypoint + 5.0);
                draw_line(xpoint, ypoint + 10.0, xpoint - 5.0, ypoint + 5.0);
            }
        }//end hud_top or hud_bottom
    }  //end of horizontal/vertical scales
}//end draw


