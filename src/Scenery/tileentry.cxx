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
//
// $Id$


#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <simgear/compiler.h>
#include <plib/ul.h>
#include <Main/main.hxx>


#include STL_STRING
#include <sstream>

#include <osg/Array>
#include <osg/Geometry>
#include <osg/Geode>
#include <osg/LOD>
#include <osg/MatrixTransform>
#include <osg/NodeCallback>
#include <osg/Switch>

#include <osgDB/FileNameUtils>
#include <osgDB/ReaderWriter>
#include <osgDB/ReadFile>
#include <osgDB/Registry>

#include <simgear/bucket/newbucket.hxx>
#include <simgear/debug/logstream.hxx>
#include <simgear/math/polar3d.hxx>
#include <simgear/math/sg_geodesy.hxx>
#include <simgear/math/sg_random.h>
#include <simgear/misc/sgstream.hxx>
#include <simgear/scene/material/mat.hxx>
#include <simgear/scene/material/matlib.hxx>
#include <simgear/scene/model/ModelRegistry.hxx>
#include <simgear/scene/tgdb/apt_signs.hxx>
#include <simgear/scene/tgdb/obj.hxx>
#include <simgear/scene/tgdb/SGReaderWriterBTGOptions.hxx>
#include <simgear/scene/model/placementtrans.hxx>
#include <simgear/scene/util/SGUpdateVisitor.hxx>

#include <Aircraft/aircraft.hxx>
#include <Include/general.hxx>
#include <Main/fg_props.hxx>
#include <Main/globals.hxx>
#include <Main/viewer.hxx>
#include <Scenery/scenery.hxx>
#include <Time/light.hxx>

#include "tileentry.hxx"
#include "tilemgr.hxx"

SG_USING_STD(string);
using namespace simgear;

// FIXME: investigate what huge update flood is clamped away here ...
class FGTileUpdateCallback : public osg::NodeCallback {
public:
  virtual void operator()(osg::Node* node, osg::NodeVisitor* nv)
  {
    assert(dynamic_cast<SGUpdateVisitor*>(nv));
    SGUpdateVisitor* updateVisitor = static_cast<SGUpdateVisitor*>(nv);

    osg::Vec3 center = node->getBound().center();
    double distance = dist(updateVisitor->getGlobalEyePos(),
                           SGVec3d(center[0], center[1], center[2]));
    if (updateVisitor->getVisibility() + node->getBound().radius() < distance)
      return;

    traverse(node, nv);
  }
};

// Constructor
FGTileEntry::FGTileEntry ( const SGBucket& b )
    : tile_bucket( b ),
      terra_transform( new osg::Group ),
      terra_range( new osg::LOD ),
      loaded(false),
      pending_models(0),
      is_inner_ring(false),
      free_tracker(0)
{
    terra_transform->setUpdateCallback(new FGTileUpdateCallback);
}


// Destructor
FGTileEntry::~FGTileEntry () {
}

static void WorldCoordinate( osg::Matrix& obj_pos, double lat,
                             double lon, double elev, double hdg )
{
    double lon_rad = lon * SGD_DEGREES_TO_RADIANS;
    double lat_rad = lat * SGD_DEGREES_TO_RADIANS;
    double hdg_rad = hdg * SGD_DEGREES_TO_RADIANS;

    // setup transforms
    Point3D geod( lon_rad, lat_rad, elev );
    Point3D world_pos = sgGeodToCart( geod );

    double sin_lat = sin( lat_rad );
    double cos_lat = cos( lat_rad );
    double cos_lon = cos( lon_rad );
    double sin_lon = sin( lon_rad );
    double sin_hdg = sin( hdg_rad ) ;
    double cos_hdg = cos( hdg_rad ) ;

    obj_pos(0, 0) =  cos_hdg * sin_lat * cos_lon - sin_hdg * sin_lon;
    obj_pos(0, 1) =  cos_hdg * sin_lat * sin_lon + sin_hdg * cos_lon;
    obj_pos(0, 2) = -cos_hdg * cos_lat;
    obj_pos(0, 3) =  SG_ZERO;

    obj_pos(1, 0) = -sin_hdg * sin_lat * cos_lon - cos_hdg * sin_lon;
    obj_pos(1, 1) = -sin_hdg * sin_lat * sin_lon + cos_hdg * cos_lon;
    obj_pos(1, 2) =  sin_hdg * cos_lat;
    obj_pos(1, 3) =  SG_ZERO;

    obj_pos(2, 0) = cos_lat * cos_lon;
    obj_pos(2, 1) = cos_lat * sin_lon;
    obj_pos(2, 2) = sin_lat;
    obj_pos(2, 3) =  SG_ZERO;

    obj_pos(3, 0) = world_pos.x();
    obj_pos(3, 1) = world_pos.y();
    obj_pos(3, 2) = world_pos.z();
    obj_pos(3, 3) = SG_ONE ;
}


