// transponder.hxx -- class to impliment a transponder
//
// Written by Roy Vegard Ovesen, started September 2004.
//
// Copyright (C) 2004  Roy Vegard Ovesen - rvovesen@tiscali.no
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

#ifndef TRANSPONDER_HXX
#define TRANSPONDER_HXX 1

#ifndef __cplusplus
# error This library requires C++
#endif

#include <Main/fg_props.hxx>

#include <simgear/structure/subsystem_mgr.hxx>


class Transponder : public SGSubsystem
{
public:
    Transponder(SGPropertyNode *node);
    ~Transponder();

    void init ();
    void update (double dt);

private:
    // Inputs
    SGPropertyNode_ptr pressureAltitudeNode;
    SGPropertyNode_ptr busPowerNode;
    SGPropertyNode_ptr serviceableNode;

    // Outputs
    SGPropertyNode_ptr idCodeNode;
    SGPropertyNode_ptr flightLevelNode;

    // Internal
    std::string _name;
    int _num;
    std::string _mode_c_altitude;
};

#endif // TRANSPONDER_HXX
