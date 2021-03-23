// native_ctrls.cxx -- FGFS "Native" controls I/O class
//
// Written by Curtis Olson, started July 2001.
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


#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <simgear/debug/logstream.hxx>
#include <simgear/io/iochannel.hxx>
#include <simgear/io/lowlevel.hxx> // endian tests

#if FG_HAVE_DDS
#include <simgear/io/SGDataDistributionService.hxx>
#endif

#include <Main/fg_props.hxx>
#include <Scenery/scenery.hxx>	// ground elevation

#include "native_structs.hxx"
#include "native_ctrls.hxx"

// FreeBSD works better with this included last ... (?)
#if defined( _MSC_VER )
#  include <windows.h>
#elif defined( __MINGW32__ )
#  include <winsock2.h>
#else
#  include <netinet/in.h>	// htonl() ntohl()
#endif

// open hailing frequencies
bool FGNativeCtrls::open() {
    if ( is_enabled() ) {
        SG_LOG( SG_IO, SG_ALERT, "This shouldn't happen, but the channel "
                << "is already in use, ignoring" );
        return false;
    }

    SGIOChannel *io = get_io_channel();

#if FG_HAVE_DDS
    if ( io->get_type() == sgDDSType ) {
        SG_DDS_Topic *dds = static_cast<SG_DDS_Topic*>(io);
        dds->setup("FG_DDS_Ctrls", &FG_DDS_Ctrls_desc, sizeof(FG_DDS_Ctrls));
    }
#endif

    if ( ! io->open( get_direction() ) ) {
        SG_LOG( SG_IO, SG_ALERT, "Error opening channel communication layer." );
        return false;
    }

    set_enabled( true );

    return true;
}

// process work for this port
bool FGNativeCtrls::process() {
    SGIOChannel *io = get_io_channel();
    int length;
    char *buf;

    if ( io->get_type() == sgDDSType ) {
        buf = reinterpret_cast<char*>(&ctrls.dds);
        length = sizeof(FG_DDS_Ctrls);
    } else {
        buf = reinterpret_cast<char*>(&ctrls.net);
        length = sizeof(FGNetCtrls);
    }

    if ( get_direction() == SG_IO_OUT )
    {
        if ( io->get_type() == sgDDSType ) {
            FGProps2Ctrls( globals->get_props(), &ctrls.dds, true, true );
        } else {
            FGProps2Ctrls( globals->get_props(), &ctrls.net, true, true );
        }

        if ( ! io->write( buf, length ) ) {
            SG_LOG( SG_IO, SG_ALERT, "Error writing data." );
            return false;
        }
    }
    else if ( get_direction() == SG_IO_IN )
    {
        if ( io->get_type() == sgFileType ) {
            if ( io->read( buf, length ) == length ) {
                SG_LOG( SG_IO, SG_INFO, "Success reading data." );
                FGCtrls2Props( globals->get_props(), &ctrls.net, true, true );
            }
        } else if ( io->get_type() == sgDDSType ) {
            while ( io->read( buf, length ) == length ) {
                SG_LOG( SG_IO, SG_INFO, "Success reading data." );
                FGCtrls2Props( globals->get_props(), &ctrls.dds, true, true );
            }
        } else {
            while ( io->read( buf, length ) == length ) {
                SG_LOG( SG_IO, SG_INFO, "Success reading data." );
                FGCtrls2Props( globals->get_props(), &ctrls.net, true, true );
            }
        }
    }

    return true;
}

// close the channel
bool FGNativeCtrls::close() {
    SGIOChannel *io = get_io_channel();

    set_enabled( false );

    if ( ! io->close() ) {
        return false;
    }

    return true;
}

