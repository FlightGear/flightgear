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

using std::vector;
using simgear::PropertyList;
using FGXMLAutopilot::Autopilot;

class FGXMLAutopilotGroupImplementation : public FGXMLAutopilotGroup
{
public:
    FGXMLAutopilotGroupImplementation(const std::string& nodeName):
        FGXMLAutopilotGroup(),
        _nodeName(nodeName)
    {}

    // Subsystem API.
    void init() override;
    InitStatus incrementalInit() override;
    void reinit() override;

    // Subsystem identification.
    static const char* staticSubsystemClassId() { return "xml-autopilot-group"; }

    virtual void addAutopilot( const std::string& name,
                               SGPropertyNode_ptr apNode,
                               SGPropertyNode_ptr config );
    virtual void removeAutopilot( const std::string & name );

private:
    void initFrom( SGPropertyNode_ptr rootNode, const char * childName );
    std::string _nodeName;
};

//------------------------------------------------------------------------------
void FGXMLAutopilotGroupImplementation::addAutopilot( const std::string& name,
                                                      SGPropertyNode_ptr apNode,
                                                      SGPropertyNode_ptr config )
{
  if( has_subsystem(name) )
  {
    SG_LOG( SG_AUTOPILOT,
            SG_ALERT,
            "NOT adding duplicate " << _nodeName << " name '" << name << "'");
    return;
  }

  Autopilot* ap = new Autopilot(apNode, config);
  ap->set_name( name );

  double updateInterval = config->getDoubleValue("update-interval-secs", 0.0);
  set_subsystem( name, ap, updateInterval );
}

//------------------------------------------------------------------------------
void FGXMLAutopilotGroupImplementation::removeAutopilot(const std::string& name)
{
  Autopilot* ap = static_cast<Autopilot*>(get_subsystem(name));
  if( !ap )
  {
    SG_LOG( SG_AUTOPILOT,
            SG_ALERT,
            "CAN NOT remove unknown " << _nodeName << " '" << name << "'");
    return;
  }

  remove_subsystem(name);
}

//------------------------------------------------------------------------------
void FGXMLAutopilotGroupImplementation::reinit()
{
  SGSubsystemGroup::unbind();
  clearSubsystems();
  
  // ensure we bind again, so the SGSubsystemGroup state is correct before
  // we call init. Since there's no actual group members at this point (we
  // cleared them just above) this is purely to ensure SGSubsystemGroup::_state
  // is BIND, so that ::init doesn't assert
  SGSubsystemGroup::bind();
  init();
}

//------------------------------------------------------------------------------
SGSubsystem::InitStatus FGXMLAutopilotGroupImplementation::incrementalInit()
{
  init();
  return INIT_DONE;
}

//------------------------------------------------------------------------------
void FGXMLAutopilotGroupImplementation::init()
{
  initFrom(fgGetNode("/sim/systems"), _nodeName.c_str());
  SGSubsystemGroup::init();
}

//------------------------------------------------------------------------------
void FGXMLAutopilotGroupImplementation::initFrom( SGPropertyNode_ptr rootNode,
                                                  const char* childName )
{
  if( !rootNode )
    return;

  for( auto autopilotNode : rootNode->getChildren(childName) )
  {
    SGPropertyNode_ptr pathNode = autopilotNode->getNode("path");
    if( !pathNode )
    {
      SG_LOG
      (
        SG_AUTOPILOT,
        SG_WARN,
        "No configuration file specified for this " << childName << "!"
      );
      continue;
    }

    std::string apName;
    SGPropertyNode_ptr nameNode = autopilotNode->getNode( "name" );
    if( nameNode != NULL ) {
        apName = nameNode->getStringValue();
    } else {
      std::ostringstream buf;
      buf <<  "unnamed_autopilot_" << autopilotNode->getIndex();
      apName = buf.str();
    }

    {
      // check for duplicate names
      std::string name = apName;
      for( unsigned i = 0; get_subsystem( apName.c_str() ) != NULL; i++ ) {
          std::ostringstream buf;
          buf <<  name << "_" << i;
          apName = buf.str();
      }
      if( apName != name )
        SG_LOG
        (
          SG_AUTOPILOT,
          SG_DEV_WARN,
          "Duplicate " << childName << " configuration name " << name
                                    << ", renamed to " << apName
        );
    }

    addAutopilotFromFile(apName, autopilotNode, pathNode->getStringValue());
  }
}

void FGXMLAutopilotGroup::addAutopilotFromFile( const std::string& name,
                                                SGPropertyNode_ptr apNode,
                                                const char* path )
{
  SGPath config = globals->resolve_maybe_aircraft_path(path);
  if( config.isNull() )
  {
    SG_LOG
    (
      SG_AUTOPILOT,
      SG_ALERT,
      "Cannot find property-rule configuration file '" << path << "'."
    );
    return;
  }
  SG_LOG
  (
    SG_AUTOPILOT,
    SG_INFO,
    "Reading property-rule configuration from " << config
  );

  try
  {
    SGPropertyNode_ptr configNode = new SGPropertyNode();
    readProperties( config, configNode );

    SG_LOG(SG_AUTOPILOT, SG_INFO, "adding  property-rule subsystem " << name);
    addAutopilot(name, apNode, configNode);
  }
  catch (const sg_exception& e)
  {
    SG_LOG
    (
      SG_AUTOPILOT,
      SG_ALERT,
      "Failed to load property-rule configuration: " << config
                                             << ": " << e.getMessage()
    );
    return;
  }
}

//------------------------------------------------------------------------------
FGXMLAutopilotGroup*
FGXMLAutopilotGroup::createInstance(const std::string& nodeName)
{
  return new FGXMLAutopilotGroupImplementation(nodeName);
}
