// jsonprops.hxx -- convert properties from/to json
//
// Written by Torsten Dreyer, started April 2014.
//
// Copyright (C) 2014  Torsten Dreyer
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

#ifndef JSONPROPS_HXX_
#define JSONPROPS_HXX_

#include <simgear/props/props.hxx>
#include <cJSON.h>
#include <string>

namespace flightgear {
namespace http {

class JSON {
public:
  static cJSON * toJson(SGPropertyNode_ptr n, int depth, double timestamp = -1.0 );
  static std::string toJsonString(bool indent, SGPropertyNode_ptr n, int depth, double timestamp = -1.0 );

  static const char * getPropertyTypeString(simgear::props::Type type);
  static cJSON * valueToJson(SGPropertyNode_ptr n);

  static void toProp(cJSON * json, SGPropertyNode_ptr base);
  static void addChildrenToProp(cJSON * json, SGPropertyNode_ptr base);
};

}  // namespace http
}  // namespace flightgear

#endif /* JSONPROPS_HXX_ */
