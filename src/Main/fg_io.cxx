// fg_io.cxx -- higher level I/O channel management routines
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

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <simgear/compiler.h>

#include <cstdlib>             // atoi()

#include <string>
#include <algorithm>

#include <simgear/debug/logstream.hxx>
#include <simgear/io/iochannel.hxx>
#include <simgear/io/sg_file.hxx>
#include <simgear/io/sg_serial.hxx>
#include <simgear/io/sg_socket.hxx>
#include <simgear/io/sg_socket_udp.hxx>
#include <simgear/math/sg_types.hxx>
#include <simgear/timing/timestamp.hxx>
#include <simgear/misc/strutils.hxx>

#include <Network/protocol.hxx>
#include <Network/ATC-Main.hxx>
#include <Network/atlas.hxx>
#include <Network/AV400.hxx>
#include <Network/AV400Sim.hxx>
#include <Network/AV400WSim.hxx>
#include <Network/garmin.hxx>
#include <Network/httpd.hxx>
#ifdef FG_JPEG_SERVER
#  include <Network/jpg-httpd.hxx>
#endif
#include <Network/joyclient.hxx>
#include <Network/jsclient.hxx>
#include <Network/native.hxx>
#include <Network/native_ctrls.hxx>
#include <Network/native_fdm.hxx>
#include <Network/native_gui.hxx>
#include <Network/opengc.hxx>
#include <Network/nmea.hxx>
#include <Network/props.hxx>
#include <Network/pve.hxx>
#include <Network/ray.hxx>
#include <Network/rul.hxx>
#include <Network/generic.hxx>
#include <Network/HTTPClient.hxx>

#ifdef FG_HAVE_HLA
#include <Network/HLA/hla.hxx>
#endif

#include "globals.hxx"
#include "fg_io.hxx"

using std::atoi;
using std::string;


FGIO::FGIO()
{
}





FGIO::~FGIO()
{

}


