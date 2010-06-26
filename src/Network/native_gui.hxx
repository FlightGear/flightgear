// native_gui.hxx -- FGFS external gui data export class
//
// Written by Curtis Olson, started January 2002.
//
// Copyright (C) 2002  Curtis L. Olson - http://www.flightgear.org/~curt
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


#ifndef _FG_NATIVE_GUI_HXX
#define _FG_NATIVE_GUI_HXX


#include <simgear/compiler.h>

#include "protocol.hxx"
#include "net_gui.hxx"

class FGNativeGUI : public FGProtocol {

    FGNetGUI buf;
    int length;
    
public:

    FGNativeGUI();
    ~FGNativeGUI();

    // open hailing frequencies
    bool open();

    // process work for this port
    bool process();

    // close the channel
    bool close();
};


// Helper functions which may be useful outside this class

// Populate the FGNetGUI structure from the property tree.
void FGProps2NetGUI( FGNetGUI *net );

// Update the property tree from the FGNetGUI structure.
void FGNetGUI2Props( FGNetGUI *net );


#endif // _FG_NATIVE_GUI_HXX


