// tileentry.hxx -- routines to handle an individual scenery tile
//
// Written by Curtis Olson, started May 1998.
//
// Copyright (C) 1998 - 2001  Curtis L. Olson  - http://www.flightgear.org/~curt
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


#ifndef _TILEENTRY_HXX
#define _TILEENTRY_HXX


#ifndef __cplusplus
# error This library requires C++
#endif

#include <Main/globals.hxx>
#include <Main/fg_props.hxx>

#include <simgear/compiler.h>

#include <vector>
#include <string>

#include <simgear/bucket/newbucket.hxx>
#include <simgear/misc/sg_path.hxx>
#include <simgear/scene/util/OrthophotoManager.hxx>

#include <osg/ref_ptr>
#include <osgDB/ReaderWriter>
#include <osg/Group>
#include <osg/LOD>

/**
 * A class to encapsulate everything we need to know about a scenery tile.
 */
class TileEntry {

public:
    // this tile's official location in the world
    SGBucket tile_bucket;
    std::string tileFileName;

private:

    // pointer to ssg range selector for this tile
    osg::ref_ptr<osg::LOD> _node;
    // Reference to DatabaseRequest object set and used by the
    // osgDB::DatabasePager.
    osg::ref_ptr<osg::Referenced> _databaseRequest;
    // Overlay image/orthophoto for this tile
    simgear::OrthophotoRef _orthophoto;

    /**
     * This value is used by the tile scheduler/loader to load tiles
     * in a useful sequence. The priority is set to reflect the tiles
     * distance from the center, so all tiles are loaded in an innermost
     * to outermost sequence.
     */
    float _priority;
    /** Flag indicating if tile belongs to current view. */
    bool _current_view;
    /** Time when tile expires. */
    double _time_expired;

    void _create_orthophoto();
    void _free_orthophoto();

public:

    // Constructor
    TileEntry( const SGBucket& b );
    TileEntry( const TileEntry& t );

    // Destructor
    ~TileEntry();

    // Update the ssg transform node for this tile so it can be
    // properly drawn relative to our (0,0,0) point
    void prep_ssg_node(float vis);

    /**
     * Transition to OSG database pager
     */
    static osg::Node* loadTileByFileName(const std::string& index_str,
                                         const osgDB::Options*);
    /**
     * Return true if the tile entry is loaded, otherwise return false
     * indicating that the loading thread is still working on this.
     */
    inline bool is_loaded() const
    {
        return _node->getNumChildren() > 0;
    }

    /**
     * Return the "bucket" for this tile
     */
    inline const SGBucket& get_tile_bucket() const { return tile_bucket; }

    /**
     * Add terrain mesh and ground lighting to scene graph.
     */
    void addToSceneGraph( osg::Group *terrain_branch);

    /**
     * disconnect terrain mesh and ground lighting nodes from scene
     * graph for this tile.
     */
    void removeFromSceneGraph();

    /**
     * return the scenegraph node for the terrain
     */
    osg::LOD *getNode() const { return _node.get(); }

    inline double get_time_expired() const { return _time_expired; }
    inline void update_time_expired( double time_expired ) { if (_time_expired<time_expired) _time_expired = time_expired; }

    inline void set_priority(float priority) { _priority=priority; }
    inline float get_priority() const { return _priority; }
    inline void set_current_view(bool current_view) { _current_view = current_view; }
    inline bool is_current_view() const { return _current_view; }

    /**
     * Return false if the tile entry is still needed, otherwise return true
     * indicating that the tile is no longer in active use.
     */
    inline bool is_expired(double current_time) const { return (_current_view) ? false : (current_time > _time_expired); }

    // Get the ref_ptr to the DatabaseRequest object, in order to pass
    // this to the pager.
    osg::ref_ptr<osg::Referenced>& getDatabaseRequest()
    {
        return _databaseRequest;
    }
};

#endif // _TILEENTRY_HXX
