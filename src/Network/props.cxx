// \file props.cxx
// Property server class.
//
// Written by Curtis Olson, started September 2000.
// Modified by Bernie Bright, May 2002.
// Modified by Jean-Paul Anceaux, Dec 2015.
//
// Copyright (C) 2000  Curtis L. Olson - http://www.flightgear.org/~curt
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

#include <simgear/compiler.h>
#include <simgear/debug/logstream.hxx>
#include <simgear/structure/commands.hxx>
#include <simgear/misc/strutils.hxx>
#include <simgear/props/props.hxx>
#include <simgear/props/props_io.hxx>
#include <simgear/structure/exception.hxx>

#include <sstream>
#include <iostream>
#include <errno.h>
#include <algorithm>
#include <functional>

#include <Main/globals.hxx>
#include <Viewer/viewmgr.hxx>
#include <Scripting/NasalSys.hxx>

#include <simgear/io/sg_netChat.hxx>

#include <simgear/misc/strutils.hxx>

#include "props.hxx"

#include <map>
#include <vector>
#include <string>
#include <set>


using std::stringstream;
using std::ends;

using std::cout;
using std::endl;

/**
 * Props connection class.
 * This class represents a connection to props client.
 */
class FGProps::PropsChannel : public simgear::NetChat, public SGPropertyChangeListener
{
    simgear::NetBuffer buffer;

    /**
     * Current property node name.
     */
    string path= "/";

    enum Mode {
        PROMPT,
        DATA
    };
    Mode mode = PROMPT;

public:
    /**
     * Constructor.
     */
    PropsChannel(FGProps* owner);
    ~PropsChannel();

    /**
     * Append incoming data to our request buffer.
     *
     * @param s Character string to append to buffer
     * @param n Number of characters to append.
     */
    void collectIncomingData( const char* s, int n ) override;

    /**
     * Process a complete request from the props client.
     */
    void foundTerminator() override;

    // callback for registered listeners (subscriptions)
    void valueChanged(SGPropertyNode *node) override;

    void publishDirtySubscriptions();
private:

    typedef string_list ParameterList;

    inline void node_not_found_error( const string& s ) const {
        throw "node '" + s + "' not found";
    }

    void error(std::string msg) {  // wrapper: prints errors to STDERR and to the telnet client
	    push( msg.c_str() ); push( getTerminator() );
	    SG_LOG(SG_NETWORK, SG_ALERT, __FILE__<<"@" << __LINE__ <<" in " << __FUNCTION__ <<":"<< msg.c_str() << std::endl);
    }


    bool check_args(const ParameterList &tok, const unsigned int num, const char* func) {
	    if (tok.size()-1 < num) {
		    error(string("Error:Wrong argument count for:")+string(func) );
		    return false;
	    }
	    return true;
    }

    std::vector<SGPropertyNode_ptr> _listeners;
    std::set<SGPropertyNode_ptr> _dirtySubscriptions;

    typedef void (PropsChannel::*TelnetCallback) (const ParameterList&);
    std::map<std::string, TelnetCallback> callback_map;

    // callback implementations:
    void subscribe(const ParameterList &p);
    void unsubscribe(const ParameterList &p);
    void beginNasal(const ParameterList &p);

    FGProps* _owner = nullptr;
    bool _colletingNasal = false;
};

/**
 *
 */
FGProps::PropsChannel::PropsChannel(FGProps* owner)
    : buffer(8192)
    , _owner(owner)
{
    setTerminator( "\r\n" );
    callback_map["subscribe"] 	= 	&PropsChannel::subscribe;
    callback_map["unsubscribe"]	=	&PropsChannel::unsubscribe;
    callback_map["nasal"] =         &PropsChannel::beginNasal;
}

FGProps::PropsChannel::~PropsChannel()
{
  // clean up all registered listeners
    for (SGPropertyNode_ptr l : _listeners) {
        l->removeChangeListener(this);
    }

    _owner->removeChannel(this);
}

