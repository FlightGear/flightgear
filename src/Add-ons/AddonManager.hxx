// -*- coding: utf-8 -*-
//
// AddonManager.hxx --- Manager class for FlightGear add-ons
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

#ifndef FG_ADDONMANAGER_HXX
#define FG_ADDONMANAGER_HXX

#include <string>
#include <map>
#include <memory>               // std::unique_ptr, std::shared_ptr
#include <vector>

#include <simgear/misc/sg_path.hxx>
#include <simgear/props/props.hxx>

#include "addon_fwd.hxx"
#include "Addon.hxx"
#include "AddonVersion.hxx"

namespace flightgear
{

namespace addons
{

class AddonManager
{
public:
  AddonManager(const AddonManager&) = delete;
  AddonManager& operator=(const AddonManager&) = delete;
  AddonManager(AddonManager&&) = delete;
  AddonManager& operator=(AddonManager&&) = delete;
  // The instance is created by createInstance() -> private constructor
  // but it should be deleted by its owning std::unique_ptr -> public destructor
  ~AddonManager() = default;

  // Static creator
  static const std::unique_ptr<AddonManager>& createInstance();
  // Singleton accessor
  static const std::unique_ptr<AddonManager>& instance();
  // Reset the static smart pointer, i.e., shut down the AddonManager.
  static void reset();

  // Register an add-on and return its id.
  // 'addonPath': directory containing the add-on to register
  //
  // This comprises the following steps, where $path = addonPath.realpath():
  //  - load add-on metadata from $path/addon-metadata.xml and register it
  //    inside _idToAddonMap (this step is done via registerAddonMetadata());
  //  - load $path/addon-config.xml into the Property Tree;
  //  - append $path to the list of aircraft paths;
  //  - make part of the add-on metadata available in the Property Tree under
  //    the /addons node (/addons/by-id/<addonId>/{id,version,path,...});
  //  - append a ref to the Addon instance to _registeredAddons.
  std::string registerAddon(const SGPath& addonPath);
  // Return the list of registered add-ons in registration order (which, BTW,
  // is the same as load order).
  std::vector<AddonRef> registeredAddons() const;
  bool isAddonRegistered(const std::string& addonId) const;

  // A loaded add-on is one whose addon-main.nas file has been loaded. The
  // returned vector is sorted by add-on id (cheap sorting based on UTF-8 code
  // units, only guaranteed correct for ASCII chars).
  std::vector<AddonRef> loadedAddons() const;
  bool isAddonLoaded(const std::string& addonId) const;

  AddonRef getAddon(const std::string& addonId) const;
  AddonVersionRef addonVersion(const std::string& addonId) const;
  SGPath addonBasePath(const std::string& addonId) const;

  // Base node pertaining to the add-on in the Global Property Tree
  SGPropertyNode_ptr addonNode(const std::string& addonId) const;

  // Add the 'menu' nodes defined by each registered add-on to
  // /sim/menubar/default
  void addAddonMenusToFGMenubar() const;

private:
  // Constructor called from createInstance() only
  explicit AddonManager() = default;

  static void loadConfigFileIfExists(const SGPath& configFile);
  // Register add-on metadata inside _idToAddonMap and return the add-on id
  std::string registerAddonMetadata(const SGPath& addonPath);

  // Map each add-on id to the corresponding Addon instance.
  std::map<std::string, AddonRef> _idToAddonMap;
  // The order in _registeredAddons is the registration order.
  std::vector<AddonRef> _registeredAddons;
  // 0 for the first loaded add-on, 1 for the second, etc.
  // Also note that add-ons are loaded in their registration order.
  int _loadSequenceNumber = 0;
};

} // of namespace addons

} // of namespace flightgear

#endif  // of FG_ADDONMANAGER_HXX
