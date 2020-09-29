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

#include "config.h"

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
#include <simgear/structure/commands.hxx>

#include <Network/protocol.hxx>
#include <Network/ATC-Main.hxx>
#include <Network/atlas.hxx>
#include <Network/AV400.hxx>
#include <Network/AV400Sim.hxx>
#include <Network/AV400WSim.hxx>
#include <Network/flarm.hxx>
#include <Network/garmin.hxx>
#include <Network/igc.hxx>
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

#if FG_HAVE_HLA
#include <Network/HLA/hla.hxx>
#endif

#include "globals.hxx"
#include "fg_io.hxx"

using std::atoi;
using std::string;
using std::to_string;

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
        return nullptr;
    }
    
    return parse_port_config(tokens);
}

FGProtocol*
FGIO::parse_port_config( const string_list& tokens )
{
    const string protocol = tokens[0];
    SG_LOG( SG_IO, SG_INFO, "  protocol = " << protocol );

    FGProtocol *io = nullptr;
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
            io = new FGAtlas;
        } else if ( protocol == "opengc" ) {
            io = new FGOpenGC;
        } else if ( protocol == "AV400" ) {
            io = new FGAV400;
        } else if ( protocol == "AV400Sim" ) {
            io = new FGAV400Sim;
        } else if ( protocol == "AV400WSimA" ) {
            io = new FGAV400WSimA;
        } else if ( protocol == "AV400WSimB" ) {
            io = new FGAV400WSimB;
        } else if ( protocol == "flarm" ) {
            io = new FGFlarm();
        } else if ( protocol == "garmin" ) {
            io = new FGGarmin();
        } else if ( protocol == "igc" ) {
            io = new IGCProtocol;
        } else if ( protocol == "joyclient" ) {
            io = new FGJoyClient;
        } else if ( protocol == "jsclient" ) {
            io = new FGJsClient;
        } else if ( protocol == "native" ) {
            io = new FGNative;
        } else if ( protocol == "native-ctrls" ) {
            io = new FGNativeCtrls;
        } else if ( protocol == "native-fdm" ) {
            io = new FGNativeFDM;
        } else if ( protocol == "native-gui" ) {
            io = new FGNativeGUI;
        } else if ( protocol == "nmea" ) {
            io = new FGNMEA();
        } else if ( protocol == "props" || protocol == "telnet" ) {
            io = new FGProps( tokens );
            return io;
        } else if ( protocol == "pve" ) {
            io = new FGPVE;
        } else if ( protocol == "ray" ) {
            io = new FGRAY;
        } else if ( protocol == "rul" ) {
            io = new FGRUL;
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
            // retains compatibility with existing command-line syntax
            fgSetInt("/sim/multiplay/tx-rate-hz", rate);
            if (dir == "in") {
                fgSetInt("/sim/multiplay/rxport", port);
                fgSetString("/sim/multiplay/rxhost", host.c_str());
            } else if (dir == "out") {
                fgSetInt("/sim/multiplay/txport", port);
                fgSetString("/sim/multiplay/txhost", host.c_str());
            }

            return NULL;
        }
#if FG_HAVE_HLA
        else if ( protocol == "hla" ) {
            return new FGHLA(tokens);
        }
        else if ( protocol == "hla-local" ) {
            // This is just about to bring up some defaults
            if (tokens.size() != 2) {
                SG_LOG( SG_IO, SG_ALERT, "Ignoring invalid --hla-local option "
                        "(one argument expected: --hla-local=<federationname>" );
                return NULL;
            }
            std::vector<std::string> HLA_tokens (tokens);
            HLA_tokens.insert(HLA_tokens.begin(), "");
            HLA_tokens.insert(HLA_tokens.begin(), "60");
            HLA_tokens.insert(HLA_tokens.begin(), "bi");
            HLA_tokens.push_back("fg-local.xml");
            return new FGHLA(HLA_tokens);
        }
