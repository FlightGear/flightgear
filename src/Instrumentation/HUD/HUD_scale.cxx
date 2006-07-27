// HUD_scale.cxx -- HUD Common Scale Base (inherited from Gauge/Tape/Dial)
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


HUD::Scale::Scale( HUD *hud, const SGPropertyNode *n, float x, float y) :
    Item(hud, n, x, y),
    _input(n->getNode("input", false)),
    _major_divs(n->getFloatValue("major-divisions")),
    _minor_divs(n->getFloatValue("minor-divisions")),
    _modulo(n->getIntValue("modulo"))
{
    if (n->hasValue("display-span"))
        _range_shown = n->getFloatValue("display-span");
    else
        _range_shown = _input.max() - _input.min();

    _display_factor = get_span() / _range_shown;
    if (_range_shown < 0)
        _range_shown = -_range_shown;

}


