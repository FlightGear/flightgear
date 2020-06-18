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

#include <config.h>

#include <algorithm>

#include <osgViewer/Viewer>
#include <osgDB/Registry>

#include <simgear/structure/commands.hxx>
#include <simgear/structure/exception.hxx>
#include <simgear/misc/sg_path.hxx>
#include <simgear/misc/sg_dir.hxx>
#include <simgear/timing/sg_time.hxx>
#include <simgear/ephemeris/ephemeris.hxx>
#include <simgear/structure/subsystem_mgr.hxx>
#include <simgear/structure/event_mgr.hxx>

#include <simgear/misc/ResourceManager.hxx>
#include <simgear/props/propertyObject.hxx>
#include <simgear/props/props_io.hxx>
#include <simgear/props/AtomicChangeListener.hxx>
#include <simgear/scene/model/modellib.hxx>
#include <simgear/package/Root.hxx>

#include <Add-ons/AddonResourceProvider.hxx>
#include <Aircraft/controls.hxx>
#include <Airports/runways.hxx>
#include <Autopilot/route_mgr.hxx>
#include <Navaids/navlist.hxx>

#include <GUI/gui.h>
#include <Viewer/viewmgr.hxx>

#include <Scenery/scenery.hxx>
#include <Scenery/tilemgr.hxx>
#include <Viewer/renderer.hxx>
#include <GUI/FGFontCache.hxx>
#include <GUI/MessageBox.hxx>

