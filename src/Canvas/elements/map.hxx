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

#ifndef CANVAS_MAP_HXX_
#define CANVAS_MAP_HXX_

#include "group.hxx"

#include <boost/shared_ptr.hpp>
#include <boost/unordered_map.hpp>

namespace canvas
{
  class GeoNodePair;
  class Map:
    public Group
  {
    public:
      Map(SGPropertyNode_ptr node);
      virtual ~Map();

      virtual void update(double dt);

      virtual void childAdded( SGPropertyNode * parent,
                               SGPropertyNode * child );
      virtual void childRemoved( SGPropertyNode * parent,
                                 SGPropertyNode * child );
      virtual void valueChanged(SGPropertyNode * child);

    protected:

      virtual void childChanged(SGPropertyNode * child);

      typedef boost::unordered_map< SGPropertyNode*,
                                    boost::shared_ptr<GeoNodePair>
                                  > GeoNodes;
      GeoNodes _geo_nodes;
      bool _projection_dirty;

      struct GeoCoord
      {
        GeoCoord():
          type(INVALID)
        {}
        enum
        {
          INVALID,
          LATITUDE,
          LONGITUDE
        } type;
        double value;
      };

      GeoCoord parseGeoCoord(const std::string& val) const;
  };

} // namespace canvas

#endif /* CANVAS_MAP_HXX_ */
