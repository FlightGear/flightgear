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
    _bank_scale(n->getBoolValue("bank-scale", false))
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

    if (_bank_scale)
        draw_scale();
    else
        draw_tee();
}


void HUD::TurnBankIndicator::draw_tee()
{
    // ___________  /\  ___________
    //            | \/ |

    float bank = _bank.getFloatValue();
    float sideslip = _sideslip.getFloatValue();

    float span = get_span();
    float tee = -_h;

    // sideslip angle pixels per deg (width represents 40 deg)
    float ss_const = 2 * sideslip * span / 40.0;

    glPushMatrix();
    glTranslatef(_center_x, _center_y, 0.0);
    glRotatef(-bank, 0.0, 0.0, 1.0);

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

    glPopMatrix();
}


void HUD::TurnBankIndicator::draw_scale()
{
    // MIL-STD 1878B/4.2.2.4 bank scale
    float bank = _bank.getFloatValue();
    float sideslip = _sideslip.getFloatValue();

    float cx = _center_x;
    float cy = _center_y;

    float r = _w / 2.0;
    float minor = r - r * 3.0 / 70.0;
    float major = r - r * 5.0 / 70.0;

    // hollow 0 degree mark
    float w = r / 70.0;
    if (w < 1.0)
        w = 1.0;
    float h = r * 6.0 / 70.0;
    draw_line(cx - w, _y, cx + w, _y);
    draw_line(cx - w, _y, cx - w, _y + h);
    draw_line(cx + w, _y, cx + w, _y + h);
    draw_line(cx - w, _y + h, cx + w, _y + h);

    // tick lines
    draw_tick(10, r, minor, 0);
    draw_tick(20, r, minor, 0);
    draw_tick(30, r, major, 0);

    int dir = bank > 0 ? 1 : -1;

    if (fabs(bank) > 25) {
        draw_tick(45, r, minor, dir);
        draw_tick(60, r, major, dir);
    }

    if (fabs(bank) > 55) {
        draw_tick(90, r, major, dir);
        draw_tick(135, r, major, dir);
    }

    // bank marker
    float a;
    float rr = r + r * 0.5 / 70.0;  // little gap for the arrow peak

    a = (bank + 270.0) * SGD_DEGREES_TO_RADIANS;
    float x1 = cx + rr * cos(a);
    float y1 = cy + rr * sin(a);

    rr = r * 3.0 / 70.0;
    a = (bank + 240.0) * SGD_DEGREES_TO_RADIANS;
    float x2 = x1 + rr * cos(a);
    float y2 = y1 + rr * sin(a);

    a = (bank + 300.0) * SGD_DEGREES_TO_RADIANS;
    float x3 = x1 + rr * cos(a);
    float y3 = y1 + rr * sin(a);

    draw_line(x1, y1, x2, y2);
    draw_line(x2, y2, x3, y3);
    draw_line(x3, y3, x1, y1);


    // sideslip marker
    rr = r + r * 0.5 / 70.0;
    a = (bank + sideslip + 270.0) * SGD_DEGREES_TO_RADIANS;
    x1 = cx + rr * cos(a);
    y1 = cy + rr * sin(a);

    rr = r * 3.0 / 70.0;
    a = (bank + sideslip + 240.0) * SGD_DEGREES_TO_RADIANS;
    x2 = x1 + rr * cos(a);
    y2 = y1 + rr * sin(a);

    a = (bank + sideslip + 300.0) * SGD_DEGREES_TO_RADIANS;
    x3 = x1 + rr * cos(a);
    y3 = y1 + rr * sin(a);

    rr = r * 6.0 / 70.0;
    a = (bank + sideslip + 240.0) * SGD_DEGREES_TO_RADIANS;
    float x4 = x1 + rr * cos(a);
    float y4 = y1 + rr * sin(a);

    a = (bank + sideslip + 300.0) * SGD_DEGREES_TO_RADIANS;
    float x5 = x1 + rr * cos(a);
    float y5 = y1 + rr * sin(a);

    draw_line(x2, y2, x3, y3);
    draw_line(x3, y3, x5, y5);
    draw_line(x5, y5, x4, y4);
    draw_line(x4, y4, x2, y2);
}


void HUD::TurnBankIndicator::draw_tick(float angle, float r1, float r2, int side)
{
    float a = (270 - angle) * SGD_DEGREES_TO_RADIANS;
    float c = cos(a);
    float s = sin(a);
    float x1 = r1 * c;
    float x2 = r2 * c;
    float y1 = _center_y + r1 * s;
    float y2 = _center_y + r2 * s;
    if (side >= 0)
        draw_line(_center_x - x1, y1, _center_x - x2, y2);
    if (side <= 0)
        draw_line(_center_x + x1, y1, _center_x + x2, y2);
}


void HUD::TurnBankIndicator::draw_line(float x1, float y1, float x2, float y2)
{
    if (option_top()) {
        float y = 2.0 * _center_y;  // mirror vertically
        Item::draw_line(x1, y - y1, x2, y - y2);
    } else
        Item::draw_line(x1, y1, x2, y2);
}