#include <simgear/sound/soundmgr.hxx>
#include <simgear/scene/material/matlib.hxx>

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
        } else {
          // Stop here, otherwise we could end up returning a resource that
          // belongs to an unrelated version of the same aircraft (from a
          // different aircraft directory).
          return SGPath();
        }
    }

  // try each aircraft dir in turn
    std::string res(aResource, 9); // resource path with 'Aircraft/' removed
    const PathList& dirs(globals->get_aircraft_paths());
    PathList::const_iterator it = dirs.begin();
    for (; it != dirs.end(); ++it) {
        SGPath p(*it);
        p.append(res);
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
      SGPath p = SGPath::fromUtf8(fgGetString("/sim/aircraft-dir"));
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
    commands( SGCommandMgr::instance() ),
    channel_options_list( NULL ),
    initial_waypoints( NULL ),
    channellist( NULL ),
    haveUserSettings(false)
{
    SGPropertyNode* root = new SGPropertyNode;
    props = SGPropertyNode_ptr(root);
    locale = new FGLocale(props);

    auto resMgr = simgear::ResourceManager::instance();
    resMgr->addProvider(new AircraftResourceProvider());
    resMgr->addProvider(new CurrentAircraftDirProvider());
    resMgr->addProvider(new flightgear::addons::ResourceProvider());
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

    // stop OSG threading first, to avoid thread races while we tear down
    // scene-graph pieces
    // there are some scenarios where renderer is already gone.
    osg::ref_ptr<osgViewer::Viewer> vw;
    if (renderer) {
        vw = renderer->getViewer();
        if (vw) {
            // https://code.google.com/p/flightgear-bugs/issues/detail?id=1291
            // explicitly stop trheading before we delete the renderer or
            // viewMgr (which ultimately holds refs to the CameraGroup, and
            // GraphicsContext)
            vw->stopThreading();
        }
    }

    subsystem_mgr->shutdown();
    subsystem_mgr->unbind();

    // don't cancel the pager until after shutdown, since AIModels (and
    // potentially others) can queue delete requests on the pager.
    if (vw && vw->getDatabasePager()) {
        vw->getDatabasePager()->cancel();
        vw->getDatabasePager()->clear();
    }

    osgDB::Registry::instance()->clearObjectCache();
    if (subsystem_mgr->get_subsystem(FGScenery::staticSubsystemClassId())) {
        subsystem_mgr->remove(FGScenery::staticSubsystemClassId());
    }

    // renderer touches subsystems during its destruction
    set_renderer(nullptr);

    FGFontCache::shutdown();
    fgCancelSnapShot();

    delete subsystem_mgr;
    subsystem_mgr = nullptr; // important so ::get_subsystem returns NULL
    vw = nullptr;
    set_matlib(NULL);

    delete time_params;
    delete channel_options_list;
    delete initial_waypoints;
    delete channellist;

    // delete commands before we release the property root
    // this avoids crash where commands might be storing a propery
    // ref/pointer.
    // see https://sentry.io/organizations/flightgear/issues/1890563449
    delete commands;
    commands = nullptr;
    
    simgear::PropertyObjectBase::setDefaultRoot(NULL);
    simgear::SGModelLib::resetPropertyRoot();
    delete locale;
    locale = NULL;

    cleanupListeners();

    props.clear();

    delete simgear::ResourceManager::instance();
}

// set the fg_root path
void FGGlobals::set_fg_root (const SGPath &root) {
    SGPath tmp(root);
    fg_root = tmp.realpath();

    // append /data to root if it exists
    tmp.append( "data" );
    tmp.append( "version" );
    if ( tmp.exists() ) {
        fgGetNode("BAD_FG_ROOT", true)->setStringValue(fg_root.utf8Str());
        fg_root.append("data");
        fgGetNode("GOOD_FG_ROOT", true)->setStringValue(fg_root.utf8Str());
        SG_LOG(SG_GENERAL, SG_ALERT, "***\n***\n*** Warning: changing bad FG_ROOT/--fg-root to '"
                << fg_root << "'\n***\n***");
    }

    // deliberately not a tied property, for fgValidatePath security
    // write-protect to avoid accidents
    SGPropertyNode *n = fgGetNode("/sim", true);
    n->removeChild("fg-root", 0);
    n = n->getChild("fg-root", 0, true);
    n->setStringValue(fg_root.utf8Str());
    n->setAttribute(SGPropertyNode::WRITE, false);

    simgear::ResourceManager::instance()->addBasePath(fg_root,
      simgear::ResourceManager::PRIORITY_DEFAULT);
}

// set the fg_home path
void FGGlobals::set_fg_home (const SGPath &home)
{
    fg_home = home.realpath();
}

void FGGlobals::set_texture_cache_dir(const SGPath &textureCache)
{
    texture_cache_dir = textureCache.realpath();
}


PathList FGGlobals::get_data_paths() const
{
    PathList r(additional_data_paths);
    r.push_back(fg_root);
    r.insert(r.end(), _dataPathsAfterFGRoot.begin(), _dataPathsAfterFGRoot.end());
    return r;
}

PathList FGGlobals::get_data_paths(const std::string& suffix) const
{
    PathList r;
    for (SGPath p : get_data_paths()) {
        p.append(suffix);
        if (p.exists()) {
            r.push_back(p);
        }
    }

    return r;
}

void FGGlobals::append_data_path(const SGPath& path, bool afterFGRoot)
{
    if (!path.exists()) {
        SG_LOG(SG_GENERAL, SG_WARN, "adding non-existant data path:" << path);
    }

    using RM = simgear::ResourceManager;
    auto resManager = RM::instance();
    if (afterFGRoot) {
        _dataPathsAfterFGRoot.push_back(path);
        // after FG_ROOT
        resManager->addBasePath(path, static_cast<RM::Priority>(RM::PRIORITY_DEFAULT - 10));
    } else {
        additional_data_paths.push_back(path);
        // after NORMAL prioirty, but ahead of FG_ROOT
        resManager->addBasePath(path, static_cast<RM::Priority>(RM::PRIORITY_DEFAULT + 10));
    }
}

SGPath FGGlobals::findDataPath(const std::string& pathSuffix) const
{
    for (SGPath p : additional_data_paths) {
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

    for (SGPath p : _dataPathsAfterFGRoot) {
        p.append(pathSuffix);
        if (p.exists()) {
            return p;
        }
    }

    SG_LOG(SG_IO, SG_WARN, "not found in any data path: '" << pathSuffix << "'");
    return SGPath{};
}

void FGGlobals::append_fg_scenery (const PathList &paths)
{
    for (const SGPath& path : paths) {
        append_fg_scenery(path);
    }
}

void FGGlobals::append_fg_scenery (const SGPath &path)
{
    SGPropertyNode* sim = fgGetNode("/sim", true);

    // find first unused fg-scenery property in /sim
    int propIndex = 0;
    while (sim->getChild("fg-scenery", propIndex) != NULL) {
        ++propIndex;
    }

    SGPath abspath(path.realpath());
    if (!abspath.exists()) {
        SG_LOG(SG_GENERAL, SG_WARN, "scenery path not found:" << abspath);
        return;
    }

    // check for duplicates
    PathList::const_iterator ex = std::find(fg_scenery.begin(), fg_scenery.end(), abspath);
    if (ex != fg_scenery.end()) {
        SG_LOG(SG_GENERAL, SG_INFO, "skipping duplicate add of scenery path:" << abspath);
        return;
    }

    // tell the ResouceManager about the scenery path
    // needed to load Models from this scenery path
    simgear::ResourceManager::instance()->addBasePath(abspath, simgear::ResourceManager::PRIORITY_DEFAULT);

    fg_scenery.push_back(abspath);
    extra_read_allowed_paths.push_back(abspath);

    // make scenery dirs available to Nasal
    SGPropertyNode* n = sim->getChild("fg-scenery", propIndex++, true);
    n->setStringValue(abspath.utf8Str());
    n->setAttribute(SGPropertyNode::WRITE, false);

    // temporary fix so these values survive reset
    n->setAttribute(SGPropertyNode::PRESERVE, true);
}

void FGGlobals::append_read_allowed_paths(const SGPath &path)
{
    SGPath abspath(path.realpath());
    if (!abspath.exists()) {
        SG_LOG(SG_IO, SG_DEBUG, "read-allowed path not found:" << abspath);
        return;
    }
    extra_read_allowed_paths.push_back(abspath);
}

void FGGlobals::clear_fg_scenery()
{
  fg_scenery.clear();
  fgGetNode("/sim", true)->removeChildren("fg-scenery");
}

// The 'path' argument to this method must come from trustworthy code, because
// the method grants read permissions to Nasal code for many files beneath
// 'path'. In particular, don't call this method with a 'path' value taken
// from the property tree or any other Nasal-writable place.
void FGGlobals::set_download_dir(const SGPath& path)
{
  SGPath abspath(path.realpath());
  download_dir = abspath;

  append_read_allowed_paths(abspath / "Aircraft");
  append_read_allowed_paths(abspath / "AI");
  append_read_allowed_paths(abspath / "Liveries");
  // If in use, abspath / TerraSync will be added to 'extra_read_allowed_paths'
  // by FGGlobals::append_fg_scenery(), as any scenery path.

  SGPropertyNode *n = fgGetNode("/sim/paths/download-dir", true);
  n->setAttribute(SGPropertyNode::WRITE, true);
  n->setStringValue(abspath.utf8Str());
  n->setAttribute(SGPropertyNode::WRITE, false);
}

// The 'path' argument to this method must come from trustworthy code, because
// the method grants read permissions to Nasal code for all files beneath
// 'path'. In particular, don't call this method with a 'path' value taken
// from the property tree or any other Nasal-writable place.
void FGGlobals::set_terrasync_dir(const SGPath &path)
{
  if (terrasync_dir.realpath() != SGPath(fgGetString("/sim/terrasync/scenery-dir")).realpath()) {
    // if they don't match, /sim/terrasync/scenery-dir has been set by something else
    SG_LOG(SG_GENERAL, SG_WARN, "/sim/terrasync/scenery-dir is no longer stored across runs: if you wish to keep using a non-standard Terrasync directory, use --terrasync-dir or the launcher's settings");
  }
  SGPath abspath(path.realpath());
  terrasync_dir = abspath;
  // deliberately not a tied property, for fgValidatePath security
  // write-protect to avoid accidents
  SGPropertyNode *n = fgGetNode("/sim/terrasync/scenery-dir", true);
  n->setAttribute(SGPropertyNode::WRITE, true);
  n->setStringValue(abspath.utf8Str());
  n->setAttribute(SGPropertyNode::WRITE, false);
  // don't add it to fg_scenery yet, as we want it ordered after explicit --fg-scenery
}



void FGGlobals::set_catalog_aircraft_path(const SGPath& path)
{
    catalog_aircraft_dir = path;
}

PathList FGGlobals::get_aircraft_paths() const
{
    PathList r;
    if (!catalog_aircraft_dir.isNull()) {
        r.push_back(catalog_aircraft_dir);
    }

    r.insert(r.end(), fg_aircraft_dirs.begin(), fg_aircraft_dirs.end());
    return r;
}

void FGGlobals::append_aircraft_path(const SGPath& path)
{
  SGPath dirPath(path);
  if (!dirPath.exists()) {
    SG_LOG(SG_GENERAL, SG_WARN, "aircraft path not found:" << path);
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

  fg_aircraft_dirs.push_back(dirPath.realpath());
  extra_read_allowed_paths.push_back(dirPath.realpath());
}

void FGGlobals::append_aircraft_paths(const PathList& paths)
{
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
FGGlobals::get_subsystem (const char * name) const
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
    n->setStringValue(fg_root.utf8Str());
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

SGPath FGGlobals::autosaveFilePath(SGPath userDataPath) const
{
    if (userDataPath.isNull()) {
        userDataPath = get_fg_home();
    }

    return simgear::Dir(userDataPath).file(autosaveName());
}

static void deleteProperties(SGPropertyNode* props, const string_list& blacklist)
{
    const std::string path(props->getPath());
    auto it = std::find_if(blacklist.begin(), blacklist.end(), [path](const std::string& black)
                           { return simgear::strutils::matchPropPathToTemplate(path, black); });
    if (it != blacklist.end()) {
        SGPropertyNode* pr = props->getParent();
        pr->removeChild(props);
        return;
    }

    // recurse
    for (int c=0; c < props->nChildren(); ++c) {
        deleteProperties(props->getChild(c), blacklist);
    }

}

using VersionPair = std::pair<int, int>;

static bool operator<(const VersionPair& a, const VersionPair& b)
{
    if (a.first == b.first) {
        return a.second < b.second;
    }

    return a.first < b.first;
}

static void tryAutosaveMigration(const SGPath& userDataPath, SGPropertyNode* props)
{
    const string_list versionParts = simgear::strutils::split(VERSION, ".");
    if (versionParts.size() < 2) {
        return;
    }

    simgear::Dir userDataDir(userDataPath);
    SGPath migratePath;
    VersionPair foundVersion(0, 0);
    const VersionPair currentVersion(simgear::strutils::to_int(versionParts[0]),
                                     simgear::strutils::to_int(versionParts[1]));

    for (auto previousSave : userDataDir.children(simgear::Dir::TYPE_FILE, ".xml")) {
        const std::string base = previousSave.file_base();
        VersionPair v;
        // extract version from name
        const int matches = ::sscanf(base.c_str(), "autosave_%d_%d", &v.first, &v.second);
        if (matches != 2) {
            continue;
        }

        if (currentVersion < v) {
            // ignore autosaves from more recent versions; this happens when
            // running unsable and stable at the same time
            continue;
        }

        if (v.first < 2000) {
            // ignore autosaves from older versions, too much change to deal
            // with.
            continue;
        }

        if (foundVersion < v) {
            foundVersion = v;
            migratePath = previousSave;
        }
    }

    if (!migratePath.exists()) {
        return;
    }

    SG_LOG(SG_GENERAL, SG_INFO, "Migrating old autosave:" << migratePath);
    SGPropertyNode oldProps;

    try {
        readProperties(migratePath, &oldProps, SGPropertyNode::USERARCHIVE);
    } catch (sg_exception& e) {
        SG_LOG(SG_GENERAL, SG_WARN, "failed to read previous user settings:" << e.getMessage()
               << "(from " << e.getOrigin() << ")");
        return;
    }

    // read migration blacklist
    string_list blacklist;
    SGPropertyNode_ptr blacklistNode = fgGetNode("/sim/autosave-migration/blacklist", true);
    for (auto node : blacklistNode->getChildren("path")) {
        blacklist.push_back(node->getStringValue());
    }

    // apply migration filters for each property in turn
    deleteProperties(&oldProps, blacklist);

    // manual migrations
    // migrate pre-2019.1 sense of /sim/mouse/invert-mouse-wheel
    if (foundVersion.first < 2019) {
        SGPropertyNode_ptr wheelNode = oldProps.getNode("/sim/mouse/invert-mouse-wheel");
        if (wheelNode) {
            wheelNode->setBoolValue(!wheelNode->getBoolValue());
        }
    }
    // copy remaining props out
    copyProperties(&oldProps, props);

    // we can't inform the user yet, becuase embedded resources and the locale
    // are not done. So we set a flag and check it once those things are done.
    fgSetBool("/sim/autosave-migration/did-migrate", true);
}

// Load user settings from the autosave file (normally in $FG_HOME)
void
FGGlobals::loadUserSettings(SGPath userDataPath)
{
    if (userDataPath.isNull()) {
        userDataPath = get_fg_home();
    }

    // Remember that we have (tried) to load any existing autosave file
    haveUserSettings = true;

    SGPath autosaveFile = autosaveFilePath(userDataPath);
    SGPropertyNode autosave;
    if (autosaveFile.exists()) {
      SG_LOG(SG_INPUT, SG_INFO,
             "Reading user settings from " << autosaveFile);
      try {
          readProperties(autosaveFile, &autosave, SGPropertyNode::USERARCHIVE);
      } catch (sg_exception& e) {
          SG_LOG(SG_INPUT, SG_WARN, "failed to read user settings:" << e.getMessage()
            << "(from " << e.getOrigin() << ")");
      }
    } else {
        tryAutosaveMigration(userDataPath, &autosave);
    }
    /* Before 2020-03-10, we could save portions of the /ai/models/ tree, which
    confuses things when loaded again. So delete any such items if they have
    been loaded. */
    SGPropertyNode* ai = autosave.getNode("ai");
    if (ai) {
        ai->removeChildren("models");
    }
    copyProperties(&autosave, globals->get_props());
}

// Save user settings to the autosave file.
//
// When calling this method, make sure the value of 'userDataPath' is
// trustworthy. In particular, make sure it can't be influenced by Nasal code,
// not even indirectly via a Nasal-writable place such as the property tree.
//
// Note: the default value, which causes the autosave file to be written to
//       $FG_HOME, is safe---if not, it would be a bug.
void
FGGlobals::saveUserSettings(SGPath userDataPath)
{
    if (userDataPath.isNull()) userDataPath = get_fg_home();

    // only save settings when we have (tried) to load the previous
    // settings (otherwise user data was lost)
    if (!haveUserSettings)
        return;

    if (fgGetBool("/sim/startup/save-on-exit")) {
      // don't save settings more than once on shutdown
      haveUserSettings = false;

      SGPath autosaveFile = autosaveFilePath(userDataPath);
      autosaveFile.create_dir( 0700 );
      SG_LOG(SG_IO, SG_INFO, "Saving user settings to " << autosaveFile);
      try {
        writeProperties(autosaveFile, globals->get_props(), false, SGPropertyNode::USERARCHIVE);
      } catch (const sg_exception &e) {
        guiErrorMessage("Error writing autosave:", e);
      }
      SG_LOG(SG_INPUT, SG_DEBUG, "Finished Saving user settings");
    }
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
    return get_subsystem<FGScenery>();
}

FGViewMgr *FGGlobals::get_viewmgr() const
{
    return get_subsystem<FGViewMgr>();
}

flightgear::View* FGGlobals::get_current_view () const
{
    FGViewMgr* vm = get_viewmgr();
    return vm ? vm->get_current_view() : 0;
}

void FGGlobals::set_matlib( SGMaterialLib *m )
{
    matlib = m;
}

FGControls *FGGlobals::get_controls() const
{
    return get_subsystem<FGControls>();
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

    simgear::AtomicChangeListener::clearPendingChanges();
}

simgear::pkg::Root* FGGlobals::packageRoot()
{
  return _packageRoot.get();
}

void FGGlobals::setPackageRoot(const SGSharedPtr<simgear::pkg::Root>& p)
{
  _packageRoot = p;
}

bool FGGlobals::is_headless()
{
    return flightgear::isHeadlessMode();
}

void FGGlobals::set_headless(bool mode)
{
    flightgear::setHeadlessMode(mode);
}

// end of globals.cxx
