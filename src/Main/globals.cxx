// globals.cxx -- Global state that needs to be shared among the sim modules
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
// along with this program; if not, write to the Free Software Foundation,
// Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//
// $Id$

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <boost/foreach.hpp>
#include <algorithm>

#include <simgear/structure/commands.hxx>
#include <simgear/misc/sg_path.hxx>
#include <simgear/misc/sg_dir.hxx>
#include <simgear/timing/sg_time.hxx>
#include <simgear/ephemeris/ephemeris.hxx>
#include <simgear/magvar/magvar.hxx>
#include <simgear/scene/material/matlib.hxx>
#include <simgear/structure/subsystem_mgr.hxx>
#include <simgear/structure/event_mgr.hxx>
#include <simgear/sound/soundmgr_openal.hxx>
#include <simgear/misc/ResourceManager.hxx>
#include <simgear/props/propertyObject.hxx>
#include <simgear/props/props_io.hxx>
#include <simgear/scene/model/placement.hxx>

#include <Aircraft/controls.hxx>
#include <Airports/runways.hxx>
#include <ATCDCL/ATISmgr.hxx>
#include <Autopilot/route_mgr.hxx>
#include <GUI/FGFontCache.hxx>
#include <GUI/gui.h>
#include <Model/acmodel.hxx>
#include <Model/modelmgr.hxx>
#include <MultiPlayer/multiplaymgr.hxx>
#include <Scenery/scenery.hxx>
#include <Scenery/tilemgr.hxx>
#include <Navaids/navlist.hxx>
#include <Viewer/renderer.hxx>
#include <Viewer/viewmgr.hxx>

#include "globals.hxx"
#include "locale.hxx"

#include "fg_props.hxx"
#include "fg_io.hxx"

class AircraftResourceProvider : public simgear::ResourceProvider
{
public:
  AircraftResourceProvider() :
    simgear::ResourceProvider(simgear::ResourceManager::PRIORITY_HIGH)
  {
  }
  
  virtual SGPath resolve(const std::string& aResource, SGPath&) const
  {
    string_list pieces(sgPathBranchSplit(aResource));
    if ((pieces.size() < 3) || (pieces.front() != "Aircraft")) {
      return SGPath(); // not an Aircraft path
    }
    
  // test against the aircraft-dir property
    const char* aircraftDir = fgGetString("/sim/aircraft-dir");
    string_list aircraftDirPieces(sgPathBranchSplit(aircraftDir));
    if (!aircraftDirPieces.empty() && (aircraftDirPieces.back() == pieces[1])) {
        // current aircraft-dir matches resource aircraft
        SGPath r(aircraftDir);
        for (unsigned int i=2; i<pieces.size(); ++i) {
          r.append(pieces[i]);
        }
        
        if (r.exists()) {
          SG_LOG(SG_IO, SG_DEBUG, "found path:" << aResource << " via /sim/aircraft-dir: " << r.str());
          return r;
        }
    }
  
  // try each aircraft dir in turn
    std::string res(aResource, 9); // resource path with 'Aircraft/' removed
    const string_list& dirs(globals->get_aircraft_paths());
    string_list::const_iterator it = dirs.begin();
    for (; it != dirs.end(); ++it) {
      SGPath p(*it, res);
      if (p.exists()) {
        SG_LOG(SG_IO, SG_DEBUG, "found path:" << aResource << " in aircraft dir: " << *it);
        return p;
      }
    } // of aircraft path iteration
    
    return SGPath(); // not found
  }
};

////////////////////////////////////////////////////////////////////////
// Implementation of FGGlobals.
////////////////////////////////////////////////////////////////////////

// global global :-)
FGGlobals *globals;