void FGProps::PropsChannel::subscribe(const ParameterList &param) {
	if (! check_args(param,1,"subscribe")) return;

	std::string command = param[0];
	const char* p = param[1].c_str();
	if (!p) return;

  //SG_LOG(SG_GENERAL, SG_ALERT, p << std::endl);
  push( command.c_str() ); push ( " " );
  push( p );
  push( getTerminator() );

  SGPropertyNode *n = globals->get_props()->getNode( p,true );
	if ( n->isTied() ) {
		error("Error:Tied properties cannot register listeners");
		return;
	}

 	if (n) {
    n->addChangeListener( this );
	 _listeners.push_back( n ); // housekeeping, save for deletion in dtor later on
  } else {
		 error("listener could not be added");
  }
}

void FGProps::PropsChannel::unsubscribe(const ParameterList &param) {
  if (!check_args(param,1,"unsubscribe")) return;

  try {
   SGPropertyNode *n = globals->get_props()->getNode( param[1].c_str() );
   if (n) {
       n->removeChangeListener( this );
       _dirtySubscriptions.erase(n);
   }
  } catch (sg_exception&) {
	  error("Error:Listener could not be removed");
  }
}

void FGProps::PropsChannel::beginNasal(const ParameterList &param)
{
    std::string eofMarker = "##EOF##";
    if (param.size() > 1) {
        if ((param.at(1) == "eof") && (param.size() >= 3)) {
            eofMarker = param.at(2);
        }
    } // of optional argument parsing
    
    _colletingNasal = true;
    setTerminator(eofMarker);
}

//TODO: provide support for different types of subscriptions MODES ? (child added/removed, thesholds, min/max)
  void FGProps::PropsChannel::valueChanged(SGPropertyNode* ptr)
  {
      _dirtySubscriptions.insert(ptr);
  }

  void FGProps::PropsChannel::publishDirtySubscriptions()
  {
      if (_dirtySubscriptions.empty())
          return; // nothing to send

      std::stringstream response;
      for (auto sub : _dirtySubscriptions) {
          response << sub->getPath(true) << "=" <<  sub->getStringValue() << getTerminator();
      }

      push(response.str().c_str());
      _dirtySubscriptions.clear();
  }

/**
 *
 */
void
FGProps::PropsChannel::collectIncomingData( const char* s, int n )
{
    buffer.append( s, n );
}

// return a human readable form of the value "type"
static string
getValueTypeString( const SGPropertyNode *node )
{
    using namespace simgear;

    string result;

    if ( node == NULL )
    {
        return "unknown";
    }

    props::Type type = node->getType();
    if ( type == props::UNSPECIFIED ) {
        result = "unspecified";
    } else if ( type == props::NONE ) {
        result = "none";
    } else if ( type == props::BOOL ) {
        result = "bool";
    } else if ( type == props::INT ) {
        result = "int";
    } else if ( type == props::LONG ) {
        result = "long";
    } else if ( type == props::FLOAT ) {
        result = "float";
    } else if ( type == props::DOUBLE ) {
        result = "double";
    } else if ( type == props::STRING ) {
        result = "string";
    }

    return result;
}

/**
 * We have a command.
 *
 */
