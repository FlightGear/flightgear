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

static const float TICK_OFFSET = 2.f;


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
    _marker_offset(n->getFloatValue("marker-offset")),
    _label_offset(n->getFloatValue("label-offset", 3.0)),
    _label_gap(n->getFloatValue("label-gap-width") / 2.0),
    _pointer(n->getBoolValue("enable-pointer", true)),
    _format(n->getStringValue("format", "%d"))
{
    _half_width_units = range_to_show() / 2.0;

    const char *s;
    s = n->getStringValue("pointer-type");
    _pointer_type = strcmp(s, "moving") ? FIXED : MOVING;    // "fixed", "moving"

    s = n->getStringValue("tick-type");
    _tick_type = strcmp(s, "bullet") ? LINE : CIRCLE;        // "bullet", "line"

    s = n->getStringValue("tick-length");                    // "variable", "constant"
    _tick_length = strcmp(s, "constant") ? VARIABLE : CONSTANT;

    _label_fmt = check_format(_format.c_str());
    if (_label_fmt != INT && _label_fmt != LONG
            && _label_fmt != FLOAT && _label_fmt != DOUBLE) {
        SG_LOG(SG_INPUT, SG_ALERT, "HUD: invalid <format> '" << _format.c_str()
                << "' in <tape> '" << _name << "' (must be number format)");
        _label_fmt = INT;
        _format = "%d";
    }

    if (_minor_divs != 0.0f)
        _div_ratio = int(_major_divs / _minor_divs + 0.5f);
    else
        _div_ratio = 0, _minor_divs = _major_divs;

//    int k; //odd or even values for ticks		// FIXME odd scale
    _odd_type = false;
    if (_input.max() + .5f < float(SGLimits<long>::max()))
        _odd_type = long(floorf(_input.max() + 0.5f)) & 1 ? true : false;
}


void HUD::Tape::draw(void) //  (HUD_scale * pscale)
{
    if (!_input.isValid())
        return;

    float value = _input.getFloatValue();

    if (option_vert())
        draw_vertical(value);
    else
        draw_horizontal(value);
}