// Free "n" leaf elements of an ssg tree.  returns the number of
// elements freed.  An empty branch node is considered a leaf.  This
// is intended to spread the load of freeing a complex tile out over
// several frames.
static int fgPartialFreeSSGtree( osg::Group *b, int n ) {
    int num_deletes = b->getNumChildren();

    b->removeChildren(0, b->getNumChildren());

    return num_deletes;
}


// Clean up the memory used by this tile and delete the arrays used by
// ssg as well as the whole ssg branch
bool FGTileEntry::free_tile() {
    int delete_size = 100;
    SG_LOG( SG_TERRAIN, SG_DEBUG,
            "FREEING TILE = (" << tile_bucket << ")" );

    SG_LOG( SG_TERRAIN, SG_DEBUG, "(start) free_tracker = " << free_tracker );
    
    if ( !(free_tracker & NODES) ) {
        free_tracker |= NODES;
    } else if ( !(free_tracker & VEC_PTRS) ) {
        free_tracker |= VEC_PTRS;
    } else if ( !(free_tracker & TERRA_NODE) ) {
        // delete the terrain branch (this should already have been
        // disconnected from the scene graph)
        SG_LOG( SG_TERRAIN, SG_DEBUG, "FREEING terra_transform" );
        if ( fgPartialFreeSSGtree( terra_transform.get(), delete_size ) == 0 ) {
            terra_transform = 0;
            free_tracker |= TERRA_NODE;
        }
    } else if ( !(free_tracker & LIGHTMAPS) ) {
        free_tracker |= LIGHTMAPS;
    } else {
        return true;
    }

    SG_LOG( SG_TERRAIN, SG_DEBUG, "(end) free_tracker = " << free_tracker );

    // if we fall down to here, we still have work todo, return false
    return false;
}


// Update the ssg transform node for this tile so it can be
// properly drawn relative to our (0,0,0) point
void FGTileEntry::prep_ssg_node(float vis) {
    if ( !loaded ) return;

    // visibility can change from frame to frame so we update the
    // range selector cutoff's each time.
    float bounding_radius = terra_range->getChild(0)->getBound().radius();
    terra_range->setRange( 0, 0, vis + bounding_radius );
}

bool FGTileEntry::obj_load( const string& path,
                            osg::Group *geometry, bool is_base )
{
    bool use_random_objects =
        fgGetBool("/sim/rendering/random-objects", true);

    // try loading binary format
    osg::ref_ptr<SGReaderWriterBTGOptions> options
        = new SGReaderWriterBTGOptions();
    options->setMatlib(globals->get_matlib());
    options->setCalcLights(is_base);
    options->setUseRandomObjects(use_random_objects);
    osg::Node* node = osgDB::readNodeFile(path, options.get());
    if (node)
      geometry->addChild(node);

    return node;
}


typedef enum {
    OBJECT,
    OBJECT_SHARED,
    OBJECT_STATIC,
    OBJECT_SIGN,
    OBJECT_RUNWAY_SIGN
} object_type;


// storage class for deferred object processing in FGTileEntry::load()
struct Object {
    Object(object_type t, const string& token, const SGPath& p, istream& in)
        : type(t), path(p)
    {
        in >> name;
        if (type != OBJECT)
            in >> lon >> lat >> elev >> hdg;
        in >> ::skipeol;

        if (type == OBJECT)
            SG_LOG(SG_TERRAIN, SG_INFO, "    " << token << "  " << name);
        else
            SG_LOG(SG_TERRAIN, SG_INFO, "    " << token << "  " << name << "  lon=" <<
                    lon << "  lat=" << lat << "  elev=" << elev << "  hdg=" << hdg);
    }
    object_type type;
    string name;
    SGPath path;
    double lon, lat, elev, hdg;
};

// Work in progress... load the tile based entirely by name cuz that's
// what we'll want to do with the database pager.
void
FGTileEntry::load( const string_list &path_list, bool is_base )
{
    osgDB::ReaderWriter::ReadResult result
        = osgDB::Registry::instance()->readNode(tile_bucket.gen_index_str()
                                                + ".stg", 0);        
    if (result.validNode()) {
        osg::Node* new_tile = result.getNode();
        terra_range->addChild( new_tile );
    }
    terra_transform->addChild( terra_range.get() );
}