// Constructor
FGGlobals::FGGlobals() :
    props( new SGPropertyNode ),
    initial_state( NULL ),
    locale( new FGLocale(props) ),
    renderer( new FGRenderer ),
    subsystem_mgr( new SGSubsystemMgr ),
    event_mgr( new SGEventMgr ),
    sim_time_sec( 0.0 ),
    fg_root( "" ),
    time_params( NULL ),
    ephem( NULL ),
    mag( NULL ),
    matlib( NULL ),
    route_mgr( NULL ),
    ATIS_mgr( NULL ),
    controls( NULL ),
    viewmgr( NULL ),
    commands( SGCommandMgr::instance() ),
    acmodel( NULL ),
    model_mgr( NULL ),
    channel_options_list( NULL ),
    initial_waypoints( NULL ),
    scenery( NULL ),
    tile_mgr( NULL ),
    fontcache ( new FGFontCache ),
    navlist( NULL ),
    loclist( NULL ),
    gslist( NULL ),
    dmelist( NULL ),
    tacanlist( NULL ),
    carrierlist( NULL ),
    channellist( NULL ),
    haveUserSettings(false)
{
  simgear::ResourceManager::instance()->addProvider(new AircraftResourceProvider());
  simgear::PropertyObjectBase::setDefaultRoot(props);
}

// Destructor
FGGlobals::~FGGlobals() 
{
    // save user settings (unless already saved)
    saveUserSettings();

    // The AIModels manager performs a number of actions upon
    // Shutdown that implicitly assume that other subsystems
    // are still operational (Due to the dynamic allocation and
    // deallocation of AIModel objects. To ensure we can safely
    // shut down all subsystems, make sure we take down the 
    // AIModels system first.
    SGSubsystem* ai = subsystem_mgr->remove("ai-model");
    if (ai) {
        ai->unbind();
        delete ai;
    }
    SGSubsystem* sound = subsystem_mgr->remove("sound");

    subsystem_mgr->shutdown();
    subsystem_mgr->unbind();
    delete subsystem_mgr;
    
    delete renderer;
    renderer = NULL;
    
    delete time_params;
    delete mag;
    delete matlib;
    delete route_mgr;

    delete ATIS_mgr;

    if (controls)
    {
        controls->unbind();
        delete controls;
    }

    delete channel_options_list;
    delete initial_waypoints;
    delete scenery;
    delete fontcache;

    delete navlist;
    delete loclist;
    delete gslist;
    delete dmelist;
    delete tacanlist;
    delete carrierlist;
    delete channellist;
    delete sound;

    delete locale;
    locale = NULL;
}


// set the fg_root path
void FGGlobals::set_fg_root (const string &root) {
    SGPath tmp(root);
    fg_root = tmp.realpath();

    // append /data to root if it exists
    tmp.append( "data" );
    tmp.append( "version" );
    if ( tmp.exists() ) {
        fgGetNode("BAD_FG_ROOT", true)->setStringValue(fg_root);
        fg_root += "/data";
        fgGetNode("GOOD_FG_ROOT", true)->setStringValue(fg_root);
        SG_LOG(SG_GENERAL, SG_ALERT, "***\n***\n*** Warning: changing bad FG_ROOT/--fg-root to '"
                << fg_root << "'\n***\n***");
    }

    // remove /sim/fg-root before writing to prevent hijacking
    SGPropertyNode *n = fgGetNode("/sim", true);
    n->removeChild("fg-root", 0, false);
    n = n->getChild("fg-root", 0, true);
    n->setStringValue(fg_root.c_str());
    n->setAttribute(SGPropertyNode::WRITE, false);
    
    simgear::ResourceManager::instance()->addBasePath(fg_root, 
      simgear::ResourceManager::PRIORITY_DEFAULT);
}

