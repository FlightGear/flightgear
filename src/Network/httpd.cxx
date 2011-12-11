// httpd.cxx -- FGFS http property manager interface / external script
//              and control class
//
// Written by Curtis Olson, started June 2001.
//
// Copyright (C) 2001  Curtis L. Olson - http://www.flightgear.org/~curt
//
// Jpeg Image Support added August 2001
//  by Norman Vine - nhv@cape.com
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

#include <algorithm>		// sort()
#include <cstdlib>		// atoi() atof()
#include <cstring>
#include <string>

#include <simgear/debug/logstream.hxx>
#include <simgear/io/iochannel.hxx>
#include <simgear/math/sg_types.hxx>
#include <simgear/structure/commands.hxx>
#include <simgear/props/props.hxx>

#include <Main/fg_props.hxx>
#include <Main/globals.hxx>

#include "httpd.hxx"

using std::string;

bool FGHttpd::open() {
    if ( is_enabled() ) {
	SG_LOG( SG_IO, SG_ALERT, "This shouldn't happen, but the channel " 
		<< "is already in use, ignoring" );
	return false;
    }

    server = new HttpdServer( port );
    
    set_hz( 15 );                // default to processing requests @ 15Hz
    set_enabled( true );

    return true;
}


bool FGHttpd::process() {
    simgear::NetChannel::poll();

    return true;
}


bool FGHttpd::close() {
    SG_LOG( SG_IO, SG_INFO, "closing FGHttpd" );   

    delete server;
    set_enabled( false );

    return true;
}


class CompareNodes {
public:
    bool operator() (const SGPropertyNode *a, const SGPropertyNode *b) const {
        int r = strcmp(a->getName(), b->getName());
        return r ? r < 0 : a->getIndex() < b->getIndex();
    }
};


