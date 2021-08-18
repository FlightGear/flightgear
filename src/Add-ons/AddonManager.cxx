// -*- coding: utf-8 -*-
//
// AddonManager.cxx --- Manager class for FlightGear add-ons
// Copyright (C) 2017  Florent Rougon
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License along
// with this program; if not, write to the Free Software Foundation, Inc.,
// 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

#include "config.h"

#include <algorithm>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <cstdlib>
#include <cassert>

#include <simgear/debug/logstream.hxx>
#include <simgear/misc/sg_path.hxx>
#include <simgear/misc/strutils.hxx>
#include <simgear/props/props.hxx>
#include <simgear/props/props_io.hxx>
#include <simgear/structure/exception.hxx>

#include <Main/fg_props.hxx>
#include <Main/globals.hxx>

#include "addon_fwd.hxx"
#include "Addon.hxx"
#include "AddonManager.hxx"
#include "AddonVersion.hxx"
#include "exceptions.hxx"
#include "pointer_traits.hxx"

namespace strutils = simgear::strutils;

using std::string;
using std::vector;
using std::shared_ptr;
using std::unique_ptr;

namespace flightgear
{

namespace addons
{

static unique_ptr<AddonManager> staticInstance;

// ***************************************************************************
// *                              AddonManager                               *
// ***************************************************************************

// Static method
const unique_ptr<AddonManager>&
AddonManager::createInstance()
{
  SG_LOG(SG_GENERAL, SG_DEBUG, "Initializing the AddonManager");

  staticInstance.reset(new AddonManager());
  return staticInstance;
}

// Static method
const unique_ptr<AddonManager>&
AddonManager::instance()
{
  return staticInstance;
}

// Static method
void
AddonManager::reset()
{
  SG_LOG(SG_GENERAL, SG_DEBUG, "Resetting the AddonManager");
  staticInstance.reset();
}

// Static method
void
AddonManager::loadConfigFileIfExists(const SGPath& configFile)
{
  if (!configFile.exists()) {
    return;
  }

  SGPropertyNode_ptr configProps(new SGPropertyNode);
  try {
    readProperties(configFile, configProps);
  } catch (const sg_exception &e) {
    throw errors::error_loading_config_file(
      "unable to load add-on config file '" + configFile.utf8Str() + "': " +
      e.getFormattedMessage());
  }
  
  // bug https://sourceforge.net/p/flightgear/codetickets/2059/
  // since we're loading this after autosave.xml is loaded, the defaults
  // always take precedence. To fix this, only copy a value from the
  // addon-config if it's not makred as ARCHIVE. (We assume ARCHIVE props
  // came from autosave.xml)
  copyPropertiesIf(configProps, globals->get_props(), [](const SGPropertyNode* src) {
    if (src->nChildren() > 0)
      return true;
    
    // find the correspnding destination node
    auto dstNode = globals->get_props()->getNode(src->getPath());
    if (!dstNode)
      return true; // easy, just copy it
    
    // copy if it's NOT marked archive. In other words, we can replace
    // values from defaults, but not autosave
    return dstNode->getAttribute(SGPropertyNode::USERARCHIVE) == false;
  });

  SG_LOG(SG_GENERAL, SG_INFO,
         "Loaded add-on config file: '" << configFile.utf8Str() + "'");
}

string
AddonManager::registerAddonMetadata(const SGPath& addonPath)
{
  using ptr_traits = shared_ptr_traits<AddonRef>;
  AddonRef addon = ptr_traits::makeStrongRef(Addon::fromAddonDir(addonPath));
  string addonId = addon->getId();

  SGPropertyNode* addonPropNode = fgGetNode("addons", true)
                                 ->getChild("by-id", 0, 1)
                                 ->getChild(addonId, 0, 1);
  addon->setAddonNode(addonPropNode);
  addon->setLoadSequenceNumber(_loadSequenceNumber++);

  // Check that the FlightGear version satisfies the add-on requirements
  std::string minFGversion = addon->getMinFGVersionRequired();
  if (strutils::compare_versions(FLIGHTGEAR_VERSION, minFGversion) < 0) {
    throw errors::fg_version_too_old(
      "add-on '" + addonId + "' requires FlightGear " + minFGversion +
      " or later, however this is FlightGear " + FLIGHTGEAR_VERSION);
  }

  std::string maxFGversion = addon->getMaxFGVersionRequired();
  if (maxFGversion != "none" &&
      strutils::compare_versions(FLIGHTGEAR_VERSION, maxFGversion) > 0) {
    throw errors::fg_version_too_recent(
      "add-on '" + addonId + "' requires FlightGear " + maxFGversion +
      " or earlier, however this is FlightGear " + FLIGHTGEAR_VERSION);
  }

  // Store the add-on metadata in _idToAddonMap
  auto emplaceRetval = _idToAddonMap.emplace(addonId, std::move(addon));

  // Prevent registration of two add-ons with the same id
  if (!emplaceRetval.second) {
    auto existingElt = _idToAddonMap.find(addonId);
    assert(existingElt != _idToAddonMap.end());
    throw errors::duplicate_registration_attempt(
      "attempt to register add-on '" + addonId + "' with base path '"
      + addonPath.utf8Str() + "', however it is already registered with base "
      "path '" + existingElt->second->getBasePath().utf8Str() + "'");
  }

  return addonId;
}

string
AddonManager::registerAddon(const SGPath& addonPath)
{
  // Use realpath() as in FGGlobals::append_aircraft_path(), otherwise
  // fgValidatePath() will deny access to resources under the add-on path if
  // one of its components is a symlink.
  const SGPath addonRealPath = addonPath.realpath();
  const string addonId = registerAddonMetadata(addonRealPath);
  loadConfigFileIfExists(addonRealPath / "addon-config.xml");
  globals->append_aircraft_path(addonRealPath);

  AddonRef addon{getAddon(addonId)};
  addon->getLoadedFlagNode()->setBoolValue(false);
  SGPropertyNode_ptr addonNode = addon->getAddonNode();

  // Set a few properties for the add-on under this node
  addonNode->getNode("id", true)->setStringValue(addonId);
  addonNode->getNode("name", true)->setStringValue(addon->getName());
  addonNode->getNode("version", true)
           ->setStringValue(addonVersion(addonId)->str());
  addonNode->getNode("path", true)->setStringValue(addonRealPath.utf8Str());
  addonNode->getNode("load-seq-num", true)
           ->setIntValue(addon->getLoadSequenceNumber());

  // “Legacy node”. Should we remove these two lines?
  SGPropertyNode* seqNumNode = fgGetNode("addons", true)->addChild("addon");
  seqNumNode->getNode("path", true)->setStringValue(addonRealPath.utf8Str());

  string msg = "Registered add-on '" + addon->getName() + "' (" + addonId +
               ") version " + addonVersion(addonId)->str() + "; "
               "base path is '" + addonRealPath.utf8Str() + "'";

  auto dataPath = addonRealPath / "FGData";
  if (dataPath.exists()) {
      SG_LOG(SG_GENERAL, SG_INFO, "Registering data path for add-on: " << addon->getName());
      globals->append_data_path(dataPath, true /* after FG_ROOT */);
  }
    
    // This preserves the registration order
    _registeredAddons.push_back(addon);
    SG_LOG(SG_GENERAL, SG_INFO, msg);

  return addonId;
}

bool
AddonManager::isAddonRegistered(const string& addonId) const
{
  return (_idToAddonMap.find(addonId) != _idToAddonMap.end());
}

bool
AddonManager::isAddonLoaded(const string& addonId) const
{
  return (isAddonRegistered(addonId) &&
          getAddon(addonId)->getLoadedFlagNode()->getBoolValue());
}

vector<AddonRef>
AddonManager::registeredAddons() const
{
  return _registeredAddons;
}

vector<AddonRef>
AddonManager::loadedAddons() const
{
  vector<AddonRef> v;
  v.reserve(_idToAddonMap.size()); // will be the right size most of the times

  for (const auto& elem: _idToAddonMap) {
    if (isAddonLoaded(elem.first)) {
      v.push_back(elem.second);
    }
  }

  return v;
}

AddonRef
AddonManager::getAddon(const string& addonId) const
{
  const auto it = _idToAddonMap.find(addonId);

  if (it == _idToAddonMap.end()) {
    throw sg_exception("tried to get add-on '" + addonId + "', however no "
                       "such add-on has been registered.");
  }

  return it->second;
}

AddonVersionRef
AddonManager::addonVersion(const string& addonId) const
{
  return getAddon(addonId)->getVersion();
}

SGPath
AddonManager::addonBasePath(const string& addonId) const
{
  return getAddon(addonId)->getBasePath();
}

SGPropertyNode_ptr AddonManager::addonNode(const string& addonId) const
{
  return getAddon(addonId)->getAddonNode();
}

void
AddonManager::addAddonMenusToFGMenubar() const
{
  for (const auto& addon: _registeredAddons) {
    addon->addToFGMenubar();
  }
}

} // of namespace addons

} // of namespace flightgear