// configure a port based on the config string
FGProtocol*
FGIO::parse_port_config( const string& config )
{
    SG_LOG( SG_IO, SG_INFO, "Parse I/O channel request: " << config );

    string_list tokens = simgear::strutils::split( config, "," );
    if (tokens.empty())
    {
	SG_LOG( SG_IO, SG_ALERT,
		"Port configuration error: empty config string" );
	return 0;
    }

    string protocol = tokens[0];
    SG_LOG( SG_IO, SG_INFO, "  protocol = " << protocol );

    FGProtocol *io = 0;

    try
    {
	if ( protocol == "atcsim" ) {
            FGATCMain *atcsim = new FGATCMain;
	    atcsim->set_hz( 30 );
            if ( tokens.size() != 6 ) {
                SG_LOG( SG_IO, SG_ALERT, "Usage: --atcsim=[no-]pedals,"
                        << "input0_config,input1_config,"
                        << "output0_config,output1_config,file.nas" );
		delete atcsim;
                return NULL;
            }
            if ( tokens[1] == "no-pedals" ) {
                fgSetBool( "/input/atcsim/ignore-pedal-controls", true );
            } else {
                fgSetBool( "/input/atcsim/ignore-pedal-controls", false );
            }
            atcsim->set_path_names(tokens[2], tokens[3], tokens[4], tokens[5]);
	    return atcsim;
	} else if ( protocol == "atlas" ) {
	    FGAtlas *atlas = new FGAtlas;
	    io = atlas;
	} else if ( protocol == "opengc" ) {
	    // char wait;
	    // printf("Parsed opengc\n"); cin >> wait;
	    FGOpenGC *opengc = new FGOpenGC;
	    io = opengc;
	} else if ( protocol == "AV400" ) {
	    FGAV400 *av400 = new FGAV400;
	    io = av400;
	} else if ( protocol == "AV400Sim" ) {
	    FGAV400Sim *av400sim = new FGAV400Sim;
	    io = av400sim;
        } else if ( protocol == "AV400WSimA" ) {
            FGAV400WSimA *av400wsima = new FGAV400WSimA;
            io = av400wsima;
        } else if ( protocol == "AV400WSimB" ) {
            FGAV400WSimB *av400wsimb = new FGAV400WSimB;
            io = av400wsimb;
 	} else if ( protocol == "garmin" ) {
	    FGGarmin *garmin = new FGGarmin;
	    io = garmin;
	} else if ( protocol == "httpd" ) {
	    // determine port
	    string port = tokens[1];
	    return new FGHttpd( atoi(port.c_str()) );
#ifdef FG_JPEG_SERVER
	} else if ( protocol == "jpg-httpd" ) {
	    // determine port
	    string port = tokens[1];
	    return new FGJpegHttpd( atoi(port.c_str()) );
#endif
	} else if ( protocol == "joyclient" ) {
	    FGJoyClient *joyclient = new FGJoyClient;
	    io = joyclient;
        } else if ( protocol == "jsclient" ) {
            FGJsClient *jsclient = new FGJsClient;
            io = jsclient;
	} else if ( protocol == "native" ) {
	    FGNative *native = new FGNative;
	    io = native;
	} else if ( protocol == "native-ctrls" ) {
	    FGNativeCtrls *native_ctrls = new FGNativeCtrls;
	    io = native_ctrls;
	} else if ( protocol == "native-fdm" ) {
	    FGNativeFDM *native_fdm = new FGNativeFDM;
	    io = native_fdm;
	} else if ( protocol == "native-gui" ) {
	    FGNativeGUI *net_gui = new FGNativeGUI;
	    io = net_gui;
	} else if ( protocol == "nmea" ) {
	    FGNMEA *nmea = new FGNMEA;
	    io = nmea;
	} else if ( protocol == "props" || protocol == "telnet" ) {
	    io = new FGProps( tokens );
	    return io;
	} else if ( protocol == "pve" ) {
	    FGPVE *pve = new FGPVE;
	    io = pve;
	} else if ( protocol == "ray" ) {
	    FGRAY *ray = new FGRAY;
	    io = ray;
	} else if ( protocol == "rul" ) {
	    FGRUL *rul = new FGRUL;
	    io = rul;
    } else if ( protocol == "generic" ) {
        FGGeneric *generic = new FGGeneric( tokens );
        if (!generic->getInitOk())
        {
            // failed to initialize (i.e. invalid configuration)
            delete generic;
            return NULL;
        }
        io = generic;
    } else if ( protocol == "multiplay" ) {
	    if ( tokens.size() != 5 ) {
		SG_LOG( SG_IO, SG_ALERT, "Ignoring invalid --multiplay option "
			"(4 arguments expected: --multiplay=dir,hz,hostname,port)" );
		return NULL;
	    }
	    string dir = tokens[1];
	    int rate = atoi(tokens[2].c_str());
	    string host = tokens[3];

	     short port = atoi(tokens[4].c_str());

        // multiplay used to be handled by an FGProtocol, but no longer. This code
        // retains compatability with existing command-line syntax
          fgSetInt("/sim/multiplay/tx-rate-hz", rate);
          if (dir == "in") {
            fgSetInt("/sim/multiplay/rxport", port);
            fgSetString("/sim/multiplay/rxhost", host.c_str());
          } else if (dir == "out") {
            fgSetInt("/sim/multiplay/txport", port);
            fgSetString("/sim/multiplay/txhost", host.c_str());
          }

          return NULL;
#ifdef FG_HAVE_HLA
	} else if ( protocol == "hla" ) {
	    return new FGHLA(tokens);
#endif
	} else {
	    return NULL;
	}
    }
    catch (FGProtocolConfigError& err)
    {
	SG_LOG( SG_IO, SG_ALERT, "Port configuration error: " << err.what() );
	delete io;
	return 0;
    }

    if (tokens.size() < 3) {
      SG_LOG( SG_IO, SG_ALERT, "Incompatible number of network arguments.");
      return NULL;
    }
    string medium = tokens[1];
    SG_LOG( SG_IO, SG_INFO, "  medium = " << medium );

    string direction = tokens[2];
    io->set_direction( direction );
    SG_LOG( SG_IO, SG_INFO, "  direction = " << direction );

    string hertz_str = tokens[3];
    double hertz = atof( hertz_str.c_str() );
    io->set_hz( hertz );
    SG_LOG( SG_IO, SG_INFO, "  hertz = " << hertz );

    if ( medium == "serial" ) {
        if ( tokens.size() < 5) {
          SG_LOG( SG_IO, SG_ALERT, "Incompatible number of arguments for serial communications.");
	  return NULL;
        }
	// device name
	string device = tokens[4];
	SG_LOG( SG_IO, SG_INFO, "  device = " << device );

	// baud
	string baud = tokens[5];
	SG_LOG( SG_IO, SG_INFO, "  baud = " << baud );


	SGSerial *ch = new SGSerial( device, baud );
	io->set_io_channel( ch );

        if ( protocol == "AV400WSimB" ) {
            if ( tokens.size() < 7 ) {
                SG_LOG( SG_IO, SG_ALERT, "Missing second hz for AV400WSimB.");
                return NULL;
            }
            FGAV400WSimB *fgavb = static_cast<FGAV400WSimB*>(io);
            string hz2_str = tokens[6];
            double hz2 = atof(hz2_str.c_str());
            fgavb->set_hz2(hz2);
        }
    } else if ( medium == "file" ) {
	// file name
        if ( tokens.size() < 4) {
          SG_LOG( SG_IO, SG_ALERT, "Incompatible number of arguments for file I/O.");
	  return NULL;
        }

	string file = tokens[4];
	SG_LOG( SG_IO, SG_INFO, "  file name = " << file );
        int repeat = 1;
        if (tokens.size() >= 7 && tokens[6] == "repeat") {
            if (tokens.size() >= 8) {
                repeat = atoi(tokens[7].c_str());
                FGGeneric* generic = dynamic_cast<FGGeneric*>(io);
                if (generic)
                    generic->setExitOnError(true);
            } else {
                repeat = -1;
            }
        }
	SGFile *ch = new SGFile( file, repeat );
	io->set_io_channel( ch );
    } else if ( medium == "socket" ) {
        if ( tokens.size() < 6) {
          SG_LOG( SG_IO, SG_ALERT, "Incompatible number of arguments for socket communications.");
	  return NULL;
        }
      	string hostname = tokens[4];
	string port = tokens[5];
	string style = tokens[6];

	SG_LOG( SG_IO, SG_INFO, "  hostname = " << hostname );
	SG_LOG( SG_IO, SG_INFO, "  port = " << port );
	SG_LOG( SG_IO, SG_INFO, "  style = " << style );

	io->set_io_channel( new SGSocket( hostname, port, style ) );
    }

    return io;
}


