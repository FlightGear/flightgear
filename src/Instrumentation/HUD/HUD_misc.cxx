// HUD_misc.cxx -- HUD miscellaneous elements
//
// Written by Melchior FRANZ, started September 2006.
//
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

#include <simgear/props/condition.hxx>
#include "HUD.hxx"


// MIL-STD-1787B aiming reticle

HUD::AimingReticle::AimingReticle(HUD *hud, const SGPropertyNode *n, float x, float y) :
    Item(hud, n, x, y),
    _active_condition(0),
    _diameter(n->getNode("diameter-input", false)),
    _bullet_size(_w / 3.0),
    _inner_radius(_w)
{
    const SGPropertyNode *node = n->getNode("active-condition");
    if (node)
       _active_condition = sgReadCondition(globals->get_props(), node);
}


void HUD::AimingReticle::draw(void)
{
    bool active = _active_condition ? _active_condition->test() : true;
    float diameter = _diameter.isValid() ? _diameter.getFloatValue() : 2.0f; // outer circle

    Point centroid = get_centroid();
    float x = centroid.x;
    float y = centroid.y;

    if (active) { // stadiametric (4.2.4.4)
        draw_bullet(x, y, _bullet_size);
        draw_circle(x, y, _inner_radius);
        draw_circle(x, y, diameter * _inner_radius);

    } else { // standby (4.2.4.5)
        // TODO
    }
}


