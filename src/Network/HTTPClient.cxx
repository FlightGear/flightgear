// HTTPClient.cxx -- Singleton HTTP client object
//
// Written by James Turner, started April 2012.
//
// Copyright (C) 2012  James Turner
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

#include "HTTPClient.hxx"

#include <cassert>

#include <Main/fg_props.hxx>
#include <Include/version.h>

#include <simgear/sg_inlines.h>

#include <simgear/package/Root.hxx>
#include <simgear/package/Catalog.hxx>
#include <simgear/package/Delegate.hxx>
#include <simgear/package/Install.hxx>
#include <simgear/package/Package.hxx>

#include <simgear/nasal/cppbind/from_nasal.hxx>
#include <simgear/nasal/cppbind/to_nasal.hxx>
#include <simgear/nasal/cppbind/NasalHash.hxx>
#include <simgear/nasal/cppbind/Ghost.hxx>

#include <Scripting/NasalSys.hxx>

using namespace simgear;

typedef nasal::Ghost<pkg::RootRef> NasalPackageRoot;
typedef nasal::Ghost<pkg::PackageRef> NasalPackage;
typedef nasal::Ghost<pkg::CatalogRef> NasalCatalog;
typedef nasal::Ghost<pkg::InstallRef> NasalInstall;

#define ENABLE_PACKAGE_SYSTEM 1

namespace {
  
  class FGDelegate : public pkg::Delegate
{
public:
  virtual void refreshComplete()
  {
    SG_LOG(SG_IO, SG_INFO, "all Catalogs refreshed");
    
    // auto-update; make this controlled by a property
    pkg::Root* r = globals->packageRoot();
    
    pkg::PackageList toBeUpdated(r->packagesNeedingUpdate());
    pkg::PackageList::const_iterator it;
    for (it = toBeUpdated.begin(); it != toBeUpdated.end(); ++it) {
      assert((*it)->isInstalled());
      SG_LOG(SG_IO, SG_INFO, "updating:" << (*it)->id());
      r->scheduleToUpdate((*it)->install());
    }
  }

  virtual void failedRefresh(pkg::Catalog* aCat, FailureCode aReason)
  {
    switch (aReason) {
    case pkg::Delegate::FAIL_SUCCESS:
        SG_LOG(SG_IO, SG_WARN, "refresh of Catalog done");
        break;
        
    default:
        SG_LOG(SG_IO, SG_WARN, "refresh of Catalog " << aCat->url() << " failed:" << aReason);
    }
  }
  
  virtual void startInstall(pkg::Install* aInstall)
  {
    SG_LOG(SG_IO, SG_INFO, "beginning install of:" << aInstall->package()->id()
           << " to local path:" << aInstall->path());

  }
  
  virtual void installProgress(pkg::Install* aInstall, unsigned int aBytes, unsigned int aTotal)
  {
    SG_LOG(SG_IO, SG_INFO, "installing:" << aInstall->package()->id() << ":"
           << aBytes << " of " << aTotal);
  }
  
  virtual void finishInstall(pkg::Install* aInstall)
  {
    SG_LOG(SG_IO, SG_INFO, "finished install of:" << aInstall->package()->id()
           << " to local path:" << aInstall->path());

  }
  
  virtual void failedInstall(pkg::Install* aInstall, FailureCode aReason)
  {
    SG_LOG(SG_IO, SG_WARN, "install failed of:" << aInstall->package()->id()
           << " to local path:" << aInstall->path());
  }

};

} // of anonymous namespace

FGHTTPClient::FGHTTPClient()
{
}

FGHTTPClient::~FGHTTPClient()
{
}

void FGHTTPClient::init()
{
  _http.reset(new simgear::HTTP::Client);
  
  std::string proxyHost(fgGetString("/sim/presets/proxy/host"));
  int proxyPort(fgGetInt("/sim/presets/proxy/port"));
  std::string proxyAuth(fgGetString("/sim/presets/proxy/auth"));
  
  if (!proxyHost.empty()) {
    _http->setProxy(proxyHost, proxyPort, proxyAuth);
  }
  
#ifdef ENABLE_PACKAGE_SYSTEM
  pkg::Root* packageRoot = globals->packageRoot();
  if (packageRoot) {
    // package system needs access to the HTTP engine too
    packageRoot->setHTTPClient(_http.get());
    
    packageRoot->setDelegate(new FGDelegate);
    
    // setup default catalog if not present
    pkg::Catalog* defaultCatalog = packageRoot->getCatalogById("org.flightgear.default");
    if (!defaultCatalog) {
      // always show this message
      SG_LOG(SG_GENERAL, SG_ALERT, "default catalog not found, installing...");
      pkg::Catalog::createFromUrl(packageRoot,
          "http://fgfs.goneabitbursar.com/pkg/" FLIGHTGEAR_VERSION "/default-catalog.xml");
    }
    
    // start a refresh now
    packageRoot->refresh();
  }
#endif // of ENABLE_PACKAGE_SYSTEM
}

static naRef f_package_existingInstall( pkg::Package& pkg,
                                        const nasal::CallContext& ctx )
{
  return ctx.to_nasal(
    pkg.existingInstall( ctx.getArg<pkg::Package::InstallCallback>(0) )
  );
}

static naRef f_package_uninstall(pkg::Package& pkg, const nasal::CallContext& ctx)
{
    pkg::InstallRef ins = pkg.existingInstall();
    if (ins) {
        ins->uninstall();
    }

    return naNil();
}

