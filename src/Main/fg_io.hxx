// fg_io.hxx -- Higher level I/O management routines
//
// Written by Curtis Olson, started November 1999.
//
// Copyright (C) 1999  Curtis L. Olson - http://www.flightgear.org/~curt
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


#ifndef _FG_IO_HXX
#define _FG_IO_HXX


#include <simgear/compiler.h>
#include <simgear/structure/subsystem_mgr.hxx>
#include <simgear/props/props.hxx>

#include <vector>
#include <string>

class FGProtocol;

class FGIO : public SGSubsystem
{
public:
    FGIO();
    ~FGIO();

    // Subsystem API.
    void bind() override;
    void init() override;
    void reinit() override;
    void shutdown() override;
    void unbind() override;
    void update(double dt) override;

    // Subsystem identification.
    static const char* staticSubsystemClassId() { return "io"; }

    /**
     * helper to determine early in startup, if MP will be used.
     * This information is needed in the position-init code, to adjust the
     * start position off active runways.
     */
    static bool isMultiplayerRequested();

private:
    void add_channel(const std::string& config);
    FGProtocol* parse_port_config( const std::string& cfgstr );

private:
    // define the global I/O channel list
    //io_container global_io_list;

    typedef std::vector< FGProtocol* > ProtocolVec;
    ProtocolVec io_channels;

    SGPropertyNode_ptr _realDeltaTime;
};

#endif // _FG_IO_HXX
