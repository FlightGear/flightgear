// HUD_gauge.cxx -- HUD Gauge Instrument
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


HUD::Gauge::Gauge(HUD *hud, const SGPropertyNode *n, float x, float y) :
    Scale(hud, n, x, y)
{
}


// As implemented, draw only correctly draws a horizontal or vertical
// scale. It should contain a variation that permits clock type displays.
// Now is supports "tickless" displays such as control surface indicators.
// This routine should be worked over before using. Current value would be
// fetched and not used if not commented out. Clearly that is intollerable.

void HUD::Gauge::draw(void)
{
    if (!_input.isValid())
        return;

    float marker_xs, marker_xe;
    float marker_ys, marker_ye;
    float text_y;
    int i;
    const int BUFSIZE = 80;
    char buf[BUFSIZE];
    bool condition;
    int disp_val = 0;
    float vmin = _input.min();
    float vmax = _input.max();
    float cur_value = _input.getFloatValue();

    float right = _x + _w;
    float top = _y + _h;
    float bottom_4 = _h / 4.0;		// FIXME

    // Draw the basic markings for the scale...
    if (option_vert()) { // Vertical scale
        // Bottom tick bar
        draw_line(_x, _y, right, _y);

        // Top tick bar
        draw_line(_x, top, right, top);

        marker_xs = _x;
        marker_xe = right;

        if (option_left()) { // Read left, so line down right side
            draw_line(right, _y, right, top);
            marker_xs  = marker_xe - _w / 3.0;   // Adjust tick
        }

        if (option_right()) {     // Read  right, so down left sides
            draw_line(_x, _y, _x, top);
            marker_xe = _x + _w / 3.0;   // Adjust tick
        }

        // At this point marker x_start and x_end values are transposed.
        // To keep this from confusing things they are now interchanged.
        if (option_both()) {
            marker_ye = marker_xs;
            marker_xs = marker_xe;
            marker_xe = marker_ye;
        }

        // Work through from bottom to top of scale. Calculating where to put
        // minor and major ticks.

        if (!option_noticks()) {    // If not no ticks...:)
            // Calculate x marker offsets
            int last = (int)vmax + 1;
            i = (int)vmin;

            for (; i < last; i++) {
                // Calculate the location of this tick
                marker_ys = _y + (i - vmin) * factor()/* +.5f*/;

                // We compute marker_ys even though we don't know if we will use
                // either major or minor divisions. Simpler.

                if (_minor_divs) {                  // Minor tick marks
                    if (!(i % (int)_minor_divs)) {
                        if (option_left() && option_right()) {
                            draw_line(_x, marker_ys, marker_xs - 3, marker_ys);
                            draw_line(marker_xe + 3, marker_ys, right, marker_ys);

                        } else if (option_left()) {
                            draw_line(marker_xs + 3, marker_ys, marker_xe, marker_ys);
                        } else {
                            draw_line(marker_xs, marker_ys, marker_xe - 3, marker_ys);
                        }
                    }
                }

                // Now we work on the major divisions. Since these are also labeled
                // and no labels are drawn otherwise, we label inside this if
                // statement.

                if (_major_divs) {                  // Major tick mark
                    if (!(i % (int)_major_divs)) {
                        if (option_left() && option_right()) {
                            draw_line(_x, marker_ys, marker_xs, marker_ys);
                            draw_line(marker_xe, marker_ys, right, marker_ys);
                        } else {
                            draw_line(marker_xs, marker_ys, marker_xe, marker_ys);
                        }

                        if (!option_notext()) {
                            disp_val = i;
                            snprintf(buf, BUFSIZE, "%d",
                                    int(disp_val * _input.factor()/*+.5*/));  /// was data_scaling(), which makes no sense

                            if (option_left() && option_right())
                                draw_text(_center_x, marker_ys, buf, CENTER);
                            else if (option_left())
                                draw_text(marker_xs, marker_ys, buf, RIGHT|VCENTER);
                            else
                                draw_text(marker_xe, marker_ys, buf, LEFT|VCENTER);
                        }
                    }
                }
            }
        }

        // Now that the scale is drawn, we draw in the pointer(s).
        text_y = _y + ((cur_value - vmin) * factor() /*+.5f*/);

        if (option_right()) {
            glBegin(GL_LINE_STRIP);
            glVertex2f(_x, text_y + 5);
            glVertex2f(marker_xe, text_y);
            glVertex2f(_x, text_y - 5);
            glEnd();
        }
        if (option_left()) {
            glBegin(GL_LINE_STRIP);
            glVertex2f(right, text_y + 5);
            glVertex2f(marker_xs, text_y);
            glVertex2f(right, text_y - 5);
            glEnd();
        }
        // End if VERTICAL SCALE TYPE

    } else {                             // Horizontal scale by default
        // left tick bar
        draw_line(_x, _y, _x, top);

        // right tick bar
        draw_line(right, _y, right, top );

        marker_ys = _y;                       // Starting point for
        marker_ye = top;                              // tick y location calcs
        marker_xs = _x + (cur_value - vmin) * factor() /*+ .5f*/;

        if (option_top()) {
            // Bottom box line
            draw_line(_x, _y, right, _y);

            marker_ye = _y + _h / 2.0;   // Tick point adjust
            // Bottom arrow
            glBegin(GL_LINE_STRIP);
            glVertex2f(marker_xs - bottom_4, _y);
            glVertex2f(marker_xs, marker_ye);
            glVertex2f(marker_xs + bottom_4, _y);
            glEnd();
        }

        if (option_bottom()) {
            // Top box line
            draw_line(_x, top, right, top);
            // Tick point adjust
            marker_ys = top - _h / 2.0;

            // Top arrow
            glBegin(GL_LINE_STRIP);
            glVertex2f(marker_xs + bottom_4, top);
            glVertex2f(marker_xs, marker_ys );
            glVertex2f(marker_xs - bottom_4, top);
            glEnd();
        }


        int last = (int)vmax + 1;
        i = (int)vmin;
        for (; i <last ; i++) {
            condition = true;
            if (!_modulo && i < _input.min())
                    condition = false;

            if (condition) {
                marker_xs = _x + (i - vmin) * factor()/* +.5f*/;
                //        marker_xs = _x + (int)((i - vmin) * factor() + .5f);
                if (_minor_divs) {
                    if (!(i % (int)_minor_divs)) {
                        // draw in ticks only if they aren't too close to the edge.
                        if (((marker_xs + 5) > _x)
                               || ((marker_xs - 5) < right)) {

                            if (option_both()) {
                                draw_line(marker_xs, _y, marker_xs, marker_ys - 4);
                                draw_line(marker_xs, marker_ye + 4, marker_xs, top);

                            } else if (option_top()) {
                                draw_line(marker_xs, marker_ys, marker_xs, marker_ye - 4);
                            } else {
                                draw_line(marker_xs, marker_ys + 4, marker_xs, marker_ye);
                            }
                        }
                    }
                }

                if (_major_divs) {
                    if (!(i % (int)_major_divs)) {
                        if (_modulo) {
                            if (disp_val < 0) {
                                while (disp_val < 0)
                                    disp_val += _modulo;
                            }
                            disp_val = i % (int)_modulo;
                        } else {
                            disp_val = i;
                        }
                        snprintf(buf, BUFSIZE, "%d",
                                int(disp_val * _input.factor()/* +.5*/)); // was data_scaling(), which makes no sense

                        // Draw major ticks and text only if far enough from the edge.
                        if (((marker_xs - 10) > _x)
                                && ((marker_xs + 10) < right)) {
                            if (option_both()) {
                                draw_line(marker_xs, _y, marker_xs, marker_ys);
                                draw_line(marker_xs, marker_ye, marker_xs, top);

                                if (!option_notext())
                                    draw_text(marker_xs, marker_ys, buf, CENTER);

                            } else {
                                draw_line(marker_xs, marker_ys, marker_xs, marker_ye);

                                if (!option_notext()) {
                                    if (option_top())
                                        draw_text(marker_xs, top, buf, TOP|HCENTER);
                                    else
                                        draw_text(marker_xs, _y, buf, BOTTOM|HCENTER);
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}