static SGPropertyNode_ptr queryPropsFromHash(const nasal::Hash& h)
{
    SGPropertyNode_ptr props(new SGPropertyNode);

    for (nasal::Hash::const_iterator it = h.begin(); it != h.end(); ++it) {
        std::string const key = it->getKey();
        if ((key == "name") || (key == "description")) {
            props->setStringValue(key, it->getValue<std::string>());
        } else if (strutils::starts_with(key, "rating-")) {
            props->setIntValue(key, it->getValue<int>());
        } else if (key == "tags") {
            string_list tags = it->getValue<string_list>();
            string_list::const_iterator tagIt;
            int tagCount = 0;
            for (tagIt = tags.begin(); tagIt != tags.end(); ++tagIt) {
                SGPropertyNode_ptr tag = props->getChild("tag", tagCount++, true);
                tag->setStringValue(*tagIt);
            }
        } else if (key == "installed") {
            props->setBoolValue(key, it->getValue<bool>());
        } else {
            SG_LOG(SG_GENERAL, SG_WARN, "unknown filter term in hash:" << key);
        }
    }

    return props;
}

static naRef f_root_search(pkg::Root& root, const nasal::CallContext& ctx)
{
    SGPropertyNode_ptr query = queryPropsFromHash(ctx.requireArg<nasal::Hash>(0));
    pkg::PackageList result = root.packagesMatching(query);
    return ctx.to_nasal(result);
}

static naRef f_catalog_search(pkg::Catalog& cat, const nasal::CallContext& ctx)
{
    SGPropertyNode_ptr query = queryPropsFromHash(ctx.requireArg<nasal::Hash>(0));
    pkg::PackageList result = cat.packagesMatching(query);
    return ctx.to_nasal(result);
}

static naRef f_package_variants(pkg::Package& pack, naContext c)
{
    nasal::Hash h(c);
    string_list vars(pack.variants());
    for (string_list_iterator it  = vars.begin(); it != vars.end(); ++it) {
        h.set(*it, pack.nameForVariant(*it));
    }

    return h.get_naRef();
}

void FGHTTPClient::postinit()
{
#ifdef ENABLE_PACKAGE_SYSTEM
  NasalPackageRoot::init("PackageRoot")
  .member("path", &pkg::Root::path)
  .member("version", &pkg::Root::catalogVersion)
  .method("refresh", &pkg::Root::refresh)
  .method("catalogs", &pkg::Root::catalogs)
  .method("packageById", &pkg::Root::getPackageById)
  .method("catalogById", &pkg::Root::getCatalogById)
  .method("search", &f_root_search);

  NasalCatalog::init("Catalog")
  .member("installRoot", &pkg::Catalog::installRoot)
  .member("id", &pkg::Catalog::id)
  .member("url", &pkg::Catalog::url)
  .member("description", &pkg::Catalog::description)
  .method("packages", &pkg::Catalog::packages)
  .method("packageById", &pkg::Catalog::getPackageById)
  .method("refresh", &pkg::Catalog::refresh)
  .method("needingUpdate", &pkg::Catalog::packagesNeedingUpdate)
  .member("installed", &pkg::Catalog::installedPackages)
  .method("search", &f_catalog_search);
  
  NasalPackage::init("Package")
  .member("id", &pkg::Package::id)
  .member("name", &pkg::Package::name)
  .member("description", &pkg::Package::description)
  .member("installed", &pkg::Package::isInstalled)
  .member("thumbnails", &pkg::Package::thumbnailUrls)
  .member("variants", &f_package_variants)
  .member("revision", &pkg::Package::revision)
  .member("catalog", &pkg::Package::catalog)
  .method("install", &pkg::Package::install)
  .method("uninstall", &f_package_uninstall)
  .method("existingInstall", &f_package_existingInstall)
  .method("lprop", &pkg::Package::getLocalisedProp)
  .member("fileSize", &pkg::Package::fileSizeBytes);
  
  typedef pkg::Install* (pkg::Install::*InstallCallback)
                        (const pkg::Install::Callback&);
  typedef pkg::Install* (pkg::Install::*ProgressCallback)
                        (const pkg::Install::ProgressCallback&);
  NasalInstall::init("Install")
  .member("revision", &pkg::Install::revsion)
  .member("pkg", &pkg::Install::package)
  .member("path", &pkg::Install::path)
  .member("hasUpdate", &pkg::Install::hasUpdate)
  .method("startUpdate", &pkg::Install::startUpdate)
  .method("uninstall", &pkg::Install::uninstall)
  .method("done", static_cast<InstallCallback>(&pkg::Install::done))
  .method("fail", static_cast<InstallCallback>(&pkg::Install::fail))
  .method("always", static_cast<InstallCallback>(&pkg::Install::always))
  .method("progress", static_cast<ProgressCallback>(&pkg::Install::progress));
  
  pkg::Root* packageRoot = globals->packageRoot();
  if (packageRoot) {
    FGNasalSys* nasalSys = (FGNasalSys*) globals->get_subsystem("nasal");
    nasal::Hash nasalGlobals = nasalSys->getGlobals();
    nasal::Hash nasalPkg = nasalGlobals.createHash("pkg"); // module
    nasalPkg.set("root", packageRoot);
  }
#endif // of ENABLE_PACKAGE_SYSTEM
}

void FGHTTPClient::shutdown()
{
  _http.reset();
}

void FGHTTPClient::update(double)
{
  _http->update();
}

void FGHTTPClient::makeRequest(const simgear::HTTP::Request_ptr& req)
{
  _http->makeRequest(req);
}
