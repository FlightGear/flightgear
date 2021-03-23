// native_fdm.hxx -- FGFS "Native" flight dynamics protocal class
//
// Written by Curtis Olson, started September 2001.
//
// Copyright (C) 2001  Curtis L. Olson - http://www.flightgear.org/~curt
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


#pragma once

// Helper functions which may be useful outside this class

// Populate the FGNetFDM/FG_DDS_FDM structure from the property tree.
template<typename T>
void FGProps2FDM( SGPropertyNode *props, T *net, bool net_byte_order = true );

// Update the property tree from the FGNetFDM/FG_DDS_FDM structure.
template<typename T>
void FGFDM2Props( SGPropertyNode *props, T *net, bool net_byte_order = true );


// Populate the FGNetGUI/FG_DDS_GUI structure from the property tree.
template<typename T>
void FGProps2GUI( SGPropertyNode *props, T *net );

// Update the property tree from the FGNetGUI/FG_DDS_GUI structure.
template<typename T>
void FGGUI2Props( SGPropertyNode *props, T *net );


// Populate the FGNetCtrls/FG_DDS_Ctrls structure from the property tree.
template<typename T>
void FGProps2Ctrls( SGPropertyNode *props, T *net, bool honor_freezes, bool net_byte_order );

// Update the property tree from the FGNetCtrls/FG_DDS_Ctrls structure.
template<typename T>
void FGCtrls2Props( SGPropertyNode *props, T *net, bool honor_freezes, bool net_byte_order );
