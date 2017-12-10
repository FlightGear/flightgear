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

#include <algorithm>
#include <memory>
#include <utility>
#include <string>
#include <vector>

#include <cstdlib>
#include <cassert>

#include <simgear/debug/logstream.hxx>
#include <simgear/misc/sg_path.hxx>
#include <simgear/misc/strutils.hxx>
#include <simgear/props/props.hxx>
#include <simgear/props/props_io.hxx>
#include <simgear/structure/exception.hxx>

#include <Include/version.h>
#include <Main/fg_props.hxx>
#include <Main/globals.hxx>

#include "addon_fwd.hxx"
#include "Addon.hxx"
#include "AddonManager.hxx"
#include "AddonVersion.hxx"

namespace strutils = simgear::strutils;

using std::string;
using std::vector;
using std::shared_ptr;
using std::unique_ptr;

namespace flightgear
{

static unique_ptr<AddonManager> staticInstance;

namespace addon_errors
{
// ***************************************************************************
// *                    Base class for custom exceptions                     *
// ***************************************************************************

// Prepending a prefix such as "Add-on error: " would be redundant given the
// messages used below.
error::error(const string& message, const string& origin)
  : sg_exception(message, origin)
{ }

error::error(const char* message, const char* origin)
  : error(string(message), string(origin))
{ }

} // of namespace addon_errors

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

  try {
    readProperties(configFile, globals->get_props());
  } catch (const sg_exception &e) {
    throw addon_errors::error_loading_config_file(
      "unable to load add-on config file '" + configFile.utf8Str() + "': " +
      e.getFormattedMessage());
  }

  SG_LOG(SG_GENERAL, SG_INFO,
         "Loaded add-on config file: '" << configFile.utf8Str() + "'");
}

