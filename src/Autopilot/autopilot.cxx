// autopilot.cxx - an even more flexible, generic way to build autopilots
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

#include <simgear/structure/StateMachine.hxx>
#include <simgear/sg_inlines.h>

#include "component.hxx"
#include "functor.hxx"
#include "predictor.hxx"
#include "digitalfilter.hxx"
#include "pisimplecontroller.hxx"
#include "pidcontroller.hxx"
#include "logic.hxx"
#include "flipflop.hxx"

#include "Main/fg_props.hxx"

using std::map;
using std::string;

using namespace FGXMLAutopilot;

class StateMachineComponent : public Component
{
public:
    StateMachineComponent( SGPropertyNode& props_root,
                           SGPropertyNode& cfg ) {
        inner = simgear::StateMachine::createFromPlist(&cfg, &props_root);
    }

    // Subsystem identification.
    static const char* staticSubsystemClassId() { return "state-machine"; }

    virtual bool configure( const std::string & nodeName, SGPropertyNode_ptr config) {
        return false;
    }
    
    virtual void update( bool firstTime, double dt ) {
        SG_UNUSED(firstTime);
        inner->update(dt);
    }
  
private:
    simgear::StateMachine_ptr inner;
};


// Register the subsystem.
#if 0
SGSubsystemMgr::Registrant<StateMachineComponent> registrantStateMachineComponent;
#endif


class StateMachineFunctor : public FunctorBase<Component>
{
public:
  virtual ~StateMachineFunctor() {}
  virtual Component* operator()( SGPropertyNode& cfg,
                                 SGPropertyNode& prop_root )
  {
    return new StateMachineComponent(cfg, prop_root);
  }
};


class ComponentForge : public map<string,FunctorBase<Component> *> {
public:
    virtual ~ ComponentForge();
};

ComponentForge::~ComponentForge()
{
    for( iterator it = begin(); it != end(); ++it )
        delete it->second;
}

void readInterfaceProperties( SGPropertyNode_ptr prop_root,
                              SGPropertyNode_ptr cfg )
{
  simgear::PropertyList cfg_props = cfg->getChildren("property");
  for( simgear::PropertyList::iterator it = cfg_props.begin();
                                       it != cfg_props.end();
                                     ++it )
  {
    SGPropertyNode_ptr prop = prop_root->getNode((*it)->getStringValue(), true);
    SGPropertyNode* val = (*it)->getNode("_attr_/value");

    if( val )
    {
      prop->setDoubleValue( val->getDoubleValue() );

      // TODO should we keep the _attr_ node, as soon as the property browser is
      //      able to cope with it?
      (*it)->removeChild("_attr_", 0);
    }
  }
}

static ComponentForge componentForge;

Autopilot::Autopilot( SGPropertyNode_ptr rootNode, SGPropertyNode_ptr configNode ) :
  _name("unnamed autopilot"),
  _serviceable(true),
  _rootNode(rootNode)
{
  if (componentForge.empty())
  {
      componentForge["pid-controller"]       = new CreateAndConfigureFunctor<PIDController,Component>();
      componentForge["pi-simple-controller"] = new CreateAndConfigureFunctor<PISimpleController,Component>();
      componentForge["predict-simple"]       = new CreateAndConfigureFunctor<Predictor,Component>();
      componentForge["filter"]               = new CreateAndConfigureFunctor<DigitalFilter,Component>();
      componentForge["logic"]                = new CreateAndConfigureFunctor<Logic,Component>();
      componentForge["flipflop"]             = new CreateAndConfigureFunctor<FlipFlop,Component>();
      componentForge["state-machine"]        = new StateMachineFunctor();
  }

  if( !configNode )
    configNode = rootNode;

  // property-root can be set in config file and overridden in the local system
  // node. This allows using the same autopilot multiple times but with
  // different paths (with all relative property paths being relative to the
  // node specified with property-root)
  SGPropertyNode_ptr prop_root_node = rootNode->getChild("property-root");
  if( !prop_root_node )
    prop_root_node = configNode->getChild("property-root");

  SGPropertyNode_ptr prop_root =
    fgGetNode(prop_root_node ? prop_root_node->getStringValue() : "/", true);

  // Just like the JSBSim interface properties for systems, create properties
  // given in the autopilot file and set to given (default) values.
  readInterfaceProperties(prop_root, configNode);

  // Afterwards read the properties specified in local system node to allow
  // overriding initial or default values. This allows reusing components with
  // just different "parameter" values.
  readInterfaceProperties(prop_root, rootNode);

  int count = configNode->nChildren();
  for( int i = 0; i < count; ++i )
  {
    SGPropertyNode_ptr node = configNode->getChild(i);
    string childName = node->getName();
    if(    childName == "property"
        || childName == "property-root" )
      continue;
    if( componentForge.count(childName) == 0 )
    {
      SG_LOG(SG_AUTOPILOT, SG_BULK, "unhandled element <" << childName << ">");
      continue;
    }

    Component * component = (*componentForge[childName])(*prop_root, *node);
    if( component->subsystemId().length() == 0 ) {
      std::ostringstream buf;
      buf <<  "unnamed_component_" << i;
    }

    double updateInterval = node->getDoubleValue( "update-interval-secs", 0.0 );

    SG_LOG( SG_AUTOPILOT, SG_DEBUG, "adding  autopilot component \"" << childName << "\" as \"" << component->subsystemId() << "\" with interval=" << updateInterval );
    add_component(component,updateInterval);
  }
}

Autopilot::~Autopilot() 
{
}

void Autopilot::bind() 
{
  fgTie( _rootNode->getNode("serviceable", true)->getPath().c_str(), this, 
    &Autopilot::is_serviceable, &Autopilot::set_serviceable );
  SGSubsystemGroup::bind();
}

void Autopilot::unbind() 
{
  _rootNode->untie( "serviceable" );
  SGSubsystemGroup::unbind();
}

void Autopilot::add_component( Component * component, double updateInterval )
{
  if( component == NULL ) return;

  // check for duplicate name
  const auto originalName = string{component->subsystemId()};
  std::string name = originalName;
  if (name.empty()) {
    name = "unnamed_autopilot";
  } 

  for( unsigned int i = 0; get_subsystem( name) != nullptr; i++ ) {
      std::ostringstream buf;
      buf <<  component->subsystemId() << "_" << i;
      name = buf.str();
  }

  if (!originalName.empty() && (name != originalName)) {
    SG_LOG( SG_AUTOPILOT, SG_DEV_WARN, "Duplicate autopilot component " << originalName << ", renamed to " << name );
  }

  set_subsystem( name, component, updateInterval );
}

void Autopilot::update( double dt ) 
{
  if( !_serviceable || dt <= SGLimitsd::min() )
    return;
  SGSubsystemGroup::update( dt );
}
