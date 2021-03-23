// native_gui.cxx -- FGFS external gui data export class
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

// TODO FIXME Module still contains lots of "static SGPropertyNode"s below.

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <simgear/debug/logstream.hxx>
#include <simgear/io/lowlevel.hxx> // endian tests
#include <simgear/io/iochannel.hxx>
#include <simgear/timing/sg_time.hxx>

#if FG_HAVE_DDS
#include <simgear/io/SGDataDistributionService.hxx>
#endif

#include <Main/fg_props.hxx>
#include <Main/globals.hxx>
#include <Scenery/scenery.hxx>
#include <FDM/flightProperties.hxx>

#include "native_structs.hxx"
#include "native_gui.hxx"

// FreeBSD works better with this included last ... (?)
#if defined( _MSC_VER )
#  include <windows.h>
#elif defined( __MINGW32__ )
#  include <winsock2.h>
#else
#  include <netinet/in.h>	// htonl() ntohl()
#endif

// open hailing frequencies
bool FGNativeGUI::open() {
    if ( is_enabled() ) {
        SG_LOG( SG_IO, SG_ALERT, "This shouldn't happen, but the channel "
                << "is already in use, ignoring" );
        return false;
    }

    SGIOChannel *io = get_io_channel();

#if FG_HAVE_DDS
    if ( io->get_type() == sgDDSType ) {
        SG_DDS_Topic *dds = static_cast<SG_DDS_Topic*>(io);
        dds->setup("FG_DDS_GUI" , &FG_DDS_GUI_desc, sizeof (FG_DDS_GUI));
    }
#endif

    if ( ! io->open( get_direction() ) ) {
        SG_LOG( SG_IO, SG_ALERT, "Error opening channel communication layer." );
        return false;
    }

    set_enabled( true );

    fgSetDouble("/position/sea-level-radius-ft", SG_EQUATORIAL_RADIUS_FT);
    return true;
}

// process work for this port
bool FGNativeGUI::process() {
    SGIOChannel *io = get_io_channel();
    int length;
    char *buf;

    if ( io->get_type() == sgDDSType ) {
        buf = reinterpret_cast<char*>(&gui.dds);
        length = sizeof(FG_DDS_GUI);
    } else {
        buf = reinterpret_cast<char*>(&gui.net);
        length = sizeof(FGNetGUI);
    }

    if ( get_direction() == SG_IO_OUT ) {
        // cout << "size of fdm_state = " << length << endl;
        if ( io->get_type() == sgDDSType ) {
            FGProps2GUI( globals->get_props(), &gui.dds );
        } else {
            FGProps2GUI( globals->get_props(), &gui.net );
        }

        if ( ! io->write( buf, length ) ) {
            SG_LOG( SG_IO, SG_ALERT, "Error writing data." );
            return false;
        }
    } else if ( get_direction() == SG_IO_IN ) {
        if ( io->get_type() == sgFileType ) {
            if ( io->read( buf, length ) == length ) {
                SG_LOG( SG_IO, SG_DEBUG, "Success reading data." );
                FGGUI2Props( globals->get_props(), &gui.net );
            }
        } if ( io->get_type() == sgDDSType ) {
            while ( io->read( buf, length ) == length ) {
                SG_LOG( SG_IO, SG_DEBUG, "Success reading data." );
                FGGUI2Props( globals->get_props(), &gui.dds );
            }
        } else {
            while ( io->read( buf, length ) == length ) {
                SG_LOG( SG_IO, SG_DEBUG, "Success reading data." );
                FGGUI2Props( globals->get_props(), &gui.net );
            }
        }
    }

    return true;
}

// close the channel
bool FGNativeGUI::close() {
    SGIOChannel *io = get_io_channel();

    set_enabled( false );

    if ( ! io->close() ) {
        return false;
    }

    return true;
}
