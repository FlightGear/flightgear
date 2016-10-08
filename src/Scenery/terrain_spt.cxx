// scenery.cxx -- data structures and routines for managing scenery.
//
// Written by Curtis Olson, started May 1997.
//
// Copyright (C) 1997  Curtis L. Olson  - http://www.flightgear.org/~curt
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

#include <boost/lexical_cast.hpp>

#include <simgear/scene/material/mat.hxx>
#include <simgear/scene/util/SGReaderWriterOptions.hxx>

#include <Main/globals.hxx>
#include <Main/fg_props.hxx>
#include <Viewer/splash.hxx>

#include "terrain_spt.hxx"
#include "scenery.hxx"

using namespace flightgear;
using namespace simgear;

using flightgear::SceneryPager;

// Terrain Management system
FGSptTerrain::FGSptTerrain() : 
    _scenery_loaded(fgGetNode("/sim/sceneryloaded", true)),
    _scenery_override(fgGetNode("/sim/sceneryloaded-override", true))
{
    _inited  = false;
}

FGSptTerrain::~FGSptTerrain()
{
}

// Initialize the Scenery Management system
void FGSptTerrain::init( osg::Group* terrain ) {
    // Already set up.
    if (_inited)
        return;

    SG_LOG(SG_TERRAIN, SG_INFO, "FGSptTerrain::init");

    // remember the scene terrain branch on scenegraph
    terrain_branch = terrain;

    // load the whole planet tile - database pager handles 
    // the quad tree / loading the highres tiles
    osg::ref_ptr<simgear::SGReaderWriterOptions> options;

    // drops the previous options reference
    options = new simgear::SGReaderWriterOptions;
    options->setPropertyNode(globals->get_props());

    osgDB::FilePathList &fp = options->getDatabasePathList();
    const PathList &sc = globals->get_fg_scenery();
    fp.clear();
    PathList::const_iterator it;
    for (it = sc.begin(); it != sc.end(); ++it) {
        fp.push_back(it->local8BitStr());
    }

    options->setPluginStringData("SimGear::FG_ROOT", globals->get_fg_root().local8BitStr());

    options->setPluginStringData("SimGear::BARE_LOD_RANGE", fgGetString("/sim/rendering/static-lod/bare", boost::lexical_cast<string>(SG_OBJECT_RANGE_BARE)));
    options->setPluginStringData("SimGear::ROUGH_LOD_RANGE", fgGetString("/sim/rendering/static-lod/rough", boost::lexical_cast<string>(SG_OBJECT_RANGE_ROUGH)));
    options->setPluginStringData("SimGear::ROUGH_LOD_DETAILED", fgGetString("/sim/rendering/static-lod/detailed", boost::lexical_cast<string>(SG_OBJECT_RANGE_DETAILED)));
    options->setPluginStringData("SimGear::RENDER_BUILDING_MESH", fgGetBool("/sim/rendering/building-mesh", false) ? "true" : "false");

    options->setPluginStringData("SimGear::FG_EARTH", "ON");

    // tunables
    options->setPluginStringData("SimGear::SPT_PAGE_LEVELS", fgGetString("/sim/scenery/lod-levels", "1 3 5 7 9" ));
    options->setPluginStringData("SimGear::SPT_LOD_LEVEL_RES", fgGetString("/sim/scenery/lod-level-res", "3 5 9 17 33 49 65 93 129" ));
    options->setPluginStringData("SimGear::SPT_RANGE_MULTIPLIER", fgGetString("/sim/scenery/lod-range-mult", "2" ));
    options->setPluginStringData("SimGear::SPT_MESH_RESOLUTION", fgGetString("/sim/scenery/lod-res", "1" ));

    options->setMaterialLib(globals->get_matlib());

    // TODO: for now we only support one dem db.  See if we can support multiple to pick 'best'
    _dem = new SGDem;
    for (osgDB::FilePathList::const_iterator i = fp.begin(); i != fp.end(); ++i) {
        SGPath demPath(*i);
        demPath.append("DEM");

        if ( _dem ) {
            int numLevels = _dem->init(demPath);
            if ( numLevels ) {
                SG_LOG(SG_TERRAIN, SG_INFO, "Terrain init - dem path " << demPath << " has " << numLevels << " LOD Levels " );
                break;
            } else {
                SG_LOG(SG_TERRAIN, SG_INFO, "Terrain init - dem path " << demPath << " has NO LOD Levels " );
            }
        }
    }


    options->setDem(_dem);
    osg::ref_ptr<osg::Node> loadedModel = osgDB::readNodeFile("w180s90-360x180.spt", options.get());

    terrain_branch->addChild( loadedModel.get() );

    // Toggle the setup flag.
    _inited = true;
}