void FGGlobals::append_fg_scenery (const string &paths)
{
//    fg_scenery.clear();
    SGPropertyNode* sim = fgGetNode("/sim", true);

  // find first unused fg-scenery property in /sim
    int propIndex = 0;
    while (sim->getChild("fg-scenery", propIndex) != NULL) {
      ++propIndex; 
    }
  
    BOOST_FOREACH(const SGPath& path, sgPathSplit( paths )) {
        SGPath abspath(path.realpath());
        if (!abspath.exists()) {
          SG_LOG(SG_GENERAL, SG_WARN, "scenery path not found:" << abspath.str());
          continue;
        }

      // check for duplicates
      string_list::const_iterator ex = std::find(fg_scenery.begin(), fg_scenery.end(), abspath.str());
      if (ex != fg_scenery.end()) {
        SG_LOG(SG_GENERAL, SG_INFO, "skipping duplicate add of scenery path:" << abspath.str());
        continue;
      }
      
        simgear::Dir dir(abspath);
        SGPath terrainDir(dir.file("Terrain"));
        SGPath objectsDir(dir.file("Objects"));
        
      // this code used to add *either* the base dir, OR add the 
      // Terrain and Objects subdirs, but the conditional logic was commented
      // out, such that all three dirs are added. Unfortunately there's
      // no information as to why the change was made.
        fg_scenery.push_back(abspath.str());
        
        if (terrainDir.exists()) {
          fg_scenery.push_back(terrainDir.str());
        }
        
        if (objectsDir.exists()) {
          fg_scenery.push_back(objectsDir.str());
        }
        
        // insert a marker for FGTileEntry::load(), so that
        // FG_SCENERY=A:B becomes list ["A/Terrain", "A/Objects", "",
        // "B/Terrain", "B/Objects", ""]
        fg_scenery.push_back("");
        
      // make scenery dirs available to Nasal
        SGPropertyNode* n = sim->getChild("fg-scenery", propIndex++, true);
        n->setStringValue(abspath.str());
        n->setAttribute(SGPropertyNode::WRITE, false);
    } // of path list iteration
}

void FGGlobals::append_aircraft_path(const std::string& path)
{
  SGPath dirPath(path);
  if (!dirPath.exists()) {
    SG_LOG(SG_GENERAL, SG_WARN, "aircraft path not found:" << path);
    return;
  }
  std::string abspath = dirPath.realpath();
  
  unsigned int index = fg_aircraft_dirs.size();  
  fg_aircraft_dirs.push_back(abspath);
  
// make aircraft dirs available to Nasal
  SGPropertyNode* sim = fgGetNode("/sim", true);
  sim->removeChild("fg-aircraft", index, false);
  SGPropertyNode* n = sim->getChild("fg-aircraft", index, true);
  n->setStringValue(abspath);
  n->setAttribute(SGPropertyNode::WRITE, false);
}

void FGGlobals::append_aircraft_paths(const std::string& path)
{
  string_list paths = sgPathSplit(path);
  for (unsigned int p = 0; p<paths.size(); ++p) {
    append_aircraft_path(paths[p]);
  }
}

SGPath FGGlobals::resolve_aircraft_path(const std::string& branch) const
{
  return simgear::ResourceManager::instance()->findPath(branch);
}

SGPath FGGlobals::resolve_maybe_aircraft_path(const std::string& branch) const
{
  return simgear::ResourceManager::instance()->findPath(branch);
}

SGPath FGGlobals::resolve_ressource_path(const std::string& branch) const
{
  return simgear::ResourceManager::instance()
    ->findPath(branch, SGPath(fgGetString("/sim/aircraft-dir")));
}

