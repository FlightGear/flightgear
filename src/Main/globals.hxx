// globals.hxx -- Global state that needs to be shared among the sim modules
//
// Written by Curtis Olson, started July 2000.
//
// Copyright (C) 2000  Curtis L. Olson - http://www.flightgear.org/~curt
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


#ifndef _GLOBALS_HXX
#define _GLOBALS_HXX

#include <simgear/compiler.h>
#include <simgear/props/props.hxx>
#include <simgear/structure/subsystem_mgr.hxx>
#include <simgear/misc/sg_path.hxx>

#include <vector>
#include <string>
#include <memory>

typedef std::vector<std::string> string_list;
typedef std::vector<SGPath> PathList;

// Forward declarations

// This file is included, directly or indirectly, almost everywhere in
// FlightGear, so if any of its dependencies changes, most of the sim
// has to be recompiled.  Using these forward declarations helps us to
// avoid including a lot of header files (and thus, a lot of
// dependencies).  Since Most of the methods simply set or return
// pointers, we don't need to know anything about the class details
// anyway.

class SGEphemeris;
class SGCommandMgr;
class SGMaterialLib;
class SGPropertyNode;
class SGTime;
class SGEventMgr;
class SGSubsystemMgr;
class SGSubsystem;
class SGSoundMgr;

class FGControls;
class FGTACANList;
class FGLocale;
class FGRouteMgr;
class FGScenery;
class FGTileMgr;
class FGViewMgr;
class FGViewer;
class FGRenderer;
class FGFontCache;
class FGSampleQueue;

namespace simgear { namespace pkg {
  class Root;
}}

/**
 * Bucket for subsystem pointers representing the sim's state.
 */

class FGGlobals
{

private:

    // properties, destroy last
    SGPropertyNode_ptr props;

    // localization
    FGLocale* locale;

    FGRenderer *renderer;
    SGSubsystemMgr *subsystem_mgr;
    SGEventMgr *event_mgr;

    // Number of milliseconds elapsed since the start of the program.
    double sim_time_sec;

    // Root of FlightGear data tree
    std::string fg_root;

    /**
     * locations to search for (non-scenery) data. 
     */
    PathList additional_data_paths;
    
    // Users home directory for data
    std::string fg_home;

    // Roots of FlightGear scenery tree
    string_list fg_scenery;

    std::string browser;

    // Time structure
    SGTime *time_params;

    // Sky structures
    SGEphemeris *ephem;

    // Material properties library
    SGSharedPtr<SGMaterialLib> matlib;

    // Global autopilot "route"
    FGRouteMgr *route_mgr;

    // control input state
    FGControls *controls;

    // viewer manager
    FGViewMgr *viewmgr;

    SGCommandMgr *commands;

    // list of serial port-like configurations
    string_list *channel_options_list;

    // A list of initial waypoints that are read from the command line
    // and or flight-plan file during initialization
    string_list *initial_waypoints;

    // FlightGear scenery manager
    SGSharedPtr<FGScenery> _scenery;

    // Tile manager
    SGSharedPtr<FGTileMgr> _tile_mgr;

    FGFontCache *fontcache;

    // Navigational Aids
    FGTACANList *channellist;

    /// roots of Aircraft trees
    string_list fg_aircraft_dirs;
    SGPath catalog_aircraft_dir;

    bool haveUserSettings;

    SGPropertyNode_ptr positionLon, positionLat, positionAlt;
    SGPropertyNode_ptr viewLon, viewLat, viewAlt;
    SGPropertyNode_ptr orientHeading, orientPitch, orientRoll;
    
    /**
     * helper to initialise standard properties on a new property tree
     */
    void initProperties();
    
    SGSharedPtr<FGSampleQueue> _chatter_queue;
    
    void cleanupListeners();
    
    typedef std::vector<SGPropertyChangeListener*> SGPropertyChangeListenerVec;
    SGPropertyChangeListenerVec _listeners_to_cleanup;
  
    SGSharedPtr<simgear::pkg::Root> _packageRoot;
public:

    FGGlobals();
    virtual ~FGGlobals();

    virtual FGRenderer *get_renderer () const;
    void set_renderer(FGRenderer* render);
    
    virtual SGSubsystemMgr *get_subsystem_mgr () const;

    virtual SGSubsystem *get_subsystem (const char * name);

    virtual void add_subsystem (const char * name,
                                SGSubsystem * subsystem,
                                SGSubsystemMgr::GroupType
                                type = SGSubsystemMgr::GENERAL,
                                double min_time_sec = 0);

    virtual SGEventMgr *get_event_mgr () const;

    virtual SGSoundMgr *get_soundmgr () const;

    inline double get_sim_time_sec () const { return sim_time_sec; }
    inline void inc_sim_time_sec (double dt) { sim_time_sec += dt; }
    inline void set_sim_time_sec (double t) { sim_time_sec = t; }

    inline const std::string &get_fg_root () const { return fg_root; }
    void set_fg_root (const std::string &root);

