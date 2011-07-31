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

#include <string>
#include <vector>

#include <simgear/props/props_io.hxx>
#include <simgear/structure/subsystem_mgr.hxx>
#include <simgear/structure/exception.hxx>
#include <Main/fg_props.hxx>


using simgear::PropertyList;

class FGXMLAutopilotGroupImplementation : public FGXMLAutopilotGroup
{
public:
    void init();
    void reinit();
    void update( double dt );
private:
    void initFrom( SGPropertyNode_ptr rootNode, const char * childName );
    vector<string> _autopilotNames;

};

void FGXMLAutopilotGroupImplementation::update( double dt )
{
    // update all configured autopilots
    SGSubsystemGroup::update( dt );
}

void FGXMLAutopilotGroupImplementation::reinit()
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

void FGXMLAutopilotGroupImplementation::init()
{
    static const char * nodeNames[] = {
        "autopilot",
        "property-rule"
    };
    for( unsigned i = 0; i < sizeof(nodeNames)/sizeof(nodeNames[0]); i++ )
        initFrom( fgGetNode( "/sim/systems" ), nodeNames[i] );

    SGSubsystemGroup::bind();
    SGSubsystemGroup::init();
}

void FGXMLAutopilotGroupImplementation::initFrom( SGPropertyNode_ptr rootNode, const char * childName )
{
    if( rootNode == NULL )
        return;

    PropertyList autopilotNodes = rootNode->getChildren(childName);
    for( PropertyList::size_type i = 0; i < autopilotNodes.size(); i++ ) {
        SGPropertyNode_ptr pathNode = autopilotNodes[i]->getNode( "path" );
        if( pathNode == NULL ) {
            SG_LOG( SG_ALL, SG_WARN, "No configuration file specified for this property-rule!");
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

        {
          // check for duplicate names
          string name = apName;
          for( unsigned i = 0; get_subsystem( apName.c_str() ) != NULL; i++ ) {
              ostringstream buf;
              buf <<  name << "_" << i;
              apName = buf.str();
          }
          if( apName != name )
            SG_LOG( SG_ALL, SG_WARN, "Duplicate property-rule configuration name " << name << ", renamed to " << apName );
        }

        SGPath config = globals->resolve_maybe_aircraft_path(pathNode->getStringValue());
        if (config.isNull())
        {
            SG_LOG( SG_ALL, SG_ALERT, "Cannot find property-rule configuration file '" << pathNode->getStringValue() << "'." );
        }
        else
        {
            SG_LOG( SG_ALL, SG_INFO, "Reading property-rule configuration from " << config.str() );

            try {
                SGPropertyNode_ptr root = new SGPropertyNode();
                readProperties( config.str(), root );

                SG_LOG( SG_AUTOPILOT, SG_INFO, "adding  property-rule subsystem " << apName );
                FGXMLAutopilot::Autopilot * ap = new FGXMLAutopilot::Autopilot( autopilotNodes[i], root );
                ap->set_name( apName );
                set_subsystem( apName, ap );
                _autopilotNames.push_back( apName );

            } catch (const sg_exception& e) {
                SG_LOG( SG_AUTOPILOT, SG_ALERT, "Failed to load property-rule configuration: "
                        << config.str() << ":" << e.getMessage() );
                continue;
            }
        }
    }
}

FGXMLAutopilotGroup * FGXMLAutopilotGroup::createInstance()
{
    return new FGXMLAutopilotGroupImplementation();
}
