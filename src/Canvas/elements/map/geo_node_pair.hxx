/*
 * geo_node_pair.hxx
 *
 *  Created on: 11.07.2012
 *      Author: tom
 */

#ifndef CANVAS_GEO_NODE_PAIR_HXX_
#define CANVAS_GEO_NODE_PAIR_HXX_

namespace canvas
{
  class GeoNodePair
  {
    public:
      enum StatusFlags
      {
        LAT_MISSING = 1,
        LON_MISSING = LAT_MISSING << 1,
        INCOMPLETE = LAT_MISSING | LON_MISSING,
        DIRTY = LON_MISSING << 1
      };

      GeoNodePair():
        _status(INCOMPLETE),
        _node_lat(0),
        _node_lon(0)
      {}

      uint8_t getStatus() const
      {
        return _status;
      }

      void setDirty(bool flag = true)
      {
        if( flag )
          _status |= DIRTY;
        else
          _status &= ~DIRTY;
      }

      bool isDirty() const
      {
        return _status & DIRTY;
      }

      bool isComplete() const
      {
        return !(_status & INCOMPLETE);
      }

      void setNodeLat(SGPropertyNode* node)
      {
        _node_lat = node;
        _status &= ~LAT_MISSING;

        if( node == _node_lon )
        {
          _node_lon = 0;
          _status |= LON_MISSING;
        }
      }

      void setNodeLon(SGPropertyNode* node)
      {
        _node_lon = node;
        _status &= ~LON_MISSING;

        if( node == _node_lat )
        {
          _node_lat = 0;
          _status |= LAT_MISSING;
        }
      }

      const char* getLat() const
      {
        return _node_lat ? _node_lat->getStringValue() : "";
      }

      const char* getLon() const
      {
        return _node_lon ? _node_lon->getStringValue() : "";
      }

      void setTargetName(const std::string& name)
      {
        _target_name = name;
      }

      void setScreenPos(float x, float y)
      {
        assert( isComplete() );
        SGPropertyNode *parent = _node_lat->getParent();
        parent->getChild(_target_name, _node_lat->getIndex(), true)
              ->setDoubleValue(x);
        parent->getChild(_target_name, _node_lon->getIndex(), true)
              ->setDoubleValue(y);
      }

      void print()
      {
        std::cout << "lat=" << (_node_lat ? _node_lat->getPath() : "")
                  << ", lon=" << (_node_lon ? _node_lon->getPath() : "")
                  << std::endl;
      }

    private:

      uint8_t _status;
      SGPropertyNode *_node_lat,
                     *_node_lon;
      std::string   _target_name;

  };

} // namespace canvas

#endif /* CANVAS_GEO_NODE_PAIR_HXX_ */
