// controls.hxx -- defines a standard interface to all flight sim controls
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


#ifndef _CONTROLS_HXX
#define _CONTROLS_HXX

#include <simgear/misc/props.hxx>

#include <Main/fgfs.hxx>
#include <Main/globals.hxx>

#ifndef __cplusplus                                                          
# error This library requires C++
#endif                                   


// Define a structure containing the control parameters

class FGControls : public FGSubsystem
{

public:

    enum
    {
	ALL_ENGINES = -1,
	MAX_ENGINES = 10
    };

    enum
    {
	ALL_WHEELS = -1,
	MAX_WHEELS = 3
    };

private:

    double aileron;
    double aileron_trim;
    double elevator;
    double elevator_trim;
    double rudder;
    double rudder_trim;
    double flaps;
    double throttle[MAX_ENGINES];
    double mixture[MAX_ENGINES];
    double prop_advance[MAX_ENGINES];
    double brake[MAX_WHEELS];
    int magnetos[MAX_ENGINES];
    bool throttle_idle;
    bool starter[MAX_ENGINES];
    bool gear_down;

    SGPropertyNode * auto_coordination;

    inline void CLAMP(double *x, double min, double max ) {
	if ( *x < min ) { *x = min; }
	if ( *x > max ) { *x = max; }
    }

    inline void CLAMP(int *i, int min, int max ) {
	if ( *i < min ) { *i = min; }
	if ( *i > max ) { *i = max; }
    }
    
public:

    FGControls();
    ~FGControls();

    // Implementation of FGSubsystem.
    void init ();
    void bind ();
    void unbind ();
    void update ();

    // Reset function
    void reset_all(void);
	
    // Query functions
    inline double get_aileron() const { return aileron; }
    inline double get_aileron_trim() const { return aileron_trim; }
    inline double get_elevator() const { return elevator; }
    inline double get_elevator_trim() const { return elevator_trim; }
    inline double get_rudder() const { return rudder; }
    inline double get_rudder_trim() const { return rudder_trim; }
    inline double get_flaps() const { return flaps; }
    inline double get_throttle(int engine) const { return throttle[engine]; }
    inline double get_mixture(int engine) const { return mixture[engine]; }
    inline double get_prop_advance(int engine) const {
	return prop_advance[engine];
    }
    inline double get_brake(int wheel) const { return brake[wheel]; }
    inline int get_magnetos(int engine) const { return magnetos[engine]; }
    inline bool get_starter(int engine) const { return starter[engine]; }
    inline bool get_gear_down() const { return gear_down; }

    // Update functions
    inline void set_aileron( double pos ) {
	aileron = pos;
	CLAMP( &aileron, -1.0, 1.0 );
			
	// check for autocoordination
	if ( auto_coordination->getBoolValue() ) 
	{
	    set_rudder( aileron / 2.0 );
	}
    }
    inline void move_aileron( double amt ) {
	aileron += amt;
	CLAMP( &aileron, -1.0, 1.0 );
			
	// check for autocoordination
	if ( auto_coordination->getBoolValue() ) 
	{
	    set_rudder( aileron / 2.0 );
	}
    }
    inline void set_aileron_trim( double pos ) {
	aileron_trim = pos;
	CLAMP( &aileron_trim, -1.0, 1.0 );
    }
    inline void move_aileron_trim( double amt ) {
	aileron_trim += amt;
	CLAMP( &aileron_trim, -1.0, 1.0 );
    }
    inline void set_elevator( double pos ) {
	elevator = pos;
	CLAMP( &elevator, -1.0, 1.0 );
    }
    inline void move_elevator( double amt ) {
	elevator += amt;
	CLAMP( &elevator, -1.0, 1.0 );
    }
    inline void set_elevator_trim( double pos ) {
	elevator_trim = pos;
	CLAMP( &elevator_trim, -1.0, 1.0 );
    }
    inline void move_elevator_trim( double amt ) {
	elevator_trim += amt;
	CLAMP( &elevator_trim, -1.0, 1.0 );
    }
    inline void set_rudder( double pos ) {
	rudder = pos;
	CLAMP( &rudder, -1.0, 1.0 );
    }
    inline void move_rudder( double amt ) {
	rudder += amt;
	CLAMP( &rudder, -1.0, 1.0 );
    }
    inline void set_rudder_trim( double pos ) {
	rudder_trim = pos;
	CLAMP( &rudder_trim, -1.0, 1.0 );
    }
    inline void move_rudder_trim( double amt ) {
	rudder_trim += amt;
	CLAMP( &rudder_trim, -1.0, 1.0 );
    }
    inline void set_flaps( double pos ) {
	flaps = pos;
	CLAMP( &flaps, 0.0, 1.0 );
    }
    inline void move_flaps( double amt ) {
	flaps += amt;
	CLAMP( &flaps, 0.0, 1.0 );
    }
    inline void set_throttle( int engine, double pos ) {
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
    inline void move_throttle( int engine, double amt ) {
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
    inline void set_mixture( int engine, double pos ) {
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
    inline void move_mixture( int engine, double amt ) {
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
    inline void set_prop_advance( int engine, double pos ) {
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
    inline void move_prop_advance( int engine, double amt ) {
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
    inline void set_magnetos( int engine, int pos ) {
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
    inline void move_magnetos( int engine, int amt ) {
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
    inline void set_starter( int engine, bool flag ) {
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
    inline void set_brake( int wheel, double pos ) {
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
    inline void move_brake( int wheel, double amt ) {
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
    inline void set_gear_down( bool gear ) { gear_down = gear; }
};


#endif // _CONTROLS_HXX


