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
namespace flightgear
{
    class View;
    typedef SGSharedPtr<flightgear::View> ViewPtr;
}

// Define a structure containing view information
class FGViewMgr : public SGSubsystem
{

public:

    // Constructor
    FGViewMgr( void );

    // Destructor
    ~FGViewMgr( void );

    virtual void init ();
    virtual void postinit();
    virtual void bind ();
    virtual void unbind ();
    virtual void update (double dt);
    virtual void reinit ();
    virtual void shutdown();
    
    // getters
    inline int size() const { return views.size(); }
    inline int get_current() const { return current; }
    
    flightgear::View* get_current_view();
    const flightgear::View* get_current_view() const;
    
    flightgear::View* get_view( int i );
    const flightgear::View* get_view( int i ) const;
      
    flightgear::View* next_view();
    flightgear::View* prev_view();
      
    // setters
    void clear();

    void add_view( flightgear::View * v );
    
    static const char* subsystemName() { return "view-manager"; }
private:
    simgear::TiedPropertyList _tiedProperties;

    int getView () const;
    void setView (int newview);

    bool inited;
    std::vector<SGPropertyNode_ptr> config_list;
    SGPropertyNode_ptr _viewNumberProp;
    typedef std::vector<flightgear::ViewPtr> viewer_list;
    viewer_list views;

    int current;

    SGPropertyNode_ptr current_x_offs, current_y_offs, current_z_offs;
    SGPropertyNode_ptr target_x_offs, target_y_offs, target_z_offs;
};


#endif // _VIEWMGR_HXX
