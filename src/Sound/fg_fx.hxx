// fg_fx.hxx -- Sound effect management class
//
// Started by David Megginson, October 2001
// (Reuses some code from main.cxx, probably by Curtis Olson)
//
// Copyright (C) 2001  Curtis L. Olson - http://www.flightgear.org/~curt
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

#ifndef __FGFX_HXX
#define __FGFX_HXX 1

#include <simgear/structure/subsystem_mgr.hxx>

class SGXmlSound;

/**
 * Generator for FlightGear sound effects.
 *
 * This module uses FGSoundMgr to generate sound effects based
 * on current flight conditions.  The sound manager must be initialized
 * before this object is.
 */
class FGFX : public SGSubsystem
{

public:

  FGFX ();
  virtual ~FGFX ();

  virtual void init ();
  virtual void reinit ();
  virtual void bind ();
  virtual void unbind ();
  virtual void update (double dt);

private:

  vector<SGXmlSound *> _sound;

  bool last_pause;
  double last_volume;

  SGPropertyNode *_pause;
  SGPropertyNode *_volume;

};


#endif

// end of fg_fx.hxx