#endif
        else {
            return NULL;
        }
    }
    catch (FGProtocolConfigError& err)
    {
        SG_LOG( SG_IO, SG_ALERT, "Port configuration error: " << err.what() );
        delete io;
        return NULL;
    }

    if (tokens.size() < 4) {
        SG_LOG( SG_IO, SG_ALERT, "Too few arguments for network protocol. At least 3 arguments required. " <<
                "Usage: --" << protocol << "=(file|socket|serial), (in|out|bi), hertz");
        delete io;
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

    // name
    const auto name = generateName(protocol);
    io->set_name(name);
    SG_LOG(SG_IO, SG_INFO, "  name = " << name);

    if ( medium == "serial" ) {
        if ( tokens.size() < 6) {
            SG_LOG( SG_IO, SG_ALERT, "Too few arguments for serial communications. " <<
                    "Usage --" << protocol << "=serial, (in|out|bi), hertz, device, baudrate");
            delete io;
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
                delete io;
                return NULL;
            }
            FGAV400WSimB *fgavb = static_cast<FGAV400WSimB*>(io);
            string hz2_str = tokens[6];
            double hz2 = atof(hz2_str.c_str());
            fgavb->set_hz2(hz2);
        }
    } else if ( medium == "file" ) {
        // file name
        if ( tokens.size() < 5) {
            SG_LOG( SG_IO, SG_ALERT, "Too few arguments for file I/O. " <<
                    "Usage --" << protocol << "=file, (in|out), hertz, filename (,repeat)");
            delete io;
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
        if ( tokens.size() < 7) {
            SG_LOG( SG_IO, SG_ALERT, "Too few arguments for socket communications. " <<
                    "Usage --" << protocol << "=socket, (in|out|bi), hertz, hostname, port, (tcp|udp)");
            delete io;
            return NULL;
        }
        string hostname = tokens[4];
        string port = tokens[5];
        string style = tokens[6];

        SG_LOG( SG_IO, SG_INFO, "  hostname = " << hostname );
        SG_LOG( SG_IO, SG_INFO, "  port = " << port );
        SG_LOG( SG_IO, SG_INFO, "  style = " << style );

        if (hertz <= 0) {
            SG_LOG(SG_IO, SG_ALERT, "Non-Positive Hz rate may block generic I/O ");
        }

        io->set_io_channel( new SGSocket( hostname, port, style ) );
    }
    else
    {
        SG_LOG( SG_IO, SG_ALERT, "Unknown transport medium \"" << medium << "\" for \"" << protocol << "\"");
        delete io;
        return nullptr;
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

    // we could almost do this in a single step except pushing a valid
    // port onto the port list copies the structure and destroys the
    // original, which closes the port and frees up the fd ... doh!!!

    for (const auto& config : *(globals->get_channel_options_list())) {
        FGProtocol* p = add_channel(config);
        if (p) {
            addToPropertyTree(p->get_name(), config);
        }
    } // of channel options iteration
    
    auto cmdMgr = globals->get_commands();
    cmdMgr->addCommand("add-io-channel", this, &FGIO::commandAddChannel);
    cmdMgr->addCommand("remove-io-channel", this, &FGIO::commandRemoveChannel);
}

// add another I/O channel
FGProtocol* FGIO::add_channel(const string& config)
{
    // parse the configuration string and store the results in the
    // appropriate FGIOChannel structure
    FGProtocol *p = parse_port_config( config );
    if (!p)
    {
        return nullptr;
    }

    p->open();
    if ( !p->is_enabled() ) {
        SG_LOG( SG_IO, SG_ALERT, "I/O Channel config failed." );
        delete p;
        return nullptr;
    }

    io_channels.push_back( p );
    return p;
}

void
FGIO::reinit()
{
    SG_LOG(SG_IO, SG_INFO, "FGIO::reinit()");

    std::for_each(io_channels.begin(), io_channels.end(), [](FGProtocol* p) {
        SG_LOG(SG_IO, SG_INFO, "Restarting channel \"" << p->get_name() << "\"");
        p->reinit();
    });
}

// process any IO channel work
void
FGIO::update( double /* delta_time_sec */ )
{
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
    ProtocolVec::iterator i = io_channels.begin();
    ProtocolVec::iterator end = io_channels.end();
    for (; i != end; ++i )
    {
        FGProtocol *p = *i;
        if ( p->is_enabled() ) {
            p->close();
        }
        SG_LOG(SG_IO, SG_INFO, "Shutting down channel \"" << p->get_name() << "\"");

        delete p;
    }

    io_channels.clear();
    
    auto cmdMgr = globals->get_commands();
    cmdMgr->removeCommand("add-io-channel");
    cmdMgr->removeCommand("remove-io-channel");
}

void
FGIO::bind()
{
}

void
FGIO::unbind()
{
}

bool FGIO::isMultiplayerRequested()
{
    // launcher sets these properties directly, as does the in-sim dialog
    std::string txAddress = fgGetString("/sim/multiplay/txhost");
    if (!txAddress.empty()) return true;
    
    // check the channel options list for a multiplay setting - this
    // is easier than checking the raw Options arguments, but works before
    // this subsytem is actually created.
    auto channels = globals->get_channel_options_list();
    if (!channels)
        return false; // happens running tests
    
    auto it = std::find_if(channels->begin(), channels->end(),
                           [](const std::string& channelOption)
                           { return (channelOption.find("multiplay") == 0); });
    return it != channels->end();
}

bool FGIO::commandAddChannel(const SGPropertyNode * arg, SGPropertyNode * root)
{
    if (!arg->hasChild("config")) {
        SG_LOG(SG_NETWORK, SG_WARN, "add-io-channel: missing 'config' argument");
        return false;
    }

    string name = arg->getStringValue("name");
    const string config = arg->getStringValue("config");
    auto protocol = add_channel(config);
    if (!protocol) {
        SG_LOG(SG_NETWORK, SG_WARN, "add-io-channel: adding channel failed");
        return false;
    }
    
    if (!name.empty()) {
        const string validName = simgear::strutils::makeStringSafeForPropertyName(name);
        if (name.compare(validName) != 0) {
            SG_LOG(SG_IO, SG_WARN, "add-io-channel: replaced illegal characters: " << name << " -> " << validName);
        }
        if (!validName.empty()) {
            auto it = std::find_if(io_channels.begin(), io_channels.end(), [&validName](const FGProtocol* proto) {
                return proto->get_name() == validName;
            });

            if (it != io_channels.end()) {
                SG_LOG(SG_IO, SG_WARN, "add-io-channel: channel name \"" << validName << "\" already exists, using " << protocol->get_name());
            } else {
                // set custom name instead of auto-generated name:
                SG_LOG(SG_IO, SG_INFO, "add-io-channel: setting name to \"" << validName << "\"");
                protocol->set_name(validName);
            }
        }
    }
    // add entry to /io/channels/<name>
    addToPropertyTree(protocol->get_name(), config);

    return true;
}

bool FGIO::commandRemoveChannel(const SGPropertyNode * arg, SGPropertyNode * root)
{
    if (!arg->hasChild("name")) {
        SG_LOG(SG_NETWORK, SG_WARN, "remove-io-channel: missing 'name' argument");
    }
    
    const string name = arg->getStringValue("name");
    auto it = find_if(io_channels.begin(), io_channels.end(),
                      [name](const FGProtocol* proto)
                      { return proto->get_name() == name; });
    if (it == io_channels.end()) {
        SG_LOG(SG_NETWORK, SG_WARN, "remove-io-channel: no channel with name:" + name);
        return false;
    }

    removeFromPropertyTree(name);

    FGProtocol* p = *it;
    if (p->is_enabled()) {
        p->close();
    }
    delete p;
    io_channels.erase(it);
    return true;
}

string
FGIO::generateName(const string protocol)
{
    string name;
    // Find first unused name:
    for (int i = 1; i < 1000; i++) {
        name = protocol + "-" + to_string(i);

        auto it = find_if(io_channels.begin(), io_channels.end(),
                          [name](const FGProtocol* proto) { return proto->get_name() == name; });
        if (it == io_channels.end()) {
            break;
        }
    }
    return name;
}

void FGIO::addToPropertyTree(const string name, const string config)
{
    auto channelNode = fgGetNode("/io/channels/" + name, true);
    channelNode->setStringValue("config", config);
    channelNode->setStringValue("name", name);
}

void FGIO::removeFromPropertyTree(const string name)
{
    auto node = fgGetNode("/io/channels");
    if (node->hasChild(name)) {
        node->removeChild(name);
    }
}


// Register the subsystem.
SGSubsystemMgr::Registrant<FGIO> registrantFGIO;
