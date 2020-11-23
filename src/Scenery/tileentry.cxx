// tileentry.cxx -- routines to handle a scenery tile
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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <simgear/compiler.h>

#include <string>
#include <sstream>
#include <istream>

#include <osg/LOD>

#include <simgear/bucket/newbucket.hxx>
#include <simgear/debug/logstream.hxx>

#include "tileentry.hxx"

using std::string;

// Constructor
TileEntry::TileEntry ( const SGBucket& b )
    : tile_bucket( b ),
      tileFileName(b.gen_index_str()),
      _node( new osg::LOD ),
      _priority(-FLT_MAX),
      _current_view(false),
      _time_expired(-1.0)
{
    _create_orthophoto();
    
    tileFileName += ".stg";
    _node->setName(tileFileName);
    // Give a default LOD range so that traversals that traverse
    // active children (like the groundcache lookup) will work before
    // tile manager has had a chance to update this node.
    _node->setRange(0, 0.0, 10000.0);
}

TileEntry::TileEntry( const TileEntry& t )
: tile_bucket( t.tile_bucket ),
  tileFileName(t.tileFileName),
  _node( new osg::LOD ),
  _priority(t._priority),
  _current_view(t._current_view),
  _time_expired(t._time_expired)
{
    _create_orthophoto();

    _node->setName(tileFileName);
    // Give a default LOD range so that traversals that traverse
    // active children (like the groundcache lookup) will work before
    // tile manager has had a chance to update this node.
    _node->setRange(0, 0.0, 10000.0);
}

void TileEntry::_create_orthophoto() {
    bool use_photoscenery = fgGetBool("/sim/rendering/photoscenery/enabled");
    if (use_photoscenery) {
        _orthophoto = simgear::Orthophoto::fromBucket(tile_bucket, globals->get_fg_scenery());
        if (_orthophoto) {
            simgear::OrthophotoManager::instance()->registerOrthophoto(tile_bucket.gen_index(), _orthophoto);
        }
    }
}

void TileEntry::_free_orthophoto() {
    if (_orthophoto) {
        simgear::OrthophotoManager::instance()->unregisterOrthophoto(tile_bucket.gen_index());
    }
}

// Destructor
TileEntry::~TileEntry ()
{
}

// Update the ssg transform node for this tile so it can be
// properly drawn relative to our (0,0,0) point
void TileEntry::prep_ssg_node(float vis) {
    if (!is_loaded())
        return;
    // visibility can change from frame to frame so we update the
    // range selector cutoff's each time.
    float bounding_radius = _node->getChild(0)->getBound().radius();
    _node->setRange( 0, 0, vis + bounding_radius );
}

void
TileEntry::addToSceneGraph(osg::Group *terrain_branch)
{
    terrain_branch->addChild( _node.get() );

    SG_LOG( SG_TERRAIN, SG_DEBUG,
            "connected a tile into scene graph.  _node = "
            << _node.get() );
    SG_LOG( SG_TERRAIN, SG_DEBUG, "num parents now = "
            << _node->getNumParents() );
}


void
TileEntry::removeFromSceneGraph()
{
    if (! is_loaded()) {
        SG_LOG( SG_TERRAIN, SG_DEBUG, "removing a not-fully loaded tile!" );
    } else {
        SG_LOG( SG_TERRAIN, SG_DEBUG, "removing a fully loaded tile!  _node = " << _node.get() );
    }

    // find the nodes branch parent
    if ( _node->getNumParents() > 0 ) {
        // find the first parent (should only be one)
        osg::Group *parent = _node->getParent( 0 ) ;
        if( parent ) {
            parent->removeChild( _node.get() );
        }
    }

    _free_orthophoto();
}