    /**
     * Get list of data locations. fg_root is always the final item in the
     * result.
     */
    PathList get_data_paths() const;
    
    /**
     * Get data locations which contain the file path suffix. Eg pass ing
     * 'AI/Traffic' to get all data paths which define <path>/AI/Traffic subdir
     */
    PathList get_data_paths(const std::string& suffix) const;
    
    void append_data_path(const SGPath& path);
    
    /**
     * Given a path suffix (eg 'Textures' or 'AI/Traffic'), find the
     * first data directory which defines it.
     */
    SGPath find_data_dir(const std::string& pathSuffix) const;
    
    inline const std::string &get_fg_home () const { return fg_home; }
    void set_fg_home (const std::string &home);

    inline const string_list &get_fg_scenery () const { return fg_scenery; }
    void append_fg_scenery (const std::string &scenery);

    void clear_fg_scenery();

    /**
     * specify a path we'll prepend to the aircraft paths list if non-empty.
     * This is used with packaged aircraft, to ensure their catalog (and hence,
     * dependency packages) are found correctly.
     */
    void set_catalog_aircraft_path(const SGPath& path);

    string_list get_aircraft_paths() const;

    void append_aircraft_path(const std::string& path);
    void append_aircraft_paths(const std::string& path);
    
    /**
     * Given a path to an aircraft-related resource file, resolve it
     * against the appropriate root. This means looking at the location
     * defined by /sim/aircraft-dir, and then aircraft_path in turn,
     * finishing with fg_root/Aircraft.
     *
     * if the path could not be resolved, an empty path is returned.
     */
    SGPath resolve_aircraft_path(const std::string& branch) const;
    
    /**
     * Same as above, but test for non 'Aircraft/' branch paths, and
     * always resolve them against fg_root.
     */
    SGPath resolve_maybe_aircraft_path(const std::string& branch) const;
    
    /**
     * Search in the following directories:
     *
     *  1. Root directory of current aircraft (defined by /sim/aircraft-dir)
     *  2. All aircraft directories if branch starts with Aircraft/
     *  3. fg_data directory
     */
    SGPath resolve_resource_path(const std::string& branch) const;

    inline const std::string &get_browser () const { return browser; }
    void set_browser (const std::string &b) { browser = b; }

    long int get_warp() const;
    void set_warp( long int w );

    long int get_warp_delta() const;
    void set_warp_delta( long int d );

    inline SGTime *get_time_params() const { return time_params; }
    inline void set_time_params( SGTime *t ) { time_params = t; }

    inline SGEphemeris *get_ephem() const { return ephem; }
    inline void set_ephem( SGEphemeris *e ) { ephem = e; }

    inline SGMaterialLib *get_matlib() const { return matlib; }
    void set_matlib( SGMaterialLib *m );

    inline FGControls *get_controls() const { return controls; }
    inline void set_controls( FGControls *c ) { controls = c; }

    inline FGViewMgr *get_viewmgr() const { return viewmgr; }
    inline void set_viewmgr( FGViewMgr *vm ) { viewmgr = vm; }
    FGViewer *get_current_view() const;

    inline SGPropertyNode *get_props () { return props; }

    /**
     * @brief reset the property tree to new, empty tree. Ensure all
     * subsystems are shutdown and unbound before call this.
     */
    void resetPropertyRoot();
    
    inline FGLocale* get_locale () { return locale; }

    inline SGCommandMgr *get_commands () { return commands; }

    SGGeod get_aircraft_position() const;

    SGVec3d get_aircraft_position_cart() const;

    void get_aircraft_orientation(double& heading, double& pitch, double& roll);
  
    SGGeod get_view_position() const;
  
    SGVec3d get_view_position_cart() const;
  
    inline string_list *get_channel_options_list () {
	return channel_options_list;
    }
    inline void set_channel_options_list( string_list *l ) {
	channel_options_list = l;
    }

    inline string_list *get_initial_waypoints () {
        return initial_waypoints;
    }
  
    inline void set_initial_waypoints (string_list *list) {
        initial_waypoints = list;
    }

    FGScenery * get_scenery () const;
    void set_scenery ( FGScenery *s );

    FGTileMgr * get_tile_mgr () const;
    void set_tile_mgr ( FGTileMgr *t );

    inline FGFontCache *get_fontcache() const { return fontcache; }
  
    inline FGTACANList *get_channellist() const { return channellist; }
    inline void set_channellist( FGTACANList *c ) { channellist = c; }

    /**
     * Load user settings from autosave.xml
     */
    void loadUserSettings(const SGPath& datapath);

    /**
     * Save user settings in autosave.xml
     */
    void saveUserSettings();
    
    FGSampleQueue* get_chatter_queue() const;
    void set_chatter_queue(FGSampleQueue* queue);
    
    void addListenerToCleanup(SGPropertyChangeListener* l);
  
    simgear::pkg::Root* packageRoot();
    void setPackageRoot(const SGSharedPtr<simgear::pkg::Root>& p);
};


extern FGGlobals *globals;


#endif // _GLOBALS_HXX
