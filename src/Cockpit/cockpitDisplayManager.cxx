// cockpitDisplayManager.cxx -- manage cockpit displays, typically
// rendered using a sub-camera or render-texture
//
// Copyright (C) 2012 James Turner  zakalawe@mac.com
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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include "cockpitDisplayManager.hxx"

#include <simgear/structure/exception.hxx>
#include <simgear/misc/sg_path.hxx>
#include <simgear/sg_inlines.h>
#include <simgear/props/props_io.hxx>

#include <Main/fg_props.hxx>
#include <Main/globals.hxx>

#include "agradar.hxx"
#include "NavDisplay.hxx"
#include "groundradar.hxx"
#include "wxradar.hxx"

namespace flightgear
{

CockpitDisplayManager::CockpitDisplayManager ()
{
}

CockpitDisplayManager::~CockpitDisplayManager ()
{
}

SGSubsystem::InitStatus CockpitDisplayManager::incrementalInit()
{
  init();
  return INIT_DONE;
}

void CockpitDisplayManager::init()
{
  SGPropertyNode_ptr config_props = new SGPropertyNode;
  SGPropertyNode* path_n = fgGetNode("/sim/instrumentation/path");
  if (!path_n) {
    SG_LOG(SG_COCKPIT, SG_WARN, "No instrumentation model specified for this model!");
    return;
  }

  SGPath config = globals->resolve_aircraft_path(path_n->getStringValue());
  if (!config.exists()) {
    SG_LOG(SG_COCKPIT, SG_DEV_ALERT, "CockpitDisplaysManager: Missing instrumentation file at:" << config);
    return;
  }

  SG_LOG( SG_COCKPIT, SG_INFO, "Reading cockpit displays from " << config );

  try {
    readProperties( config, config_props );
    if (!build(config_props)) {
      throw sg_exception(
                    "Detected an internal inconsistency in the instrumentation\n"
                    "system specification file.  See earlier errors for details.");
    }
  } catch (const sg_exception& e) {
    SG_LOG(SG_COCKPIT, SG_ALERT, "Failed to load instrumentation system model: "
                    << config << ":" << e.getFormattedMessage() );
  }

  SGSubsystemGroup::init();
}

bool CockpitDisplayManager::build (SGPropertyNode* config_props)
{
    for ( int i = 0; i < config_props->nChildren(); ++i ) {
        SGPropertyNode *node = config_props->getChild(i);
        std::string name = node->getName();

        std::ostringstream subsystemname;
        subsystemname << "instrument-" << i << '-'
                << node->getStringValue("name", name.c_str());
        int index = node->getIntValue("number", 0);
        if (index > 0)
            subsystemname << '['<< index << ']';
        std::string id = subsystemname.str();

        if ( name == "radar" ) {
            set_subsystem( id, new wxRadarBg ( node ) );

        } else if ( name == "groundradar" ) {
            set_subsystem( id, new GroundRadar( node ) );

        } else if ( name == "air-ground-radar" ) {
            set_subsystem( id, new agRadar( node ) );

        } else if ( name == "navigation-display" ) {
            set_subsystem( id, new NavDisplay( node ) );

        } else {
            // probably a regular instrument
            continue;
        }
        // only add to our list if we build a display
        _displays.push_back(id);
    }
    return true;
}


// Register the subsystem.
SGSubsystemMgr::Registrant<CockpitDisplayManager> registrantCockpitDisplayManager(
    SGSubsystemMgr::DISPLAY);

} // of namespace flightgear
