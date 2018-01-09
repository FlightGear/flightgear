// -*- coding: utf-8 -*-
//
// NasalAddons.cxx --- Expose add-on classes to Nasal
// Copyright (C) 2017, 2018  Florent Rougon
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

#include <memory>
#include <string>

#include <cassert>

#include <simgear/nasal/cppbind/Ghost.hxx>
#include <simgear/nasal/cppbind/NasalCallContext.hxx>
#include <simgear/nasal/cppbind/NasalHash.hxx>
#include <simgear/nasal/nasal.h>
#include <simgear/structure/exception.hxx>

#include <Add-ons/addon_fwd.hxx>
#include <Add-ons/Addon.hxx>
#include <Add-ons/AddonManager.hxx>
#include <Add-ons/AddonVersion.hxx>
#include <Add-ons/contacts.hxx>

namespace flightgear
{

namespace addons
{

// ***************************************************************************
// *                              AddonManager                               *
// ***************************************************************************

static const std::unique_ptr<AddonManager>& getAddonMgrWithCheck()
{
  const auto& addonMgr = AddonManager::instance();

  if (!addonMgr) {
    throw sg_exception("trying to access the AddonManager from Nasal, "
                       "however it is not initialized");
  }

  return addonMgr;
}

static naRef f_registeredAddons(const nasal::CallContext& ctx)
{
  const auto& addonMgr = getAddonMgrWithCheck();
  return ctx.to_nasal(addonMgr->registeredAddons());
}

static naRef f_isAddonRegistered(const nasal::CallContext& ctx)
{
  const auto& addonMgr = getAddonMgrWithCheck();
  std::string addonId = ctx.requireArg<std::string>(0);
  return ctx.to_nasal(addonMgr->isAddonRegistered(addonId));
}

static naRef f_loadedAddons(const nasal::CallContext& ctx)
{
  const auto& addonMgr = getAddonMgrWithCheck();
  return ctx.to_nasal(addonMgr->loadedAddons());
}

static naRef f_isAddonLoaded(const nasal::CallContext& ctx)
{
  const auto& addonMgr = getAddonMgrWithCheck();
  std::string addonId = ctx.requireArg<std::string>(0);
  return ctx.to_nasal(addonMgr->isAddonLoaded(addonId));
}

static naRef f_getAddon(const nasal::CallContext& ctx)
{
  const auto& addonMgr = getAddonMgrWithCheck();
  return ctx.to_nasal(addonMgr->getAddon(ctx.requireArg<std::string>(0)));
}

static void wrapAddonManagerMethods(nasal::Hash& addonsModule)
{
  addonsModule.set("registeredAddons", &f_registeredAddons);
  addonsModule.set("isAddonRegistered", &f_isAddonRegistered);
  addonsModule.set("loadedAddons", &f_loadedAddons);
  addonsModule.set("isAddonLoaded", &f_isAddonLoaded);
  addonsModule.set("getAddon", &f_getAddon);
}

// ***************************************************************************
// *                              AddonVersion                               *
// ***************************************************************************

// Create a new AddonVersion instance.
//
//   addons.AddonVersion.new(versionString)
//   addons.AddonVersion.new(major[, minor[, patchLevel[, suffix]]])
//
// where:
//   - 'major', 'minor' and 'patchLevel' are integers;
//   - 'suffix' is a string such as "", "a3", "b12" or "rc4" (resp. meaning:
//     release, alpha 3, beta 12, release candidate 4) Suffixes for
//     development releases are also allowed, such as ".dev4" (sorts before
//     the release as well as any alphas, betas and rcs for the release) and
//     "a3.dev10" (sorts before "a3.dev11", "a3.dev12", "a3", etc.).
//
// For details, see <https://www.python.org/dev/peps/pep-0440/> which is a
// proper superset of what is allowed here.
naRef f_createAddonVersion(const nasal::CallContext& ctx)
{
  int major = 0, minor = 0, patchLevel = 0;
  std::string suffix;

  if (ctx.argc == 0 || ctx.argc > 4) {
    ctx.runtimeError(
      "AddonVersion.new(versionString) or "
      "AddonVersion.new(major[, minor[, patchLevel[, suffix]]])"
    );
  }

  if (ctx.argc == 1) {
    naRef arg1 = ctx.args[0];

    if (naIsString(arg1)) {
      return ctx.to_nasal(AddonVersionRef(new AddonVersion(naStr_data(arg1))));
    } else if (naIsNum(arg1)) {
      AddonVersionRef ref{new AddonVersion(arg1.num, minor, patchLevel,
                                           AddonVersionSuffix(suffix))};
      return ctx.to_nasal(std::move(ref));
    } else {
      ctx.runtimeError(
        "AddonVersion.new(versionString) or "
        "AddonVersion.new(major[, minor[, patchLevel[, suffix]]])"
      );
    }
  }

  assert(ctx.argc > 0);
  if (!ctx.isNumeric(0)) {
    ctx.runtimeError(
      "addons.AddonVersion.new() requires major number as an integer"
    );
  }

  major = ctx.requireArg<int>(0);

  if (ctx.argc > 1) {
    if (!ctx.isNumeric(1)) {
      ctx.runtimeError(
        "addons.AddonVersion.new() requires minor number as an integer"
      );
    }

    minor = ctx.requireArg<int>(1);
  }

  if (ctx.argc > 2) {
    if (!ctx.isNumeric(2)) {
      ctx.runtimeError(
        "addons.AddonVersion.new() requires patch level as an integer"
      );
    }

    patchLevel = ctx.requireArg<int>(2);
  }

  if (ctx.argc > 3) {
    if (!ctx.isString(3)) {
      ctx.runtimeError(
        "addons.AddonVersion.new() requires suffix as a string"
      );
    }

    suffix = ctx.requireArg<std::string>(3);
  }

  assert(ctx.argc <= 4);

  return ctx.to_nasal(
    AddonVersionRef(new AddonVersion(major, minor, patchLevel,
                                     AddonVersionSuffix(suffix))));
}

void initAddonClassesForNasal(naRef globals, naContext c)
{
  nasal::Hash globalsModule(globals, c);
  nasal::Hash addonsModule = globalsModule.createHash("addons");

  wrapAddonManagerMethods(addonsModule);

  Addon::setupGhost(addonsModule);
  Contact::setupGhost(addonsModule);
  Author::setupGhost(addonsModule);
  Maintainer::setupGhost(addonsModule);

  AddonVersion::setupGhost(addonsModule);
  addonsModule.createHash("AddonVersion").set("new", &f_createAddonVersion);
}

} // of namespace addons

} // of namespace flightgear
