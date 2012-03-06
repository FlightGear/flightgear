// fgjs.cxx -- assign joystick axes to flightgear properties
//
// Updated to allow xml output & added a few bits & pieces
// Laurie Bradshaw, Jun 2005
//
// Written by Tony Peden, started May 2001
//
// Copyright (C) 2001  Tony Peden (apeden@earthlink.net)
// Copyright (C) 2006  Stefan Seifert
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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <simgear/compiler.h>

#ifdef _WIN32
#  include <winsock2.h>
#endif

#include <math.h>

#include <iostream>
#include <fstream>
#include <string>

using std::fstream;
using std::cout;
using std::cin;
using std::endl;
using std::ios;
using std::string;

#include <simgear/constants.h>
#include <simgear/debug/logstream.hxx>
#include <simgear/misc/sg_path.hxx>
#include <simgear/misc/sgstream.hxx>
#include <simgear/structure/exception.hxx>
#include <simgear/props/props_io.hxx>

#include <Main/fg_io.hxx>
#include <Main/fg_props.hxx>
#include <Main/globals.hxx>

#include "jsinput.h"

#ifdef __APPLE__
#  include <CoreFoundation/CoreFoundation.h>
#endif

using simgear::PropertyList;

bool confirmAnswer() {
    char answer;
    do {
        cout << "Is this correct? (y/n) $ ";
        cin >> answer;
        cin.ignore(256, '\n');
        if (answer == 'y')
            return true;
        if (answer == 'n')
            return false;
    } while (true);
}

string getFGRoot( int argc, char *argv[] );

int main( int argc, char *argv[] ) {

    for (int i = 1; i < argc; i++) {
        if (strcmp("--help", argv[i]) == 0) {
            cout << "Usage:" << endl;
            cout << "  --help\t\t\tShow this help" << endl;
            exit(0);
	} else if (strncmp("--fg-root=", argv[i], 10) == 0) {
	    // used later
        } else {
            cout << "Unknown option \"" << argv[i] << "\"" << endl;
            exit(0);
        }
    }

    jsInit();

    jsSuper *jss = new jsSuper();
    jsInput *jsi = new jsInput(jss);
    jsi->displayValues(false);

    cout << "Found " << jss->getNumJoysticks() << " joystick(s)" << endl;

    if(jss->getNumJoysticks() <= 0) {
        cout << "Can't find any joysticks ..." << endl;
        exit(1);
    }
    cout << endl << "Now measuring the dead band of your joystick. The dead band is the area " << endl
                 << "where the joystick is centered and should not generate any input. Move all " << endl
                 << "axes around in this dead zone during the ten seconds this test will take." << endl;
    cout << "Press enter to continue." << endl;
    cin.ignore(1024, '\n');
    jsi->findDeadBand();
    cout << endl << "Dead band calibration finished. Press enter to start control assignment." << endl;
    cin.ignore(1024, '\n');

    jss->firstJoystick();
    fstream *xfs = new fstream[ jss->getNumJoysticks() ];
    SGPropertyNode_ptr *jstree = new SGPropertyNode_ptr[ jss->getNumJoysticks() ];
    do {
        cout << "Joystick #" << jss->getCurrentJoystickId()
             << " \"" << jss->getJoystick()->getName() << "\" has "
             << jss->getJoystick()->getNumAxes() << " axes" << endl;

        char filename[16];
        snprintf(filename, 16, "js%i.xml", jss->getCurrentJoystickId());
        xfs[ jss->getCurrentJoystickId() ].open(filename, ios::out);
        jstree[ jss->getCurrentJoystickId() ] = new SGPropertyNode();
    } while ( jss->nextJoystick() );

    SGPath templatefile( getFGRoot(argc, argv) );
    templatefile.append("Input");
    templatefile.append("Joysticks");
    templatefile.append("template.xml");

    SGPropertyNode *templatetree = new SGPropertyNode();
    try {
        readProperties(templatefile.str().c_str(), templatetree);
    } catch (sg_io_exception & e) {
        cout << e.getFormattedMessage ();
    }

    PropertyList axes = templatetree->getChildren("axis");
    for(PropertyList::iterator iter = axes.begin(); iter != axes.end(); ++iter) {
        cout << "Move the control you wish to use for " << (*iter)->getStringValue("desc")
             << " " << (*iter)->getStringValue("direction") << endl;
        cout << "Pressing a button skips this axis" << endl;
        fflush( stdout );
        jsi->getInput();
        if (jsi->getInputAxis() != -1) {
            cout << endl << "Assigned axis " << jsi->getInputAxis()
                 << " on joystick " << jsi->getInputJoystick()
                 << " to control " << (*iter)->getStringValue("desc") << endl;
            if ( confirmAnswer() ) {
                SGPropertyNode *axis = jstree[ jsi->getInputJoystick() ]->getChild("axis", jsi->getInputAxis(), true);
                copyProperties(*iter, axis);
                axis->setDoubleValue("dead-band", jss->getJoystick(jsi->getInputJoystick())
                        ->getDeadBand(jsi->getInputAxis()));
                axis->setDoubleValue("binding/factor", jsi->getInputAxisPositive() ? 1.0 : -1.0);
            } else {
                --iter;
            }
        } else {
            cout << "Skipping control" << endl;
            if ( ! confirmAnswer() )
                --iter;
        }
        cout << endl;
    }

    PropertyList buttons = templatetree->getChildren("button");
    for(PropertyList::iterator iter = buttons.begin(); iter != buttons.end(); ++iter) {
        cout << "Press the button you wish to use for " << (*iter)->getStringValue("desc") << endl;
        cout << "Moving a joystick axis skips this button" << endl;
        fflush( stdout );
        jsi->getInput();
        if (jsi->getInputButton() != -1) {
            cout << endl << "Assigned button " << jsi->getInputButton()
                 << " on joystick " << jsi->getInputJoystick()
                 << " to control " << (*iter)->getStringValue("desc") << endl;
            if ( confirmAnswer() ) {
                SGPropertyNode *button = jstree[ jsi->getInputJoystick() ]->getChild("button", jsi->getInputButton(), true);
                copyProperties(*iter, button);
            } else {
                --iter;
            }
        } else {
            cout << "Skipping control" << endl;
            if (! confirmAnswer())
                --iter;
        }
        cout << endl;
    }

    cout << "Your joystick settings are in ";
    for (int i = 0; i < jss->getNumJoysticks(); i++) {
        try {
            cout << "js" << i << ".xml";
            if (i + 2 < jss->getNumJoysticks())
                cout << ", ";
            else if (i + 1 < jss->getNumJoysticks())
                cout << " and ";

            jstree[i]->setStringValue("name", jss->getJoystick(i)->getName());
            writeProperties(xfs[i], jstree[i], true);
        } catch (sg_io_exception & e) {
            cout << e.getFormattedMessage ();
        }
        xfs[i].close();
    }
    cout << "." << endl << "Check and edit as desired. Once you are happy," << endl
         << "move relevant js<n>.xml files to $FG_ROOT/Input/Joysticks/ (if you didn't use" << endl
         << "an attached controller, you don't need to move the corresponding file)" << endl;

    delete jsi;
    delete[] xfs;
    delete jss;
    delete[] jstree;

    return 1;
}