string
AddonManager::registerAddonMetadata(const SGPath& addonPath)
{
  SGPropertyNode addonRoot;
  const SGPath metadataFile = addonPath / "addon-metadata.xml";

  if (!metadataFile.exists()) {
    throw addon_errors::no_metadata_file_found(
      "unable to find add-on metadata file '" + metadataFile.utf8Str() + "'");
  }

  try {
    readProperties(metadataFile, &addonRoot);
  } catch (const sg_exception &e) {
    throw addon_errors::error_loading_metadata_file(
      "unable to load add-on metadata file '" + metadataFile.utf8Str() + "': " +
      e.getFormattedMessage());
  }

  // Check the 'meta' section
  SGPropertyNode *metaNode = addonRoot.getChild("meta");
  if (metaNode == nullptr) {
    throw addon_errors::error_loading_metadata_file(
      "no /meta node found in add-on metadata file '" +
      metadataFile.utf8Str() + "'");
  }

  // Check the file type
  SGPropertyNode *fileTypeNode = metaNode->getChild("file-type");
  if (fileTypeNode == nullptr) {
    throw addon_errors::error_loading_metadata_file(
      "no /meta/file-type node found in add-on metadata file '" +
      metadataFile.utf8Str() + "'");
  }

  string fileType = fileTypeNode->getStringValue();
  if (fileType != "FlightGear add-on metadata") {
    throw addon_errors::error_loading_metadata_file(
      "Invalid /meta/file-type value for add-on metadata file '" +
      metadataFile.utf8Str() + "': '" + fileType + "' "
      "(expected 'FlightGear add-on metadata')");
  }

  // Check the format version
  SGPropertyNode *fmtVersionNode = metaNode->getChild("format-version");
  if (fmtVersionNode == nullptr) {
    throw addon_errors::error_loading_metadata_file(
      "no /meta/format-version node found in add-on metadata file '" +
      metadataFile.utf8Str() + "'");
  }

  int formatVersion = fmtVersionNode->getIntValue();
  if (formatVersion != 1) {
    throw addon_errors::error_loading_metadata_file(
      "unknown format version in add-on metadata file '" +
      metadataFile.utf8Str() + "': " + std::to_string(formatVersion));
  }

  // Now the data we are really interested in
  SGPropertyNode *addonNode = addonRoot.getChild("addon");
  if (addonNode == nullptr) {
    throw addon_errors::error_loading_metadata_file(
      "no /addon node found in add-on metadata file '" +
      metadataFile.utf8Str() + "'");
  }

  SGPropertyNode *idNode = addonNode->getChild("identifier");
  if (idNode == nullptr) {
    throw addon_errors::error_loading_metadata_file(
      "no /addon/identifier node found in add-on metadata file '" +
      metadataFile.utf8Str() + "'");
  }
  std::string addonId = strutils::strip(idNode->getStringValue());

  // Require a non-empty identifier for the add-on
  if (addonId.empty()) {
    throw addon_errors::error_loading_metadata_file(
      "empty or whitespace-only value for the /addon/identifier node in "
      "add-on metadata file '" + metadataFile.utf8Str() + "'");
  } else if (addonId.find('.') == string::npos) {
    SG_LOG(SG_GENERAL, SG_WARN,
           "Add-on identifier '" << addonId << "' does not use reverse DNS "
           "style (e.g., org.flightgear.addons.MyAddon) in add-on metadata "
           "file '" << metadataFile.utf8Str() + "'");
  }

  SGPropertyNode *nameNode = addonNode->getChild("name");
  if (nameNode == nullptr) {
    throw addon_errors::error_loading_metadata_file(
      "no /addon/name node found in add-on metadata file '" +
      metadataFile.utf8Str() + "'");
  }
  std::string addonName = strutils::strip(nameNode->getStringValue());

  // Require a non-empty name for the add-on
  if (addonName.empty()) {
    throw addon_errors::error_loading_metadata_file(
      "empty or whitespace-only value for the /addon/name node in add-on "
      "metadata file '" + metadataFile.utf8Str() + "'");
  }

  SGPropertyNode *versionNode = addonNode->getChild("version");
  if (versionNode == nullptr) {
    throw addon_errors::error_loading_metadata_file(
      "no /addon/version node found in add-on metadata file '" +
      metadataFile.utf8Str() + "'");
  }
  AddonVersion addonVersion(versionNode->getStringValue());

  std::string addonShortDescription;
  SGPropertyNode *shortDescNode = addonNode->getChild("short-description");
  if (shortDescNode != nullptr) {
    addonShortDescription = strutils::strip(shortDescNode->getStringValue());
  }

  std::string addonLongDescription;
  SGPropertyNode *longDescNode = addonNode->getChild("long-description");
  if (longDescNode != nullptr) {
    addonLongDescription = strutils::strip(longDescNode->getStringValue());
  }

  std::string addonMinFGVersionRequired;
  SGPropertyNode *minNode = addonNode->getChild("min-FG-version");
  if (minNode != nullptr) {
    addonMinFGVersionRequired = minNode->getStringValue();
  }

  std::string addonMaxFGVersionRequired;
  SGPropertyNode *maxNode = addonNode->getChild("max-FG-version");
  if (maxNode != nullptr) {
    addonMaxFGVersionRequired = maxNode->getStringValue();
  }

  std::string addonHomePage;
  SGPropertyNode *homePageNode = addonNode->getChild("home-page");
  if (homePageNode != nullptr) {
    addonHomePage = strutils::strip(homePageNode->getStringValue());
  }

  std::string addonDownloadUrl;
  SGPropertyNode *downloadUrlNode = addonNode->getChild("download-url");
  if (downloadUrlNode != nullptr) {
    addonDownloadUrl = strutils::strip(downloadUrlNode->getStringValue());
  }

  std::string addonSupportUrl;
  SGPropertyNode *supportUrlNode = addonNode->getChild("support-url");
  if (supportUrlNode != nullptr) {
    addonSupportUrl = strutils::strip(supportUrlNode->getStringValue());
  }

  SGPropertyNode* addonPropNode = fgGetNode("addons", true)
                                 ->getChild("by-id", 0, 1)
                                 ->getChild(addonId, 0, 1);
  // Object holding all the add-on metadata
  AddonRef addon(new Addon(
                   addonId, std::move(addonVersion), addonPath,
                   addonMinFGVersionRequired, addonMaxFGVersionRequired,
                   addonPropNode));
  addon->setName(addonName);
  addon->setShortDescription(addonShortDescription);
  addon->setLongDescription(addonLongDescription);
  addon->setHomePage(addonHomePage);
  addon->setDownloadUrl(addonDownloadUrl);
  addon->setSupportUrl(addonSupportUrl);
  addon->setLoadSequenceNumber(_loadSequenceNumber++);

  // Check that the FlightGear version satisfies the add-on requirements
  std::string minFGversion = addon->getMinFGVersionRequired();
  if (strutils::compare_versions(FLIGHTGEAR_VERSION, minFGversion) < 0) {
    throw addon_errors::fg_version_too_old(
      "add-on '" + addonId + "' requires FlightGear " + minFGversion +
      " or later, however this is FlightGear " + FLIGHTGEAR_VERSION);
  }

  std::string maxFGversion = addon->getMaxFGVersionRequired();
  if (maxFGversion != "none" &&
      strutils::compare_versions(FLIGHTGEAR_VERSION, maxFGversion) > 0) {
    throw addon_errors::fg_version_too_recent(
      "add-on '" + addonId + "' requires FlightGear " + maxFGversion +
      " or earlier, however this is FlightGear " + FLIGHTGEAR_VERSION);
  }

  // Store the add-on metadata in _idToAddonMap
  auto emplaceRetval = _idToAddonMap.emplace(addonId, std::move(addon));

  // Prevent registration of two add-ons with the same id
  if (!emplaceRetval.second) {
    auto existingElt = _idToAddonMap.find(addonId);
    assert(existingElt != _idToAddonMap.end());
    throw addon_errors::duplicate_registration_attempt(
      "attempt to register add-on '" + addonId + "' with base path '"
      + addonPath.utf8Str() + "', however it is already registered with base "
      "path '" + existingElt->second->getBasePath().utf8Str() + "'");
  }

  SG_LOG(SG_GENERAL, SG_DEBUG,
         "Loaded add-on metadata file: '" << metadataFile.utf8Str() + "'");

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
  loadConfigFileIfExists(addonRealPath / "config.xml");
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
  // This preserves the registration order
  _registeredAddons.push_back(std::move(addon));
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

} // of namespace flightgear
