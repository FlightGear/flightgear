// autopilotgroup.hxx - an even more flexible, generic way to build autopilots
//
// Written by Torsten Dreyer
// Based heavily on work created by Curtis Olson, started January 2004.
//
// Copyright (C) 2004  Curtis L. Olson  - http://www.flightgear.org/~curt
// Copyright (C) 2010  Torsten Dreyer - Torsten (at) t3r (dot) de
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

#ifndef _XMLAUTO_HXX
#define _XMLAUTO_HXX 1

/**
 * @brief Model an autopilot system by implementing a SGSubsystemGroup
 * 
 */
class FGXMLAutopilotGroup : public SGSubsystemGroup
{
public:
    static FGXMLAutopilotGroup * createInstance(const std::string& nodeName);
    void addAutopilotFromFile( const std::string & name, SGPropertyNode_ptr apNode, const char * path );
    virtual void addAutopilot( const std::string & name, SGPropertyNode_ptr apNode, SGPropertyNode_ptr config ) = 0;
    virtual void removeAutopilot( const std::string & name ) = 0;
protected:
    FGXMLAutopilotGroup() : SGSubsystemGroup() {}

};

#endif // _XMLAUTO_HXX
