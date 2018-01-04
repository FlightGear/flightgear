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
#include <regex>
#include <sstream>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include <cassert>

#include <simgear/misc/sg_path.hxx>
#include <simgear/misc/strutils.hxx>
#include <simgear/nasal/cppbind/Ghost.hxx>
#include <simgear/nasal/cppbind/NasalHash.hxx>
#include <simgear/nasal/naref.h>
#include <simgear/props/props.hxx>
#include <simgear/props/props_io.hxx>

#include <Main/globals.hxx>
#include <Scripting/NasalSys.hxx>

#include "addon_fwd.hxx"
#include "Addon.hxx"
#include "AddonVersion.hxx"
#include "contacts.hxx"
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
  return addonPath / "addon-metadata.xml";
}

SGPath Addon::getMetadataFile() const
{
  return getMetadataFile(getBasePath());
}

// Static method
Addon Addon::fromAddonDir(const SGPath& addonPath)
{
  SGPath metadataFile = getMetadataFile(addonPath);
  SGPropertyNode addonRoot;

  if (!metadataFile.exists()) {
    throw errors::no_metadata_file_found(
      "unable to find add-on metadata file '" + metadataFile.utf8Str() + "'");
  }

  try {
    readProperties(metadataFile, &addonRoot);
  } catch (const sg_exception &e) {
    throw errors::error_loading_metadata_file(
      "unable to load add-on metadata file '" + metadataFile.utf8Str() + "': " +
      e.getFormattedMessage());
  }

  // Check the 'meta' section
  SGPropertyNode *metaNode = addonRoot.getChild("meta");
  if (metaNode == nullptr) {
    throw errors::error_loading_metadata_file(
      "no /meta node found in add-on metadata file '" +
      metadataFile.utf8Str() + "'");
  }

  // Check the file type
  SGPropertyNode *fileTypeNode = metaNode->getChild("file-type");
  if (fileTypeNode == nullptr) {
    throw errors::error_loading_metadata_file(
      "no /meta/file-type node found in add-on metadata file '" +
      metadataFile.utf8Str() + "'");
  }

  string fileType = fileTypeNode->getStringValue();
  if (fileType != "FlightGear add-on metadata") {
    throw errors::error_loading_metadata_file(
      "Invalid /meta/file-type value for add-on metadata file '" +
      metadataFile.utf8Str() + "': '" + fileType + "' "
      "(expected 'FlightGear add-on metadata')");
  }

  // Check the format version
  SGPropertyNode *fmtVersionNode = metaNode->getChild("format-version");
  if (fmtVersionNode == nullptr) {
    throw errors::error_loading_metadata_file(
      "no /meta/format-version node found in add-on metadata file '" +
      metadataFile.utf8Str() + "'");
  }

  int formatVersion = fmtVersionNode->getIntValue();
  if (formatVersion != 1) {
    throw errors::error_loading_metadata_file(
      "unknown format version in add-on metadata file '" +
      metadataFile.utf8Str() + "': " + std::to_string(formatVersion));
  }

  // Now the data we are really interested in
  SGPropertyNode *addonNode = addonRoot.getChild("addon");
  if (addonNode == nullptr) {
    throw errors::error_loading_metadata_file(
      "no /addon node found in add-on metadata file '" +
      metadataFile.utf8Str() + "'");
  }

  SGPropertyNode *idNode = addonNode->getChild("identifier");
  if (idNode == nullptr) {
    throw errors::error_loading_metadata_file(
      "no /addon/identifier node found in add-on metadata file '" +
      metadataFile.utf8Str() + "'");
  }
  string addonId = strutils::strip(idNode->getStringValue());

  // Require a non-empty identifier for the add-on
  if (addonId.empty()) {
    throw errors::error_loading_metadata_file(
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
    throw errors::error_loading_metadata_file(
      "no /addon/name node found in add-on metadata file '" +
      metadataFile.utf8Str() + "'");
  }
  string addonName = strutils::strip(nameNode->getStringValue());

  // Require a non-empty name for the add-on
  if (addonName.empty()) {
    throw errors::error_loading_metadata_file(
      "empty or whitespace-only value for the /addon/name node in add-on "
      "metadata file '" + metadataFile.utf8Str() + "'");
  }

  SGPropertyNode *versionNode = addonNode->getChild("version");
  if (versionNode == nullptr) {
    throw errors::error_loading_metadata_file(
      "no /addon/version node found in add-on metadata file '" +
      metadataFile.utf8Str() + "'");
  }
  AddonVersion addonVersion(strutils::strip(versionNode->getStringValue()));

  auto addonAuthors = parseContactsNode<Author>(metadataFile,
                                                addonNode->getChild("authors"));
  auto addonMaintainers = parseContactsNode<Maintainer>(
    metadataFile, addonNode->getChild("maintainers"));

  string addonShortDescription;
  SGPropertyNode *shortDescNode = addonNode->getChild("short-description");
  if (shortDescNode != nullptr) {
    addonShortDescription = strutils::strip(shortDescNode->getStringValue());
  }

  string addonLongDescription;
  SGPropertyNode *longDescNode = addonNode->getChild("long-description");
  if (longDescNode != nullptr) {
    addonLongDescription = strutils::strip(longDescNode->getStringValue());
  }

  string addonLicenseDesignation, addonLicenseUrl;
  SGPath addonLicenseFile;
  std::tie(addonLicenseDesignation, addonLicenseFile, addonLicenseUrl) =
    parseLicenseNode(addonPath, addonNode);

  vector<string> addonTags;
  SGPropertyNode *tagsNode = addonNode->getChild("tags");
  if (tagsNode != nullptr) {
    auto tagNodes = tagsNode->getChildren("tag");
    for (const auto& node: tagNodes) {
      addonTags.push_back(strutils::strip(node->getStringValue()));
    }
  }

  string addonMinFGVersionRequired;
  SGPropertyNode *minNode = addonNode->getChild("min-FG-version");
  if (minNode != nullptr) {
    addonMinFGVersionRequired = strutils::strip(minNode->getStringValue());
  }

  string addonMaxFGVersionRequired;
  SGPropertyNode *maxNode = addonNode->getChild("max-FG-version");
  if (maxNode != nullptr) {
    addonMaxFGVersionRequired = strutils::strip(maxNode->getStringValue());
  }

  string addonHomePage, addonDownloadUrl, addonSupportUrl, addonCodeRepoUrl;
  SGPropertyNode *urlsNode = addonNode->getChild("urls");
  if (urlsNode != nullptr) {
    SGPropertyNode *homePageNode = urlsNode->getChild("home-page");
    if (homePageNode != nullptr) {
      addonHomePage = strutils::strip(homePageNode->getStringValue());
    }

    SGPropertyNode *downloadUrlNode = urlsNode->getChild("download");
    if (downloadUrlNode != nullptr) {
      addonDownloadUrl = strutils::strip(downloadUrlNode->getStringValue());
    }

    SGPropertyNode *supportUrlNode = urlsNode->getChild("support");
    if (supportUrlNode != nullptr) {
      addonSupportUrl = strutils::strip(supportUrlNode->getStringValue());
    }

    SGPropertyNode *codeRepoUrlNode = urlsNode->getChild("code-repository");
    if (codeRepoUrlNode != nullptr) {
      addonCodeRepoUrl = strutils::strip(codeRepoUrlNode->getStringValue());
    }
  }

  // Object holding all the add-on metadata
  Addon addon{addonId, std::move(addonVersion), addonPath,
              addonMinFGVersionRequired, addonMaxFGVersionRequired};
  addon.setName(addonName);
  addon.setAuthors(addonAuthors);
  addon.setMaintainers(addonMaintainers);
  addon.setShortDescription(addonShortDescription);
  addon.setLongDescription(addonLongDescription);
  addon.setLicenseDesignation(addonLicenseDesignation);
  addon.setLicenseFile(addonLicenseFile);
  addon.setLicenseUrl(addonLicenseUrl);
  addon.setTags(addonTags);
  addon.setHomePage(addonHomePage);
  addon.setDownloadUrl(addonDownloadUrl);
  addon.setSupportUrl(addonSupportUrl);
  addon.setCodeRepositoryUrl(addonCodeRepoUrl);

  return addon;
}

// Utility function for Addon::parseContactsNode<>()
//
// Read a node such as "name", "email" or "url", child of a contact node (e.g.,
// of an "author" or "maintainer" node).
static string
parseContactsNode_readNode(const SGPath& metadataFile,
                           SGPropertyNode* contactNode,
                           string subnodeName, bool allowEmpty)
{
  SGPropertyNode *node = contactNode->getChild(subnodeName);
  string contents;

  if (node != nullptr) {
    contents = simgear::strutils::strip(node->getStringValue());
  }

  if (!allowEmpty && contents.empty()) {
    throw errors::error_loading_metadata_file(
      "in add-on metadata file '" + metadataFile.utf8Str() + "': "
      "when the node " + contactNode->getPath(true) + " exists, it must have "
      "a non-empty '" + subnodeName + "' child node");
  }

  return contents;
};

// Static method template (private and only used in this file)
template <class T>
vector<typename contact_traits<T>::strong_ref>
Addon::parseContactsNode(const SGPath& metadataFile, SGPropertyNode* mainNode)
{
  using contactTraits = contact_traits<T>;
  vector<typename contactTraits::strong_ref> res;

  if (mainNode != nullptr) {
    auto contactNodes = mainNode->getChildren(contactTraits::xmlNodeName());
    res.reserve(contactNodes.size());

    for (const auto& contactNode: contactNodes) {
      string name, email, url;

      name = parseContactsNode_readNode(metadataFile, contactNode.get(),
                                        "name", false /* allowEmpty */);
      email = parseContactsNode_readNode(metadataFile, contactNode.get(),
                                         "email", true);
      url = parseContactsNode_readNode(metadataFile, contactNode.get(),
                                       "url", true);

      using ptr_traits = shared_ptr_traits<typename contactTraits::strong_ref>;
      res.push_back(ptr_traits::makeStrongRef(name, email, url));
    }
  }

  return res;
};

// Static method
std::tuple<string, SGPath, string>
Addon::parseLicenseNode(const SGPath& addonPath, SGPropertyNode* addonNode)
{
  SGPath metadataFile = getMetadataFile(addonPath);
  string licenseDesignation;
  SGPath licenseFile;
  string licenseUrl;

  SGPropertyNode *licenseNode = addonNode->getChild("license");
  if (licenseNode == nullptr) {
    return std::tuple<string, SGPath, string>();
  }

  SGPropertyNode *licenseDesigNode = licenseNode->getChild("designation");
  if (licenseDesigNode != nullptr) {
    licenseDesignation = strutils::strip(licenseDesigNode->getStringValue());
  }

  SGPropertyNode *licenseFileNode = licenseNode->getChild("file");
  if (licenseFileNode != nullptr) {
    // This effectively disallows filenames starting or ending with whitespace
    string licenseFile_s = strutils::strip(licenseFileNode->getStringValue());

    if (!licenseFile_s.empty()) {
      if (licenseFile_s.find('\\') != string::npos) {
        throw errors::error_loading_metadata_file(
          "in add-on metadata file '" + metadataFile.utf8Str() + "': the "
          "value of /addon/license/file contains '\\'; please use '/' "
          "separators only");
      }

      if (licenseFile_s.find_first_of("/\\") == 0) {
        throw errors::error_loading_metadata_file(
          "in add-on metadata file '" + metadataFile.utf8Str() + "': the "
          "value of /addon/license/file must be relative to the add-on folder, "
          "however it starts with '" + licenseFile_s[0] + "'");
      }

      std::regex winDriveRegexp("([a-zA-Z]:).*");
      std::smatch results;

      if (std::regex_match(licenseFile_s, results, winDriveRegexp)) {
        string winDrive = results.str(1);
        throw errors::error_loading_metadata_file(
          "in add-on metadata file '" + metadataFile.utf8Str() + "': the "
          "value of /addon/license/file must be relative to the add-on folder, "
          "however it starts with a Windows drive letter (" + winDrive + ")");
      }

      licenseFile = addonPath / licenseFile_s;
      if ( !(licenseFile.exists() && licenseFile.isFile()) ) {
        throw errors::error_loading_metadata_file(
          "in add-on metadata file '" + metadataFile.utf8Str() + "': the "
          "value of /addon/license/file (pointing to '" + licenseFile.utf8Str() +
          "') doesn't correspond to an existing file");
      }
    } // of if (!licenseFile_s.empty())
  }   // of if (licenseFileNode != nullptr)

  SGPropertyNode *licenseUrlNode = licenseNode->getChild("url");
  if (licenseUrlNode != nullptr) {
    licenseUrl = strutils::strip(licenseUrlNode->getStringValue());
  }

  return std::make_tuple(licenseDesignation, licenseFile, licenseUrl);
}

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
    .member("node", &Addon::getAddonPropsNode)
    .member("loadSequenceNumber", &Addon::getLoadSequenceNumber);
}

std::ostream& operator<<(std::ostream& os, const Addon& addon)
{
  return os << addon.str();
}

} // of namespace addons

} // of namespace flightgear
