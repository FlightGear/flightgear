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

#include <osgViewer/Viewer>
#include <osgDB/Registry>

#include <simgear/structure/commands.hxx>
#include <simgear/misc/sg_path.hxx>
#include <simgear/misc/sg_dir.hxx>
#include <simgear/timing/sg_time.hxx>
#include <simgear/ephemeris/ephemeris.hxx>
#include <simgear/scene/material/matlib.hxx>
#include <simgear/structure/subsystem_mgr.hxx>
#include <simgear/structure/event_mgr.hxx>
#include <simgear/sound/soundmgr_openal.hxx>
#include <simgear/misc/ResourceManager.hxx>
#include <simgear/props/propertyObject.hxx>
#include <simgear/props/props_io.hxx>
#include <simgear/scene/model/modellib.hxx>
#include <simgear/package/Root.hxx>

#include <Aircraft/controls.hxx>
#include <Airports/runways.hxx>
#include <Autopilot/route_mgr.hxx>
#include <GUI/FGFontCache.hxx>
#include <GUI/gui.h>
#include <MultiPlayer/multiplaymgr.hxx>
#include <Scenery/scenery.hxx>
#include <Scenery/tilemgr.hxx>
#include <Navaids/navlist.hxx>
#include <Viewer/renderer.hxx>
#include <Viewer/viewmgr.hxx>
#include <Sound/sample_queue.hxx>

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
        return p;
      }
    } // of aircraft path iteration
    
    return SGPath(); // not found
  }
};

class CurrentAircraftDirProvider : public simgear::ResourceProvider
{
public:
  CurrentAircraftDirProvider() :
    simgear::ResourceProvider(simgear::ResourceManager::PRIORITY_HIGH)
  {
  }
  
  virtual SGPath resolve(const std::string& aResource, SGPath&) const
  {
    const char* aircraftDir = fgGetString("/sim/aircraft-dir");
    SGPath p(aircraftDir);
    p.append(aResource);
    return p.exists() ? p : SGPath();
  }
};

////////////////////////////////////////////////////////////////////////
// Implementation of FGGlobals.
////////////////////////////////////////////////////////////////////////

// global global :-)
FGGlobals *globals = NULL;


// Constructor
FGGlobals::FGGlobals() :
    renderer( new FGRenderer ),
    subsystem_mgr( new SGSubsystemMgr ),
    event_mgr( new SGEventMgr ),
    sim_time_sec( 0.0 ),
    fg_root( "" ),
    fg_home( "" ),
    time_params( NULL ),
    ephem( NULL ),
    route_mgr( NULL ),
    controls( NULL ),
    viewmgr( NULL ),
    commands( SGCommandMgr::instance() ),
    channel_options_list( NULL ),
    initial_waypoints( NULL ),
    fontcache ( new FGFontCache ),
    channellist( NULL ),
    haveUserSettings(false),
    _chatter_queue(NULL)
{
    SGPropertyNode* root = new SGPropertyNode;
    props = SGPropertyNode_ptr(root);
    locale = new FGLocale(props);
    
    simgear::ResourceManager::instance()->addProvider(new AircraftResourceProvider);
    simgear::ResourceManager::instance()->addProvider(new CurrentAircraftDirProvider);
    initProperties();
}

