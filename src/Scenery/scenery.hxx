// scenery.hxx -- data structures and routines for managing scenery.
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


#ifndef _SCENERY_HXX
#define _SCENERY_HXX


#ifndef __cplusplus
# error This library requires C++
#endif

#include <osg/ref_ptr>
#include <osg/Switch>

#include <simgear/compiler.h>
#include <simgear/math/SGMath.hxx>
#include <simgear/scene/model/particles.hxx>
#include <simgear/structure/subsystem_mgr.hxx>

#include "SceneryPager.hxx"
#include "terrain.hxx"

namespace simgear {
class BVHMaterial;
}

class FGTerrain;

// Define a structure containing global scenery parameters
class FGScenery : public SGSubsystem
{
    class ScenerySwitchListener;
    friend class ScenerySwitchListener;

    // scene graph
    osg::ref_ptr<osg::Switch> scene_graph;
    osg::ref_ptr<osg::Group> terrain_branch;
    osg::ref_ptr<osg::Group> models_branch;
    osg::ref_ptr<osg::Group> aircraft_branch;
    osg::ref_ptr<osg::Group> interior_branch;
    osg::ref_ptr<osg::Group> particles_branch;
    osg::ref_ptr<osg::Group> precipitation_branch;

    osg::ref_ptr<flightgear::SceneryPager> _pager;
    ScenerySwitchListener* _listener;

    class TextureCacheListener;
    friend class TextureCacheListener;
    TextureCacheListener* _textureCacheListener;

    class ElevationMeshListener;
    friend class ElevationMeshListener;
    ElevationMeshListener* _elevationMeshListener;

public:
    FGScenery();
    ~FGScenery();

    // Subsystem API.
    void bind() override;
    void init() override;
    void reinit() override;
    void shutdown() override;
    void unbind() override;
    void update(double dt) override;

    // Subsystem identification.
    static const char* staticSubsystemClassId() { return "scenery"; }

    /// Compute the elevation of the scenery at geodetic latitude lat,
    /// geodetic longitude lon and not higher than max_alt.
    /// If the exact flag is set to true, the scenery center is moved to
    /// gain a higher accuracy of that query. The center is restored past
    /// that to the original value.
    /// The altitude hit is returned in the alt argument.
    /// The method returns true if the scenery is available for the given
    /// lat/lon pair. If there is no scenery for that point, the altitude
    /// value is undefined.
    /// All values are meant to be in meters or degrees.
    bool get_elevation_m(const SGGeod& geod, double& alt,
                         const simgear::BVHMaterial** material,
                         const osg::Node* butNotFrom = 0);

    /// Compute the elevation of the scenery below the cartesian point pos.
    /// you the returned scenery altitude is not higher than the position
    /// pos plus an offset given with max_altoff.
    /// If the exact flag is set to true, the scenery center is moved to
    /// gain a higher accuracy of that query. The center is restored past
    /// that to the original value.
    /// The altitude hit is returned in the alt argument.
    /// The method returns true if the scenery is available for the given
    /// lat/lon pair. If there is no scenery for that point, the altitude
    /// value is undefined.
    /// All values are meant to be in meters.
    bool get_cart_elevation_m(const SGVec3d& pos, double max_altoff,
                              double& elevation,
                              const simgear::BVHMaterial** material,
                              const osg::Node* butNotFrom = 0);

    /// Compute the nearest intersection point of the line starting from
    /// start going in direction dir with the terrain.
    /// The input and output values should be in cartesian coordinates in the
    /// usual earth centered wgs84 coordinate system. Units are meters.
    /// On success, true is returned.
    bool get_cart_ground_intersection(const SGVec3d& start, const SGVec3d& dir,
                                      SGVec3d& nearestHit,
                                      const osg::Node* butNotFrom = 0);

    osg::Group *get_scene_graph () const { return scene_graph.get(); }
    osg::Group *get_terrain_branch () const { return terrain_branch.get(); }
    osg::Group *get_models_branch () const { return models_branch.get(); }
    osg::Group *get_aircraft_branch () const { return aircraft_branch.get(); }
    osg::Group *get_interior_branch () const { return interior_branch.get(); }
    osg::Group *get_particles_branch () const { return particles_branch.get(); }
    osg::Group *get_precipitation_branch () const { return precipitation_branch.get(); }

    /// Returns true if scenery is available for the given lat, lon position
    /// within a range of range_m.
    /// lat and lon are expected to be in degrees.
    bool scenery_available(const SGGeod& position, double range_m);

    // Static because access to the pager is needed before the rest of
    // the scenery is initialized.
    static flightgear::SceneryPager* getPagerSingleton();
    static void resetPagerSingleton();

    flightgear::SceneryPager* getPager() { return _pager.get(); }

    // tile mgr api
    bool schedule_scenery(const SGGeod& position, double range_m, double duration=0.0);
    void materialLibChanged();
private:
    // the terrain engine
    std::unique_ptr<FGTerrain> _terrain;

    // The state of the scene graph.
    bool _inited;
};

#endif // _SCENERY_HXX
