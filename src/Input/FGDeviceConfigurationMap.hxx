// FGDeviceConfigurationMap.hxx -- a map to access xml device configuration
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

#ifndef _FGDEVICECONFIGURATIONMAP_HXX
#define _FGDEVICECONFIGURATIONMAP_HXX

#ifndef __cplusplus                                                          
# error This library requires C++
#endif

#include <simgear/props/props.hxx>
#include <simgear/misc/sg_path.hxx>

#include <map>

class FGDeviceConfigurationMap
{
public:
  FGDeviceConfigurationMap();

  FGDeviceConfigurationMap ( const std::string& relative_path,
                            SGPropertyNode* nodePath,
                            const std::string& nodeName);
  virtual ~FGDeviceConfigurationMap();

  SGPropertyNode_ptr configurationForDeviceName(const std::string& name);
  
  bool hasConfiguration(const std::string& name) const;

private:
  void scan_dir(const SGPath & path);
  
  void readCachedData(const SGPath& path);
  void refreshCacheForFile(const SGPath& path);
  
  std::string computeSuffix(SGPropertyNode_ptr node);

  typedef std::map<std::string, SGPropertyNode_ptr> NameNodeMap;
// dictionary of over-ridden configurations, where the config data
// was explicitly loaded and shoudl be picked over a file search
  NameNodeMap overrideDict;
  
  typedef std::map<std::string, SGPath> NamePathMap;
// mapping from joystick name to XML configuration file path
  NamePathMap namePathMap;
};

#endif