char *homedir = ::getenv( "HOME" );
char *hostname = ::getenv( "HOSTNAME" );
bool free_hostname = false;

// Scan the command line options for the specified option and return
// the value.
static string fgScanForOption( const string& option, int argc, char **argv ) {
    int i = 1;

    if (hostname == NULL)
    {
        char _hostname[256];
        gethostname(_hostname, 256);
        hostname = strdup(_hostname);
        free_hostname = true;
    }

    SG_LOG(SG_INPUT, SG_INFO, "Scanning command line for: " << option );

    int len = option.length();

    while ( i < argc ) {
        SG_LOG( SG_INPUT, SG_DEBUG, "argv[" << i << "] = " << argv[i] );

        string arg = argv[i];
        if ( arg.find( option ) == 0 ) {
            return arg.substr( len );
        }

        i++;
    }

    return "";
}

// Scan the user config files for the specified option and return
// the value.
static string fgScanForOption( const string& option, const string& path ) {
    sg_gzifstream in( path );
    if ( !in.is_open() ) {
        return "";
    }

    SG_LOG( SG_INPUT, SG_INFO, "Scanning " << path << " for: " << option );

    int len = option.length();

    in >> skipcomment;
    while ( ! in.eof() ) {
        string line;
        getline( in, line, '\n' );

        // catch extraneous (DOS) line ending character
        if ( line[line.length() - 1] < 32 ) {
            line = line.substr( 0, line.length()-1 );
        }

        if ( line.find( option ) == 0 ) {
            return line.substr( len );
        }

        in >> skipcomment;
    }

    return "";
}

// Scan the user config files for the specified option and return
// the value.
static string fgScanForOption( const string& option ) {
    string arg("");

#if defined( unix ) || defined( __CYGWIN__ )
    // Next check home directory for .fgfsrc.hostname file
    if ( arg.empty() ) {
        if ( homedir != NULL ) {
            SGPath config( homedir );
            config.append( ".fgfsrc" );
            config.concat( "." );
            config.concat( hostname );
            arg = fgScanForOption( option, config.str() );
        }
    }
#endif

    // Next check home directory for .fgfsrc file
    if ( arg.empty() ) {
        if ( homedir != NULL ) {
            SGPath config( homedir );
            config.append( ".fgfsrc" );
            arg = fgScanForOption( option, config.str() );
        }
    }

    return arg;
}

// Read in configuration (files and command line options) but only set
// fg_root
string getFGRoot ( int argc, char **argv ) {
    string root;

    // First parse command line options looking for --fg-root=, this
    // will override anything specified in a config file
    root = fgScanForOption( "--fg-root=", argc, argv);

    // Check in one of the user configuration files.
    if (root.empty() )
        root = fgScanForOption( "--fg-root=" );

    // Next check if fg-root is set as an env variable
    if ( root.empty() ) {
        char *envp = ::getenv( "FG_ROOT" );
        if ( envp != NULL ) {
            root = envp;
        }
    }

    // Otherwise, default to a random compiled-in location if we can't
    // find fg-root any other way.
    if ( root.empty() ) {
#if defined( __CYGWIN__ )
        root = "../data";
#elif defined( _WIN32 )
        root = "..\\data";
#elif defined(__APPLE__) 
        /*
        The following code looks for the base package inside the application 
        bundle, in the standard Contents/Resources location. 
        */

        CFURLRef resourcesUrl = CFBundleCopyResourcesDirectoryURL(CFBundleGetMainBundle());

        // look for a 'data' subdir
        CFURLRef dataDir = CFURLCreateCopyAppendingPathComponent(NULL, resourcesUrl, CFSTR("data"), true);

        // now convert down to a path, and the a c-string
        CFStringRef path = CFURLCopyFileSystemPath(dataDir, kCFURLPOSIXPathStyle);
        root = CFStringGetCStringPtr(path, CFStringGetSystemEncoding());

        // tidy up.
        CFRelease(resourcesUrl);
        CFRelease(dataDir);
        CFRelease(path);
#else
        root = PKGLIBDIR;
#endif
    }

    SG_LOG(SG_INPUT, SG_INFO, "fg_root = " << root );

    return root;
}