void HUD::Tape::draw_vertical(float value)
{
    float vmin = 0.0;//, vmax = 0.0;
    float marker_xs;
    float marker_xe;
    float marker_ye;
    float text_y = 0.0;

    float top = _y + _h;
    float right = _x + _w;


    if (!_pointer) {
        vmin = value - _half_width_units; // width units == needle travel
//        vmax = value + _half_width_units; // or picture unit span.
        text_y = _center_y;

    } else if (_pointer_type == MOVING) {
        vmin = _input.min();
//        vmax = _input.max();

    } else { // FIXED
        vmin = value - _half_width_units; // width units == needle travel
//        vmax = value + _half_width_units; // or picture unit span.
        text_y = _center_y;
    }

    // Bottom tick bar
    if (_draw_tick_bottom)
        draw_line(_x, _y, right, _y);

    // Top tick bar
    if (_draw_tick_top)
        draw_line(_x, top, right, top);

    marker_xs = _x;       // x start
    marker_xe = right;    // x extent
    marker_ye = top;

    // We do not use else in the following so that combining the
    // two options produces a "caged" display with double
    // carrots. The same is done for horizontal card indicators.

    // draw capping lines and pointers
    if (option_left()) {    // Calculate x marker offset

        if (_draw_cap_right)
            draw_line(marker_xe, _y, marker_xe, marker_ye);

        marker_xs = marker_xe - _w / 3.0;

        // draw_line(marker_xs, _center_y, marker_xe, _center_y + _w / 6);
        // draw_line(marker_xs, _center_y, marker_xe, _center_y - _w / 6);

        if (_pointer) {
            if (_pointer_type == MOVING) {
                float ycentre, ypoint, xpoint;
                float range, right;

                if (_input.min() >= 0.0)
                    ycentre = _y;
                else if (_input.max() + _input.min() == 0.0)
                    ycentre = _center_y;
                else if (_odd_type)
                    ycentre = _y + (1.0 - _input.min()) * _h / (_input.max() - _input.min());
                else
                    ycentre = _y + _input.min() * _h / (_input.max() - _input.min());

                range = _h;
                right = _x + _w;

                if (_odd_type)
                    ypoint = ycentre + ((value - 1.0) * range / _val_span);
                else
                    ypoint = ycentre + (value * range / _val_span);

                xpoint = right + _marker_offset;
                draw_line(xpoint, ycentre, xpoint, ypoint);
                draw_line(xpoint, ypoint, xpoint - _marker_offset, ypoint);
                draw_line(xpoint - _marker_offset, ypoint, xpoint - 5.0, ypoint + 5.0);
                draw_line(xpoint - _marker_offset, ypoint, xpoint - 5.0, ypoint - 5.0);

            } else { // FIXED
                draw_fixed_pointer(_marker_offset + marker_xe, text_y + _w / 6,
                        _marker_offset + marker_xs, text_y, _marker_offset + marker_xe,
                        text_y - _w / 6);
            }
        }
    } // if (option_left())


    // draw capping lines and pointers
    if (option_right()) {

        if (_draw_cap_left)
            draw_line(_x, _y, _x, marker_ye);

        marker_xe = _x + _w / 3.0;
        // Indicator carrot
        // draw_line(_x, _center_y +  _w / 6, marker_xe, _center_y);
        // draw_line(_x, _center_y -  _w / 6, marker_xe, _center_y);

        if (_pointer) {
            if (_pointer_type == MOVING) {
                float ycentre, ypoint, xpoint;
                float range;

                if (_input.min() >= 0.0)
                    ycentre = _y;
                else if (_input.max() + _input.min() == 0.0)
                    ycentre = _center_y;
                else if (_odd_type)
                    ycentre = _y + (1.0 - _input.min()) * _h / (_input.max() - _input.min());
                else
                    ycentre = _y + _input.min() * _h / (_input.max() - _input.min());

                range = _h;

                if (_odd_type)
                    ypoint = ycentre + ((value - 1.0) * range / _val_span);
                else
                    ypoint = ycentre + (value * range / _val_span);

                xpoint = _x - _marker_offset;
                draw_line(xpoint, ycentre, xpoint, ypoint);
                draw_line(xpoint, ypoint, xpoint + _marker_offset, ypoint);
                draw_line(xpoint + _marker_offset, ypoint, xpoint + 5.0, ypoint + 5.0);
                draw_line(xpoint + _marker_offset, ypoint, xpoint + 5.0, ypoint - 5.0);

            } else { // FIXED
                draw_fixed_pointer(-_marker_offset + _x, text_y +  _w / 6,
                        -_marker_offset + marker_xe, text_y, -_marker_offset + _x,
                        text_y - _w / 6);
            }
        } // if (_pointer)
    } // if (option_right())


    // At this point marker x_start and x_end values are transposed.
    // To keep this from confusing things they are now swapped.
    if (option_both())
        marker_ye = marker_xs, marker_xs = marker_xe, marker_xe = marker_ye;



    // Work through from bottom to top of scale. Calculating where to put
    // minor and major ticks.

    // draw scale or tape
    float vstart = floorf(vmin / _major_divs) * _major_divs;
    float min_diff = _w / 6.0;    // length difference between major & minor tick

    // FIXME consider oddtype
    for (int i = 0; ; i++) {
        float v = vstart + i * _minor_divs;

        if (!_modulo) {
            if (v < _input.min())
                continue;
            else if (v > _input.max())
                break;
        }

        float y = _y + (v - vmin) * factor();

        if (y < _y + TICK_OFFSET)
            continue;
        if (y > top - TICK_OFFSET)
            break;

        if (_div_ratio && i % _div_ratio) { // minor div
            if (option_both()) {
                if (_tick_type == LINE) {
                    if (_tick_length == VARIABLE) {
                        draw_line(_x, y, marker_xs, y);
                        draw_line(marker_xe, y, right, y);
                    } else {
                        draw_line(_x, y, marker_xs, y);
                        draw_line(marker_xe, y, right, y);
                    }

                } else { // _tick_type == CIRCLE
                    draw_bullet(_x, y, 3.0);
                }

            } else if (option_left()) {
                if (_tick_type == LINE) {
                    if (_tick_length == VARIABLE) {
                        draw_line(marker_xs + min_diff, y, marker_xe, y);
                    } else {
                        draw_line(marker_xs, y, marker_xe, y);
                    }
                } else { // _tick_type == CIRCLE
                    draw_bullet(marker_xs + 4, y, 3.0);
                }

            } else { // if (option_right())
                if (_tick_type == LINE) {
                    if (_tick_length == VARIABLE) {
                        draw_line(marker_xs, y, marker_xe - min_diff, y);
                    } else {
                        draw_line(marker_xs, y, marker_xe, y);
                    }

                } else { // _tick_type == CIRCLE
                    draw_bullet(marker_xe - 4, y, 3.0);
                }
            } // end huds both

        } else { // major div
            if (_modulo)
                v = fmodf(v + _modulo, _modulo);

            float x;
            int align;

            if (option_both()) {
                if (_tick_type == LINE) {
                    draw_line(_x, y, marker_xs, y);
                    draw_line(marker_xs, y, right, y);

                } else { // _tick_type == CIRCLE
                    draw_bullet(_x, y, 5.0);
                }

                x = marker_xs, align = CENTER;

            } else {
                if (_tick_type == LINE)
                    draw_line(marker_xs, y, marker_xe, y);
                else // _tick_type == CIRCLE
                    draw_bullet(marker_xs + 4, y, 5.0);

                if (option_left())
                    x = marker_xs - _label_offset, align = RIGHT|VCENTER;
                else
                    x = marker_xe + _label_offset, align = LEFT|VCENTER;
            }

            if (!option_notext()) {
                char *s = format_value(v);

                float l, r, b, t;
                _hud->_text_list.align(s, align, &x, &y, &l, &r, &b, &t);

                if (b < _y || t > top)
                    continue;

                if (_label_gap == 0.0
                        || (b < _center_y - _label_gap && t < _center_y - _label_gap)
                        || (b > _center_y + _label_gap && t > _center_y + _label_gap)) {
                    draw_text(x, y, s);
                }
            }
        }
    } // for
}


