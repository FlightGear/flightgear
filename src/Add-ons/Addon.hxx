// -*- coding: utf-8 -*-
//
// Addon.hxx --- FlightGear class holding add-on metadata
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

#ifndef FG_ADDON_HXX
#define FG_ADDON_HXX

#include <map>
#include <ostream>
#include <string>
#include <vector>

#include <simgear/misc/sg_path.hxx>
#include <simgear/nasal/cppbind/NasalHash.hxx>
#include <simgear/nasal/naref.h>
#include <simgear/props/props.hxx>
#include <simgear/structure/SGReferenced.hxx>

#include "addon_fwd.hxx"
#include "contacts.hxx"
#include "AddonVersion.hxx"
#include "pointer_traits.hxx"

namespace flightgear
{

namespace addons
{

enum class UrlType {
  author,
  maintainer,
  homePage,
  download,
  support,
  codeRepository,
  license
};

class QualifiedUrl
{
public:
  QualifiedUrl(UrlType type, std::string url, std::string detail = "");

  UrlType getType() const;
  void setType(UrlType type);

  std::string getUrl() const;
  void setUrl(const std::string& url);

  std::string getDetail() const;
  void setDetail(const std::string& detail);

private:
  UrlType _type;
  std::string _url;
  // Used to store the author or maintainer name when _type is UrlType::author
  // or UrlType::maintainer. Could be used to record details about a website
  // too (e.g., for a UrlType::support, something like “official forum”).
  std::string _detail;
};

class Addon : public SGReferenced
{
public:
  // An empty value for 'minFGVersionRequired' is translated into "2017.4.0".
  // An empty value for 'maxFGVersionRequired' is translated into "none".
  Addon(std::string id, AddonVersion version = AddonVersion(),
        SGPath basePath = SGPath(), std::string minFGVersionRequired = "",
        std::string maxFGVersionRequired = "",
        SGPropertyNode* addonNode = nullptr);

  // Parse the add-on metadata file inside 'addonPath' (as defined by
  // getMetadataFile()) and return the corresponding Addon instance.
  static Addon fromAddonDir(const SGPath& addonPath);

  template<class T>
  static T fromAddonDir(const SGPath& addonPath)
  {
    using ptr_traits = shared_ptr_traits<T>;
    return ptr_traits::makeStrongRef(fromAddonDir(addonPath));
  }

  std::string getId() const;

  std::string getName() const;
  void setName(const std::string& addonName);

  AddonVersionRef getVersion() const;
  void setVersion(const AddonVersion& addonVersion);

  std::vector<AuthorRef> getAuthors() const;
  void setAuthors(const std::vector<AuthorRef>& addonAuthors);

  std::vector<MaintainerRef> getMaintainers() const;
  void setMaintainers(const std::vector<MaintainerRef>& addonMaintainers);

  std::string getShortDescription() const;
  void setShortDescription(const std::string& addonShortDescription);

  std::string getLongDescription() const;
  void setLongDescription(const std::string& addonLongDescription);

  std::string getLicenseDesignation() const;
  void setLicenseDesignation(const std::string& addonLicenseDesignation);

  SGPath getLicenseFile() const;
  void setLicenseFile(const SGPath& addonLicenseFile);

  std::string getLicenseUrl() const;
  void setLicenseUrl(const std::string& addonLicenseUrl);

  std::vector<std::string> getTags() const;
  void setTags(const std::vector<std::string>& addonTags);

  SGPath getBasePath() const;
  void setBasePath(const SGPath& addonBasePath);

  // Return $FG_HOME/Export/Addons/ADDON_ID as an SGPath instance.
  SGPath getStoragePath() const;
  // Create directory $FG_HOME/Export/Addons/ADDON_ID, including any parent,
  // if it doesn't already exist. Throw an exception in case of problems.
  // Return an SGPath instance for the directory (same as getStoragePath()).
  SGPath createStorageDir() const;

  // Return a resource path suitable for use with the simgear::ResourceManager.
  // 'relativePath' is relative to the add-on base path, and should not start
  // with a '/'.
  std::string resourcePath(const std::string& relativePath) const;

