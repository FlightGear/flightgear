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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include "HUD.hxx"

// MIL-STD-1787B aiming reticle

HUD::AimingReticle::AimingReticle(HUD *hud, const SGPropertyNode *n, float x, float y) :
Item(hud, n, x, y),
_active_condition(0),
_tachy_condition(0),
_align_condition(0),
_diameter(n->getNode("diameter-input", false)),
_pitch(n->getNode("pitch-input", false)),
_yaw(n->getNode("yaw-input", false)),
_speed(n->getNode("speed-input", false)),
_range(n->getNode("range-input", false)),
_t0(n->getNode("arc-start-input", false)),
_t1(n->getNode("arc-stop-input", false)),
_offset_x(n->getNode("offset-x-input", false)),
_offset_y(n->getNode("offset-y-input", false)),
_bullet_size(_w / 6.0),
_inner_radius(_w / 2.0),
_compression(n->getFloatValue("compression-factor")),
_limit_x(n->getFloatValue("limit-x")),
_limit_y(n->getFloatValue("limit-y"))

{
    const SGPropertyNode *node = n->getNode("active-condition");
    if (node)
        _active_condition = sgReadCondition(globals->get_props(), node);

    const SGPropertyNode *tnode = n->getNode("tachy-condition");
    if (tnode)
        _tachy_condition = sgReadCondition(globals->get_props(), tnode);

    const SGPropertyNode *anode = n->getNode("align-condition");
    if (anode)
        _align_condition = sgReadCondition(globals->get_props(), anode);

}


void HUD::AimingReticle::draw(void)
{
    bool active = _active_condition ? _active_condition->test() : true;
    bool tachy = _tachy_condition ? _tachy_condition->test() : false;
    bool align = _align_condition ? _align_condition->test() : false;

    float diameter = _diameter.isValid() ? _diameter.getFloatValue() : 2.0f; // outer circle
    float x = _center_x + (_offset_x.isValid() ? _offset_x.getFloatValue() : 0);
    float y = _center_y + (_offset_y.isValid() ? _offset_y.getFloatValue() : 0);

    if (active) { // stadiametric (4.2.4.4)
        draw_bullet(x, y, _bullet_size);
        draw_circle(x, y, _inner_radius);
        draw_circle(x, y, diameter * _inner_radius);
    } else if (tachy){//tachiametric
        float t0 = _t0.isValid() ? _t0.getFloatValue() : 2.0f; // start arc
        float t1 = _t1.isValid() ? _t1.getFloatValue() : 2.0f; // stop arc
        float yaw_value = _yaw.getFloatValue();
        float pitch_value = _pitch.getFloatValue();
        float tof_value = _range.getFloatValue()* 3 / _speed.getFloatValue();
        draw_bullet(x, y, _bullet_size);
        draw_circle(x, y, _inner_radius);
        draw_line(x + _inner_radius, y, x + _inner_radius * 3, y);
        draw_line(x - _inner_radius, y, x - _inner_radius * 3, y);
        draw_line(x, y + _inner_radius, x, y + _inner_radius * 3);
        draw_line(x, y - _inner_radius, x, y - _inner_radius * 3);

        if(align){
            draw_line(x + _limit_x, y + _limit_y, x - _limit_x, y + _limit_y);
            draw_line(x + _limit_x, y - _limit_y, x - _limit_x, y - _limit_y);
            draw_line(x + _limit_x, y + _limit_y, x + _limit_x, y - _limit_y);
            draw_line(x - _limit_x, y + _limit_y, x - _limit_x, y - _limit_y);
        }

        float limit_offset = diameter * _inner_radius;

        float pos_x = x + (yaw_value * tof_value)
            * _compression;

        pos_x > x + _limit_x - limit_offset ? 
            pos_x = x + _limit_x - limit_offset : pos_x;

        pos_x < x - _limit_x + limit_offset ? 
            pos_x = x - _limit_x + limit_offset: pos_x;

        float pos_y = y + (pitch_value * tof_value)
            * _compression;

        pos_y > y + _limit_y - limit_offset ? 
            pos_y = y + _limit_y - limit_offset : pos_y;

        pos_y < y - _limit_y + limit_offset? 
            pos_y = y - _limit_y + limit_offset: pos_y;

        draw_circle(pos_x, pos_y, diameter * _inner_radius);

        draw_arc(x, y, t0, t1, (diameter + 2) * _inner_radius );

    } else { // standby (4.2.4.5)
        // TODO
    }

}


