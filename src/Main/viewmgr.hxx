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
#include <simgear/props/props.hxx>
#include <simgear/props/tiedpropertylist.hxx>
#include <simgear/math/SGMath.hxx>

// forward decls
class FGViewer;
class SGSoundMgr;
typedef SGSharedPtr<FGViewer> FGViewerPtr;

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
    
    FGViewer *get_current_view();
    const FGViewer *get_current_view() const;
    
    FGViewer *get_view( int i ); 
    const FGViewer *get_view( int i ) const;
      
    FGViewer *next_view();
    FGViewer *prev_view();
      
    // setters
    void clear();

    void add_view( FGViewer * v );
    
private:
    void do_bind();

    simgear::TiedPropertyList _tiedProperties;

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

// quaternion accessors, for debugging:
    double getCurrentViewOrientation_w() const;
    double getCurrentViewOrientation_x() const;
    double getCurrentViewOrientation_y() const;
    double getCurrentViewOrientation_z() const;
    double getCurrentViewOrOffset_w() const;
    double getCurrentViewOrOffset_x() const;
    double getCurrentViewOrOffset_y() const;
    double getCurrentViewOrOffset_z() const;
    double getCurrentViewFrame_w() const;
    double getCurrentViewFrame_x() const;
    double getCurrentViewFrame_y() const;
    double getCurrentViewFrame_z() const;

    bool stationary () const;

    // copies current offset settings to current-view path...
    void copyToCurrent ();
    
    bool inited;
    SGPropertyNode_ptr view_number;
    std::vector<SGPropertyNode_ptr> config_list;
    typedef std::vector<FGViewerPtr> viewer_list;
    viewer_list views;
    SGVec3d abs_viewer_position;

    int current;
    SGQuatd current_view_orientation, current_view_or_offset;

    SGSoundMgr *smgr;

};

// This takes the conventional aviation XYZ body system 
// i.e.  x=forward, y=starboard, z=bottom
// which is widely used in FGFS
// and rotates it into the OpenGL camera system 
// i.e. Xprime=starboard, Yprime=top, Zprime=aft.
inline const SGQuatd fsb2sta()
{
    return SGQuatd(-0.5, -0.5, 0.5, 0.5);
}

#endif // _VIEWMGR_HXX
