// HUD_instrument.cxx -- HUD Common Instrument Base
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

#include <simgear/math/SGLimits.hxx>
#include "HUD.hxx"


HUD::Item::Item(HUD *hud, const SGPropertyNode *n, float x, float y) :
    _hud(hud),
    _name(n->getStringValue("name", "[unnamed]")),
    _options(0),
    _condition(0),
    _digits(n->getIntValue("digits"))
{
    const SGPropertyNode *node = n->getNode("condition");
    if (node)
        _condition = sgReadCondition(globals->get_props(), node);

    _x = n->getFloatValue("x") + x;
    _y = n->getFloatValue("y") + y;
    _w = n->getFloatValue("width");
    _h = n->getFloatValue("height");

    vector<SGPropertyNode_ptr> opt = n->getChildren("option");
    for (unsigned int i = 0; i < opt.size(); i++) {
        const char *o = opt[i]->getStringValue();
        if (!strcmp(o, "vertical"))
            _options |= VERTICAL;
        else if (!strcmp(o, "horizontal"))
            _options |= HORIZONTAL;
        else if (!strcmp(o, "top"))
            _options |= TOP;
        else if (!strcmp(o, "left"))
            _options |= LEFT;
        else if (!strcmp(o, "bottom"))
            _options |= BOTTOM;
        else if (!strcmp(o, "right"))
            _options |= RIGHT;
        else if (!strcmp(o, "both"))
            _options |= (LEFT|RIGHT);
        else if (!strcmp(o, "noticks"))
            _options |= NOTICKS;
        else if (!strcmp(o, "notext"))
            _options |= NOTEXT;
        else
            SG_LOG(SG_INPUT, SG_WARN, "HUD: unsupported option: " << o);
    }

    // Set up convenience values for centroid of the box and
    // the span values according to orientation

    if (_options & VERTICAL) {
        _scr_span = _h;
    } else {
        _scr_span = _w;
    }

    _center_x = _x + _w / 2.0;
    _center_y = _y + _h / 2.0;
}


bool HUD::Item::isEnabled()
{
    return _condition ? _condition->test() : true;
}


void HUD::Item::draw_line(float x1, float y1, float x2, float y2)
{
    _hud->_line_list.add(LineSegment(x1, y1, x2, y2));
}


void HUD::Item::draw_stipple_line(float x1, float y1, float x2, float y2)
{
    _hud->_stipple_line_list.add(LineSegment(x1, y1, x2, y2));
}


void HUD::Item::draw_text(float x, float y, const char *msg, int align, int digit)
{
    _hud->_text_list.add(x, y, msg, align, digit);
}


void HUD::Item::draw_circle(float xoffs, float yoffs, float r) const
{
    glBegin(GL_LINE_LOOP);
    float step = SG_PI / r;
    for (float alpha = 0; alpha < SG_PI * 2.0; alpha += step) {
        float x = r * cos(alpha);
        float y = r * sin(alpha);
        glVertex2f(x + xoffs, y + yoffs);
    }
    glEnd();
}

void HUD::Item::draw_arc(float xoffs, float yoffs, float t0, float t1, float r) const
{
    glBegin(GL_LINE_STRIP);
    float step = SG_PI / r;
    t0 = t0 * SG_DEGREES_TO_RADIANS;
    t1 = t1 * SG_DEGREES_TO_RADIANS;

    for (float alpha = t0; alpha < t1; alpha += step) {
        float x = r * cos(alpha);
        float y = r * sin(alpha);
        glVertex2f(x + xoffs, y + yoffs);
    }
    glEnd();
}

void HUD::Item::draw_bullet(float x, float y, float size)
{
    glEnable(GL_POINT_SMOOTH);
    glPointSize(size);

    glBegin(GL_POINTS);
    glVertex2f(x, y);
    glEnd();

    glPointSize(1.0);
    glDisable(GL_POINT_SMOOTH);
}


// make sure the format matches '[ -+#]?\d*(\.\d*)?(l?[df]|s)'
//
HUD::Item::Format HUD::Item::check_format(const char *f) const
{
    bool l = false;
    Format fmt = STRING;

    for (; *f; f++) {
        if (*f == '%') {
            if (f[1] == '%')
                f++;
            else
                break;
        }
    }
    if (*f++ != '%')
        return NONE;
    if (*f == ' ' || *f == '+' || *f == '-' || *f == '#')
        f++;
    while (*f && isdigit(*f))
        f++;
    if (*f == '.') {
        f++;
        while (*f && isdigit(*f))
            f++;
    }
    if (*f == 'l')
        l = true, f++;

    if (*f == 'd')
        fmt = l ? LONG : INT;
    else if (*f == 'f')
        fmt = l ? DOUBLE : FLOAT;
    else if (*f == 's') {
        if (l)
            return INVALID;
        fmt = STRING;
    } else
        return INVALID;

    for (++f; *f; f++) {
        if (*f == '%') {
            if (f[1] == '%')
                f++;
            else
                return INVALID;
        }
    }
    return fmt;
}


