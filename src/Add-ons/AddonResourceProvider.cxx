// -*- coding: utf-8 -*-
//
// AddonResourceProvider.cxx --- ResourceProvider subclass for add-on files
// Copyright (C) 2018  Florent Rougon
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

#include <string>

#include <simgear/misc/ResourceManager.hxx>
#include <simgear/misc/sg_path.hxx>
#include <simgear/misc/strutils.hxx>

#include <Main/util.hxx>

#include "AddonManager.hxx"
#include "AddonResourceProvider.hxx"

namespace strutils = simgear::strutils;

using std::string;

namespace flightgear
{

namespace addons
{

ResourceProvider::ResourceProvider()
  : simgear::ResourceProvider(simgear::ResourceManager::PRIORITY_NORMAL)
{ }

SGPath
ResourceProvider::resolve(const string& resource, SGPath& context) const
{
  if (!strutils::starts_with(resource, "[addon=")) {
    return SGPath();
  }

  string rest = resource.substr(7); // what follows '[addon='
  auto endOfAddonId = rest.find(']');

  if (endOfAddonId == string::npos) {
    return SGPath();
  }

  string addonId = rest.substr(0, endOfAddonId);
  // Extract what follows '[addon=ADDON_ID]'
  string relPath = rest.substr(endOfAddonId + 1);

  if (relPath.empty()) {
    return SGPath();
  }

  const auto& addonMgr = AddonManager::instance();
  SGPath addonDir = addonMgr->addonBasePath(addonId);
  SGPath candidate = addonDir / relPath;

  if (!candidate.isFile()) {
    return SGPath();
  }

  return fgValidatePath(candidate, /* write */ false);
}

} // of namespace addons

} // of namespace flightgear
