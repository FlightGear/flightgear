// ephemeris.hxx -- wrap SGEphemeris code in a subsystem
//
// Written by James Turner, started June 2010.
//
// Copyright (C) 2010  Curtis L. Olson  - http://www.flightgear.org/~curt
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

#ifndef FG_ENVIRONMENT_EPHEMERIS_HXX
#define FG_ENVIRONMENT_EPHEMERIS_HXX

#include <simgear/structure/subsystem_mgr.hxx>

class SGEphemeris;
class SGPropertyNode;

/**
 * Wrap SGEphemeris in a susbsytem/property interface
 */
class Ephemeris : public SGSubsystem
{
public:

	Ephemeris();
	~Ephemeris();		
	
	virtual void bind();
	virtual void unbind();
	virtual void update(double dt);
	virtual void init();
  virtual void postinit();
  
private:
  SGEphemeris* _impl;
  SGPropertyNode* _latProp;
};

#endif // of FG_ENVIRONMENT_EPHEMERIS_HXX

