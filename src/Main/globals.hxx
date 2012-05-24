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

typedef std::vector<std::string> string_list;

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
class SGMagVar;
class SGMaterialLib;
class SGPropertyNode;
class SGTime;
class SGEventMgr;
class SGSubsystemMgr;
class SGSubsystem;
class SGSoundMgr;

class FGATISMgr;
class FGAircraftModel;
class FGControls;
class FGFlightPlanDispatcher;
class FGNavList;
class FGTACANList;
class FGLocale;
class FGModelMgr;
class FGRouteMgr;
class FGScenery;
class FGTileMgr;
class FGViewMgr;
class FGViewer;
class FGRenderer;
class FGFontCache;


/**
 * Bucket for subsystem pointers representing the sim's state.
 */

class FGGlobals
{

private:

    // properties, destroy last
    SGPropertyNode_ptr props;
    SGPropertyNode_ptr initial_state;

    // localization
    FGLocale* locale;

    FGRenderer *renderer;
    SGSubsystemMgr *subsystem_mgr;
    SGEventMgr *event_mgr;

    // Number of milliseconds elapsed since the start of the program.
    double sim_time_sec;

    // Root of FlightGear data tree
    std::string fg_root;

    // Roots of FlightGear scenery tree
    string_list fg_scenery;

    std::string browser;

    // Time structure
    SGTime *time_params;

    // Sky structures
    SGEphemeris *ephem;

    // Magnetic Variation
    SGMagVar *mag;

    // Material properties library
    SGMaterialLib *matlib;

    // Global autopilot "route"
    FGRouteMgr *route_mgr;

    // ATC manager
    FGATISMgr *ATIS_mgr;

    // control input state
    FGControls *controls;

    // viewer manager
    FGViewMgr *viewmgr;

    SGCommandMgr *commands;

  //FGFlightPlanDispatcher *fpDispatcher;

    FGAircraftModel *acmodel;

    FGModelMgr * model_mgr;

    // list of serial port-like configurations
    string_list *channel_options_list;

    // A list of initial waypoints that are read from the command line
    // and or flight-plan file during initialization
    string_list *initial_waypoints;

    // FlightGear scenery manager
    FGScenery *scenery;

    // Tile manager
    FGTileMgr *tile_mgr;

    FGFontCache *fontcache;

    // Navigational Aids
    FGNavList *navlist;
    FGNavList *loclist;
    FGNavList *gslist;
    FGNavList *dmelist;
    FGNavList *tacanlist;
    FGNavList *carrierlist;
    FGTACANList *channellist;

    /// roots of Aircraft trees
    string_list fg_aircraft_dirs;

    bool haveUserSettings;

public:

    FGGlobals();
    virtual ~FGGlobals();

    virtual FGRenderer *get_renderer () const;

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

    inline const string_list &get_fg_scenery () const { return fg_scenery; }
    void append_fg_scenery (const std::string &scenery);

    const string_list& get_aircraft_paths() const { return fg_aircraft_dirs; }
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
    SGPath resolve_ressource_path(const std::string& branch) const;

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

    inline SGMagVar *get_mag() const { return mag; }
    inline void set_mag( SGMagVar *m ) { mag = m; }

    inline SGMaterialLib *get_matlib() const { return matlib; }
    inline void set_matlib( SGMaterialLib *m ) { matlib = m; }

    inline FGATISMgr *get_ATIS_mgr() const { return ATIS_mgr; }
    inline void set_ATIS_mgr( FGATISMgr *a ) {ATIS_mgr = a; }

    inline FGControls *get_controls() const { return controls; }
    inline void set_controls( FGControls *c ) { controls = c; }

    inline FGViewMgr *get_viewmgr() const { return viewmgr; }
    inline void set_viewmgr( FGViewMgr *vm ) { viewmgr = vm; }
    FGViewer *get_current_view() const;

    inline SGPropertyNode *get_props () { return props; }
    inline void set_props( SGPropertyNode *n ) { props = n; }

    inline FGLocale* get_locale () { return locale; }

    inline SGCommandMgr *get_commands () { return commands; }

    inline FGAircraftModel *get_aircraft_model () { return acmodel; }

    inline void set_aircraft_model (FGAircraftModel * model)
    {
        acmodel = model;
    }

    const SGGeod & get_aircraft_position() const;

    SGVec3d get_aircraft_positon_cart() const;
    
    inline FGModelMgr *get_model_mgr () { return model_mgr; }

    inline void set_model_mgr (FGModelMgr * mgr)
    {
      model_mgr = mgr;
    }

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

    inline FGScenery * get_scenery () const { return scenery; }
    inline void set_scenery ( FGScenery *s ) { scenery = s; }

    inline FGTileMgr * get_tile_mgr () const { return tile_mgr; }
    inline void set_tile_mgr ( FGTileMgr *t ) { tile_mgr = t; }

    inline FGFontCache *get_fontcache() const { return fontcache; }

    inline FGNavList *get_navlist() const { return navlist; }
    inline void set_navlist( FGNavList *n ) { navlist = n; }
    inline FGNavList *get_loclist() const { return loclist; }
    inline void set_loclist( FGNavList *n ) { loclist = n; }
    inline FGNavList *get_gslist() const { return gslist; }
    inline void set_gslist( FGNavList *n ) { gslist = n; }
    inline FGNavList *get_dmelist() const { return dmelist; }
    inline void set_dmelist( FGNavList *n ) { dmelist = n; }
    inline FGNavList *get_tacanlist() const { return tacanlist; }
    inline void set_tacanlist( FGNavList *n ) { tacanlist = n; }
    inline FGNavList *get_carrierlist() const { return carrierlist; }
    inline void set_carrierlist( FGNavList *n ) { carrierlist = n; }
    inline FGTACANList *get_channellist() const { return channellist; }
    inline void set_channellist( FGTACANList *c ) { channellist = c; }

   /**
     * Save the current state as the initial state.
     */
    void saveInitialState ();


    /**
     * Restore the saved initial state, if any.
     */
    void restoreInitialState ();

    /**
     * Load user settings from autosave.xml
     */
    void loadUserSettings(const SGPath& datapath);

    /**
     * Save user settings in autosave.xml
     */
    void saveUserSettings();
};


extern FGGlobals *globals;


#endif // _GLOBALS_HXX
