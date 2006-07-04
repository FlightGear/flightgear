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

#include <simgear/math/SGLimits.hxx>
#include <simgear/props/condition.hxx>
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

    _scrn_pos.left = n->getIntValue("x") + x;
    _scrn_pos.top = n->getIntValue("y") + y;
    _scrn_pos.right = n->getIntValue("width");
    _scrn_pos.bottom = n->getIntValue("height");

    vector<SGPropertyNode_ptr> opt = n->getChildren("option");
    for (unsigned int i = 0; i < opt.size(); i++) {
        const char *o = opt[i]->getStringValue();
        if (!strcmp(o, "autoticks"))
            _options |= AUTOTICKS;
        else if (!strcmp(o, "vertical"))
            _options |= VERT;
        else if (!strcmp(o, "horizontal"))
            _options |= HORZ;
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
        else if (!strcmp(o, "arithtic"))
            _options |= ARITHTIC;
        else if (!strcmp(o, "decitics"))
            _options |= DECITICS;
        else if (!strcmp(o, "notext"))
            _options |= NOTEXT;
        else
            SG_LOG(SG_INPUT, SG_WARN, "HUD: unsupported option: " << o);
    }

    // Set up convenience values for centroid of the box and
    // the span values according to orientation

    if (_options & VERT) {
        _scr_span = _scrn_pos.bottom;
    } else {
        _scr_span = _scrn_pos.right;
    }

    _mid_span.x = _scrn_pos.left + _scrn_pos.right / 2.0;
    _mid_span.y = _scrn_pos.top + _scrn_pos.bottom / 2.0;
}


bool HUD::Item::isEnabled()
{
    if (!_condition)
        return true;

    return _condition->test();
}


