// subsystemFactory.hxx - factory for subsystems
//
// Copyright (C) 2012 James Turner  zakalawe@mac.com
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


#ifndef FG_SUBSYSTEM_FACTORY_HXX
#define FG_SUBSYSTEM_FACTORY_HXX

#include <string>

// forward decls
class SGCommandMgr;
class SGSubsystem;

namespace flightgear
{

/**
 * create a subsystem by name, and return it.
 * Will throw an exception if the subsytem name is invalid, or
 * if the subsytem could not be created for some other reason.
 * ownership of the subsystem passes to the caller
 */    
SGSubsystem* createSubsystemByName(const std::string& name);

void registerSubsystemCommands(SGCommandMgr* cmdMgr);

} // of namespace flightgear

#endif // of FG_POSITION_INIT_HXX