void FGGlobals::initProperties()
{
    simgear::PropertyObjectBase::setDefaultRoot(props);
    
    positionLon = props->getNode("position/longitude-deg", true);
    positionLat = props->getNode("position/latitude-deg", true);
    positionAlt = props->getNode("position/altitude-ft", true);
    
    viewLon = props->getNode("sim/current-view/viewer-lon-deg", true);
    viewLat = props->getNode("sim/current-view/viewer-lat-deg", true);
    viewAlt = props->getNode("sim/current-view/viewer-elev-ft", true);
    
    orientPitch = props->getNode("orientation/pitch-deg", true);
    orientHeading = props->getNode("orientation/heading-deg", true);
    orientRoll = props->getNode("orientation/roll-deg", true);

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
    SGSubsystemRef ai = subsystem_mgr->get_subsystem("ai-model");
    if (ai) {
        subsystem_mgr->remove("ai-model");
        ai->unbind();
        ai.clear(); // ensure AI is deleted now, not at end of this method
    }
    
    subsystem_mgr->shutdown();
    subsystem_mgr->unbind();    

    subsystem_mgr->remove("aircraft-model");
    subsystem_mgr->remove("tile-manager");
    subsystem_mgr->remove("model-manager");
    _tile_mgr.clear();

    osg::ref_ptr<osgViewer::Viewer> vw(renderer->getViewer());
    if (vw) {
        // https://code.google.com/p/flightgear-bugs/issues/detail?id=1291
        // explicitly stop trheading before we delete the renderer or
        // viewMgr (which ultimately holds refs to the CameraGroup, and
        // GraphicsContext)
        vw->stopThreading();
    }
    
    // don't cancel the pager until after shutdown, since AIModels (and
    // potentially others) can queue delete requests on the pager.
    if (vw && vw->getDatabasePager()) {
        vw->getDatabasePager()->cancel();
        vw->getDatabasePager()->clear();
    }
    
    osgDB::Registry::instance()->clearObjectCache();
    
    // renderer touches subsystems during its destruction
    set_renderer(NULL);
    _scenery.clear();
    _chatter_queue.clear();
    
    delete subsystem_mgr;
    subsystem_mgr = NULL; // important so ::get_subsystem returns NULL
    vw = 0; // don't delete the viewer until now

    delete time_params;
    set_matlib(NULL);
    delete route_mgr;
    delete channel_options_list;
    delete initial_waypoints;
    delete fontcache;
    delete channellist;

    simgear::PropertyObjectBase::setDefaultRoot(NULL);
    simgear::SGModelLib::resetPropertyRoot();
    
    delete locale;
    locale = NULL;
    
    cleanupListeners();
    
    props.clear();
    
    delete commands;
}

// set the fg_root path
void FGGlobals::set_fg_root (const std::string &root) {
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
    n->removeChild("fg-root", 0);
    n = n->getChild("fg-root", 0, true);
    n->setStringValue(fg_root.c_str());
    n->setAttribute(SGPropertyNode::WRITE, false);
    
    simgear::ResourceManager::instance()->addBasePath(fg_root, 
      simgear::ResourceManager::PRIORITY_DEFAULT);
}

// set the fg_home path
void FGGlobals::set_fg_home (const std::string &home) {
    SGPath tmp(home);
    fg_home = tmp.realpath();
}

PathList FGGlobals::get_data_paths() const
{
    PathList r(additional_data_paths);
    r.push_back(SGPath(fg_root));
    return r;
}

PathList FGGlobals::get_data_paths(const std::string& suffix) const
{
    PathList r;
    BOOST_FOREACH(SGPath p, get_data_paths()) {
        p.append(suffix);
        if (p.exists()) {
            r.push_back(p);
        }
    }

    return r;
}

void FGGlobals::append_data_path(const SGPath& path)
{
    if (!path.exists()) {
        SG_LOG(SG_GENERAL, SG_WARN, "adding non-existant data path:" << path);
    }
    
    additional_data_paths.push_back(path);
}

SGPath FGGlobals::find_data_dir(const std::string& pathSuffix) const
{
    BOOST_FOREACH(SGPath p, additional_data_paths) {
        p.append(pathSuffix);
        if (p.exists()) {
            return p;
        }
    }
    
    SGPath rootPath(fg_root);
    rootPath.append(pathSuffix);
    if (rootPath.exists()) {
        return rootPath;
    }
    
    SG_LOG(SG_GENERAL, SG_WARN, "dir not found in any data path:" << pathSuffix);
    return SGPath();
}