void
FGProps::PropsChannel::foundTerminator()
{
    if (_colletingNasal) {
        std::string nasalSource = buffer.getData(); // make a copy
        _colletingNasal = false;
        setTerminator("\r\n");
        buffer.remove(); // safe since we copied the source above

        if (globals->get_props()->getBoolValue("sim/secure-flag", true) == true) {
            SG_LOG(SG_IO, SG_ALERT, "Telnet connection trying to run Nasal, blocked it.\n"
                   "Run the simulator with --allow-nasal-from-sockets to allow this.");
            error("Simulator running in secure mode, Nasal execution blocked.");
        } else {
            auto nasal = globals->get_subsystem<FGNasalSys>();
            if (nasal) {
                std::string errors, output;
                bool ok = nasal->parseAndRunWithOutput(nasalSource, output, errors);
                if (!ok) {
                    error("Nasal error" + errors);
                } else if (!output.empty()) {
                    // success and we have output: push it
                    push(output.c_str());
                }
            }
        }
        
        return;
    }
    
    const char* cmd = buffer.getData();
    SG_LOG( SG_IO, SG_DEBUG, "processing command = \"" << cmd << "\"" );

    ParameterList tokens = simgear::strutils::split( cmd );

    SGPropertyNode* node = globals->get_props()->getNode( path.c_str() );

    try {
        if (!tokens.empty()) {
            string command = tokens[0];

            if (command == "ls") {
                SGPropertyNode* dir = node;
                if (tokens.size() == 2) {
                    if (tokens[1][0] == '/') {
                        dir = globals->get_props()->getNode( tokens[1].c_str() );
                    } else {
                        string s = path;
                        s += "/";
                        s += tokens[1];
                        dir = globals->get_props()->getNode( s.c_str() );
                    }

                    if (dir == 0) {
                        node_not_found_error( tokens[1] );
                    }
                }

                for (int i = 0; i < dir->nChildren(); i++) {
                    SGPropertyNode * child = dir->getChild(i);
                    string line = child->getDisplayName(true);

                    if ( child->nChildren() > 0 ) {
                        line += "/";
                    } else {
                        if (mode == PROMPT) {
                            string value = child->getStringValue();
                            line += " =\t'" + value + "'\t(";
                            line += getValueTypeString( child );
                            line += ")";
                        }
                    }

                    line += getTerminator();
                    push( line.c_str() );
                }
            } else if ( command == "dump" ) {
                stringstream buf;
                if ( tokens.size() <= 1 ) {
                    writeProperties( buf, node );
                    buf << ends; // null terminate the string
                    push( buf.str().c_str() );
                    push( getTerminator() );
                } else {
                    SGPropertyNode *child = node->getNode( tokens[1].c_str() );
                    if ( child ) {
                        writeProperties ( buf, child );
                        buf << ends; // null terminate the string
                        push( buf.str().c_str() );
                        push( getTerminator() );
                    } else {
                        node_not_found_error( tokens[1] );
                    }
                }
            }
            else if ( command == "cd" ) {
                if (tokens.size() == 2) {
                    SGPropertyNode* child = node->getNode( tokens[1].c_str() );
                    if ( child ) {
                        node = child;
                        path = node->getPath();
                    } else {
                        node_not_found_error( tokens[1] );
                    }
                }
            } else if ( command == "pwd" ) {
                string pwd = node->getPath();
                if (pwd.empty()) {
                    pwd = "/";
                }

                push( pwd.c_str() );
                push( getTerminator() );
            } else if ( command == "get" || command == "show" ) {
                if ( tokens.size() == 2 ) {
                    string tmp;
                    string value = node->getStringValue ( tokens[1].c_str(), "" );
                    if ( mode == PROMPT ) {
                        tmp = tokens[1];
                        tmp += " = '";
                        tmp += value;
                        tmp += "' (";
                        tmp += getValueTypeString(
                                         node->getNode( tokens[1].c_str() ) );
                        tmp += ")";
                    } else {
                        tmp = value;
                    }
                    push( tmp.c_str() );
                    push( getTerminator() );
                }
            } else if ( command == "set" ) {
                if ( tokens.size() >= 2 ) {
                    string value, tmp;
                    for (unsigned int i = 2; i < tokens.size(); i++) {
                        if (i > 2)
                            value += " ";
                        value += tokens[i];
                    }
                    node->getNode( tokens[1].c_str(), true )
                        ->setStringValue(value.c_str());

                    if ( mode == PROMPT ) {
                        // now fetch and write out the new value as confirmation
                        // of the change
                        value = node->getStringValue ( tokens[1].c_str(), "" );
                        tmp = tokens[1] + " = '" + value + "' (";
                        tmp += getValueTypeString( node->getNode( tokens[1].c_str() ) );
                        tmp += ")";
                        push( tmp.c_str() );
                        push( getTerminator() );
                    }
                }
            } else if ( command == "reinit" ) {
                if ( tokens.size() == 2 ) {
                    string tmp;
                    SGPropertyNode args;
                    for ( unsigned int i = 1; i < tokens.size(); ++i ) {
                        cout << "props: adding subsystem = " << tokens[i] << endl;
                        SGPropertyNode *node = args.getNode("subsystem", i-1, true);
                        node->setStringValue( tokens[i].c_str() );
                    }
                    if ( !globals->get_commands()
                             ->execute( "reinit", &args, nullptr) )
                    {
                        SG_LOG( SG_NETWORK, SG_ALERT,
                                "Command " << tokens[1] << " failed.");
                        if ( mode == PROMPT ) {
                            tmp += "*failed*";
                            push( tmp.c_str() );
                            push( getTerminator() );
                        }
                    } else {
                        if ( mode == PROMPT ) {
                            tmp += "<completed>";
                            push( tmp.c_str() );
                            push( getTerminator() );
                        }
                    }
                }
            } else if ( command == "run" ) {
                string tmp;
                if ( tokens.size() >= 2 ) {
                    SGPropertyNode_ptr args(new SGPropertyNode);
                    if ( tokens[1] == "reinit" ) {
                        for ( unsigned int i = 2; i < tokens.size(); ++i ) {
                            cout << "props: adding subsystem = " << tokens[i]
                                 << endl;
                            SGPropertyNode *node
                                = args->getNode("subsystem", i-2, true);
                            node->setStringValue( tokens[i].c_str() );
                        }
                    } else if ( tokens[1] == "set-sea-level-air-temp-degc" ) {
                        for ( unsigned int i = 2; i < tokens.size(); ++i ) {
                            cout << "props: set-sl command = " << tokens[i]
                                 << endl;
                            SGPropertyNode *node
                                = args->getNode("temp-degc", i-2, true);
                            node->setStringValue( tokens[i].c_str() );
                        }
                    } else if ( tokens[1] == "set-outside-air-temp-degc" ) {
                        for ( unsigned int i = 2; i < tokens.size(); ++i ) {
                            cout << "props: set-oat command = " << tokens[i]
                                 << endl;
                            SGPropertyNode *node
                                = args->getNode("temp-degc", i-2, true);
                            node->setStringValue( tokens[i].c_str() );
                        }
                    } else if ( tokens[1] == "timeofday" ) {
                        for ( unsigned int i = 2; i < tokens.size(); ++i ) {
                            cout << "props: time of day command = " << tokens[i]
                                 << endl;
                            SGPropertyNode *node
                                = args->getNode("timeofday", i-2, true);
                            node->setStringValue( tokens[i].c_str() );
                        }
                    } else if ( tokens[1] == "play-audio-message" ) {
                        if ( tokens.size() == 4 ) {
                            cout << "props: play audio message = " << tokens[2]
                                 << " " << tokens[3] << endl;
                            SGPropertyNode *node;
                            node = args->getNode("path", 0, true);
                            node->setStringValue( tokens[2].c_str() );
                            node = args->getNode("file", 0, true);
                            node->setStringValue( tokens[3].c_str() );
                       }
                    } else {
                        // generic parsing
                        for ( unsigned int i = 2; i < tokens.size(); ++i ) {
                            const auto pieces = simgear::strutils::split(tokens.at(i), "=");
                            if (pieces.size() != 2) {
                                SG_LOG(SG_NETWORK, SG_WARN, "malformed argument to Props protocol run:" << tokens.at(i));
                                continue;
                            }
                            
                            SGPropertyNode_ptr node= args->getNode(pieces.at(0), 0, true);
                            node->setStringValue(pieces.at(1));
                        }
                    }
                    
                    if ( !globals->get_commands() ->execute(tokens[1].c_str(), args, nullptr) )
                    {
                        SG_LOG( SG_NETWORK, SG_ALERT,
                                "Command " << tokens[1] << " failed.");
                        if ( mode == PROMPT ) {
                            tmp += "*failed*";
                            push( tmp.c_str() );
                            push( getTerminator() );
                        }
                    } else {
                        if ( mode == PROMPT ) {
                            tmp += "<completed>";
                            push( tmp.c_str() );
                            push( getTerminator() );
                        }
                    }
                } else {
                    if ( mode == PROMPT ) {
                        tmp += "no command specified";
                        push( tmp.c_str() );
                        push( getTerminator() );
                    }
                }
            } else if ( command == "quit" || command == "exit" ) {
                close();
                shouldDelete();
                return;
            } else if ( command == "data" ) {
                mode = DATA;
            } else if ( command == "prompt" ) {
                mode = PROMPT;
            } else if (callback_map.find(command) != callback_map.end() ) {
                TelnetCallback t = callback_map[ command ];
                if (t)
                    (this->*t) (tokens);
                else
                    error("No matching callback found for command:"+command);
            }
      else if ( command == "seti" ) {
        string value, tmp;
        if (tokens.size() == 3) {
              node->getNode( tokens[1].c_str(), true )
                ->setIntValue(atoi(tokens[2].c_str()));
          }
          if ( mode == PROMPT ) {
              tmp = tokens[1].c_str();
              tmp += " " + tokens[2];
              tmp += " (";
              tmp += getValueTypeString( node->getNode( tokens[1].c_str() ) );
              tmp += ")";
              push( tmp.c_str() );
              push( getTerminator() );
          }
      }
      else if ( command == "setd" || command == "setf") {
          string value, tmp;
          if (tokens.size() == 3) {
            node->getNode( tokens[1].c_str(), true )
                ->setDoubleValue(atof(tokens[2].c_str()));
          }
          if ( mode == PROMPT ) {
              tmp = tokens[1].c_str();
              tmp += " ";
              tmp += tokens[2].c_str();
              tmp += " (";
              tmp += getValueTypeString( node->getNode( tokens[1].c_str() ) );
              tmp += ")";
              push( tmp.c_str() );
              push( getTerminator() );
          }
      }
      else if ( command == "setb" ) {
          string tmp, value;
          if (tokens.size() == 3) {
            if (tokens[2] == "false" || tokens[2] == "0") {
              node->getNode( tokens[1].c_str(), true )
                  ->setBoolValue(false);
              value = " False ";
            }
            if (tokens[2] == "true" || tokens[2] == "1"){
              node->getNode( tokens[1].c_str(), true )
                  ->setBoolValue(true);
              value = " True ";
            }
            if ( mode == PROMPT ) {
                tmp = tokens[1].c_str();
                tmp += value;
                tmp += " (";
                tmp += getValueTypeString( node->getNode( tokens[1].c_str() ) );
                tmp += ")";
                push( tmp.c_str() );
                push( getTerminator() );
            }

          }
     }
      else if ( command == "del" ) {
          string tmp;
          if (tokens.size() == 3){
             node->getNode( tokens[1].c_str(), true )->removeChild(tokens[2].c_str(),0);
          }
          if ( mode == PROMPT ) {
              tmp = "Delete ";
              tmp += tokens[1].c_str();
              tmp += tokens[2];
              push( tmp.c_str() );
              push( getTerminator() );
          }
      }
	    else {
                const char* msg = "\
Valid commands are:\r\n\
\r\n\
cd <dir>           cd to a directory, '..' to move back\r\n\
data               switch to raw data mode\r\n\
dump               dump current state (in xml)\r\n\
get <var>          show the value of a parameter\r\n\
help               show this help message\r\n\
ls [<dir>]         list directory\r\n\
prompt             switch to interactive mode (default)\r\n\
pwd                display your current path\r\n\
quit               terminate connection\r\n\
run <command>      run built in command\r\n\
set <var> <val>    set String <var> to a new <val>\r\n\
setb <var> <val>   set Bool <var> to a new <val> only work with the foling value 0, 1, true, false\r\n\
setd <var> <val>   set Double <var> to a new <val>\r\n\
setf <var> <val>   alias for setd\r\n\
seti <var> <val>   set Int <var> to a new <val>\r\n\
del <var> <nod>    delete <nod> in <var>\r\n\
subscribe <var>	   subscribe to property changes \r\n\
unsubscribe <var>  unscubscribe from property changes (var must be the property name/path used by subscribe)\r\n\
nasal [EOF <marker>]  execute arbitrary Nasal code (simulator must be running with Nasal allowed from sockets)\r\n\
";
                push( msg );
            }
        }

    } catch ( const string& msg ) {
        string error = "-ERR \"" + msg + "\"";
        push( error.c_str() );
        push( getTerminator() );
    }

    if ( (mode == PROMPT) && !_colletingNasal) {
        string prompt = node->getPath();
        if (prompt.empty()) {
            prompt = "/";
        }
        prompt += "> ";
        push( prompt.c_str() );
    }

    buffer.remove();
}

