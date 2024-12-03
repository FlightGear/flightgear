// environment-mgr.hxx -- manager for natural environment information.
//
// Written by David Megginson, started February 2002.
//
// Copyright (C) 2002  David Megginson - david@megginson.com
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

#ifndef _ENVIRONMENT_MGR_HXX
#define _ENVIRONMENT_MGR_HXX

#include <simgear/compiler.h>
#include <simgear/structure/subsystem_mgr.hxx>
#include <simgear/props/tiedpropertylist.hxx>
#include <simgear/math/SGMath.hxx>

#include <cmath>

class FGEnvironment;
class FGClimate;
class FGClouds;
class FGPrecipitationMgr;
class SGSky;
struct FGEnvironmentMgrPropertyListener;

/**
 * Manage environment information.
 */
class FGEnvironmentMgr : public SGSubsystemGroup
{
public:
    enum {
        MAX_CLOUD_LAYERS = 5
    };

    FGEnvironmentMgr ();
    virtual ~FGEnvironmentMgr ();

    // Subsystem API.
    void bind() override;
    InitStatus incrementalInit() override;
    void reinit() override;
    void shutdown() override;
    void unbind() override;
    void update(double dt) override;

    // Subsystem identification.
    static const char* staticSubsystemClassId() { return "environment"; }

    /**
     * Get the environment information for the plane's current position.
     */
    virtual FGEnvironment getEnvironment () const;

    const FGEnvironment* getAircraftEnvironment() const;
    
    virtual FGEnvironment getEnvironmentAtPosition(const SGGeod& aPos) const;

private:
    friend FGEnvironmentMgrPropertyListener;

    void updateClosestAirport();
    void updateDynamicTowerPosition();
    void onTowerAirportIDChanged();

    double get_cloud_layer_span_m (int index) const;
    void set_cloud_layer_span_m (int index, double span_m);
    double get_cloud_layer_elevation_ft (int index) const;
    void set_cloud_layer_elevation_ft (int index, double elevation_ft);
    double get_cloud_layer_thickness_ft (int index) const;
    void set_cloud_layer_thickness_ft (int index, double thickness_ft);
    double get_cloud_layer_transition_ft (int index) const;
    void set_cloud_layer_transition_ft (int index, double transition_ft);
    const char * get_cloud_layer_coverage (int index) const;
    void set_cloud_layer_coverage (int index, const char * coverage);
    int get_cloud_layer_coverage_type (int index) const;
    void set_cloud_layer_coverage_type (int index, int type );
    double get_cloud_layer_visibility_m (int index) const;
    void set_cloud_layer_visibility_m (int index, double visibility_m);
    double get_cloud_layer_maxalpha (int index ) const;
    void set_cloud_layer_maxalpha (int index, double maxalpha);

    FGClimate * _climate = nullptr;
    FGEnvironment * _environment = nullptr; // always the same, for now
    FGClouds *fgClouds = nullptr;
    bool _cloudLayersDirty = true;
    int max_tower_height_feet;
    int min_tower_height_feet;
    int default_tower_height_feet;

    simgear::TiedPropertyList _tiedProperties;
    SGPropertyChangeListener * _3dCloudsEnableListener;

    std::unique_ptr<FGEnvironmentMgrPropertyListener> _listener;
    SGSky* _sky;

    SGPropertyNode_ptr towerViewPositionLatDegNode;
    SGPropertyNode_ptr towerViewPositionLonDegNode;
    SGPropertyNode_ptr towerViewPositionAltFtNode;
    SGPropertyNode_ptr _automaticTowerEnableNode;
    
    const class FGAICarrier* nearestCarrier;
    const class FGAirport* nearestAirport;
};

#endif // _ENVIRONMENT_MGR_HXX
