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
    double parking_brake;
    double brake[MAX_WHEELS];
    int magnetos[MAX_ENGINES];
    bool throttle_idle;
    bool starter[MAX_ENGINES];
    bool gear_down;

    SGPropertyNode * auto_coordination;

public:

    FGControls();
    ~FGControls();

    // Implementation of FGSubsystem.
    void init ();
    void bind ();
    void unbind ();
    void update (double dt);

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
    inline double get_parking_brake() const { return parking_brake; }
    inline double get_brake(int wheel) const { return brake[wheel]; }
    inline int get_magnetos(int engine) const { return magnetos[engine]; }
    inline bool get_starter(int engine) const { return starter[engine]; }
    inline bool get_gear_down() const { return gear_down; }

    // Update functions
    void set_aileron( double pos );
    void move_aileron( double amt );
    void set_aileron_trim( double pos );
    void move_aileron_trim( double amt );
    void set_elevator( double pos );
    void move_elevator( double amt );
    void set_elevator_trim( double pos );
    void move_elevator_trim( double amt );
    void set_rudder( double pos );
    void move_rudder( double amt );
    void set_rudder_trim( double pos );
    void move_rudder_trim( double amt );
    void set_flaps( double pos );
    void move_flaps( double amt );
    void set_throttle( int engine, double pos );
    void move_throttle( int engine, double amt );
    void set_mixture( int engine, double pos );
    void move_mixture( int engine, double amt );
    void set_prop_advance( int engine, double pos );
    void move_prop_advance( int engine, double amt );
    void set_magnetos( int engine, int pos );
    void move_magnetos( int engine, int amt );
    void set_starter( int engine, bool flag );
    void set_parking_brake( double pos );
    void set_brake( int wheel, double pos );
    void move_brake( int wheel, double amt );
    void set_gear_down( bool gear );
};


#endif // _CONTROLS_HXX


