// autopilotgroup.cxx - an even more flexible, generic way to build autopilots
//
// Written by Torsten Dreyer
// Based heavily on work created by Curtis Olson, started January 2004.
//
// Copyright (C) 2004  Curtis L. Olson  - http://www.flightgear.org/~curt
// Copyright (C) 2010  Torsten Dreyer - Torsten (at) t3r (dot) de
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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include "autopilot.hxx"
#include "autopilotgroup.hxx"

#include <Main/fg_props.hxx>
#include <Main/globals.hxx>
#include <Main/util.hxx>

#include <simgear/misc/sg_path.hxx>
#include <simgear/props/props_io.hxx>
#include <simgear/structure/SGExpression.hxx>

using std::cout;
using std::endl;

using simgear::PropertyList;

FGXMLAutopilotGroup::FGXMLAutopilotGroup() :
  SGSubsystemGroup()
#ifdef XMLAUTO_USEHELPER
  ,average(0.0), // average/filtered prediction
  v_last(0.0),  // last velocity
  last_static_pressure(0.0),
  vel(fgGetNode( "/velocities/airspeed-kt", true )),
  // Estimate speed in 5,10 seconds
  lookahead5(fgGetNode( "/autopilot/internal/lookahead-5-sec-airspeed-kt", true )),
  lookahead10(fgGetNode( "/autopilot/internal/lookahead-10-sec-airspeed-kt", true )),
  bug(fgGetNode( "/autopilot/settings/heading-bug-deg", true )),
  mag_hdg(fgGetNode( "/orientation/heading-magnetic-deg", true )),
  bug_error(fgGetNode( "/autopilot/internal/heading-bug-error-deg", true )),
  fdm_bug_error(fgGetNode( "/autopilot/internal/fdm-heading-bug-error-deg", true )),
  target_true(fgGetNode( "/autopilot/settings/true-heading-deg", true )),
  true_hdg(fgGetNode( "/orientation/heading-deg", true )),
  true_error(fgGetNode( "/autopilot/internal/true-heading-error-deg", true )),
  target_nav1(fgGetNode( "/instrumentation/nav[0]/radials/target-auto-hdg-deg", true )),
  true_nav1(fgGetNode( "/autopilot/internal/nav1-heading-error-deg", true )),
  true_track_nav1(fgGetNode( "/autopilot/internal/nav1-track-error-deg", true )),
  nav1_course_error(fgGetNode( "/autopilot/internal/nav1-course-error", true )),
  nav1_selected_course(fgGetNode( "/instrumentation/nav[0]/radials/selected-deg", true )),
  vs_fps(fgGetNode( "/velocities/vertical-speed-fps", true )),
  vs_fpm(fgGetNode( "/autopilot/internal/vert-speed-fpm", true )),
  static_pressure(fgGetNode( "/systems/static[0]/pressure-inhg", true )),
  pressure_rate(fgGetNode( "/autopilot/internal/pressure-rate", true )),
  track(fgGetNode( "/orientation/track-deg", true ))
#endif
{
}

