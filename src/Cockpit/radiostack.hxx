// radiostack.hxx -- class to manage an instance of the radio stack
//
// Written by Curtis Olson, started April 2000.
//
// Copyright (C) 2000  Curtis L. Olson - curt@flightgear.org
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


#ifndef _FG_RADIOSTACK_HXX
#define _FG_RADIOSTACK_HXX


#include <Main/fg_props.hxx>

#include <simgear/compiler.h>
#include <simgear/structure/subsystem_mgr.hxx>
#include <simgear/math/interpolater.hxx>
#include <simgear/timing/timestamp.hxx>

#include <Navaids/navlist.hxx>
#include <Sound/beacon.hxx>
#include <Sound/morse.hxx>

#include "dme.hxx"
#include "kr_87.hxx"            // ADF
#include "kt_70.hxx"            // Transponder
#include "marker_beacon.hxx"
#include "navcom.hxx"


class FGRadioStack : public SGSubsystem
{
    FGDME dme;
    //FGKR_87 adf;                // King KR 87 Digital ADF model
    FGKT_70 xponder;            // Bendix/King KT 70 Panel-Mounted Transponder
    FGMarkerBeacon beacon;
    FGNavCom navcom1;
    FGNavCom navcom2;

public:

    FGRadioStack();
    ~FGRadioStack();

    void init ();
    void bind ();
    void unbind ();
    void update (double dt);

    // Update nav/adf radios based on current postition
    void search ();

    inline FGDME *get_dme() { return &dme; }
    inline FGNavCom *get_navcom1() { return &navcom1; }
    inline FGNavCom *get_navcom2() { return &navcom2; }
};


extern FGRadioStack *current_radiostack;

#endif // _FG_RADIOSTACK_HXX
