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



////////////////////////////////////////////////////////////////////////
// Inline utility methods.
////////////////////////////////////////////////////////////////////////

static inline void
CLAMP(double *x, double min, double max )
{
  if ( *x < min ) { *x = min; }
  if ( *x > max ) { *x = max; }
}

static inline void
CLAMP(int *i, int min, int max )
{
  if ( *i < min ) { *i = min; }
  if ( *i > max ) { *i = max; }
}


////////////////////////////////////////////////////////////////////////
// Implementation of FGControls.
////////////////////////////////////////////////////////////////////////

// Constructor
FGControls::FGControls() :
    aileron( 0.0 ),
    aileron_trim( 0.0 ),
    elevator( 0.0 ),
    elevator_trim( 0.0 ),
    rudder( 0.0 ),
    rudder_trim( 0.0 ),
    flaps( 0.0 ),
    parking_brake( 0.0 ),
    throttle_idle( true ),
    gear_down( false )
{
}


void FGControls::reset_all()
{
    set_aileron(0.0);
    set_aileron_trim(0.0);
    set_elevator(0.0);
    set_elevator_trim(0.0);
    set_rudder(0.0);
    set_rudder_trim(0.0);
    set_throttle(FGControls::ALL_ENGINES, 0.0);
    set_starter(FGControls::ALL_ENGINES, false);
    set_magnetos(FGControls::ALL_ENGINES, 0);
    throttle_idle = true;
    gear_down = true;
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
	magnetos[engine] = 0;
	starter[engine] = false;
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
  fgSetArchivable("/controls/aileron");
  fgTie("/controls/aileron-trim", this,
       &FGControls::get_aileron_trim, &FGControls::set_aileron_trim);
  fgSetArchivable("/controls/aileron-trim");
  fgTie("/controls/elevator", this,
       &FGControls::get_elevator, &FGControls::set_elevator);
  fgSetArchivable("/controls/elevator");
  fgTie("/controls/elevator-trim", this,
       &FGControls::get_elevator_trim, &FGControls::set_elevator_trim);
  fgSetArchivable("/controls/elevator-trim");
  fgTie("/controls/rudder", this,
       &FGControls::get_rudder, &FGControls::set_rudder);
  fgSetArchivable("/controls/rudder");
  fgTie("/controls/rudder-trim", this,
       &FGControls::get_rudder_trim, &FGControls::set_rudder_trim);
  fgSetArchivable("/controls/rudder-trim");
  fgTie("/controls/flaps", this,
       &FGControls::get_flaps, &FGControls::set_flaps);
  fgSetArchivable("/controls/flaps");
  int index;
  for (index = 0; index < MAX_ENGINES; index++) {
    char name[32];
    sprintf(name, "/controls/throttle[%d]", index);
    fgTie(name, this, index,
	  &FGControls::get_throttle, &FGControls::set_throttle);
    fgSetArchivable(name);
    sprintf(name, "/controls/mixture[%d]", index);
    fgTie(name, this, index,
	 &FGControls::get_mixture, &FGControls::set_mixture);
    fgSetArchivable(name);
    sprintf(name, "/controls/propeller-pitch[%d]", index);
    fgTie(name, this, index,
	 &FGControls::get_prop_advance, &FGControls::set_prop_advance);
    fgSetArchivable(name);
    sprintf(name, "/controls/magnetos[%d]", index);
    fgTie(name, this, index,
	 &FGControls::get_magnetos, &FGControls::set_magnetos);
    fgSetArchivable(name);
    sprintf(name, "/controls/starter[%d]", index);
    fgTie(name, this, index,
	 &FGControls::get_starter, &FGControls::set_starter);
    fgSetArchivable(name);
  }
  fgTie("/controls/parking-brake", this,
	&FGControls::get_parking_brake, &FGControls::set_parking_brake);
  fgSetArchivable("/controls/parking-brake");
  for (index = 0; index < MAX_WHEELS; index++) {
    char name[32];
    sprintf(name, "/controls/brakes[%d]", index);
    fgTie(name, this, index,
	 &FGControls::get_brake, &FGControls::set_brake);
    fgSetArchivable(name);
  }
  fgTie("/controls/gear-down", this,
	&FGControls::get_gear_down, &FGControls::set_gear_down);
  fgSetArchivable("/controls/gear-down");
}