osg::Node*
FGTileEntry::loadTileByName(const string& index_str,
                            const string_list &path_list)
{
    long tileIndex;
    {
        istringstream idxStream(index_str);
        idxStream >> tileIndex;
    }
    SGBucket tile_bucket(tileIndex);
    const string basePath = tile_bucket.gen_base_path();
    
    bool found_tile_base = false;

    SGPath object_base;
    vector<const Object*> objects;

    SG_LOG( SG_TERRAIN, SG_INFO, "Loading tile " << index_str );

    // scan and parse all files and store information
    for (unsigned int i = 0; i < path_list.size(); i++) {
        // If we found a terrain tile in Terrain/, we have to process the
        // Objects/ dir in the same group, too, before we can stop scanning.
        // FGGlobals::set_fg_scenery() inserts an empty string to path_list
        // as marker.
        if (path_list[i].empty()) {
            if (found_tile_base)
                break;
            else
                continue;
        }

        bool has_base = false;

        SGPath tile_path = path_list[i];
        tile_path.append(basePath);

        SGPath basename = tile_path;
        basename.append( index_str );

        SG_LOG( SG_TERRAIN, SG_INFO, "  Trying " << basename.str() );


        // Check for master .stg (scene terra gear) file
        SGPath stg_name = basename;
        stg_name.concat( ".stg" );

        sg_gzifstream in( stg_name.str() );
        if ( !in.is_open() )
            continue;

        while ( ! in.eof() ) {
            string token;
            in >> token;

            if ( token.empty() || token[0] == '#' ) {
               in >> ::skipeol;
               continue;
            }
                            // Load only once (first found)
            if ( token == "OBJECT_BASE" ) {
                string name;
                in >> name >> ::skipws;
                SG_LOG( SG_TERRAIN, SG_INFO, "    " << token << " " << name );

                if (!found_tile_base) {
                    found_tile_base = true;
                    has_base = true;

                    object_base = tile_path;
                    object_base.append(name);

                } else
                    SG_LOG(SG_TERRAIN, SG_INFO, "    (skipped)");

                            // Load only if base is not in another file
            } else if ( token == "OBJECT" ) {
                if (!found_tile_base || has_base)
                    objects.push_back(new Object(OBJECT, token, tile_path, in));
                else {
                    string name;
                    in >> name >> ::skipeol;
                    SG_LOG(SG_TERRAIN, SG_INFO, "    " << token << "  "
                            << name << "  (skipped)");
                }

                            // Always OK to load
            } else if ( token == "OBJECT_STATIC" ) {
                objects.push_back(new Object(OBJECT_STATIC, token, tile_path, in));

            } else if ( token == "OBJECT_SHARED" ) {
                objects.push_back(new Object(OBJECT_SHARED, token, tile_path, in));

            } else if ( token == "OBJECT_SIGN" ) {
                objects.push_back(new Object(OBJECT_SIGN, token, tile_path, in));

            } else if ( token == "OBJECT_RUNWAY_SIGN" ) {
                objects.push_back(new Object(OBJECT_RUNWAY_SIGN, token, tile_path, in));

            } else {
                SG_LOG( SG_TERRAIN, SG_DEBUG,
                        "Unknown token '" << token << "' in " << stg_name.str() );
                in >> ::skipws;
            }
        }
    }


    // obj_load() will generate ground lighting for us ...
    osg::Group* new_tile = new osg::Group;

    if (found_tile_base) {
        // load tile if found ...
        obj_load( object_base.str(), new_tile, true );

    } else {
        // ... or generate an ocean tile on the fly
        SG_LOG(SG_TERRAIN, SG_INFO, "  Generating ocean tile");
        if ( !SGGenTile( path_list[0], tile_bucket,
                        globals->get_matlib(), new_tile ) ) {
            SG_LOG( SG_TERRAIN, SG_ALERT,
                    "Warning: failed to generate ocean tile!" );
        }
    }


    // now that we have a valid center, process all the objects
    for (unsigned int j = 0; j < objects.size(); j++) {
        const Object *obj = objects[j];

        if (obj->type == OBJECT) {
            SGPath custom_path = obj->path;
            custom_path.append( obj->name );
            obj_load( custom_path.str(), new_tile, false );

        } else if (obj->type == OBJECT_SHARED || obj->type == OBJECT_STATIC) {
            // object loading is deferred to main render thread,
            // but lets figure out the paths right now.
            SGPath custom_path;
            if ( obj->type == OBJECT_STATIC ) {
                custom_path = obj->path;
            } else {
                custom_path = globals->get_fg_root();
            }
            custom_path.append( obj->name );

            osg::Matrix obj_pos;
            WorldCoordinate( obj_pos, obj->lat, obj->lon, obj->elev, obj->hdg );

            osg::MatrixTransform *obj_trans = new osg::MatrixTransform;
            obj_trans->setMatrix( obj_pos );

            // wire as much of the scene graph together as we can
            new_tile->addChild( obj_trans );

            osg::Node* model
                = FGTileMgr::loadTileModel(custom_path.str(),
                                           obj->type == OBJECT_SHARED);
            if (model)
                obj_trans->addChild(model);
        } else if (obj->type == OBJECT_SIGN || obj->type == OBJECT_RUNWAY_SIGN) {
            // load the object itself
            SGPath custom_path = obj->path;
            custom_path.append( obj->name );

            osg::Matrix obj_pos;
            WorldCoordinate( obj_pos, obj->lat, obj->lon, obj->elev, obj->hdg );

            osg::MatrixTransform *obj_trans = new osg::MatrixTransform;
            obj_trans->setMatrix( obj_pos );

            osg::Node *custom_obj = 0;
            if (obj->type == OBJECT_SIGN)
                custom_obj = SGMakeSign(globals->get_matlib(), custom_path.str(), obj->name);
            else
                custom_obj = SGMakeRunwaySign(globals->get_matlib(), custom_path.str(), obj->name);

            // wire the pieces together
            if ( custom_obj != NULL ) {
                obj_trans -> addChild( custom_obj );
            }
            new_tile->addChild( obj_trans );

        }
        delete obj;
    }
    return new_tile;
}

