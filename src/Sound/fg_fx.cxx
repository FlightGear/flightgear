// fg_fx.cxx -- Sound effect management class implementation
//
// Started by David Megginson, October 2001
// (Reuses some code from main.cxx, probably by Curtis Olson)
//
// Copyright (C) 2001  Curtis L. Olson - curt@flightgear.org
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
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//
// $Id$

#ifdef _MSC_VER
#pragma warning (disable: 4786)
#endif

#include <simgear/debug/logstream.hxx>
#include <simgear/structure/exception.hxx>
#ifdef __BORLANDC__
#  define exception c_exception
#endif
#include <simgear/misc/sg_path.hxx>
#include <simgear/props/props.hxx>
#include <simgear/sound/xmlsound.hxx>

#include <Main/fg_props.hxx>

#include "fg_fx.hxx"


FGFX::FGFX ()
{
}

FGFX::~FGFX ()
{
   _sound.clear();
}

void
FGFX::init()
{
   SGPropertyNode * node = fgGetNode("/sim/sound", true);
   int i;

   string path_str = node->getStringValue("path");
   SGPath path( globals->get_fg_root() );
   if (path_str.empty()) {
      SG_LOG(SG_GENERAL, SG_ALERT, "Incorrect path in configuration file.");
      return;
   }

   path.append(path_str.c_str());
   SG_LOG(SG_GENERAL, SG_INFO, "Reading sound " << node->getName()
          << " from " << path.str());

   SGPropertyNode root;
   try {
      readProperties(path.str(), &root);
   } catch (const sg_exception &e) {
      SG_LOG(SG_GENERAL, SG_ALERT,
       "Incorrect path specified in configuration file");
      return;
   }

   node = root.getNode("fx");
   for (i = 0; i < node->nChildren(); i++) {
      SGXmlSound *sound = new SGXmlSound();

      sound->init(globals->get_props(), node->getChild(i),
                  globals->get_soundmgr(), globals->get_fg_root());

      _sound.push_back(sound);
   }
}

void
FGFX::reinit()
{
   _sound.clear();
   init();
};

void
FGFX::bind ()
{
}

void
FGFX::unbind ()
{
}

void
FGFX::update (double dt)
{
    if (fgGetBool("/sim/sound/audible")) {
        for (unsigned int i = 0; i < _sound.size(); i++ )
            _sound[i]->update(dt);
    }
}

// end of fg_fx.cxx
