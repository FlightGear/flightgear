// httpd.hxx -- a http daemon subsystem based on Mongoose http
//
// Written by Torsten Dreyer, started April 2014.
//
// Copyright (C) 2014  Torsten Dreyer
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

#ifndef FG_HTTPD_HXX
#define FG_HTTPD_HXX

#include <simgear/structure/subsystem_mgr.hxx>
#include <simgear/props/props.hxx>


namespace flightgear {

namespace http {

extern const char * PROPERTY_ROOT;

class FGHttpd : public SGSubsystem
{
public:
    // Subsystem identification.
    static const char* staticSubsystemClassId() { return "httpd"; }

    static FGHttpd * createInstance( SGPropertyNode_ptr configNode );
};

} // namespace http

} // namespace flightgear

#endif // FG_HTTPD_HXX


