// aircraft.cxx -- various aircraft routines
//
// Written by Curtis Olson, started May 1997.
//
// Copyright (C) 1997  Curtis L. Olson  - http://www.flightgear.org/~curt
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
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//
// $Id$


#include <stdio.h>
#include <string.h>		// strdup

#include <plib/ul.h>

#include <simgear/constants.h>
#include <simgear/debug/logstream.hxx>
#include <simgear/misc/sg_path.hxx>
#include <simgear/structure/commands.hxx>
#include <simgear/structure/exception.hxx>

#include <Main/globals.hxx>
#include <Main/fg_props.hxx>
#include <Main/viewmgr.hxx>
#include <Cockpit/panel.hxx>
#include <Cockpit/hud.hxx>
#include <Cockpit/panel_io.hxx>
#include <Model/acmodel.hxx>

#include "aircraft.hxx"


// This is a record containing all the info for the aircraft currently
// being operated
fgAIRCRAFT current_aircraft;


// Initialize an Aircraft structure
void fgAircraftInit( void ) {
    SG_LOG( SG_AIRCRAFT, SG_INFO, "Initializing Aircraft structure" );

    current_aircraft.fdm_state   = cur_fdm_state;
    current_aircraft.controls = globals->get_controls();
}


// Display various parameters to stdout
void fgAircraftOutputCurrent(fgAIRCRAFT *a) {
    FGInterface *f;

    f = a->fdm_state;

    SG_LOG( SG_FLIGHT, SG_DEBUG,
            "Pos = ("
	    << (f->get_Longitude() * 3600.0 * SGD_RADIANS_TO_DEGREES) << "," 
	    << (f->get_Latitude()  * 3600.0 * SGD_RADIANS_TO_DEGREES) << ","
	    << f->get_Altitude() 
	    << ")  (Phi,Theta,Psi)=("
	    << f->get_Phi() << "," 
	    << f->get_Theta() << "," 
	    << f->get_Psi() << ")" );

    SG_LOG( SG_FLIGHT, SG_DEBUG,
	    "Kts = " << f->get_V_equiv_kts() 
	    << "  Elev = " << globals->get_controls()->get_elevator() 
	    << "  Aileron = " << globals->get_controls()->get_aileron() 
	    << "  Rudder = " << globals->get_controls()->get_rudder() 
	    << "  Power = " << globals->get_controls()->get_throttle( 0 ) );
}


// Show available aircraft types
void fgReadAircraft(void) {

   // SGPropertyNode *aircraft_types = fgGetNode("/sim/aircraft-types", true);

   SGPath path( globals->get_fg_root() );
   path.append("Aircraft");

   ulDirEnt* dire;
   ulDir *dirp;

   dirp = ulOpenDir(path.c_str());
   if (dirp == NULL) {
      SG_LOG( SG_AIRCRAFT, SG_ALERT, "Unable to open aircraft directory." );
      ulCloseDir(dirp);
      return;
   }

   while ((dire = ulReadDir(dirp)) != NULL) {
      char *ptr;

      if ((ptr = strstr(dire->d_name, "-set.xml")) && strlen(ptr) == 8) {

          *ptr = '\0';
#if 0
          SGPath afile = path;
          afile.append(dire->d_name);

          SGPropertyNode root;
          try {
             readProperties(afile.str(), &root);
          } catch (...) {
             continue;
          }

          SGPropertyNode *node = root.getNode("sim");
          if (node) {
             SGPropertyNode *desc = node->getNode("description");

             if (desc) {
                 SGPropertyNode *aircraft =
                                aircraft_types->getChild(dire->d_name, 0, true);

                aircraft->setStringValue(strdup(desc->getStringValue()));
             }
          }
#endif
      }
   }

   ulCloseDir(dirp);

   globals->get_commands()->addCommand("load-aircraft", fgLoadAircraft);
}

bool
fgLoadAircraft (const SGPropertyNode * arg)
{
    static const SGPropertyNode *master_freeze
        = fgGetNode("/sim/freeze/master");

    bool freeze = master_freeze->getBoolValue();
    if ( !freeze ) {
        fgSetBool("/sim/freeze/master", true);
    }

    // TODO:
    //    remove electrical system
    cur_fdm_state->unbind();

    // Save the selected aircraft model since restoreInitialState
    // will obverwrite it.
    //
    string aircraft = fgGetString("/sim/aircraft", "");
    globals->restoreInitialState();

    fgSetString("/sim/aircraft", aircraft.c_str());
    fgSetString("/sim/panel/path", "Aircraft/c172/Panels/c172-vfr-panel.xml");

    if ( aircraft.size() > 0 ) {
        SGPath aircraft_path(globals->get_fg_root());
        aircraft_path.append("Aircraft");
        aircraft_path.append(aircraft);
        aircraft_path.concat("-set.xml");
        SG_LOG(SG_INPUT, SG_INFO, "Reading default aircraft: " << aircraft
               << " from " << aircraft_path.str());
        try {
            readProperties(aircraft_path.str(), globals->get_props());
        } catch (const sg_exception &e) {
            string message = "Error reading default aircraft: ";
            message += e.getFormattedMessage();
            SG_LOG(SG_INPUT, SG_ALERT, message);
            exit(2);
        }
    } else {
        SG_LOG(SG_INPUT, SG_ALERT, "No default aircraft specified");
    }

    // Initialize the (new) 2D panel.
    //
    string panel_path = fgGetString("/sim/panel/path",
                                    "Aircraft/c172/Panels/c172-vfr-panel.xml");

    FGPanel *panel = fgReadPanel(panel_path);
    if (panel == 0) {
        SG_LOG( SG_INPUT, SG_ALERT,
                "Error reading new panel from " << panel_path );
    } else {
        SG_LOG( SG_INPUT, SG_INFO, "Loaded new panel from " << panel_path );
        globals->get_current_panel()->unbind();
        delete globals->get_current_panel();
        globals->set_current_panel( panel );
        globals->get_current_panel()->init();
        globals->get_current_panel()->bind();
        globals->get_current_panel()->update(0);
    }

    // Load the new 3D model
    //
    globals->get_aircraft_model()->unbind();
    delete globals->get_aircraft_model();
    globals->set_aircraft_model(new FGAircraftModel);
    globals->get_aircraft_model()->init();
    globals->get_aircraft_model()->bind();

    // TODO:
    //    load new electrical system
    //

    // update our position based on current presets
    fgInitPosition();

    // Update the HUD
    fgHUDInit(&current_aircraft);

    SGTime *t = globals->get_time_params();
    delete t;
    t = fgInitTime();
    globals->set_time_params( t );

    // Reinitialize some subsystems
    //
    globals->get_viewmgr()->reinit();
    globals->get_controls()->reset_all();
    globals->get_aircraft_model()->reinit();
    globals->get_subsystem("fx")->reinit();
    globals->get_subsystem("xml-autopilot")->reinit();

    fgReInitSubsystems();

    if ( !freeze ) {
        fgSetBool("/sim/freeze/master", false);
    }

    return true;
}
