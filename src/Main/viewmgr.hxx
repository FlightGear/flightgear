// viewmgr.hxx -- class for managing all the views in the flightgear world.
//
// Written by Curtis Olson, started October 2000.
//
// Copyright (C) 2000  Curtis L. Olson  - curt@flightgear.org
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


#ifndef _VIEWMGR_HXX
#define _VIEWMGR_HXX


#ifndef __cplusplus                                                          
# error This library requires C++
#endif                                   


#include <simgear/compiler.h>

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <vector>

#include "fgfs.hxx"
#include "viewer.hxx"

SG_USING_STD(vector);


// Define a structure containing view information
class FGViewMgr : public FGSubsystem
{

public:

    // Constructor
    FGViewMgr( void );

    // Destructor
    ~FGViewMgr( void );

    virtual void init ();
    virtual void bind ();
    virtual void unbind ();
    virtual void update (double dt);

    // getters
    inline int size() const { return views.size(); }
    inline int get_current() const { return current; }
    inline FGViewer *get_current_view() {
	if ( current < (int)views.size() ) {
	    return views[current];
	} else {
	    return NULL;
	}
    }
    inline const FGViewer *get_current_view() const {
	if ( current < (int)views.size() ) {
	    return views[current];
	} else {
	    return NULL;
	}
    }
    inline FGViewer *get_view( int i ) {
	if ( i < 0 ) { i = 0; }
	if ( i >= (int)views.size() ) { i = views.size() - 1; }
	return views[i];
    }
    inline const FGViewer *get_view( int i ) const {
	if ( i < 0 ) { i = 0; }
	if ( i >= (int)views.size() ) { i = views.size() - 1; }
	return views[i];
    }
    inline FGViewer *next_view() {
	++current;
	if ( current >= (int)views.size() ) {
	    current = 0;
	}
        copyToCurrent();
	return views[current];
    }
    inline FGViewer *prev_view() {
	--current;
	if ( current < 0 ) {
	    current = views.size() - 1;
	}
	return views[current];
    }

    // setters
    inline void clear() { views.clear(); }
    inline void set_view( const int v ) { current = v; }
    inline void add_view( FGViewer * v ) {
	views.push_back(v);
        v->init();
    }
    // copies current offset settings to current-view path...
    void copyToCurrent ();

private:

    double axis_long;
    double axis_lat;

    void do_axes ();

    double getViewOffset_deg () const;
    void setViewOffset_deg (double offset);
    double getGoalViewOffset_deg () const;
    void setGoalViewOffset_deg (double offset);
    double getViewTilt_deg () const;
    void setViewTilt_deg (double tilt);
    double getGoalViewTilt_deg () const;
    void setGoalViewTilt_deg (double tilt);
    double getPilotXOffset_m () const;
    void setPilotXOffset_m (double x);
    double getPilotYOffset_m () const;
    void setPilotYOffset_m (double y);
    double getPilotZOffset_m () const;
    void setPilotZOffset_m (double z);
    double getFOV_deg () const;
    void setFOV_deg (double fov);
    double getNear_m () const;
    void setNear_m (double near_m);
    void setViewAxisLong (double axis);
    void setViewAxisLat (double axis);

    typedef vector < FGViewer * > viewer_list;
    viewer_list views;

    int current;

};


#endif // _VIEWMGR_HXX
