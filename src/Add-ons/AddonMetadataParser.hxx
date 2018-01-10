// -*- coding: utf-8 -*-
//
// AddonMetadataParser.hxx --- Parser for FlightGear add-on metadata files
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

#ifndef FG_ADDON_METADATA_PARSER_HXX
#define FG_ADDON_METADATA_PARSER_HXX

#include <string>
#include <tuple>
#include <vector>

#include <simgear/misc/sg_path.hxx>

#include "addon_fwd.hxx"
#include "Addon.hxx"
#include "AddonVersion.hxx"
#include "contacts.hxx"

class SGPropertyNode;

namespace flightgear
{

namespace addons
{

class Addon::Metadata
{
public:
  // Comments about these fields can be found in Addon.hxx
  std::string id;
  std::string name;
  AddonVersion version;

  std::vector<AuthorRef> authors;
  std::vector<MaintainerRef> maintainers;

  std::string shortDescription;
  std::string longDescription;

  std::string licenseDesignation;
  SGPath licenseFile;
  std::string licenseUrl;

  std::vector<std::string> tags;

  std::string minFGVersionRequired;
  std::string maxFGVersionRequired;

  std::string homePage;
  std::string downloadUrl;
  std::string supportUrl;
  std::string codeRepositoryUrl;
};

class Addon::MetadataParser
{
public:
  // “Compute” a path to the metadata file from the add-on base path
  static SGPath getMetadataFile(const SGPath& addonPath);

  // Parse the add-on metadata file inside 'addonPath' (as defined by
  // getMetadataFile()) and return the corresponding Addon::Metadata instance.
  static Addon::Metadata parseMetadataFile(const SGPath& addonPath);

private:
  static std::tuple<string, SGPath, string>
  parseLicenseNode(const SGPath& addonPath, SGPropertyNode* addonNode);

  // Parse an addon-metadata.xml node such as <authors> or <maintainers>.
  // Return the corresponding vector<AuthorRef> or vector<MaintainerRef>. If
  // the 'mainNode' argument is nullptr, return an empty vector.
  template <class T>
  static std::vector<typename contact_traits<T>::strong_ref>
  parseContactsNode(const SGPath& metadataFile, SGPropertyNode* mainNode);
};

} // of namespace addons

} // of namespace flightgear

#endif  // of FG_ADDON_METADATA_PARSER_HXX