void FGGlobals::append_fg_scenery (const std::string &paths)
{
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
        
        // temporary fix so these values survive reset
        n->setAttribute(SGPropertyNode::PRESERVE, true);
    } // of path list iteration
}

void FGGlobals::clear_fg_scenery()
{
  fg_scenery.clear();
}

void FGGlobals::set_catalog_aircraft_path(const SGPath& path)
{
    catalog_aircraft_dir = path;
}

string_list FGGlobals::get_aircraft_paths() const
{
    string_list r;
    if (!catalog_aircraft_dir.isNull()) {
        r.push_back(catalog_aircraft_dir.str());
    }

    r.insert(r.end(), fg_aircraft_dirs.begin(), fg_aircraft_dirs.end());
    return r;
}

void FGGlobals::append_aircraft_path(const std::string& path)
{
  SGPath dirPath(path);
  if (!dirPath.exists()) {
    SG_LOG(SG_GENERAL, SG_ALERT, "aircraft path not found:" << path);
    return;
  }

  SGPath acSubdir(dirPath);
  acSubdir.append("Aircraft");
  if (acSubdir.exists()) {
      SG_LOG(
        SG_GENERAL,
        SG_WARN,
        "Specified an aircraft-dir with an 'Aircraft' subdirectory:" << dirPath
        << ", will instead use child directory:" << acSubdir
      );
      dirPath = acSubdir;
  }

  std::string abspath = dirPath.realpath();
  fg_aircraft_dirs.push_back(abspath);
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

SGPath FGGlobals::resolve_resource_path(const std::string& branch) const
{
  return simgear::ResourceManager::instance()
    ->findPath(branch, SGPath(fgGetString("/sim/aircraft-dir")));
}

FGRenderer *
FGGlobals::get_renderer () const
{
   return renderer;
}

void FGGlobals::set_renderer(FGRenderer *render)
{
    if (render == renderer) {
        return;
    }
    
    delete renderer;
    renderer = render;
}

SGSubsystemMgr *
FGGlobals::get_subsystem_mgr () const
{
    return subsystem_mgr;
}

SGSubsystem *
FGGlobals::get_subsystem (const char * name)
{
    if (!subsystem_mgr) {
        return NULL;
    }
    
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

SGGeod
FGGlobals::get_aircraft_position() const
{
  return SGGeod::fromDegFt(positionLon->getDoubleValue(),
                           positionLat->getDoubleValue(),
                           positionAlt->getDoubleValue());
}

SGVec3d
FGGlobals::get_aircraft_position_cart() const
{
    return SGVec3d::fromGeod(get_aircraft_position());
}

void FGGlobals::get_aircraft_orientation(double& heading, double& pitch, double& roll)
{
  heading = orientHeading->getDoubleValue();
  pitch = orientPitch->getDoubleValue();
  roll = orientRoll->getDoubleValue();
}

SGGeod
FGGlobals::get_view_position() const
{
  return SGGeod::fromDegFt(viewLon->getDoubleValue(),
                           viewLat->getDoubleValue(),
                           viewAlt->getDoubleValue());
}

SGVec3d
FGGlobals::get_view_position_cart() const
{
  return SGVec3d::fromGeod(get_view_position());
}

static void treeDumpRefCounts(int depth, SGPropertyNode* nd)
{
    for (int i=0; i<nd->nChildren(); ++i) {
        SGPropertyNode* cp = nd->getChild(i);
        if (SGReferenced::count(cp) > 1) {
            SG_LOG(SG_GENERAL, SG_INFO, "\t" << cp->getPath() << " refcount:" << SGReferenced::count(cp));
        }
        
        treeDumpRefCounts(depth + 1, cp);
    }
}

static void treeClearAliases(SGPropertyNode* nd)
{
    if (nd->isAlias()) {
        nd->unalias();
    }
    
    for (int i=0; i<nd->nChildren(); ++i) {
        SGPropertyNode* cp = nd->getChild(i);
        treeClearAliases(cp);
    }
}

void
FGGlobals::resetPropertyRoot()
{
    delete locale;
    
    cleanupListeners();
    
    // we don't strictly need to clear these (they will be reset when we
    // initProperties again), but trying to reduce false-positives when dumping
    // ref-counts.
    positionLon.clear();
    positionLat.clear();
    positionAlt.clear();
    viewLon.clear();
    viewLat.clear();
    viewAlt.clear();
    orientPitch.clear();
    orientHeading.clear();
    orientRoll.clear();
    
    // clear aliases so ref-counts are accurate when dumped
    treeClearAliases(props);
    
    SG_LOG(SG_GENERAL, SG_INFO, "root props refcount:" << props.getNumRefs());
    treeDumpRefCounts(0, props);

    //BaseStackSnapshot::dumpAll(std::cout);
    
    props = new SGPropertyNode;
    initProperties();
    locale = new FGLocale(props);
    
    // remove /sim/fg-root before writing to prevent hijacking
    SGPropertyNode *n = props->getNode("/sim", true);
    n->removeChild("fg-root", 0);
    n = n->getChild("fg-root", 0, true);
    n->setStringValue(fg_root.c_str());
    n->setAttribute(SGPropertyNode::WRITE, false);
}

static std::string autosaveName()
{
    std::ostringstream os;
    string_list versionParts = simgear::strutils::split(VERSION, ".");
    if (versionParts.size() < 2) {
        return "autosave.xml";
    }
    
    os << "autosave_" << versionParts[0] << "_" << versionParts[1] << ".xml";
    return os.str();
}

// Load user settings from autosave.xml
void
FGGlobals::loadUserSettings(const SGPath& dataPath)
{
    // remember that we have (tried) to load any existing autsave.xml
    haveUserSettings = true;

    SGPath autosaveFile = simgear::Dir(dataPath).file(autosaveName());
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

      SGPath autosaveFile(globals->get_fg_home());
      autosaveFile.append(autosaveName());
      autosaveFile.create_dir( 0700 );
      SG_LOG(SG_IO, SG_INFO, "Saving user settings to " << autosaveFile.str());
      try {
        writeProperties(autosaveFile.str(), globals->get_props(), false, SGPropertyNode::USERARCHIVE);
      } catch (const sg_exception &e) {
        guiErrorMessage("Error writing autosave:", e);
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

FGScenery* FGGlobals::get_scenery () const
{
    return _scenery.get();
}

void FGGlobals::set_scenery ( FGScenery *s )
{
    _scenery = s;
}

FGTileMgr* FGGlobals::get_tile_mgr () const
{
    return _tile_mgr.get();
}

void FGGlobals::set_tile_mgr ( FGTileMgr *t )
{
    _tile_mgr = t;
}

void FGGlobals::set_matlib( SGMaterialLib *m )
{
    matlib = m;
}

FGSampleQueue* FGGlobals::get_chatter_queue() const
{
    return _chatter_queue;
}

void FGGlobals::set_chatter_queue(FGSampleQueue* queue)
{
    _chatter_queue = queue;
}

void FGGlobals::addListenerToCleanup(SGPropertyChangeListener* l)
{
    _listeners_to_cleanup.push_back(l);
}

void FGGlobals::cleanupListeners()
{
    SGPropertyChangeListenerVec::iterator i = _listeners_to_cleanup.begin();
    for (; i != _listeners_to_cleanup.end(); ++i) {
        delete *i;
    }
    _listeners_to_cleanup.clear();
}

simgear::pkg::Root* FGGlobals::packageRoot()
{
  return _packageRoot.get();
}

void FGGlobals::setPackageRoot(const SGSharedPtr<simgear::pkg::Root>& p)
{
  _packageRoot = p;
}

// end of globals.cxx