void FGSptTerrain::reinit()
{

}

void FGSptTerrain::shutdown()
{
    terrain_branch = NULL;

    // Toggle the setup flag.
    _inited = false;
}


void FGSptTerrain::update(double dt)
{
    // scenery loading check, triggers after each sim (tile manager) reinit
    if (!_scenery_loaded->getBoolValue())
    {
        bool fdmInited = fgGetBool("sim/fdm-initialized");
        bool positionFinalized = fgGetBool("sim/position-finalized");
        bool sceneryOverride = _scenery_override->getBoolValue();

        // we are done if final position is set and the scenery & FDM are done.
        // scenery-override can ignore the last two, but not position finalization.
        if (positionFinalized && (sceneryOverride || fdmInited))
        {
            _scenery_loaded->setBoolValue(true);
            fgSplashProgress("");
        }
        else
        {
            if (!positionFinalized) {
                fgSplashProgress("finalize-position");
            } else {
                fgSplashProgress("loading-scenery");
            }

            // be nice to loader threads while waiting for initial scenery, reduce to 20fps
            SGTimeStamp::sleepForMSec(50);
        }
    }
}

void FGSptTerrain::bind() 
{

}

void FGSptTerrain::unbind() 
{

}

bool
FGSptTerrain::get_cart_elevation_m(const SGVec3d& pos, double max_altoff,
                                   double& alt,
                                   const simgear::BVHMaterial** material,
                                   const osg::Node* butNotFrom)
{
    SGGeod geod = SGGeod::fromCart(pos);
    geod.setElevationM(geod.getElevationM() + max_altoff);

    return get_elevation_m(geod, alt, material, butNotFrom);
}

static simgear::BVHMaterial def_mat;

bool
FGSptTerrain::get_elevation_m(const SGGeod& geod, double& alt,
                              const simgear::BVHMaterial** material,
                              const osg::Node* butNotFrom)
{
    alt = 100.0;
    if (material) {
      *material = &def_mat;
    } else {
      SG_LOG(SG_TERRAIN, SG_INFO, "FGStgTerrain::get_elevation_m: alt " << alt << " no material " );
    }

    return true;
}

bool FGSptTerrain::get_cart_ground_intersection(const SGVec3d& pos, const SGVec3d& dir,
                                           SGVec3d& nearestHit,
                                           const osg::Node* butNotFrom)
{
    return true;
}

bool FGSptTerrain::scenery_available(const SGGeod& position, double range_m)
{
    if( schedule_scenery(position, range_m, 0.0) )
    {
        return true;
    }
    else
    {
        return false;
    }
}

bool FGSptTerrain::schedule_scenery(const SGGeod& position, double range_m, double duration)
{
    // sanity check (unfortunately needed!)
    if (!position.isValid()) {
        SG_LOG(SG_TERRAIN, SG_INFO, "FGSptTerrain::schedule_scenery - position invalid");
        return false;
    }

    return true;
}

void FGSptTerrain::materialLibChanged()
{
    // PSADRO: TODO - passing down new regional textures won't work.  these need to be set in the 
    // lod tree at init time, as OSGDBPager generates the load request, not the tile cache.

    // _options->setMaterialLib(globals->get_matlib());
}
