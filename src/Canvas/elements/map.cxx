// A group of 2D canvas elements which get automatically transformed according
// to the map parameters.
//
// Copyright (C) 2012  Thomas Geymayer <tomgey@gmail.com>
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

#include "map.hxx"
#include "map/geo_node_pair.hxx"
#include "map/projection.hxx"

#include <Main/fg_props.hxx>
#include <cmath>

#include <boost/algorithm/string/predicate.hpp>

#define LOG_GEO_RET(msg) \
  {\
    SG_LOG\
    (\
      SG_GENERAL,\
      SG_WARN,\
      msg << " (" << child->getStringValue()\
                  << ", " << child->getPath() << ")"\
    );\
    return;\
  }

namespace canvas
{

  // TODO make projection configurable
  SansonFlamsteedProjection projection;
  const std::string GEO = "-geo";

  //----------------------------------------------------------------------------
  Map::Map(SGPropertyNode_ptr node):
    Group(node),
    _projection_dirty(true)
  {

  }

  //----------------------------------------------------------------------------
  Map::~Map()
  {

  }

  //----------------------------------------------------------------------------
  void Map::update(double dt)
  {
    for( GeoNodes::iterator it = _geo_nodes.begin();
         it != _geo_nodes.end();
         ++it )
    {
      GeoNodePair* geo_node = it->second.get();
      if(    !geo_node->isComplete()
          || (!geo_node->isDirty() && !_projection_dirty) )
        continue;

      GeoCoord lat = parseGeoCoord(geo_node->getLat());
      if( lat.type != GeoCoord::LATITUDE )
        continue;

      GeoCoord lon = parseGeoCoord(geo_node->getLon());
      if( lon.type != GeoCoord::LONGITUDE )
        continue;

      Projection::ScreenPosition pos =
        projection.worldToScreen(lat.value, lon.value);

      geo_node->setScreenPos(pos.x, pos.y);

//      geo_node->print();
      geo_node->setDirty(false);
    }
    _projection_dirty = false;

    Group::update(dt);
  }

  //----------------------------------------------------------------------------
  void Map::childAdded(SGPropertyNode* parent, SGPropertyNode* child)
  {
    if( !boost::ends_with(child->getNameString(), GEO) )
      return Element::childAdded(parent, child);

    _geo_nodes[child].reset(new GeoNodePair());
  }

  //----------------------------------------------------------------------------
  void Map::childRemoved(SGPropertyNode* parent, SGPropertyNode* child)
  {
    if( !boost::ends_with(child->getNameString(), GEO) )
      return Element::childRemoved(parent, child);

    // TODO remove from other node
    _geo_nodes.erase(child);
  }

  //----------------------------------------------------------------------------
  void Map::valueChanged(SGPropertyNode * child)
  {
    const std::string& name = child->getNameString();

    if( !boost::ends_with(name, GEO) )
      return Group::valueChanged(child);

    GeoNodes::iterator it_geo_node = _geo_nodes.find(child);
    if( it_geo_node == _geo_nodes.end() )
      LOG_GEO_RET("geo node not found!")
    GeoNodePair* geo_node = it_geo_node->second.get();

    geo_node->setDirty();

    if( geo_node->getStatus() & GeoNodePair::INCOMPLETE )
    {
      // Detect lat, lon tuples...
      GeoCoord coord = parseGeoCoord(child->getStringValue());
      int index_other = -1;

      switch( coord.type )
      {
        case GeoCoord::LATITUDE:
          index_other = child->getIndex() + 1;
          geo_node->setNodeLat(child);
          break;
        case GeoCoord::LONGITUDE:
          index_other = child->getIndex() - 1;
          geo_node->setNodeLon(child);
          break;
        default:
          LOG_GEO_RET("Invalid geo coord")
      }

      SGPropertyNode *other = child->getParent()->getChild(name, index_other);
      if( !other )
        return;

      GeoCoord coord_other = parseGeoCoord(other->getStringValue());
      if(    coord_other.type == GeoCoord::INVALID
          || coord_other.type == coord.type )
        return;

      GeoNodes::iterator it_geo_node_other = _geo_nodes.find(other);
      if( it_geo_node_other == _geo_nodes.end() )
        LOG_GEO_RET("other geo node not found!")
      GeoNodePair* geo_node_other = it_geo_node_other->second.get();

      // Let use both nodes use the same GeoNodePair instance
      if( geo_node_other != geo_node )
        it_geo_node_other->second = it_geo_node->second;

      if( coord_other.type == GeoCoord::LATITUDE )
        geo_node->setNodeLat(other);
      else
        geo_node->setNodeLon(other);

      // Set name for resulting screen coordinate nodes
      geo_node->setTargetName( name.substr(0, name.length() - GEO.length()) );
    }
  }

  //----------------------------------------------------------------------------
  void Map::childChanged(SGPropertyNode * child)
  {
    if(    child->getNameString() == "ref-lat"
        || child->getNameString() == "ref-lon" )
      projection.setWorldPosition( _node->getDoubleValue("ref-lat"),
                                   _node->getDoubleValue("ref-lon") );
    else if( child->getNameString() == "hdg" )
      projection.setOrientation(child->getFloatValue());
    else if( child->getNameString() == "range" )
      projection.setRange(child->getDoubleValue());
    else
      return;

    _projection_dirty = true;
  }

  //----------------------------------------------------------------------------
  Map::GeoCoord Map::parseGeoCoord(const std::string& val) const
  {
    GeoCoord coord;
    if( val.length() < 2 )
      return coord;

    if( val[0] == 'N' || val[0] == 'S' )
      coord.type = GeoCoord::LATITUDE;
    else if( val[0] == 'E' || val[0] == 'W' )
      coord.type = GeoCoord::LONGITUDE;
    else
      return coord;

    char* end;
    coord.value = strtod(&val[1], &end);

    if( end != &val[val.length()] )
    {
      coord.type = GeoCoord::INVALID;
      return coord;
    }

    if( val[0] == 'S' || val[0] == 'W' )
      coord.value *= -1;

    return coord;
  }

} // namespace canvas
