/**
 * SHPParser - parse ESRI ShapeFiles containing PolyLines */

// Written by James Turner, started 2013.
//
// Copyright (C) 2013 James Turner <zakalawe@mac.com>
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

#ifndef FG_SHP_PARSER_HXX
#define FG_SHP_PARSER_HXX

#include <Navaids/PolyLine.hxx>

// forward decls
class SGPath;

namespace flightgear
{

class SHPParser
{
public:
    /**
     * Parse a shape file containing PolyLine data.
     *
     * Throws sg_exceptions if parsing problems occur.
     */
    static void parsePolyLines(const SGPath&, PolyLine::Type aTy, PolyLineList& aResult, bool aClosed);
};

} // of namespace flightgear

#endif // of FG_SHP_PARSER_HXX