void
FGTileEntry::add_ssg_nodes( osg::Group *terrain_branch )
{
    // bump up the ref count so we can remove this later without
    // having ssg try to free the memory.
    terrain_branch->addChild( terra_transform.get() );

    SG_LOG( SG_TERRAIN, SG_DEBUG,
            "connected a tile into scene graph.  terra_transform = "
            << terra_transform.get() );
    SG_LOG( SG_TERRAIN, SG_DEBUG, "num parents now = "
            << terra_transform->getNumParents() );

    loaded = true;
}


void
FGTileEntry::disconnect_ssg_nodes()
{
    SG_LOG( SG_TERRAIN, SG_DEBUG, "disconnecting ssg nodes" );

    if ( ! loaded ) {
        SG_LOG( SG_TERRAIN, SG_DEBUG, "removing a not-fully loaded tile!" );
    } else {
        SG_LOG( SG_TERRAIN, SG_DEBUG, "removing a fully loaded tile!  terra_transform = " << terra_transform.get() );
    }
        
    // find the terrain branch parent
    int pcount = terra_transform->getNumParents();
    if ( pcount > 0 ) {
        // find the first parent (should only be one)
        osg::Group *parent = terra_transform->getParent( 0 ) ;
        if( parent ) {
            // disconnect the tile (we previously ref()'d it so it
            // won't get freed now)
            parent->removeChild( terra_transform.get() );
        } else {
            SG_LOG( SG_TERRAIN, SG_ALERT,
                    "parent pointer is NULL!  Dying" );
            exit(-1);
        }
    } else {
        SG_LOG( SG_TERRAIN, SG_ALERT,
                "Parent count is zero for an ssg tile!  Dying" );
        exit(-1);
    }
}

namespace
{

class ReaderWriterSTG : public osgDB::ReaderWriter {
public:
    virtual const char* className() const;
 
    virtual bool acceptsExtension(const string& extension) const;

    virtual ReadResult readNode(const string& fileName,
                                const osgDB::ReaderWriter::Options* options)
        const;
};

const char* ReaderWriterSTG::className() const
{
    return "STG Database reader";
}

bool ReaderWriterSTG::acceptsExtension(const string& extension) const
{
    return (osgDB::equalCaseInsensitive(extension, "gz")
            || osgDB::equalCaseInsensitive(extension, "stg"));   
}

osgDB::ReaderWriter::ReadResult
ReaderWriterSTG::readNode(const string& fileName,
                          const osgDB::ReaderWriter::Options* options) const
{
    string ext = osgDB::getLowerCaseFileExtension(fileName);
    if(!acceptsExtension(ext))
        return ReadResult::FILE_NOT_HANDLED;
    string stgFileName;
    if (osgDB::equalCaseInsensitive(ext, "gz")) {
        stgFileName = osgDB::getNameLessExtension(fileName);
        if (!acceptsExtension(
                osgDB::getLowerCaseFileExtension(stgFileName))) {
            return ReadResult::FILE_NOT_HANDLED;
        }
    } else {
        stgFileName = fileName;
    }
    osg::Node* result
        = FGTileEntry::loadTileByName(osgDB::getNameLessExtension(stgFileName),
                                      globals->get_fg_scenery());
    if (result)
        return result;           // Constructor converts to ReadResult
    else
        return ReadResult::FILE_NOT_HANDLED;
}

osgDB::RegisterReaderWriterProxy<ReaderWriterSTG> g_readerWriterSTGProxy;
ModelRegistryCallbackProxy<LoadOnlyCallback> g_stgCallbackProxy("stg");
} // namespace
