// controls.cxx -- defines a standard interface to all flight sim controls
//
// Written by Curtis Olson, started May 1997.
//
// Copyright (C) 1997  Curtis L. Olson  - curt@infoplane.com
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


#include "controls.hxx"

#include <simgear/debug/logstream.hxx>
#include <Main/fg_props.hxx>


FGControls controls;


// Constructor
FGControls::FGControls() :
    aileron( 0.0 ),
    elevator( 0.0 ),
    elevator_trim( 1.969572E-03 ),
    rudder( 0.0 ),
    throttle_idle( true )
{
}


void FGControls::reset_all()
{
    set_aileron(0.0);
    set_elevator(0.0);
    set_elevator_trim(0.0);
    set_rudder(0.0);
    set_throttle(FGControls::ALL_ENGINES, 0.0);
    throttle_idle = true;
}


// Destructor
FGControls::~FGControls() {
}


void
FGControls::init ()
{
    for ( int engine = 0; engine < MAX_ENGINES; engine++ ) {
	throttle[engine] = 0.0;
	mixture[engine] = 1.0;
	prop_advance[engine] = 1.0;
    }

    for ( int wheel = 0; wheel < MAX_WHEELS; wheel++ ) {
        brake[wheel] = 0.0;
    }

    auto_coordination = fgGetNode("/sim/auto-coordination", true);
}


void
FGControls::bind ()
{
  fgTie("/controls/aileron", this,
		    &FGControls::get_aileron, &FGControls::set_aileron);
  fgTie("/controls/elevator", this,
       &FGControls::get_elevator, &FGControls::set_elevator);
  fgTie("/controls/elevator-trim", this,
       &FGControls::get_elevator_trim, &FGControls::set_elevator_trim);
  fgTie("/controls/rudder", this,
       &FGControls::get_rudder, &FGControls::set_rudder);
  fgTie("/controls/flaps", this,
       &FGControls::get_flaps, &FGControls::set_flaps);
  int index;
  for (index = 0; index < MAX_ENGINES; index++) {
    char name[32];
    sprintf(name, "/controls/throttle[%d]", index);
    fgTie(name, this, index,
	  &FGControls::get_throttle, &FGControls::set_throttle);
    sprintf(name, "/controls/mixture[%d]", index);
    fgTie(name, this, index,
	 &FGControls::get_mixture, &FGControls::set_mixture);
    sprintf(name, "/controls/propellor-pitch[%d]", index);
    fgTie(name, this, index,
	 &FGControls::get_prop_advance, &FGControls::set_prop_advance);
  }
//   fgTie("/controls/throttle/all", this, ALL_ENGINES,
// 	&FGControls::get_throttle, &FGControls::set_throttle);
//   fgTie("/controls/mixture/all", this, ALL_ENGINES,
// 	&FGControls::get_mixture, &FGControls::set_mixture);
//   fgTie("/controls/propellor-pitch/all", this, ALL_ENGINES,
// 	&FGControls::get_prop_advance, &FGControls::set_prop_advance);
  for (index = 0; index < MAX_WHEELS; index++) {
    char name[32];
    sprintf(name, "/controls/brakes[%d]", index);
    fgTie(name, this, index,
	 &FGControls::get_brake, &FGControls::set_brake);
  }
  fgTie("/controls/brakes/all", this, ALL_WHEELS,
	&FGControls::get_brake, &FGControls::set_brake);
}


void
FGControls::unbind ()
{
				// Tie control properties.
  fgUntie("/controls/aileron");
  fgUntie("/controls/elevator");
  fgUntie("/controls/elevator-trim");
  fgUntie("/controls/rudder");
  fgUntie("/controls/flaps");
  int index;
  for (index = 0; index < MAX_ENGINES; index++) {
    char name[32];
    sprintf(name, "/controls/throttle[%d]", index);
    fgUntie(name);
    sprintf(name, "/controls/mixture[%d]", index);
    fgUntie(name);
    sprintf(name, "/controls/propellor-pitch[%d]", index);
    fgUntie(name);
  }
  for (index = 0; index < MAX_WHEELS; index++) {
    char name[32];
    sprintf(name, "/controls/brakes[%d]", index);
    fgUntie(name);
  }
}


void
FGControls::update ()
{
}