void
FGControls::unbind ()
{
				// Tie control properties.
  fgUntie("/controls/aileron");
  fgUntie("/controls/aileron-trim");
  fgUntie("/controls/elevator");
  fgUntie("/controls/elevator-trim");
  fgUntie("/controls/rudder");
  fgUntie("/controls/rudder-trim");
  fgUntie("/controls/flaps");
  int index;
  for (index = 0; index < MAX_ENGINES; index++) {
    char name[32];
    sprintf(name, "/controls/throttle[%d]", index);
    fgUntie(name);
    sprintf(name, "/controls/mixture[%d]", index);
    fgUntie(name);
    sprintf(name, "/controls/propeller-pitch[%d]", index);
    fgUntie(name);
    sprintf(name, "/controls/magnetos[%d]", index);
    fgUntie(name);
    sprintf(name, "/controls/starter[%d]", index);
    fgUntie(name);
  }
  for (index = 0; index < MAX_WHEELS; index++) {
    char name[32];
    sprintf(name, "/controls/brakes[%d]", index);
    fgUntie(name);
  }
  fgUntie("/controls/gear-down");
}


void
FGControls::update (double dt)
{
}



////////////////////////////////////////////////////////////////////////
// Setters and adjusters.
////////////////////////////////////////////////////////////////////////

void
FGControls::set_aileron (double pos)
{
  aileron = pos;
  CLAMP( &aileron, -1.0, 1.0 );
			
  // check for autocoordination
  if ( auto_coordination->getBoolValue() ) {
    set_rudder( aileron / 2.0 );
  }
}

void
FGControls::move_aileron (double amt)
{
  aileron += amt;
  CLAMP( &aileron, -1.0, 1.0 );
			
  // check for autocoordination
  if ( auto_coordination->getBoolValue() ) {
    set_rudder( aileron / 2.0 );
  }
}

void
FGControls::set_aileron_trim( double pos )
{
    aileron_trim = pos;
    CLAMP( &aileron_trim, -1.0, 1.0 );
}

void
FGControls::move_aileron_trim( double amt )
{
    aileron_trim += amt;
    CLAMP( &aileron_trim, -1.0, 1.0 );
}

void
FGControls::set_elevator( double pos )
{
    elevator = pos;
    CLAMP( &elevator, -1.0, 1.0 );
}

void
FGControls::move_elevator( double amt )
{
    elevator += amt;
    CLAMP( &elevator, -1.0, 1.0 );
}

void
FGControls::set_elevator_trim( double pos )
{
    elevator_trim = pos;
    CLAMP( &elevator_trim, -1.0, 1.0 );
}

void
FGControls::move_elevator_trim( double amt )
{
    elevator_trim += amt;
    CLAMP( &elevator_trim, -1.0, 1.0 );
}

void
FGControls::set_rudder( double pos )
{
    rudder = pos;
    CLAMP( &rudder, -1.0, 1.0 );
}

void
FGControls::move_rudder( double amt )
{
    rudder += amt;
    CLAMP( &rudder, -1.0, 1.0 );
}

void
FGControls::set_rudder_trim( double pos )
{
    rudder_trim = pos;
    CLAMP( &rudder_trim, -1.0, 1.0 );
}

void
FGControls::move_rudder_trim( double amt )
{
    rudder_trim += amt;
    CLAMP( &rudder_trim, -1.0, 1.0 );
}

void
FGControls::set_flaps( double pos )
{
    flaps = pos;
    CLAMP( &flaps, 0.0, 1.0 );
}

void
FGControls::move_flaps( double amt )
{
    flaps += amt;
    CLAMP( &flaps, 0.0, 1.0 );
}

void
FGControls::set_throttle( int engine, double pos )
{
  if ( engine == ALL_ENGINES ) {
    for ( int i = 0; i < MAX_ENGINES; i++ ) {
      throttle[i] = pos;
      CLAMP( &throttle[i], 0.0, 1.0 );
    }
  } else {
    if ( (engine >= 0) && (engine < MAX_ENGINES) ) {
      throttle[engine] = pos;
      CLAMP( &throttle[engine], 0.0, 1.0 );
    }
  }
}

void
FGControls::move_throttle( int engine, double amt )
{
    if ( engine == ALL_ENGINES ) {
	for ( int i = 0; i < MAX_ENGINES; i++ ) {
	    throttle[i] += amt;
	    CLAMP( &throttle[i], 0.0, 1.0 );
	}
    } else {
	if ( (engine >= 0) && (engine < MAX_ENGINES) ) {
	    throttle[engine] += amt;
	    CLAMP( &throttle[engine], 0.0, 1.0 );
	}
    }
}

