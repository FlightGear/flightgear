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

#include <map>
#include <ostream>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include <simgear/misc/sg_path.hxx>
#include <simgear/misc/strutils.hxx>
#include <simgear/nasal/cppbind/Ghost.hxx>
#include <simgear/nasal/cppbind/NasalHash.hxx>
#include <simgear/nasal/naref.h>
#include <simgear/props/props.hxx>
#include <simgear/props/props_io.hxx>

#include <Main/fg_props.hxx>
#include <Main/globals.hxx>
#include <Scripting/NasalSys.hxx>

#include "addon_fwd.hxx"
#include "Addon.hxx"
#include "AddonMetadataParser.hxx"
#include "AddonVersion.hxx"
#include "exceptions.hxx"
#include "pointer_traits.hxx"

namespace strutils = simgear::strutils;

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

std::string Addon::resourcePath(const std::string& relativePath) const
{
  if (strutils::starts_with(relativePath, "/")) {
    throw errors::invalid_resource_path(
      "addon-specific resource path '" + relativePath + "' shouldn't start "
      "with a '/'");
  }

  return "[addon=" + getId() + "]" + relativePath;
}

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

std::multimap<UrlType, QualifiedUrl> Addon::getUrls() const
{
  std::multimap<UrlType, QualifiedUrl> res;

  auto appendIfNonEmpty = [&res](UrlType type, string url, string detail = "") {
    if (!url.empty()) {
      res.emplace(type, QualifiedUrl(type, std::move(url), std::move(detail)));
    }
  };

  for (const auto& author: _authors) {
    appendIfNonEmpty(UrlType::author, author->getUrl(), author->getName());
  }

  for (const auto& maint: _maintainers) {
    appendIfNonEmpty(UrlType::maintainer, maint->getUrl(), maint->getName());
  }

  appendIfNonEmpty(UrlType::homePage, getHomePage());
  appendIfNonEmpty(UrlType::download, getDownloadUrl());
  appendIfNonEmpty(UrlType::support, getSupportUrl());
  appendIfNonEmpty(UrlType::codeRepository, getCodeRepositoryUrl());
  appendIfNonEmpty(UrlType::license, getLicenseUrl());

  return res;
}

std::vector<SGPropertyNode_ptr> Addon::getMenubarNodes() const
{ return _menubarNodes; }

void Addon::setMenubarNodes(const std::vector<SGPropertyNode_ptr>& menubarNodes)
{ _menubarNodes = menubarNodes; }

void Addon::addToFGMenubar() const
{
  SGPropertyNode* menuRootNode = fgGetNode("/sim/menubar/default", true);

  for (const auto& node: getMenubarNodes()) {
    SGPropertyNode* childNode = menuRootNode->addChild("menu");
    ::copyProperties(node.ptr(), childNode);
  }
}

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

  SGPath menuFile = addonPath / "addon-menubar-items.xml";

  if (menuFile.exists()) {
    addon.setMenubarNodes(readMenubarItems(menuFile));
  }

  return addon;
}

// Static method
std::vector<SGPropertyNode_ptr>
Addon::readMenubarItems(const SGPath& menuFile)
{
  SGPropertyNode rootNode;

  try {
    readProperties(menuFile, &rootNode);
  } catch (const sg_exception &e) {
    throw errors::error_loading_menubar_items_file(
      "unable to load add-on menu bar items from file '" +
      menuFile.utf8Str() + "': " + e.getFormattedMessage());
  }

  // Check the 'meta' section
  SGPropertyNode *metaNode = rootNode.getChild("meta");
  if (metaNode == nullptr) {
    throw errors::error_loading_menubar_items_file(
      "no /meta node found in add-on menu bar items file '" +
      menuFile.utf8Str() + "'");
  }

  // Check the file type
  SGPropertyNode *fileTypeNode = metaNode->getChild("file-type");
  if (fileTypeNode == nullptr) {
    throw errors::error_loading_menubar_items_file(
      "no /meta/file-type node found in add-on menu bar items file '" +
      menuFile.utf8Str() + "'");
  }

  string fileType = fileTypeNode->getStringValue();
  if (fileType != "FlightGear add-on menu bar items") {
    throw errors::error_loading_menubar_items_file(
      "Invalid /meta/file-type value for add-on menu bar items file '" +
      menuFile.utf8Str() + "': '" + fileType + "' "
      "(expected 'FlightGear add-on menu bar items')");
  }

  // Check the format version
  SGPropertyNode *fmtVersionNode = metaNode->getChild("format-version");
  if (fmtVersionNode == nullptr) {
    throw errors::error_loading_menubar_items_file(
      "no /meta/format-version node found in add-on menu bar items file '" +
      menuFile.utf8Str() + "'");
  }

  int formatVersion = fmtVersionNode->getIntValue();
  if (formatVersion != 1) {
    throw errors::error_loading_menubar_items_file(
      "unknown format version in add-on menu bar items file '" +
      menuFile.utf8Str() + "': " + std::to_string(formatVersion));
  }

  SG_LOG(SG_GENERAL, SG_DEBUG,
         "Loaded add-on menu bar items from '" << menuFile.utf8Str() + "'");

  SGPropertyNode *menubarItemsNode = rootNode.getChild("menubar-items");
  std::vector<SGPropertyNode_ptr> res;

  if (menubarItemsNode != nullptr) {
    res = menubarItemsNode->getChildren("menu");
  }

  return res;
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
    .method("resourcePath", &Addon::resourcePath)
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