// step through the port config streams (from fgOPTIONS) and setup
// serial port channels for each
void
FGIO::init()
{
    // SG_LOG( SG_IO, SG_INFO, "I/O Channel initialization, " <<
    //         globals->get_channel_options_list()->size() << " requests." );

    _realDeltaTime = fgGetNode("/sim/time/delta-realtime-sec");

    FGProtocol *p;

    // we could almost do this in a single step except pushing a valid
    // port onto the port list copies the structure and destroys the
    // original, which closes the port and frees up the fd ... doh!!!

    // parse the configuration strings and store the results in the
    // appropriate FGIOChannel structures
    string_list::iterator i = globals->get_channel_options_list()->begin();
    string_list::iterator end = globals->get_channel_options_list()->end();
    for (; i != end; ++i ) {
      p = parse_port_config( *i );
      if (!p) {
        continue;
      }

      p->open();
      if ( !p->is_enabled() ) {
        SG_LOG( SG_IO, SG_ALERT, "I/O Channel config failed." );
        delete p;
        continue;
      }

      io_channels.push_back( p );
    } // of channel options iteration
}

void
FGIO::reinit()
{
}

// process any IO channel work
void
FGIO::update( double /* delta_time_sec */ )
{
    if (FGHTTPClient::haveInstance()) {
        FGHTTPClient::instance()->update();
    }
    
  // use wall-clock, not simulation, delta time, so that network
  // protocols update when the simulation is paused
  // see http://code.google.com/p/flightgear-bugs/issues/detail?id=125
    double delta_time_sec = _realDeltaTime->getDoubleValue();

    ProtocolVec::iterator i = io_channels.begin();
    ProtocolVec::iterator end = io_channels.end();
    for (; i != end; ++i ) {
      FGProtocol* p = *i;
      if (!p->is_enabled()) {
        continue;
      }

	    p->dec_count_down( delta_time_sec );
	    double dt = 1 / p->get_hz();
	    if ( p->get_count_down() < 0.33 * dt ) {
	      p->process();
	      p->inc_count();
	      while ( p->get_count_down() < 0.33 * dt ) {
          p->inc_count_down( dt );
	      }
	    } // of channel processing
    } // of io_channels iteration
}

void
FGIO::shutdown()
{
    FGProtocol *p;

    ProtocolVec::iterator i = io_channels.begin();
    ProtocolVec::iterator end = io_channels.end();
    for (; i != end; ++i )
    {
      p = *i;
      if ( p->is_enabled() ) {
          p->close();
      }

      delete p;
    }

  io_channels.clear();
}

void
FGIO::bind()
{
}

void
FGIO::unbind()
{
}

