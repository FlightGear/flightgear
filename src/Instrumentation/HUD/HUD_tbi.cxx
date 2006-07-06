// HUD_tbi.cxx -- HUD Turn-Bank-Indicator Instrument
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


HUD::TurnBankIndicator::TurnBankIndicator(HUD *hud, const SGPropertyNode *n, float x, float y) :
    Item(hud, n, x, y),
    _bank(n->getNode("bank-input", false)),
    _sideslip(n->getNode("sideslip-input", false)),
    _gap_width(n->getFloatValue("gap-width", 5)),
    _bank_scale(n->getBoolValue("bank-scale", false)),
    _bank_scale_radius(n->getFloatValue("bank-scale-radius", 250.0))
{
    if (!_bank_scale) {
        _bank.set_max(30.0, false);
        _bank.set_min(-30.0, false);
        _sideslip.set_max(20.0, false);
        _sideslip.set_min(-20.0, false);
    }
}


void HUD::TurnBankIndicator::draw(void)
{
    if (!_bank.isValid() || !_sideslip.isValid())
        return;

    float bank = _bank.getFloatValue();
    float sideslip = _sideslip.getFloatValue();

    float span = get_span();
    Point centroid = get_centroid();

    float cen_x = centroid.x;
    float cen_y = centroid.y;

    float tee_height = _h;
    float tee = -tee_height;
    float ss_const = 2 * sideslip * span / 40.0;  // sideslip angle pixels per deg (width represents 40 deg)

    glPushMatrix();
    glTranslatef(cen_x, cen_y, 0.0);
    glRotatef(-bank, 0.0, 0.0, 1.0);

    if (!_bank_scale) {
       glBegin(GL_LINES);

       if (!_gap_width) {
           glVertex2f(-span, 0.0);
           glVertex2f(span, 0.0);

       } else {
           glVertex2f(-span, 0.0);
           glVertex2f(-_gap_width, 0.0);
           glVertex2f(_gap_width, 0.0);
           glVertex2f(span, 0.0);
       }

       // draw teemarks
       glVertex2f(_gap_width, 0.0);
       glVertex2f(_gap_width, tee);
       glVertex2f(-_gap_width, 0.0);
       glVertex2f(-_gap_width, tee);
       glEnd();

       glBegin(GL_LINE_LOOP);
       glVertex2f(ss_const, -_gap_width);
       glVertex2f(ss_const + _gap_width, 0.0);
       glVertex2f(ss_const, _gap_width);
       glVertex2f(ss_const - _gap_width, 0.0);
       glEnd();


    } else { // draw MIL-STD 1878B/4.2.2.4 bank scale
        draw_line(cen_x - 1.0, _y, cen_x + 1.0, _y);
        draw_line(cen_x - 1.0, _y, cen_x - 1.0, _y + 10.0);
        draw_line(cen_x + 1.0, _y, cen_x + 1.0, _y + 10.0);
        draw_line(cen_x - 1.0, _y + 10.0, cen_x + 1.0, _y + 10.0);

        float x1, y1, x2, y2, x3, y3, x4, y4, x5, y5;
        float xc, yc;
        float r = _bank_scale_radius;
        float r1 = r - 10.0;
        float r2 = r - 5.0;

        xc = _x + _w / 2.0;                // FIXME redunant, no?
        yc = _y + r;

        // first n last lines
        x1 = xc + r * cos(255.0 * SGD_DEGREES_TO_RADIANS);
        y1 = yc + r * sin(255.0 * SGD_DEGREES_TO_RADIANS);

        x2 = xc + r1 * cos(255.0 * SGD_DEGREES_TO_RADIANS);
        y2 = yc + r1 * sin(255.0 * SGD_DEGREES_TO_RADIANS);
        draw_line(x1, y1, x2, y2);

        x1 = xc + r * cos(285.0 * SGD_DEGREES_TO_RADIANS);
        y1 = yc + r * sin(285.0 * SGD_DEGREES_TO_RADIANS);

        x2 = xc + r1 * cos(285.0 * SGD_DEGREES_TO_RADIANS);
        y2 = yc + r1 * sin(285.0 * SGD_DEGREES_TO_RADIANS);
        draw_line(x1, y1, x2, y2);

        // second n fifth  lines
        x1 = xc + r * cos(260.0 * SGD_DEGREES_TO_RADIANS);
        y1 = yc + r * sin(260.0 * SGD_DEGREES_TO_RADIANS);

        x2 = xc + r2 * cos(260.0 * SGD_DEGREES_TO_RADIANS);
        y2 = yc + r2 * sin(260.0 * SGD_DEGREES_TO_RADIANS);
        draw_line(x1, y1, x2, y2);

        x1 = xc + r * cos(280.0 * SGD_DEGREES_TO_RADIANS);
        y1 = yc + r * sin(280.0 * SGD_DEGREES_TO_RADIANS);

        x2 = xc + r2 * cos(280.0 * SGD_DEGREES_TO_RADIANS);
        y2 = yc + r2 * sin(280.0 * SGD_DEGREES_TO_RADIANS);
        draw_line(x1, y1, x2, y2);

        // third n fourth lines
        x1 = xc + r * cos(265.0 * SGD_DEGREES_TO_RADIANS);
        y1 = yc + r * sin(265.0 * SGD_DEGREES_TO_RADIANS);


        x2 = xc + r2 * cos(265.0 * SGD_DEGREES_TO_RADIANS);
        y2 = yc + r2 * sin(265.0 * SGD_DEGREES_TO_RADIANS);
        draw_line(x1, y1, x2, y2);

        x1 = xc + r * cos(275.0 * SGD_DEGREES_TO_RADIANS);
        y1 = yc + r * sin(275.0 * SGD_DEGREES_TO_RADIANS);

        x2 = xc + r2 * cos(275.0 * SGD_DEGREES_TO_RADIANS);
        y2 = yc + r2 * sin(275.0 * SGD_DEGREES_TO_RADIANS);
        draw_line(x1, y1, x2, y2);

        // draw marker
        r = _bank_scale_radius + 5.0;  // add gap

        // upper polygon
        float valbank = bank * 15.0 / _bank.max(); // total span of TSI is 30 degrees
        float valsideslip = sideslip * 15.0 / _sideslip.max();

        // values 270, 225 and 315 are angles in degrees...
        x1 = xc + r * cos((valbank + 270.0) * SGD_DEGREES_TO_RADIANS);
        y1 = yc + r * sin((valbank + 270.0) * SGD_DEGREES_TO_RADIANS);

        x2 = x1 + 6.0 * cos(225 * SGD_DEGREES_TO_RADIANS);
        y2 = y1 + 6.0 * sin(225 * SGD_DEGREES_TO_RADIANS);

        x3 = x1 + 6.0 * cos(315 * SGD_DEGREES_TO_RADIANS);
        y3 = y1 + 6.0 * sin(315 * SGD_DEGREES_TO_RADIANS);

        draw_line(x1, y1, x2, y2);
        draw_line(x2, y2, x3, y3);
        draw_line(x3, y3, x1, y1);

        // lower polygon				// FIXME this is inefficient and wrong!
        x1 = xc + r * cos((valbank + 270.0) * SGD_DEGREES_TO_RADIANS);
        y1 = yc + r * sin((valbank + 270.0) * SGD_DEGREES_TO_RADIANS);

        x2 = x1 + 6.0 * cos(225 * SGD_DEGREES_TO_RADIANS);
        y2 = y1 + 6.0 * sin(225 * SGD_DEGREES_TO_RADIANS);

        x3 = x1 + 6.0 * cos(315 * SGD_DEGREES_TO_RADIANS);
        y3 = y1 + 6.0 * sin(315 * SGD_DEGREES_TO_RADIANS);

        x4 = x1 + 10.0 * cos(225 * SGD_DEGREES_TO_RADIANS);
        y4 = y1 + 10.0 * sin(225 * SGD_DEGREES_TO_RADIANS);

        x5 = x1 + 10.0 * cos(315 * SGD_DEGREES_TO_RADIANS);
        y5 = y1 + 10.0 * sin(315 * SGD_DEGREES_TO_RADIANS);

        float cosss = cos(valsideslip * SGD_DEGREES_TO_RADIANS);
        float sinss = sin(valsideslip * SGD_DEGREES_TO_RADIANS);

        x2 = x2 + cosss;
        y2 = y2 + sinss;
        x3 = x3 + cosss;
        y3 = y3 + sinss;
        x4 = x4 + cosss;
        y4 = y4 + sinss;
        x5 = x5 + cosss;
        y5 = y5 + sinss;

        draw_line(x2, y2, x3, y3);
        draw_line(x3, y3, x5, y5);
        draw_line(x5, y5, x4, y4);
        draw_line(x4, y4, x2, y2);
    }
    glPopMatrix();
}


