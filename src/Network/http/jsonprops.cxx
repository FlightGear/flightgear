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

namespace flightgear {
namespace http {

using std::string;

static const char * getPropertyTypeString(simgear::props::Type type)
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

cJSON * JSON::toJson(SGPropertyNode_ptr n, int depth)
{
  cJSON * json = cJSON_CreateObject();
  cJSON_AddItemToObject(json, "path", cJSON_CreateString(n->getPath(true).c_str()));
  cJSON_AddItemToObject(json, "name", cJSON_CreateString(n->getName()));
  cJSON_AddItemToObject(json, "value", cJSON_CreateString(n->getStringValue()));
  cJSON_AddItemToObject(json, "type", cJSON_CreateString(getPropertyTypeString(n->getType())));
  cJSON_AddItemToObject(json, "index", cJSON_CreateNumber(n->getIndex()));

  if (depth > 0 && n->nChildren() > 0) {
    cJSON * jsonArray = cJSON_CreateArray();
    for (int i = 0; i < n->nChildren(); i++)
      cJSON_AddItemToArray(jsonArray, toJson(n->getChild(i), depth - 1));
    cJSON_AddItemToObject(json, "children", jsonArray);
  }
  return json;
}

void JSON::toProp(cJSON * json, SGPropertyNode_ptr base)
{
  if (NULL == json) return;

  cJSON * cj = cJSON_GetObjectItem(json, "name");
  if (NULL == cj) return; // a node with no name?
  char * name = cj->valuestring;
  if (NULL == name) return; // still no name?

  //TODO: check valid name

  int index = 0;
  cj = cJSON_GetObjectItem(json, "index");
  if (NULL != cj) index = cj->valueint;
  if (index < 0) return;

  SGPropertyNode_ptr n = base->getNode(name, index, true);
  cJSON * children = cJSON_GetObjectItem(json, "children");
  if (NULL != children) {
    for (int i = 0; i < cJSON_GetArraySize(children); i++) {
      toProp(cJSON_GetArrayItem(children, i), n);
    }
  } else {
    //TODO: set correct type
    /*
     char * type = "";
     cj = cJSON_GetObjectItem( json, "type" );
     if( NULL != cj ) type = cj->valuestring;
     */
    char * value = NULL;
    cj = cJSON_GetObjectItem(json, "value");
    if (NULL != cj) value = cj->valuestring;

    if (NULL != value) n->setUnspecifiedValue(value);
  }
}

string JSON::toJsonString(bool indent, SGPropertyNode_ptr n, int depth)
{
  cJSON * json = toJson( n, depth );
  char * jsonString = indent ? cJSON_Print( json ) : cJSON_PrintUnformatted( json );
  string reply(jsonString);
  free( jsonString );
  cJSON_Delete( json );
  return reply;
}

}  // namespace http
}  // namespace flightgear