/**
 *
 */
FGProps::FGProps( const vector<string>& tokens )
{
    // tokens:
    //   props,port#
    //   props,medium,direction,hz,hostname,port#,style
    if (tokens.size() == 2) {
        port = atoi( tokens[1].c_str() );
        set_hz( 5 );                // default to processing requests @ 5Hz
    } else if (tokens.size() == 7) {
        char* endptr;
        errno = 0;
        int hz = strtol( tokens[3].c_str(), &endptr, 10 );
        if (errno != 0) {
           SG_LOG( SG_IO, SG_ALERT, "I/O poll frequency out of range" );
           set_hz( 5 );           // default to processing requests @ 5Hz
        } else {
            SG_LOG( SG_IO, SG_INFO, "Setting I/O poll frequency to "
                    << hz << " Hz");
            set_hz( hz );
        }
        port = atoi( tokens[5].c_str() );
    } else {
        throw FGProtocolConfigError( "FGProps: incorrect number of configuration arguments" );
    }
}

/**
 *
 */
FGProps::~FGProps()
{
    // ensure all channels are closed before our poller is destroyed
    if (is_enabled()) {
        close();
    }
}

/**
 *
 */
bool
FGProps::open()
{
    if ( is_enabled() )
    {
        SG_LOG( SG_IO, SG_ALERT, "This shouldn't happen, but the channel "
                << "is already in use, ignoring" );
        return false;
    }

    if (!simgear::NetChannel::open())
    {
        SG_LOG( SG_IO, SG_ALERT, "FGProps: Failed to open network socket.");
        return false;
    }

    int err = simgear::NetChannel::bind( "", port );
    if (err)
    {
        SG_LOG( SG_IO, SG_ALERT, "FGProps: Failed to open port #" << port << " - the port is already used (error " << err << ").");
        return false;
    }

    err = simgear::NetChannel::listen( 5 );
    if (err)
    {
        SG_LOG( SG_IO, SG_ALERT, "FGProps: Failed to listen on port #" << port << "(error " << err << ").");
        return false;
    }

    poller.addChannel(this);

    SG_LOG( SG_IO, SG_INFO, "Props server started on port " << port );

    set_enabled( true );
    return true;
}

/**
 *
 */
bool
FGProps::close()
{
    SG_LOG( SG_IO, SG_INFO, "closing FGProps" );
    for (auto channel : _activeChannels) {
        channel->close();
        delete channel;
    }
    _activeChannels.clear();
    return true;
}

/**
 *
 */
bool
FGProps::process()
{
    poller.poll();

    for (auto channel : _activeChannels) {
        channel->publishDirtySubscriptions();
    }

    return true;
}

/**
 *
 */
void
FGProps::handleAccept()
{
    simgear::IPAddress addr;
    int handle = accept( &addr );
    SG_LOG( SG_IO, SG_INFO, "Props server accepted connection from "
            << addr.getHost() << ":" << addr.getPort() );
    PropsChannel* channel = new PropsChannel(this);
    channel->setHandle( handle );
    poller.addChannel( channel );
    _activeChannels.push_back(channel);
}

void FGProps::removeChannel(FGProps::PropsChannel *channel)
{
    auto it = std::find(_activeChannels.begin(), _activeChannels.end(), channel);
    if (it == _activeChannels.end()) {
        SG_LOG(SG_IO, SG_WARN, "FGProps::removeChannel: unknown channel");
    } else {
        _activeChannels.erase(it);
    }
}
