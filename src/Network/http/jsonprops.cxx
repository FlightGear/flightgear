// jsonprops.cxx -- convert properties from/to json
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

#include "jsonprops.hxx"
#include <simgear/misc/strutils.hxx>
#include <simgear/math/SGMath.hxx>
namespace flightgear {
namespace http {

using std::string;

const char * JSON::getPropertyTypeString(simgear::props::Type type)
{
  switch (type) {
    case simgear::props::NONE:
      return "-";

    case simgear::props::ALIAS:
      return "alias";

    case simgear::props::BOOL:
      return "bool";

    case simgear::props::INT:
      return "int";

    case simgear::props::LONG:
      return "long";

    case simgear::props::FLOAT:
      return "float";

    case simgear::props::DOUBLE:
      return "double";

    case simgear::props::STRING:
      return "string";

    case simgear::props::UNSPECIFIED:
      return "unspecified";

    case simgear::props::EXTENDED:
      return "extended";

    case simgear::props::VEC3D:
      return "vec3d";

    case simgear::props::VEC4D:
      return "vec4d";

    default:
      return "?";
  }
}

cJSON * JSON::valueToJson(SGPropertyNode_ptr n)
{
    if( !n->hasValue() )
        return cJSON_CreateNull();

    switch( n->getType() ) {
        case simgear::props::BOOL:
            return cJSON_CreateBool(n->getBoolValue());
        case simgear::props::INT:
        case simgear::props::LONG:
        case simgear::props::FLOAT:
        case simgear::props::DOUBLE: {
            double val = n->getDoubleValue();
            return SGMiscd::isNaN(val) ? cJSON_CreateNull() : cJSON_CreateNumber(val);
        }

        default:
            return cJSON_CreateString(n->getStringValue());
    }
}

    
cJSON * JSON::toJson(SGPropertyNode_ptr n, int depth, double timestamp )
{
  cJSON * json = cJSON_CreateObject();
  cJSON_AddItemToObject(json, "path", cJSON_CreateString(n->getPath(true).c_str()));
  cJSON_AddItemToObject(json, "name", cJSON_CreateString(n->getName()));
  if( n->hasValue() ) {
    switch( n->getType() ) {
      case simgear::props::BOOL:
        cJSON_AddItemToObject(json, "value", cJSON_CreateBool(n->getBoolValue()));
        break;
      case simgear::props::INT:
      case simgear::props::LONG:
      case simgear::props::FLOAT:
      case simgear::props::DOUBLE: {
        double val = n->getDoubleValue();
	cJSON_AddItemToObject(json, "value", SGMiscd::isNaN(val) ? cJSON_CreateNull() : cJSON_CreateNumber(val));
        break;
      }
      default:
        cJSON_AddItemToObject(json, "value", cJSON_CreateString(n->getStringValue()));
        break;
    }
  }
  cJSON_AddItemToObject(json, "type", cJSON_CreateString(getPropertyTypeString(n->getType())));
  cJSON_AddItemToObject(json, "index", cJSON_CreateNumber(n->getIndex()));
  if( timestamp >= 0.0 )
    cJSON_AddItemToObject(json, "ts", cJSON_CreateNumber(timestamp));
  cJSON_AddItemToObject(json, "nChildren", cJSON_CreateNumber(n->nChildren()));

  if (depth > 0 && n->nChildren() > 0) {
    cJSON * jsonArray = cJSON_CreateArray();
    for (int i = 0; i < n->nChildren(); i++)
      cJSON_AddItemToArray(jsonArray, toJson(n->getChild(i), depth - 1, timestamp ));
    cJSON_AddItemToObject(json, "children", jsonArray);
  }
  return json;
}

void JSON::toProp(cJSON * json, SGPropertyNode_ptr base)
{
  if (NULL == json) return;

  SGPropertyNode_ptr n = base;

  // check if name is set. If so, update child with given name
  // else update base
  cJSON * cj = cJSON_GetObjectItem(json, "name");
  if ( cj ) {
    const char * name = cj->valuestring;
    if (NULL == name) name = "";

    //TODO: better check for valid name
    string namestr = simgear::strutils::strip(string(name));
    if( false == namestr.empty() ) {
      int index = 0;
      cj = cJSON_GetObjectItem(json, "index");
      if (NULL != cj) index = cj->valueint;
      if (index < 0) return;

      n = base->getNode(namestr, index, true);
    }
  }

  cJSON * children = cJSON_GetObjectItem(json, "children");
  if (NULL != children) {
    for (int i = 0; i < cJSON_GetArraySize(children); i++) {
      toProp(cJSON_GetArrayItem(children, i), n);
    }
  } else {
    cj = cJSON_GetObjectItem(json, "value");
    if (NULL != cj) {
      switch ( cj->type ) {
      case cJSON_String:
        n->setStringValue(cj->valuestring);
        break;
        
      case cJSON_Number:
        n->setDoubleValue(cj->valuedouble);
        break;
        
      case cJSON_True:
        n->setBoolValue(true);
        break;
        
      case cJSON_False:
        n->setBoolValue(false);
        break;
          
      default:
        break;
      }
    } // of have value
  } // of no children
}

void JSON::addChildrenToProp(cJSON * json, SGPropertyNode_ptr n)
{
  if (NULL == json) return;
  if (!n) return;
  
  cJSON * children = cJSON_GetObjectItem(json, "children");
  if (NULL != children) {
    for (int i = 0; i < cJSON_GetArraySize(children); i++) {
      toProp(cJSON_GetArrayItem(children, i), n);
    }
  }
}

string JSON::toJsonString(bool indent, SGPropertyNode_ptr n, int depth, double timestamp )
{
  cJSON * json = toJson( n, depth, timestamp );
  char * jsonString = indent ? cJSON_Print( json ) : cJSON_PrintUnformatted( json );
  string reply(jsonString);
  free( jsonString );
  cJSON_Delete( json );
  return reply;
}

}  // namespace http
}  // namespace flightgear

