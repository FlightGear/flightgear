// viewmgr.hxx -- class for managing all the views in the flightgear world.
//
// Written by Curtis Olson, started October 2000.
//
// Copyright (C) 2000  Curtis L. Olson  - http://www.flightgear.org/~curt
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


#ifndef _VIEWMGR_HXX
#define _VIEWMGR_HXX

#include <vector>

#include <simgear/compiler.h>
#include <simgear/structure/subsystem_mgr.hxx>

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include "fg_props.hxx"
#include "viewer.hxx"

SG_USING_STD(vector);


// Define a structure containing view information
class FGViewMgr : public SGSubsystem
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
    virtual void reinit ();

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
	setView((current+1 < (int)views.size()) ? (current + 1) : 0);
	view_number->fireValueChanged();
	return views[current];
    }
    inline FGViewer *prev_view() {
	setView((0 < current) ? (current - 1) : (views.size() - 1));
	view_number->fireValueChanged();
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

    //  callbacks in manager to access viewer methods
    double getViewHeadingOffset_deg () const;
    void setViewHeadingOffset_deg (double offset);
    double getViewGoalHeadingOffset_deg () const;
    void setViewGoalHeadingOffset_deg (double offset);
    double getViewPitchOffset_deg () const;
    void setViewPitchOffset_deg (double tilt);
    double getGoalViewPitchOffset_deg () const;
    void setGoalViewRollOffset_deg (double tilt);
    double getViewRollOffset_deg () const;
    void setViewRollOffset_deg (double tilt);
    double getGoalViewRollOffset_deg () const;
    void setGoalViewPitchOffset_deg (double tilt);
    double getViewXOffset_m () const;
    void setViewXOffset_m (double x);
    double getViewYOffset_m () const;
    void setViewYOffset_m (double y);
    double getViewZOffset_m () const;
    void setViewZOffset_m (double z);
    double getViewTargetXOffset_m () const;
    void setViewTargetXOffset_m (double x);
    double getViewTargetYOffset_m () const;
    void setViewTargetYOffset_m (double y);
    double getViewTargetZOffset_m () const;
    void setViewTargetZOffset_m (double z);
    double getFOV_deg () const;
    void setFOV_deg (double fov);
    double getARM_deg () const; // Aspect Ratio Multiplier
    void setARM_deg (double fov);
    double getNear_m () const;
    void setNear_m (double near_m);
    void setViewAxisLong (double axis);
    void setViewAxisLat (double axis);
    int getView () const;
    void setView (int newview);

    SGPropertyNode_ptr view_number;
    vector<SGPropertyNode_ptr> config_list;
    typedef vector<FGViewer *> viewer_list;
    viewer_list views;

    int current;

};


#endif // _VIEWMGR_HXX