void
FGControls::set_mixture( int engine, double pos )
{
    if ( engine == ALL_ENGINES ) {
	for ( int i = 0; i < MAX_ENGINES; i++ ) {
	    mixture[i] = pos;
	    CLAMP( &mixture[i], 0.0, 1.0 );
	}
    } else {
	if ( (engine >= 0) && (engine < MAX_ENGINES) ) {
	    mixture[engine] = pos;
	    CLAMP( &mixture[engine], 0.0, 1.0 );
	}
    }
}

void
FGControls::move_mixture( int engine, double amt )
{
    if ( engine == ALL_ENGINES ) {
	for ( int i = 0; i < MAX_ENGINES; i++ ) {
	    mixture[i] += amt;
	    CLAMP( &mixture[i], 0.0, 1.0 );
	}
    } else {
	if ( (engine >= 0) && (engine < MAX_ENGINES) ) {
	    mixture[engine] += amt;
	    CLAMP( &mixture[engine], 0.0, 1.0 );
	}
    }
}

void
FGControls::set_prop_advance( int engine, double pos )
{
    if ( engine == ALL_ENGINES ) {
	for ( int i = 0; i < MAX_ENGINES; i++ ) {
	    prop_advance[i] = pos;
	    CLAMP( &prop_advance[i], 0.0, 1.0 );
	}
    } else {
	if ( (engine >= 0) && (engine < MAX_ENGINES) ) {
	    prop_advance[engine] = pos;
	    CLAMP( &prop_advance[engine], 0.0, 1.0 );
	}
    }
}

void
FGControls::move_prop_advance( int engine, double amt )
{
    if ( engine == ALL_ENGINES ) {
	for ( int i = 0; i < MAX_ENGINES; i++ ) {
	    prop_advance[i] += amt;
	    CLAMP( &prop_advance[i], 0.0, 1.0 );
	}
    } else {
	if ( (engine >= 0) && (engine < MAX_ENGINES) ) {
	    prop_advance[engine] += amt;
	    CLAMP( &prop_advance[engine], 0.0, 1.0 );
	}
    }
}

void
FGControls::set_magnetos( int engine, int pos )
{
    if ( engine == ALL_ENGINES ) {
	for ( int i = 0; i < MAX_ENGINES; i++ ) {
	    magnetos[i] = pos;
	    CLAMP( &magnetos[i], 0, 3 );
	}
    } else {
	if ( (engine >= 0) && (engine < MAX_ENGINES) ) {
	    magnetos[engine] = pos;
	    CLAMP( &magnetos[engine], 0, 3 );
	}
    }
}

void
FGControls::move_magnetos( int engine, int amt )
{
    if ( engine == ALL_ENGINES ) {
	for ( int i = 0; i < MAX_ENGINES; i++ ) {
	    magnetos[i] += amt;
	    CLAMP( &magnetos[i], 0, 3 );
	}
    } else {
	if ( (engine >= 0) && (engine < MAX_ENGINES) ) {
	    magnetos[engine] += amt;
	    CLAMP( &magnetos[engine], 0, 3 );
	}
    }
}

void
FGControls::set_starter( int engine, bool flag )
{
    if ( engine == ALL_ENGINES ) {
	for ( int i = 0; i < MAX_ENGINES; i++ ) {
	    starter[i] = flag;
	}
    } else {
	if ( (engine >= 0) && (engine < MAX_ENGINES) ) {
	    starter[engine] = flag;
	}
    }
}

void
FGControls::set_parking_brake( double pos )
{
    parking_brake = pos;
    CLAMP(&parking_brake, 0.0, 1.0);
}

void
FGControls::set_brake( int wheel, double pos )
{
    if ( wheel == ALL_WHEELS ) {
	for ( int i = 0; i < MAX_WHEELS; i++ ) {
	    brake[i] = pos;
	    CLAMP( &brake[i], 0.0, 1.0 );
	}
    } else {
	if ( (wheel >= 0) && (wheel < MAX_WHEELS) ) {
	    brake[wheel] = pos;
	    CLAMP( &brake[wheel], 0.0, 1.0 );
	}
    }
}

void
FGControls::move_brake( int wheel, double amt )
{
    if ( wheel == ALL_WHEELS ) {
	for ( int i = 0; i < MAX_WHEELS; i++ ) {
	    brake[i] += amt;
	    CLAMP( &brake[i], 0.0, 1.0 );
	}
    } else {
	if ( (wheel >= 0) && (wheel < MAX_WHEELS) ) {
	    brake[wheel] += amt;
	    CLAMP( &brake[wheel], 0.0, 1.0 );
	}
    }
}

void
FGControls::set_gear_down( bool gear )
{
  gear_down = gear;
}


