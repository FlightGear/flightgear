// FGDeviceConfigurationMap.cxx -- a map to access xml device configuration
//
// Written by Torsten Dreyer, started August 2009
// Based on work from David Megginson, started May 2001.
//
// Copyright (C) 2009 Torsten Dreyer, Torsten (at) t3r _dot_ de
// Copyright (C) 2001 David Megginson, david@megginson.com
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
//
// $Id$

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include "FGDeviceConfigurationMap.hxx"

#include <boost/foreach.hpp>

#include <simgear/misc/sg_dir.hxx>
#include <simgear/props/props_io.hxx>
#include <simgear/structure/exception.hxx>

#include <Main/globals.hxx>
#include <Navaids/NavDataCache.hxx>

using simgear::PropertyList;
using std::string;

FGDeviceConfigurationMap::FGDeviceConfigurationMap( const string& relative_path,
                                                   SGPropertyNode* nodePath,
                                                   const std::string& nodeName)
{
  // scan for over-ride configurations, loaded via joysticks.xml, etc
  BOOST_FOREACH(SGPropertyNode_ptr preloaded, nodePath->getChildren(nodeName)) {
    BOOST_FOREACH(SGPropertyNode* nameProp, preloaded->getChildren("name")) {
      overrideDict[nameProp->getStringValue()] = preloaded;
    } // of names iteration
  } // of defined overrides iteration
  
  scan_dir( SGPath(globals->get_fg_home(), relative_path));
  scan_dir( SGPath(globals->get_fg_root(), relative_path));
}

FGDeviceConfigurationMap::~FGDeviceConfigurationMap()
{
}

SGPropertyNode_ptr
FGDeviceConfigurationMap::configurationForDeviceName(const std::string& name)
{
  NameNodeMap::iterator j = overrideDict.find(name);
  if (j != overrideDict.end()) {
    return j->second;
  }
  
// no override, check out list of config files
  NamePathMap::iterator it = namePathMap.find(name);
  if (it == namePathMap.end()) {
    return SGPropertyNode_ptr();
  }
      
  SGPropertyNode_ptr result(new SGPropertyNode);
  try {
    readProperties(it->second.str(), result);
    result->setStringValue("source", it->second.c_str());
  } catch (sg_exception&) {
    SG_LOG(SG_INPUT, SG_WARN, "parse failure reading:" << it->second);
    return NULL;
  }
  return result;
}

bool FGDeviceConfigurationMap::hasConfiguration(const std::string& name) const
{
  NameNodeMap::const_iterator j = overrideDict.find(name);
  if (j != overrideDict.end()) {
    return true;
  }
  
  return namePathMap.find(name) != namePathMap.end();
}

void FGDeviceConfigurationMap::scan_dir(const SGPath & path)
{
  if (!path.exists())
    return;
  
  flightgear::NavDataCache* cache = flightgear::NavDataCache::instance();
  flightgear::NavDataCache::Transaction txn(cache);
  
  simgear::Dir dir(path);
  simgear::PathList children = dir.children(simgear::Dir::TYPE_FILE | 
    simgear::Dir::TYPE_DIR | simgear::Dir::NO_DOT_OR_DOTDOT);
  
  BOOST_FOREACH(SGPath path, children) {
    if (path.isDir()) {
      scan_dir(path);
    } else if (path.extension() == "xml") {
      if (cache->isCachedFileModified(path)) {
        refreshCacheForFile(path);
      } else {
        readCachedData(path);
      } // of cached file stamp is valid
    } // of child is a file with '.xml' extension
  } // of directory children iteration
  
  txn.commit();
}

void FGDeviceConfigurationMap::readCachedData(const SGPath& path)
{
  flightgear::NavDataCache* cache = flightgear::NavDataCache::instance();
  NamePathMap::iterator it;
  BOOST_FOREACH(string s, cache->readStringListProperty(path.str())) {
    // important - only insert if not already present. This ensures
    // user configs can override those in the base package, since they are
    // searched first.
    it = namePathMap.find(s);
    if (it == namePathMap.end()) {
      namePathMap.insert(std::make_pair(s, path));
    }
  } // of cached names iteration
}

void FGDeviceConfigurationMap::refreshCacheForFile(const SGPath& path)
{
  SG_LOG(SG_INPUT, SG_DEBUG, "Reading joystick file " << path.str());
  SGPropertyNode_ptr n(new SGPropertyNode);
  try {
    readProperties(path.str(), n);
  } catch (sg_exception&) {
    SG_LOG(SG_INPUT, SG_WARN, "parse failure reading:" << path);
    return;
  }
  
  NamePathMap::iterator it;
  string_list names;
  BOOST_FOREACH(SGPropertyNode* nameProp, n->getChildren("name")) {
    names.push_back(nameProp->getStringValue());
    // same comment as readCachedData: only insert if not already present
    it = namePathMap.find(names.back());
    if (it == namePathMap.end()) {
      namePathMap.insert(std::make_pair(names.back(), path));
    }
  }
  
  flightgear::NavDataCache* cache = flightgear::NavDataCache::instance();
  cache->stampCacheFile(path);
  cache->writeStringListProperty(path.str(), names);
}
