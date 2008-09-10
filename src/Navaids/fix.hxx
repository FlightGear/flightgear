// fix.hxx -- fix class
//
// Written by Curtis Olson, started April 2000.
//
// Copyright (C) 2000  Curtis L. Olson - http://www.flightgear.org/~curt
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
//
// $Id$


#ifndef _FG_FIX_HXX
#define _FG_FIX_HXX

#include <simgear/compiler.h>

#include <string>

#include "positioned.hxx"

class FGFix : public FGPositioned
{
public:
  FGFix(const std::string& aIdent, const SGGeod& aPos);
  inline ~FGFix(void) {}

  inline const std::string& get_ident() const { return ident(); }
  inline double get_lon() const { return longitude(); }
  inline double get_lat() const { return latitude(); }
};

#endif // _FG_FIX_HXX
