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

#include <simgear/misc/sg_dir.hxx>
#include <simgear/props/props_io.hxx>
#include <Main/globals.hxx>

using simgear::PropertyList;

FGDeviceConfigurationMap::FGDeviceConfigurationMap( const char * relative_path, SGPropertyNode_ptr aBase, const char * aChildname ) :
  base(aBase),
  childname(aChildname)
{
  int index = 1000;
  scan_dir( SGPath(globals->get_fg_root(), relative_path), &index);

  PropertyList childNodes = base->getChildren(childname);
  for (int k = (int)childNodes.size() - 1; k >= 0; k--) {
    SGPropertyNode *n = childNodes[k];
    PropertyList names = n->getChildren("name");
    if (names.size() ) // && (n->getChildren("axis").size() || n->getChildren("button").size()))
      for (unsigned int j = 0; j < names.size(); j++)
        (*this)[names[j]->getStringValue()] = n;
  }
}

FGDeviceConfigurationMap::~FGDeviceConfigurationMap()
{
  base->removeChildren( childname );
}

void FGDeviceConfigurationMap::scan_dir(const SGPath & path, int *index)
{
  simgear::Dir dir(path);
  simgear::PathList children = dir.children(simgear::Dir::TYPE_FILE | 
    simgear::Dir::TYPE_DIR | simgear::Dir::NO_DOT_OR_DOTDOT);

  for (unsigned int c=0; c<children.size(); ++c) {
    SGPath path(children[c]);
    if (path.isDir()) {
      scan_dir(path, index);
    } else if (path.extension() == "xml") {
      SG_LOG(SG_INPUT, SG_DEBUG, "Reading joystick file " << path.str());
      SGPropertyNode_ptr n = base->getChild(childname, (*index)++, true);
      readProperties(path.str(), n);
      n->setStringValue("source", path.c_str());
    }
  }
}


