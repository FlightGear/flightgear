// -*- coding: utf-8 -*-
//
// Addon.cxx --- FlightGear class holding add-on metadata
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

#include <ostream>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include <simgear/misc/sg_path.hxx>
#include <simgear/nasal/cppbind/Ghost.hxx>
#include <simgear/nasal/cppbind/NasalHash.hxx>
#include <simgear/nasal/naref.h>
#include <simgear/props/props.hxx>

#include <Main/globals.hxx>
#include <Scripting/NasalSys.hxx>

#include "addon_fwd.hxx"
#include "Addon.hxx"
#include "AddonMetadataParser.hxx"
#include "AddonVersion.hxx"
#include "pointer_traits.hxx"

using std::string;
using std::vector;

namespace flightgear
{

namespace addons
{

// ***************************************************************************
// *                              QualifiedUrl                               *
// ***************************************************************************

QualifiedUrl::QualifiedUrl(UrlType type, string url, string detail)
  : _type(type),
    _url(std::move(url)),
    _detail(std::move(detail))
{ }

UrlType QualifiedUrl::getType() const
{ return _type; }

void QualifiedUrl::setType(UrlType type)
{ _type = type; }

std::string QualifiedUrl::getUrl() const
{ return _url; }

void QualifiedUrl::setUrl(const std::string& url)
{ _url = url; }

std::string QualifiedUrl::getDetail() const
{ return _detail; }

void QualifiedUrl::setDetail(const std::string& detail)
{ _detail = detail; }

// ***************************************************************************
// *                                  Addon                                  *
// ***************************************************************************

Addon::Addon(std::string id, AddonVersion version, SGPath basePath,
             std::string minFGVersionRequired, std::string maxFGVersionRequired,
             SGPropertyNode* addonNode)
  : _id(std::move(id)),
    _version(
      shared_ptr_traits<AddonVersionRef>::makeStrongRef(std::move(version))),
    _basePath(std::move(basePath)),
    _minFGVersionRequired(std::move(minFGVersionRequired)),
    _maxFGVersionRequired(std::move(maxFGVersionRequired)),
    _addonNode(addonNode)
{
  if (_minFGVersionRequired.empty()) {
    // This add-on metadata class appeared in FlightGear 2017.4.0
    _minFGVersionRequired = "2017.4.0";
  }

  if (_maxFGVersionRequired.empty()) {
    _maxFGVersionRequired = "none"; // special value
  }
}

Addon::Addon()
  : Addon(std::string(), AddonVersion(), SGPath(), std::string(),
          std::string(), nullptr)
{ }

std::string Addon::getId() const
{ return _id; }

void Addon::setId(const std::string& addonId)
{ _id = addonId; }

std::string Addon::getName() const
{ return _name; }

void Addon::setName(const std::string& addonName)
{ _name = addonName; }

AddonVersionRef Addon::getVersion() const
{ return _version; }

void Addon::setVersion(const AddonVersion& addonVersion)
{
  using ptr_traits = shared_ptr_traits<AddonVersionRef>;
  _version.reset(ptr_traits::makeStrongRef(addonVersion));
}

std::vector<AuthorRef> Addon::getAuthors() const
{ return _authors; }

void Addon::setAuthors(const std::vector<AuthorRef>& addonAuthors)
{ _authors = addonAuthors; }

std::vector<MaintainerRef> Addon::getMaintainers() const
{ return _maintainers; }

void Addon::setMaintainers(const std::vector<MaintainerRef>& addonMaintainers)
{ _maintainers = addonMaintainers; }

std::string Addon::getShortDescription() const
{ return _shortDescription; }

void Addon::setShortDescription(const std::string& addonShortDescription)
{ _shortDescription = addonShortDescription; }

std::string Addon::getLongDescription() const
{ return _longDescription; }

void Addon::setLongDescription(const std::string& addonLongDescription)
{ _longDescription = addonLongDescription; }

std::string Addon::getLicenseDesignation() const
{ return _licenseDesignation; }

void Addon::setLicenseDesignation(const std::string& addonLicenseDesignation)
{ _licenseDesignation = addonLicenseDesignation; }

SGPath Addon::getLicenseFile() const
{ return _licenseFile; }

void Addon::setLicenseFile(const SGPath& addonLicenseFile)
{ _licenseFile = addonLicenseFile; }

std::string Addon::getLicenseUrl() const
{ return _licenseUrl; }

void Addon::setLicenseUrl(const std::string& addonLicenseUrl)
{ _licenseUrl = addonLicenseUrl; }

std::vector<std::string> Addon::getTags() const
{ return _tags; }

void Addon::setTags(const std::vector<std::string>& addonTags)
{ _tags = addonTags; }

SGPath Addon::getBasePath() const
{ return _basePath; }

void Addon::setBasePath(const SGPath& addonBasePath)
{ _basePath = addonBasePath; }

std::string Addon::getMinFGVersionRequired() const
{ return _minFGVersionRequired; }

void Addon::setMinFGVersionRequired(const string& minFGVersionRequired)
{ _minFGVersionRequired = minFGVersionRequired; }

std::string Addon::getMaxFGVersionRequired() const
{ return _maxFGVersionRequired; }

void Addon::setMaxFGVersionRequired(const string& maxFGVersionRequired)
{
  if (maxFGVersionRequired.empty()) {
    _maxFGVersionRequired = "none"; // special value
  } else {
    _maxFGVersionRequired = maxFGVersionRequired;
  }
}

std::string Addon::getHomePage() const
{ return _homePage; }

void Addon::setHomePage(const std::string& addonHomePage)
{ _homePage = addonHomePage; }

std::string Addon::getDownloadUrl() const
{ return _downloadUrl; }

void Addon::setDownloadUrl(const std::string& addonDownloadUrl)
{ _downloadUrl = addonDownloadUrl; }

std::string Addon::getSupportUrl() const
{ return _supportUrl; }

void Addon::setSupportUrl(const std::string& addonSupportUrl)
{ _supportUrl = addonSupportUrl; }

std::string Addon::getCodeRepositoryUrl() const
{ return _codeRepositoryUrl; }

void Addon::setCodeRepositoryUrl(const std::string& addonCodeRepositoryUrl)
{ _codeRepositoryUrl = addonCodeRepositoryUrl; }

std::string Addon::getTriggerProperty() const
{ return _triggerProperty; }

void Addon::setTriggerProperty(const std::string& addonTriggerProperty)
{ _triggerProperty = addonTriggerProperty; }

SGPropertyNode_ptr Addon::getAddonNode() const
{ return _addonNode; }

void Addon::setAddonNode(SGPropertyNode* addonNode)
{ _addonNode = SGPropertyNode_ptr(addonNode); }

naRef Addon::getAddonPropsNode() const
{
  FGNasalSys* nas = globals->get_subsystem<FGNasalSys>();
  return nas->wrappedPropsNode(_addonNode.get());
}

SGPropertyNode_ptr Addon::getLoadedFlagNode() const
{
  return { _addonNode->getChild("loaded", 0, 1) };
}

int Addon::getLoadSequenceNumber() const
{ return _loadSequenceNumber; }

void Addon::setLoadSequenceNumber(int num)
{ _loadSequenceNumber = num; }

std::string Addon::str() const
{
  std::ostringstream oss;
  oss << "addon '" << _id << "' (version = " << *_version
      << ", base path = '" << _basePath.utf8Str()
      << "', minFGVersionRequired = '" << _minFGVersionRequired
      << "', maxFGVersionRequired = '" << _maxFGVersionRequired << "')";

  return oss.str();
}

// Static method
SGPath Addon::getMetadataFile(const SGPath& addonPath)
{
  return MetadataParser::getMetadataFile(addonPath);
}

SGPath Addon::getMetadataFile() const
{
  return getMetadataFile(getBasePath());
}

// Static method
Addon Addon::fromAddonDir(const SGPath& addonPath)
{
  Addon::Metadata metadata = MetadataParser::parseMetadataFile(addonPath);

  // Object holding all the add-on metadata
  Addon addon{std::move(metadata.id), std::move(metadata.version), addonPath,
              std::move(metadata.minFGVersionRequired),
              std::move(metadata.maxFGVersionRequired)};
  addon.setName(std::move(metadata.name));
  addon.setAuthors(std::move(metadata.authors));
  addon.setMaintainers(std::move(metadata.maintainers));
  addon.setShortDescription(std::move(metadata.shortDescription));
  addon.setLongDescription(std::move(metadata.longDescription));
  addon.setLicenseDesignation(std::move(metadata.licenseDesignation));
  addon.setLicenseFile(std::move(metadata.licenseFile));
  addon.setLicenseUrl(std::move(metadata.licenseUrl));
  addon.setTags(std::move(metadata.tags));
  addon.setHomePage(std::move(metadata.homePage));
  addon.setDownloadUrl(std::move(metadata.downloadUrl));
  addon.setSupportUrl(std::move(metadata.supportUrl));
  addon.setCodeRepositoryUrl(std::move(metadata.codeRepositoryUrl));

  return addon;
}

// Static method
void Addon::setupGhost(nasal::Hash& addonsModule)
{
  nasal::Ghost<AddonRef>::init("addons.Addon")
    .member("id", &Addon::getId)
    .member("name", &Addon::getName)
    .member("version", &Addon::getVersion)
    .member("authors", &Addon::getAuthors)
    .member("maintainers", &Addon::getMaintainers)
    .member("shortDescription", &Addon::getShortDescription)
    .member("longDescription", &Addon::getLongDescription)
    .member("licenseDesignation", &Addon::getLicenseDesignation)
    .member("licenseFile", &Addon::getLicenseFile)
    .member("licenseUrl", &Addon::getLicenseUrl)
    .member("tags", &Addon::getTags)
    .member("basePath", &Addon::getBasePath)
    .member("minFGVersionRequired", &Addon::getMinFGVersionRequired)
    .member("maxFGVersionRequired", &Addon::getMaxFGVersionRequired)
    .member("homePage", &Addon::getHomePage)
    .member("downloadUrl", &Addon::getDownloadUrl)
    .member("supportUrl", &Addon::getSupportUrl)
    .member("codeRepositoryUrl", &Addon::getCodeRepositoryUrl)
    .member("triggerProperty", &Addon::getTriggerProperty)
    .member("node", &Addon::getAddonPropsNode)
    .member("loadSequenceNumber", &Addon::getLoadSequenceNumber);
}

std::ostream& operator<<(std::ostream& os, const Addon& addon)
{
  return os << addon.str();
}

} // of namespace addons

} // of namespace flightgear
