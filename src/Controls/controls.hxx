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


#ifndef __cplusplus                                                          
# error This library requires C++
#endif                                   


// Define a structure containing the control parameters

class FGControls {

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
    double elevator;
    double elevator_trim;
    double rudder;
    double throttle[MAX_ENGINES];
    double brake[MAX_WHEELS];

public:

    FGControls();
    ~FGControls();

    // Reset function
    void reset_all(void);
	
    // Query functions
    inline double get_aileron() const { return aileron; }
    inline double get_elevator() const { return elevator; }
    inline double get_elevator_trim() const { return elevator_trim; }
    inline double get_rudder() const { return rudder; }
    inline double get_throttle(int engine) const { return throttle[engine]; }
    inline double get_brake(int wheel) const { return brake[wheel]; }

    // Update functions
    inline void set_aileron( double pos ) {
	aileron = pos;
	if ( aileron < -1.0 ) aileron = -1.0;
	if ( aileron >  1.0 ) aileron =  1.0;
    }
    inline void move_aileron( double amt ) {
	aileron += amt;
	if ( aileron < -1.0 ) aileron = -1.0;
	if ( aileron >  1.0 ) aileron =  1.0;
    }
    inline void set_elevator( double pos ) {
	elevator = pos;
	if ( elevator < -1.0 ) elevator = -1.0;
	if ( elevator >  1.0 ) elevator =  1.0;
    }
    inline void move_elevator( double amt ) {
	elevator += amt;
	if ( elevator < -1.0 ) elevator = -1.0;
	if ( elevator >  1.0 ) elevator =  1.0;
    }
    inline void set_elevator_trim( double pos ) {
	elevator_trim = pos;
	if ( elevator_trim < -1.0 ) elevator_trim = -1.0;
	if ( elevator_trim >  1.0 ) elevator_trim =  1.0;
    }
    inline void move_elevator_trim( double amt ) {
	elevator_trim += amt;
	if ( elevator_trim < -1.0 ) elevator_trim = -1.0;
	if ( elevator_trim >  1.0 ) elevator_trim =  1.0;
    }
    inline void set_rudder( double pos ) {
	rudder = pos;
	if ( rudder < -1.0 ) rudder = -1.0;
	if ( rudder >  1.0 ) rudder =  1.0;
    }
    inline void move_rudder( double amt ) {
	rudder += amt;
	if ( rudder < -1.0 ) rudder = -1.0;
	if ( rudder >  1.0 ) rudder =  1.0;
    }
    inline void set_throttle( int engine, double pos ) {
	if ( engine == ALL_ENGINES ) {
	    for ( int i = 0; i < MAX_ENGINES; i++ ) {
		throttle[i] = pos;
		if ( throttle[i] < 0.0 ) throttle[i] = 0.0;
		if ( throttle[i] >  1.0 ) throttle[i] =  1.0;
	    }
	} else {
	    if ( (engine >= 0) && (engine < MAX_ENGINES) ) {
		throttle[engine] = pos;
		if ( throttle[engine] < 0.0 ) throttle[engine] = 0.0;
		if ( throttle[engine] >  1.0 ) throttle[engine] =  1.0;
	    }
	}
    }
    inline void move_throttle( int engine, double amt ) {
	if ( engine == ALL_ENGINES ) {
	    for ( int i = 0; i < MAX_ENGINES; i++ ) {
		throttle[i] += amt;
		if ( throttle[i] < 0.0 ) throttle[i] = 0.0;
		if ( throttle[i] >  1.0 ) throttle[i] =  1.0;
	    }
	} else {
	    if ( (engine >= 0) && (engine < MAX_ENGINES) ) {
		throttle[engine] += amt;
		if ( throttle[engine] < 0.0 ) throttle[engine] = 0.0;
		if ( throttle[engine] >  1.0 ) throttle[engine] =  1.0;
	    }
	}
    }
    inline void set_brake( int wheel, double pos ) {
	if ( wheel == ALL_WHEELS ) {
	    for ( int i = 0; i < MAX_WHEELS; i++ ) {
		brake[i] = pos;
		if ( brake[i] < 0.0 ) brake[i] = 0.0;
		if ( brake[i] >  1.0 ) brake[i] =  1.0;
	    }
	} else {
	    if ( (wheel >= 0) && (wheel < MAX_WHEELS) ) {
		brake[wheel] = pos;
		if ( brake[wheel] < 0.0 ) brake[wheel] = 0.0;
		if ( brake[wheel] >  1.0 ) brake[wheel] =  1.0;
	    }
	}
    }
    inline void move_brake( int wheel, double amt ) {
	if ( wheel == ALL_WHEELS ) {
	    for ( int i = 0; i < MAX_WHEELS; i++ ) {
		brake[i] += amt;
		if ( brake[i] < 0.0 ) brake[i] = 0.0;
		if ( brake[i] >  1.0 ) brake[i] =  1.0;
	    }
	} else {
	    if ( (wheel >= 0) && (wheel < MAX_WHEELS) ) {
		brake[wheel] += amt;
		if ( brake[wheel] < 0.0 ) brake[wheel] = 0.0;
		if ( brake[wheel] >  1.0 ) brake[wheel] =  1.0;
	    }
	}
    }
};


extern FGControls controls;


#endif // _CONTROLS_HXX


