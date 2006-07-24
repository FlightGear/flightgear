// HUD_dial.cxx -- HUD Dial Instrument
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


HUD::Dial::Dial(HUD *hud, const SGPropertyNode *n, float x, float y) :
    Scale(hud, n, x, y),
    _radius(n->getFloatValue("radius")),
    _divisions(n->getIntValue("divisions"))
{
}


void HUD::Dial::draw(void)
{
    if (!_input.isValid())
        return;

    const int BUFSIZE = 80;
    char buf[BUFSIZE];

    glEnable(GL_POINT_SMOOTH);
    glPointSize(3.0);

    float incr = 360.0 / _divisions;
    for (float i = 0.0; i < 360.0; i += incr) {
        float i1 = i * SGD_DEGREES_TO_RADIANS;
        float x1 = _x + _radius * cos(i1);
        float y1 = _y + _radius * sin(i1);

        glBegin(GL_POINTS);
        glVertex2f(x1, y1);
        glEnd();
    }
    glPointSize(1.0);
    glDisable(GL_POINT_SMOOTH);


    float offset = 90.0 * SGD_DEGREES_TO_RADIANS;
    const float R = 10.0; //size of carrot
    float theta = _input.getFloatValue();

    float theta1 = -theta * SGD_DEGREES_TO_RADIANS + offset;
    float x1 = _x + _radius * cos(theta1);
    float y1 = _y + _radius * sin(theta1);
    float x2 = x1 - R * cos(theta1 - 30.0 * SGD_DEGREES_TO_RADIANS);
    float y2 = y1 - R * sin(theta1 - 30.0 * SGD_DEGREES_TO_RADIANS);
    float x3 = x1 - R * cos(theta1 + 30.0 * SGD_DEGREES_TO_RADIANS);
    float y3 = y1 - R * sin(theta1 + 30.0 * SGD_DEGREES_TO_RADIANS);

    // draw carrot
    draw_line(x1, y1, x2, y2);
    draw_line(x1, y1, x3, y3);
    snprintf(buf, BUFSIZE, "%3.1f\n", theta);

    // draw value
    int l = abs((int)theta);
    if (l) {
        if (l < 10)
            draw_text(_x, _y, buf, 0);
        else if (l < 100)
            draw_text(_x - 1.0, _y, buf, 0);
        else if (l < 360)
            draw_text(_x - 2.0, _y, buf, 0);
    }
}