void HUD::Tape::draw_horizontal(float value)
{
    float vmin = 0.0;//, vmax = 0.0;
    float marker_xs;
//    float marker_xe;
    float marker_ys;
    float marker_ye;
//    float text_y = 0.0;

    float top = _y + _h;
    float right = _x + _w;


    if (!_pointer) {
        vmin = value - _half_width_units; // width units == needle travel
//        vmax = value + _half_width_units; // or picture unit span.
//        text_y = _center_y;

    } else if (_pointer_type == MOVING) {
        vmin = _input.min();
//        vmax = _input.max();

    } else { // FIXED
        vmin = value - _half_width_units; // width units == needle travel
//        vmax = value + _half_width_units; // or picture unit span.
//        text_y = _center_y;
    }

    // left tick bar
    if (_draw_tick_left)
        draw_line(_x, _y, _x, top);

    // right tick bar
    if (_draw_tick_right)
        draw_line(right, _y, right, top);

    marker_ys = _y;    // Starting point for
    marker_ye = top;           // tick y location calcs
//    marker_xe = right;
    marker_xs = _x + ((value - vmin) * factor());

    if (option_top()) {
        if (_draw_cap_bottom)
            draw_line(_x, _y, right, _y);

        // Tick point adjust
        marker_ye  = _y + _h / 2;
        // Bottom arrow
        // draw_line(_center_x, marker_ye, _center_x - _h / 4, _y);
        // draw_line(_center_x, marker_ye, _center_x + _h / 4, _y);
        // draw pointer
        if (_pointer) {
            if (_pointer_type == MOVING) {
                float xcentre = _center_x;
                float range = _w;
                float xpoint = xcentre + (value * range / _val_span);
                float ypoint = _y - _marker_offset;
                draw_line(xcentre, ypoint, xpoint, ypoint);
                draw_line(xpoint, ypoint, xpoint, ypoint + _marker_offset);
                draw_line(xpoint, ypoint + _marker_offset, xpoint + 5.0, ypoint + 5.0);
                draw_line(xpoint, ypoint + _marker_offset, xpoint - 5.0, ypoint + 5.0);

            } else { // FIXED
                draw_fixed_pointer(marker_xs - _h / 4, _y, marker_xs,
                        marker_ye, marker_xs + _h / 4, _y);
            }
        }
    } // if (option_top())

    if (option_bottom()) {
        if (_draw_cap_top)
            draw_line(_x, top, right, top);

        // Tick point adjust
        marker_ys = top - _h / 2;
        // Top arrow
        // draw_line(_center_x + _h / 4, _y + _h, _center_x, marker_ys);
        // draw_line(_center_x - _h / 4, _y + _h, _center_x , marker_ys);

        if (_pointer) {
            if (_pointer_type == MOVING) {
                float xcentre = _center_x;
                float range = _w;
                float hgt = _y + _h;
                float xpoint = xcentre + (value * range / _val_span);
                float ypoint = hgt + _marker_offset;
                draw_line(xcentre, ypoint, xpoint, ypoint);
                draw_line(xpoint, ypoint, xpoint, ypoint - _marker_offset);
                draw_line(xpoint, ypoint - _marker_offset, xpoint + 5.0, ypoint - 5.0);
                draw_line(xpoint, ypoint - _marker_offset, xpoint - 5.0, ypoint - 5.0);

            } else { // FIXED
                draw_fixed_pointer(marker_xs + _h / 4, top, marker_xs, marker_ys,
                        marker_xs - _h / 4, top);
            }
        }
    } // if (option_bottom())


    float vstart = floorf(vmin / _major_divs) * _major_divs;
    float min_diff = _h / 6.0;    // length difference between major & minor tick

    // FIXME consider oddtype
    for (int i = 0; ; i++) {
        float v = vstart + i * _minor_divs;

        if (!_modulo) {
            if (v < _input.min())
                continue;
            else if (v > _input.max())
                break;
        }

        float x = _x + (v - vmin) * factor();

        if (x < _x + TICK_OFFSET)
            continue;
        if (x > right - TICK_OFFSET)
            break;

        if (_div_ratio && i % _div_ratio) { // minor div
            if (option_both()) {
                if (_tick_length == VARIABLE) {
                    draw_line(x, _y, x, marker_ys - 4);
                    draw_line(x, marker_ye + 4, x, top);
                } else {
                    draw_line(x, _y, x, marker_ys);
                    draw_line(x, marker_ye, x, top);
                }

            } else {
                if (option_top()) {
                    // draw minor ticks
                    if (_tick_length == VARIABLE)
                        draw_line(x, marker_ys, x, marker_ye - min_diff);
                    else
                        draw_line(x, marker_ys, x, marker_ye);

                } else if (_tick_length == VARIABLE) {
                    draw_line(x, marker_ys + 4, x, marker_ye);
                } else {
                    draw_line(x, marker_ys, x, marker_ye);
                }
            }

        } else { // major divs
            if (_modulo)
                v = fmodf(v + _modulo, _modulo);

            float y;
            int align;

            if (option_both()) {
                draw_line(x, _y, x, marker_ye);
                draw_line(x, marker_ye, x, _y + _h);
                y = marker_ys, align = CENTER;

            } else {
                draw_line(x, marker_ys, x, marker_ye);

                if (option_top())
                    y = top - _label_offset, align = TOP|HCENTER;
                else
                    y = _y + _label_offset, align = BOTTOM|HCENTER;
            }

            if (!option_notext()) {
                char *s = format_value(v);

                float l, r, b, t;
                _hud->_text_list.align(s, align, &x, &y, &l, &r, &b, &t);

                if (l < _x || r > right)
                    continue;

                if (_label_gap == 0.0
                        || (l < _center_x - _label_gap && r < _center_x - _label_gap)
                        || (l > _center_x + _label_gap && r > _center_x + _label_gap)) {
                    draw_text(x, y, s);
                }
            }
        }
    } // for
}


char *HUD::Tape::format_value(float v)
{
    if (fabs(v) < 1e-8)   // avoid -0.0
        v = 0.0f;

    if (_label_fmt == INT)
        snprintf(_buf, BUFSIZE, _format.c_str(), int(v));
    else if (_label_fmt == LONG)
        snprintf(_buf, BUFSIZE, _format.c_str(), long(v));
    else if (_label_fmt == FLOAT)
        snprintf(_buf, BUFSIZE, _format.c_str(), v);
    else // _label_fmt == DOUBLE
        snprintf(_buf, BUFSIZE, _format.c_str(), double(v));
    return _buf;
}


void HUD::Tape::draw_fixed_pointer(float x1, float y1, float x2, float y2, float x3, float y3)
{
    glBegin(GL_LINE_STRIP);
    glVertex2f(x1, y1);
    glVertex2f(x2, y2);
    glVertex2f(x3, y3);
    glEnd();
}