// Handle http GET requests
void HttpdChannel::foundTerminator (void) {
    
    closeWhenDone ();

    const string s = buffer.getData();

    if ( s.find( "GET " ) == 0 ) {
        SG_LOG( SG_IO, SG_INFO, "echo: " << s );   

        string rest = s.substr(4);
        string request;
        string tmp;

        string::size_type pos = rest.find( " " );
        if ( pos != string::npos ) {
            request = rest.substr( 0, pos );
        } else {
            request = "/";
        }

        SGPropertyNode *node = NULL;
        pos = request.find( "?" );
        if ( pos != string::npos ) {
            // request to update property value
            string args = request.substr( pos + 1 );
            request = request.substr( 0, pos );
            SG_LOG( SG_IO, SG_INFO, "'" << request << "' '" << args << "'" );   
            request = urlDecode(request);

            // parse args looking for "value="
            bool done = false;
            while ( ! done ) {
                string arg;
                pos = args.find("&");
                if ( pos != string::npos ) {
                    arg = args.substr( 0, pos );
                    args = args.substr( pos + 1 );
                } else {
                    arg = args;
                    done = true;
                }

                SG_LOG( SG_IO, SG_INFO, "  arg = " << arg );   
                string::size_type apos = arg.find("=");
                if ( apos != string::npos ) {
                    string a = arg.substr( 0, apos );
                    string b = arg.substr( apos + 1 );
                    SG_LOG( SG_IO, SG_INFO, "    a = " << a << "  b = " << b );
                    if ( request == "/run.cgi" ) {
                        // execute a command
                        if ( a == "value" ) {
                            SGPropertyNode args;
                            if ( !globals->get_commands()
                                 ->execute(urlDecode(b).c_str(), &args) )
                            {
                                SG_LOG( SG_NETWORK, SG_ALERT,
                                        "Command " << urlDecode(b)
                                        << " failed.");
                            }

                        }
                    } else {
                        if ( a == "value" ) {
                            // update a property value
                            fgSetString( request.c_str(),
                                         urlDecode(b).c_str() );
                        }
                    }
                }
            }
        } else {
            request = urlDecode(request);
	}

        node = globals->get_props()->getNode(request.c_str());

        string response = "";
        response += "<HTML LANG=\"en\">";
        response += getTerminator();

        response += "<HEAD>";
        response += getTerminator();

        response += "<TITLE>FlightGear - ";
        response += request;
        response += "</TITLE>";
        response += getTerminator();

        response += "</HEAD>";
        response += getTerminator();

        response += "<BODY>";
        response += getTerminator();

        if (node == NULL) {
            response += "<H3>Non-existent node requested!</H3>";
            response += getTerminator();

            response += "<B>";
            response += request.c_str();
            response += "</B> does not exist.";
            response += getTerminator();
	} else if ( node->nChildren() > 0 ) {
            // request is a path with children
            response += "<H3>Contents of \"";
            response += request;
            response += "\"</H3>";
            response += getTerminator();


            vector<SGPropertyNode *> children;
            for (int i = 0; i < node->nChildren(); i++)
                children.push_back(node->getChild(i));
            std::sort(children.begin(), children.end(), CompareNodes());

            vector<SGPropertyNode *>::iterator it, end = children.end();
            for (it = children.begin(); it != end; ++it) {
                SGPropertyNode *child = *it;
                string name = child->getDisplayName(true);
                string line = "";
                if ( child->nChildren() > 0 ) {
                    line += "<B><A HREF=\"";
                    line += request;
                    if ( request.substr(request.length() - 1, 1) != "/" ) {
                        line += "/";
                    }
                    line += urlEncode(name);
                    line += "\">";
                    line += name;
                    line += "</A></B>";
                    line += "/<BR>";
                } else {
                    string value = node->getStringValue ( name.c_str(), "" );
                    line += "<B>";
                    line += name;
                    line += "</B> <A HREF=\"";
                    line += request;
                    if ( request.substr(request.length() - 1, 1) != "/" ) {
                        line += "/";
                    }
                    line += urlEncode(name);
                    line += "\">(";
                    line += value;
                    line += ")</A><BR>";
                }
                response += line;
                response += getTerminator();
            }
        } else {
            // request for an individual data member
            string value = node->getStringValue();
            
            response += "<form method=\"GET\" action=\"";
            response += urlEncode(request);
            response += "\">";
            response += "<B>";
            response += request;
            response += "</B> = ";
            response += "<input type=text name=value size=\"15\" value=\"";
            response += value;
            response += "\" maxlength=2047>";
            response += "<input type=submit value=\"update\" name=\"submit\">";
            response += "</FORM>";
        }
        response += "</BODY>";
        response += getTerminator();

        response += "</HTML>";
        response += getTerminator();

        push( "HTTP/1.1 200 OK" );
        push( getTerminator() );
        
        SG_LOG( SG_IO, SG_INFO, "size = " << response.length() );
        char ctmp[256];
        sprintf(ctmp, "Content-Length: %u", (unsigned)response.length());
        push( ctmp );
        push( getTerminator() );

        push( "Connection: close" );
        push( getTerminator() );

        push( "Content-Type: text/html" );
        push( getTerminator() );
        push( getTerminator() );
                
        push( response.c_str() );
    }

    buffer.remove();
}


// encode everything but "a-zA-Z0-9_.-/" (see RFC2396)
string HttpdChannel::urlEncode(string s) {
    string r = "";
    
    for ( int i = 0; i < (int)s.length(); i++ ) {
        if ( isalnum(s[i]) || s[i] == '_' || s[i] == '.'
                || s[i] == '-' || s[i] == '/' ) {
            r += s[i];
        } else {
            char buf[16];
            sprintf(buf, "%%%02X", (unsigned char)s[i]);
            r += buf;
        }
    }
    return r;
}


string HttpdChannel::urlDecode(string s) {
    string r = "";
    int max = s.length();
    int a, b;

    for ( int i = 0; i < max; i++ ) {
        if ( s[i] == '+' ) {
            r += ' ';
        } else if ( s[i] == '%' && i + 2 < max
                && isxdigit(s[i + 1])
                && isxdigit(s[i + 2]) ) {
            i++;
            a = isdigit(s[i]) ? s[i] - '0' : toupper(s[i]) - 'A' + 10;
            i++;
            b = isdigit(s[i]) ? s[i] - '0' : toupper(s[i]) - 'A' + 10;
            r += (char)(a * 16 + b);
        } else {
            r += s[i];
        }
    }
    return r;
}
