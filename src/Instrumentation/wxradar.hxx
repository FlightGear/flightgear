// Wx Radar background texture
//
// Written by Harald JOHNSEN, started May 2005.
//
// Copyright (C) 2005  Harald JOHNSEN - hjohnsen@evc.net
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
// Foundation, 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA
//
//

#ifndef _INST_WXRADAR_HXX
#define _INST_WXRADAR_HXX

#include <simgear/props/props.hxx>
#include <simgear/structure/subsystem_mgr.hxx>

class ssgTexture;
class FGODGauge;

class wxRadarBg : public SGSubsystem {


public:

    wxRadarBg ( SGPropertyNode *node );
    wxRadarBg ();
    virtual ~wxRadarBg ();

    virtual void init ();
    virtual void update (double dt);

private:

    string name;
    int num;

    SGPropertyNode_ptr _serviceable_node;
    SGPropertyNode_ptr _Instrument;
    ssgTexture *resultTexture;
    ssgTexture *wxEcho;
    string last_switchKnob;
    bool sim_init_done;
    FGODGauge *odg;
};

#endif // _INST_WXRADAR_HXX