  // Should be valid for use with simgear::strutils::compare_versions()
  std::string getMinFGVersionRequired() const;
  void setMinFGVersionRequired(const std::string& minFGVersionRequired);

  // Should be valid for use with simgear::strutils::compare_versions(),
  // except for the special value "none".
  std::string getMaxFGVersionRequired() const;
  void setMaxFGVersionRequired(const std::string& maxFGVersionRequired);

  std::string getHomePage() const;
  void setHomePage(const std::string& addonHomePage);

  std::string getDownloadUrl() const;
  void setDownloadUrl(const std::string& addonDownloadUrl);

  std::string getSupportUrl() const;
  void setSupportUrl(const std::string& addonSupportUrl);

  std::string getCodeRepositoryUrl() const;
  void setCodeRepositoryUrl(const std::string& addonCodeRepositoryUrl);

  std::string getTriggerProperty() const;
  void setTriggerProperty(const std::string& addonTriggerProperty);

  // Node pertaining to the add-on in the Global Property Tree
  SGPropertyNode_ptr getAddonNode() const;
  void setAddonNode(SGPropertyNode* addonNode);
  // For Nasal: result as a props.Node object
  naRef getAddonPropsNode() const;

  // Property node indicating whether the add-on is fully loaded
  SGPropertyNode_ptr getLoadedFlagNode() const;

  // 0 for the first loaded add-on, 1 for the second, etc.
  // -1 means “not set” (as done by the default constructor)
  int getLoadSequenceNumber() const;
  void setLoadSequenceNumber(int num);

  // Get all non-empty URLs pertaining to this add-on
  std::multimap<UrlType, QualifiedUrl> getUrls() const;

  // Getter and setter for the menu bar item nodes of the add-on
  std::vector<SGPropertyNode_ptr> getMenubarNodes() const;
  void setMenubarNodes(const std::vector<SGPropertyNode_ptr>& menubarNodes);
  // Add the menus defined in addon-menubar-items.xml to /sim/menubar/default
  void addToFGMenubar() const;

  // Simple string representation
  std::string str() const;

  static void setupGhost(nasal::Hash& addonsModule);

  /**
     * @brief update string values (description, etc) based on the active locale
     */
  void retranslate();

  private:
  class Metadata;
  class MetadataParser;

  // “Compute” a path to the metadata file from the add-on base path
  static SGPath getMetadataFile(const SGPath& addonPath);
  SGPath getMetadataFile() const;

  // Read all menus from addon-menubar-items.xml (under the add-on base path)
  static std::vector<SGPropertyNode_ptr>
  readMenubarItems(const SGPath& menuFile);

  // The add-on identifier, in reverse DNS style. The AddonManager refuses to
  // register two add-ons with the same id in a given FlightGear session.
  const std::string _id;
  // Pretty name for the add-on (not constrained to reverse DNS style)
  std::string _name;
  // Use a smart pointer to expose the AddonVersion instance to Nasal without
  // needing to copy the data every time.
  AddonVersionRef _version;

  std::vector<AuthorRef> _authors;
  std::vector<MaintainerRef> _maintainers;

  // Strings describing what the add-on does
  std::string _shortDescription;
  std::string _longDescription;

  std::string _licenseDesignation;
  SGPath _licenseFile;
  std::string _licenseUrl;

  std::vector<std::string> _tags;
  SGPath _basePath;
  // $FG_HOME/Export/Addons/ADDON_ID
  const SGPath _storagePath;

  // To be used with simgear::strutils::compare_versions()
  std::string _minFGVersionRequired;
  // Ditto, but there is a special value: "none"
  std::string _maxFGVersionRequired;

  std::string _homePage;
  std::string _downloadUrl;
  std::string _supportUrl;
  std::string _codeRepositoryUrl;

  // Main node for the add-on in the Property Tree
  SGPropertyNode_ptr _addonNode;
  // The add-on will be loaded when the property referenced by
  // _triggerProperty is written to.
  std::string _triggerProperty;
  // Semantics explained above
  int _loadSequenceNumber = -1;

  std::vector<SGPropertyNode_ptr> _menubarNodes;
};

std::ostream& operator<<(std::ostream& os, const Addon& addon);

} // of namespace addons

} // of namespace flightgear

#endif  // of FG_ADDON_HXX