FGRenderer *
FGGlobals::get_renderer () const
{
   return renderer;
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

SGSoundMgr *
FGGlobals::get_soundmgr () const
{
    if (subsystem_mgr)
        return (SGSoundMgr*) subsystem_mgr->get_subsystem("sound");

    return NULL;
}

SGEventMgr *
FGGlobals::get_event_mgr () const
{
    return event_mgr;
}

const SGGeod &
FGGlobals::get_aircraft_position() const
{
    if( acmodel != NULL ) {
        SGModelPlacement * mp = acmodel->get3DModel();
        if( mp != NULL )
            return mp->getPosition();
    }
    throw sg_exception("Can't get aircraft position", "FGGlobals::get_aircraft_position()" );
}

SGVec3d
FGGlobals::get_aircraft_positon_cart() const
{
    return SGVec3d::fromGeod(get_aircraft_position());
}


// Save the current state as the initial state.
void
FGGlobals::saveInitialState ()
{
  initial_state = new SGPropertyNode();

  // copy properties which are READ/WRITEable - but not USERARCHIVEd or PRESERVEd
  int checked  = SGPropertyNode::READ+SGPropertyNode::WRITE+
                 SGPropertyNode::USERARCHIVE+SGPropertyNode::PRESERVE;
  int expected = SGPropertyNode::READ+SGPropertyNode::WRITE;
  if (!copyProperties(props, initial_state, expected, checked))
    SG_LOG(SG_GENERAL, SG_ALERT, "Error saving initial state");
    
  // delete various properties from the initial state, since we want to
  // preserve their values even if doing a restore
  // => Properties should now use the PRESERVE flag to protect their values
  // on sim-reset. Remove some specific properties for backward compatibility.
  SGPropertyNode* sim = initial_state->getChild("sim");
  SGPropertyNode* cameraGroupNode = sim->getNode("rendering/camera-group");
  if (cameraGroupNode) {
    cameraGroupNode->removeChild("camera");
    cameraGroupNode->removeChild("gui");
  }
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
    // copy properties which are READ/WRITEable - but not USERARCHIVEd or PRESERVEd
    int checked  = SGPropertyNode::READ+SGPropertyNode::WRITE+
                   SGPropertyNode::USERARCHIVE+SGPropertyNode::PRESERVE;
    int expected = SGPropertyNode::READ+SGPropertyNode::WRITE;
    if ( copyProperties(initial_state, props, expected, checked)) {
        SG_LOG( SG_GENERAL, SG_INFO, "Initial state restored successfully" );
    } else {
        SG_LOG( SG_GENERAL, SG_INFO,
                "Some errors restoring initial state (read-only props?)" );
    }

}

// Load user settings from autosave.xml
void
FGGlobals::loadUserSettings(const SGPath& dataPath)
{
    // remember that we have (tried) to load any existing autsave.xml
    haveUserSettings = true;

    SGPath autosaveFile = simgear::Dir(dataPath).file("autosave.xml");
    SGPropertyNode autosave;
    if (autosaveFile.exists()) {
      SG_LOG(SG_INPUT, SG_INFO, "Reading user settings from " << autosaveFile.str());
      try {
          readProperties(autosaveFile.str(), &autosave, SGPropertyNode::USERARCHIVE);
      } catch (sg_exception& e) {
          SG_LOG(SG_INPUT, SG_WARN, "failed to read user settings:" << e.getMessage()
            << "(from " << e.getOrigin() << ")");
      }
    }
    copyProperties(&autosave, globals->get_props());
}

// Save user settings in autosave.xml
void
FGGlobals::saveUserSettings()
{
    // only save settings when we have (tried) to load the previous
    // settings (otherwise user data was lost)
    if (!haveUserSettings)
        return;

    if (fgGetBool("/sim/startup/save-on-exit")) {
      // don't save settings more than once on shutdown
      haveUserSettings = false;

      SGPath autosaveFile(fgGetString("/sim/fg-home"));
      autosaveFile.append( "autosave.xml" );
      autosaveFile.create_dir( 0700 );
      SG_LOG(SG_IO, SG_INFO, "Saving user settings to " << autosaveFile.str());
      try {
        writeProperties(autosaveFile.str(), globals->get_props(), false, SGPropertyNode::USERARCHIVE);
      } catch (const sg_exception &e) {
        guiErrorMessage("Error writing autosave.xml: ", e);
      }
      SG_LOG(SG_INPUT, SG_DEBUG, "Finished Saving user settings");
    }
}

FGViewer *
FGGlobals::get_current_view () const
{
  return viewmgr->get_current_view();
}

long int FGGlobals::get_warp() const
{
  return fgGetInt("/sim/time/warp");
}

void FGGlobals::set_warp( long int w )
{
  fgSetInt("/sim/time/warp", w);
}

long int FGGlobals::get_warp_delta() const
{
  return fgGetInt("/sim/time/warp-delta");
}

void FGGlobals::set_warp_delta( long int d )
{
  fgSetInt("/sim/time/warp-delta", d);
}

// end of globals.cxx