void FGXMLAutopilotGroup::update( double dt )
{
    // update all configured autopilots
    SGSubsystemGroup::update( dt );
#ifdef XMLAUTO_USEHELPER
    // update helper values
    double v = vel->getDoubleValue();
    double a = 0.0;
    if ( dt > 0.0 ) {
        a = (v - v_last) / dt;

        if ( dt < 1.0 ) {
            average = (1.0 - dt) * average + dt * a;
        } else {
            average = a;
        }

        lookahead5->setDoubleValue( v + average * 5.0 );
        lookahead10->setDoubleValue( v + average * 10.0 );
        v_last = v;
    }

    // Calculate heading bug error normalized to +/- 180.0
    double diff = bug->getDoubleValue() - mag_hdg->getDoubleValue();
    SG_NORMALIZE_RANGE(diff, -180.0, 180.0);
    bug_error->setDoubleValue( diff );

    fdm_bug_error->setDoubleValue( diff );

    // Calculate true heading error normalized to +/- 180.0
    diff = target_true->getDoubleValue() - true_hdg->getDoubleValue();
    SG_NORMALIZE_RANGE(diff, -180.0, 180.0);
    true_error->setDoubleValue( diff );

    // Calculate nav1 target heading error normalized to +/- 180.0
    diff = target_nav1->getDoubleValue() - true_hdg->getDoubleValue();
    SG_NORMALIZE_RANGE(diff, -180.0, 180.0);
    true_nav1->setDoubleValue( diff );

    // Calculate true groundtrack
    diff = target_nav1->getDoubleValue() - track->getDoubleValue();
    SG_NORMALIZE_RANGE(diff, -180.0, 180.0);
    true_track_nav1->setDoubleValue( diff );

    // Calculate nav1 selected course error normalized to +/- 180.0
    diff = nav1_selected_course->getDoubleValue() - mag_hdg->getDoubleValue();
    SG_NORMALIZE_RANGE( diff, -180.0, 180.0 );
    nav1_course_error->setDoubleValue( diff );

    // Calculate vertical speed in fpm
    vs_fpm->setDoubleValue( vs_fps->getDoubleValue() * 60.0 );


    // Calculate static port pressure rate in [inhg/s].
    // Used to determine vertical speed.
    if ( dt > 0.0 ) {
        double current_static_pressure = static_pressure->getDoubleValue();
        double current_pressure_rate = 
            ( current_static_pressure - last_static_pressure ) / dt;

        pressure_rate->setDoubleValue(current_pressure_rate);
        last_static_pressure = current_static_pressure;
    }
#endif
}

void FGXMLAutopilotGroup::reinit()
{
    SGSubsystemGroup::unbind();

    for( vector<string>::size_type i = 0; i < _autopilotNames.size(); i++ ) {
      FGXMLAutopilot::Autopilot * ap = (FGXMLAutopilot::Autopilot*)get_subsystem( _autopilotNames[i] );
      if( ap == NULL ) continue; // ?
      remove_subsystem( _autopilotNames[i] );
      delete ap;
    }
    _autopilotNames.clear();
    init();
}

void FGXMLAutopilotGroup::init()
{
    PropertyList autopilotNodes = fgGetNode( "/sim/systems", true )->getChildren("autopilot");
    if( autopilotNodes.size() == 0 ) {
        SG_LOG( SG_ALL, SG_WARN, "No autopilot configuration specified for this model!");
        return;
    }

    for( PropertyList::size_type i = 0; i < autopilotNodes.size(); i++ ) {
        SGPropertyNode_ptr pathNode = autopilotNodes[i]->getNode( "path" );
        if( pathNode == NULL ) {
            SG_LOG( SG_ALL, SG_WARN, "No autopilot configuration file specified for this autopilot!");
            continue;
        }

        string apName;
        SGPropertyNode_ptr nameNode = autopilotNodes[i]->getNode( "name" );
        if( nameNode != NULL ) {
            apName = nameNode->getStringValue();
        } else {
          std::ostringstream buf;
          buf <<  "unnamed_autopilot_" << i;
          apName = buf.str();
        }

        if( get_subsystem( apName.c_str() ) != NULL ) {
            SG_LOG( SG_ALL, SG_ALERT, "Duplicate autopilot configuration name " << apName << " ignored" );
            continue;
        }

        SGPath config( globals->get_fg_root() );
        config.append( pathNode->getStringValue() );

        SG_LOG( SG_ALL, SG_INFO, "Reading autopilot configuration from " << config.str() );

        try {
            SGPropertyNode_ptr root = new SGPropertyNode();
            readProperties( config.str(), root );

            SG_LOG( SG_AUTOPILOT, SG_INFO, "adding  autopilot subsystem " << apName );
            FGXMLAutopilot::Autopilot * ap = new FGXMLAutopilot::Autopilot( autopilotNodes[i], root );
            ap->set_name( apName );
            set_subsystem( apName, ap );
            _autopilotNames.push_back( apName );

        } catch (const sg_exception& e) {
            SG_LOG( SG_AUTOPILOT, SG_ALERT, "Failed to load autopilot configuration: "
                    << config.str() << ":" << e.getMessage() );
            continue;
        }
    }

    SGSubsystemGroup::bind();
    SGSubsystemGroup::init();
}

