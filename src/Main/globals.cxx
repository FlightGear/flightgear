// globals.cxx -- Global state that needs to be shared among the sim modules
//
// Written by Curtis Olson, started July 2000.
//
// Copyright (C) 2000  Curtis L. Olson - curt@flightgear.org
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
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//
// $Id$


#include <simgear/sound/soundmgr_openal.hxx>
#include <simgear/structure/commands.hxx>
#include <simgear/misc/sg_path.hxx>

#include "globals.hxx"
#include "viewmgr.hxx"

#include "fg_props.hxx"
#include "fg_io.hxx"


////////////////////////////////////////////////////////////////////////
// Implementation of FGGlobals.
////////////////////////////////////////////////////////////////////////

// global global :-)
FGGlobals *globals;


// Constructor
FGGlobals::FGGlobals() :
    subsystem_mgr( new SGSubsystemMgr ),
    event_mgr( new SGEventMgr ),
    sim_time_sec( 0.0 ),
    fg_root( "" ),
    fg_scenery( "" ),
#if defined(FX) && defined(XMESA)
    fullscreen( true ),
#endif
    warp( 0 ),
    warp_delta( 0 ),
    time_params( NULL ),
    ephem( NULL ),
    mag( NULL ),
    route_mgr( NULL ),
    current_panel( NULL ),
    soundmgr( NULL ),
    airports( NULL ),
    ATC_mgr( NULL ),
    ATC_display( NULL ),
    AI_mgr( NULL ),
    controls( NULL ),
    viewmgr( NULL ),
    props( new SGPropertyNode ),
    initial_state( NULL ),
    locale( NULL ),
    commands( new SGCommandMgr ),
    model_lib( NULL ),
    acmodel( NULL ),
    model_mgr( NULL ),
    channel_options_list( NULL ),
    initial_waypoints(0),
    scenery( NULL ),
    tile_mgr( NULL ),
    io( new FGIO ),
    navlist( NULL ),
    loclist( NULL ),
    gslist( NULL ),
    dmelist( NULL ),
    mkrlist( NULL ),
    fixlist( NULL )
{
}


// Destructor
FGGlobals::~FGGlobals() 
{
    delete soundmgr;
    delete subsystem_mgr;
    delete event_mgr;
    delete initial_state;
    delete props;
    delete commands;
    delete io;
  
    // make sure only to delete the initial waypoints list if it acually
    // still exists. 
    if (initial_waypoints)
        delete initial_waypoints;
}


// set the fg_root path
void FGGlobals::set_fg_root (const string &root) {
    fg_root = root;
    
    // append /data to root if it exists
    SGPath tmp( fg_root );
    tmp.append( "data" );
    tmp.append( "version" );
    if ( ulFileExists( tmp.c_str() ) ) {
        fg_root += "/data";
        }
}

void FGGlobals::set_fg_scenery (const string &scenery) {

    if (scenery.empty())
        return;

    SGPath pt( scenery ), po( scenery );
    pt.append("Terrain");
    po.append("Objects");

cout << "pt: " << pt.str() << endl;
cout << "po: " << po.str() << endl;
    ulDir *td = ulOpenDir(pt.c_str());
    ulDir *od = ulOpenDir(po.c_str());

    if (td == NULL) {
        if (od == NULL) {
            fg_scenery = scenery;
        } else {
            fg_scenery = po.str();
            ulCloseDir(od);
        }
    } else {
        if (od != NULL) {
            pt.add(po.str());
            ulCloseDir(od);
        }
        fg_scenery = pt.str();
        ulCloseDir(td);
    }
cout << "fg_scenery: " << fg_scenery << endl;
}


SGSubsystemMgr *
FGGlobals::get_subsystem_mgr () const
{
    return subsystem_mgr;
}

SGSubsystem *
FGGlobals::get_subsystem (const char * name)
{
    return subsystem_mgr->get_subsystem(name);
}

void
FGGlobals::add_subsystem (const char * name,
                          SGSubsystem * subsystem,
                          SGSubsystemMgr::GroupType type,
                          double min_time_sec)
{
    subsystem_mgr->add(name, subsystem, type, min_time_sec);
}


SGEventMgr *
FGGlobals::get_event_mgr () const
{
    return event_mgr;
}


// Save the current state as the initial state.
void
FGGlobals::saveInitialState ()
{
  delete initial_state;
  initial_state = new SGPropertyNode();

  if (!copyProperties(props, initial_state))
    SG_LOG(SG_GENERAL, SG_ALERT, "Error saving initial state");
}


// Restore the saved initial state, if any
void
FGGlobals::restoreInitialState ()
{
    if ( initial_state == 0 ) {
        SG_LOG(SG_GENERAL, SG_ALERT,
               "No initial state available to restore!!!");
        return;
    }

    SGPropertyNode *currentPresets = new SGPropertyNode;
    SGPropertyNode *targetNode = fgGetNode( "/sim/presets" );

    // stash the /sim/presets tree
    if ( !copyProperties(targetNode, currentPresets) ) {
        SG_LOG( SG_GENERAL, SG_ALERT, "Failed to save /sim/presets subtree" );
    }
    
    if ( copyProperties(initial_state, props) ) {
        SG_LOG( SG_GENERAL, SG_INFO, "Initial state restored successfully" );
    } else {
        SG_LOG( SG_GENERAL, SG_INFO,
                "Some errors restoring initial state (read-only props?)" );
    }

    // recover the /sim/presets tree
    if ( !copyProperties(currentPresets, targetNode) ) {
        SG_LOG( SG_GENERAL, SG_ALERT,
                "Failed to restore /sim/presets subtree" );
    }

   delete currentPresets;
}

FGViewer *
FGGlobals::get_current_view () const
{
  return viewmgr->get_current_view();
}

// end of globals.cxx
