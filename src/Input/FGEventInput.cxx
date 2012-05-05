// FGEventInput.cxx -- handle event driven input devices
//
// Written by Torsten Dreyer, started July 2009.
//
// Copyright (C) 2009 Torsten Dreyer, Torsten (at) t3r _dot_ de
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

#include <cstring>
#include "FGEventInput.hxx"
#include <Main/fg_props.hxx>
#include <simgear/io/sg_file.hxx>
#include <simgear/props/props_io.hxx>
#include <simgear/math/SGMath.hxx>
#include <Scripting/NasalSys.hxx>

using simgear::PropertyList;
using std::cout;
using std::endl;
using std::map;

FGEventSetting::FGEventSetting( SGPropertyNode_ptr base ) :
  value(0.0)
{
  SGPropertyNode_ptr n;

  if( (n = base->getNode( "value" )) != NULL ) {  
    valueNode = NULL;
    value = n->getDoubleValue();
  } else {
    n = base->getNode( "property" );
    if( n == NULL ) {
      SG_LOG( SG_INPUT, SG_WARN, "Neither <value> nor <property> defined for event setting." );
    } else {
      valueNode = fgGetNode( n->getStringValue(), true );
    }
  }

  if( (n = base->getChild("condition")) != NULL )
    condition = sgReadCondition(base, n);
  else
    SG_LOG( SG_INPUT, SG_ALERT, "No condition for event setting." );
}

double FGEventSetting::GetValue()
{
  return valueNode == NULL ? value : valueNode->getDoubleValue();
}

bool FGEventSetting::Test()
{
  return condition == NULL ? true : condition->test();
}

static inline bool StartsWith( string & s, const char * cp )
{
  return s.find( cp ) == 0;
}

FGInputEvent * FGInputEvent::NewObject( FGInputDevice * device, SGPropertyNode_ptr node )
{
  string name = node->getStringValue( "name", "" );
  if( StartsWith( name, "button-" ) )
    return new FGButtonEvent( device, node );

  if( StartsWith( name, "rel-" ) )
    return new FGRelAxisEvent( device, node );

  if( StartsWith( name, "abs-" ) )
    return new FGAbsAxisEvent( device, node );

  return new FGInputEvent( device, node );
}

FGInputEvent::FGInputEvent( FGInputDevice * aDevice, SGPropertyNode_ptr node ) :
  device( aDevice ),
  lastDt(0.0), 
  lastSettingValue(std::numeric_limits<float>::quiet_NaN())
{
  name = node->getStringValue( "name", "" );
  desc = node->getStringValue( "desc", "" );
  intervalSec = node->getDoubleValue("interval-sec",0.0);
  
  read_bindings( node, bindings, KEYMOD_NONE, device->GetNasalModule() );

  PropertyList settingNodes = node->getChildren("setting");
  for( PropertyList::iterator it = settingNodes.begin(); it != settingNodes.end(); ++it )
    settings.push_back( new FGEventSetting( *it ) );
}

FGInputEvent::~FGInputEvent()
{
}

void FGInputEvent::update( double dt )
{
  for( setting_list_t::iterator it = settings.begin(); it != settings.end(); ++it ) {
    if( (*it)->Test() ) {
      double value = (*it)->GetValue();
      if( value != lastSettingValue ) {
        device->Send( GetName(), (*it)->GetValue() );
        lastSettingValue = value;
      }
    }
  }
}

void FGInputEvent::fire( FGEventData & eventData )
{
  lastDt += eventData.dt;
  if( lastDt >= intervalSec ) {

    for( binding_list_t::iterator it = bindings[eventData.modifiers].begin(); it != bindings[eventData.modifiers].end(); ++it )
      fire( *it, eventData );

    lastDt -= intervalSec;
  }
}

void FGInputEvent::fire( SGBinding * binding, FGEventData & eventData )
{
  binding->fire();
}



FGAxisEvent::FGAxisEvent( FGInputDevice * device, SGPropertyNode_ptr node ) :
  FGInputEvent( device, node )
{
  tolerance = node->getDoubleValue("tolerance", 0.002);
  minRange = node->getDoubleValue("min-range", 0.0 );
  maxRange = node->getDoubleValue("max-range", 0.0 );
  center = node->getDoubleValue("center", 0.0);
  deadband = node->getDoubleValue("dead-band", 0.0);
  lowThreshold = node->getDoubleValue("low-threshold", -0.9);
  highThreshold = node->getDoubleValue("high-threshold", 0.9);
  lastValue = 9999999;
}

void FGAxisEvent::fire( FGEventData & eventData )
{
  if (fabs( eventData.value - lastValue) < tolerance)
    return;
  lastValue = eventData.value;

  // We need a copy of the  FGEventData struct to set the new value and to avoid side effects
  FGEventData ed = eventData;

  if( fabs(ed.value) < deadband )
    ed.value = 0.0;

  if( minRange != maxRange )
    ed.value = 2.0*(eventData.value-minRange)/(maxRange-minRange)-1.0;

  FGInputEvent::fire( ed );
}

void FGAbsAxisEvent::fire( SGBinding * binding, FGEventData & eventData )
{
  // sets the "setting" node
  binding->fire( eventData.value );
}

FGRelAxisEvent::FGRelAxisEvent( FGInputDevice * device, SGPropertyNode_ptr node ) : 
  FGAxisEvent( device, node ) 
{
  // relative axes can't use tolerance
  tolerance = 0.0;
}

void FGRelAxisEvent::fire( SGBinding * binding, FGEventData & eventData )
{
  // sets the "offset" node
  binding->fire( eventData.value, 1.0 );
}

FGButtonEvent::FGButtonEvent( FGInputDevice * device, SGPropertyNode_ptr node ) :
  FGInputEvent( device, node ),
  repeatable(false),
  lastState(false)
{
  repeatable = node->getBoolValue("repeatable", repeatable);
}

void FGButtonEvent::fire( FGEventData & eventData )
{
  bool pressed = eventData.value > 0.0;
  if (pressed) {
    // The press event may be repeated.
    if (!lastState || repeatable) {
      SG_LOG( SG_INPUT, SG_DEBUG, "Button has been pressed" );
      FGInputEvent::fire( eventData );
    }
  } else {
    // The release event is never repeated.
    if (lastState) {
      SG_LOG( SG_INPUT, SG_DEBUG, "Button has been released" );
      eventData.modifiers|=KEYMOD_RELEASED;
      FGInputEvent::fire( eventData );
    }
  }
          
  lastState = pressed;
}

FGInputDevice::~FGInputDevice()
{
  FGNasalSys *nas = (FGNasalSys *)globals->get_subsystem("nasal");
  if (nas && deviceNode ) {
    SGPropertyNode_ptr nasal = deviceNode->getNode("nasal");
    if( nasal ) {
      SGPropertyNode_ptr nasalClose = nasal->getNode("close");
      if (nasalClose) {
        const string s = nasalClose->getStringValue();
        nas->createModule(nasalModule.c_str(), nasalModule.c_str(), s.c_str(), s.length(), deviceNode );
      }
    }
    nas->deleteModule(nasalModule.c_str());
  }
} 

void FGInputDevice::Configure( SGPropertyNode_ptr aDeviceNode )
{
  deviceNode = aDeviceNode;

  nasalModule = string("__event:") + GetName();

  PropertyList eventNodes = deviceNode->getChildren( "event" );
  for( PropertyList::iterator it = eventNodes.begin(); it != eventNodes.end(); ++it )
    AddHandledEvent( FGInputEvent::NewObject( this, *it ) );

  debugEvents = deviceNode->getBoolValue("debug-events", debugEvents );
  grab = deviceNode->getBoolValue("grab", grab );

  // TODO:
  // add nodes for the last event:
  // last-event/name [string]
  // last-event/value [double]

  SGPropertyNode_ptr nasal = deviceNode->getNode("nasal");
  if (nasal) {
    SGPropertyNode_ptr open = nasal->getNode("open");
    if (open) {
      const string s = open->getStringValue();
      FGNasalSys *nas = (FGNasalSys *)globals->get_subsystem("nasal");
      if (nas)
        nas->createModule(nasalModule.c_str(), nasalModule.c_str(), s.c_str(), s.length(), deviceNode );
    }
  }

}

void FGInputDevice::update( double dt )
{
  for( map<string,FGInputEvent_ptr>::iterator it = handledEvents.begin(); it != handledEvents.end(); it++ )
    (*it).second->update( dt );
}

void FGInputDevice::HandleEvent( FGEventData & eventData )
{
  string eventName = TranslateEventName( eventData );  
  if( debugEvents )
    cout << GetName() << " has event " << 
    eventName << " modifiers=" << eventData.modifiers << " value=" << eventData.value << endl;

  if( handledEvents.count( eventName ) > 0 ) {
    handledEvents[ eventName ]->fire( eventData );
  }
}

void FGInputDevice::SetName( string name )
{
  this->name = name; 
}

const char * FGEventInput::PROPERTY_ROOT = "/input/event";

FGEventInput::FGEventInput() : 
  configMap( "Input/Event", fgGetNode( PROPERTY_ROOT, true ), "device-named" )
{
}

FGEventInput::~FGEventInput()
{
  for( map<int,FGInputDevice*>::iterator it = input_devices.begin(); it != input_devices.end(); ++it )
    delete (*it).second;
  input_devices.clear();
}

void FGEventInput::init( )
{
  SG_LOG(SG_INPUT, SG_DEBUG, "Initializing event bindings");
//  SGPropertyNode * base = fgGetNode("/input/event", true);

}

void FGEventInput::postinit ()
{
}

void FGEventInput::update( double dt )
{
  // call each associated device's update() method
  for( map<int,FGInputDevice*>::iterator it =  input_devices.begin(); it != input_devices.end(); ++it )
    (*it).second->update( dt );
}

unsigned FGEventInput::AddDevice( FGInputDevice * inputDevice )
{
  SGPropertyNode_ptr baseNode = fgGetNode( PROPERTY_ROOT, true );
  SGPropertyNode_ptr deviceNode = NULL;

  // look for configuration in the device map
  if( configMap.count( inputDevice->GetName() ) > 0 ) {
    // found - copy to /input/event/device[n]

    // find a free index
    unsigned index;
    for( index = 0; index < MAX_DEVICES; index++ )
      if( (deviceNode = baseNode->getNode( "device", index, false ) ) == NULL )
        break;

    if( index == MAX_DEVICES ) {
      SG_LOG(SG_INPUT, SG_WARN, "To many event devices - ignoring " << inputDevice->GetName() );
      return INVALID_DEVICE_INDEX;
    }

    // create this node 
    deviceNode = baseNode->getNode( "device", index, true );

    // and copy the properties from the configuration tree
    copyProperties( configMap[ inputDevice->GetName() ], deviceNode );

  }

  if( deviceNode == NULL ) {
    SG_LOG(SG_INPUT, SG_DEBUG, "No configuration found for device " << inputDevice->GetName() );
    delete  inputDevice;
    return INVALID_DEVICE_INDEX;
  }

  inputDevice->Configure( deviceNode );

  try {	
    inputDevice->Open();
    input_devices[ deviceNode->getIndex() ] = inputDevice;
  }
  catch( ... ) {
    delete  inputDevice;
    SG_LOG(SG_INPUT, SG_ALERT, "can't open InputDevice " << inputDevice->GetName()  );
    return INVALID_DEVICE_INDEX;
  }

  SG_LOG(SG_INPUT, SG_DEBUG, "using InputDevice " << inputDevice->GetName()  );
  return deviceNode->getIndex();
}

void FGEventInput::RemoveDevice( unsigned index )
{
  // not fully implemented yet
  SGPropertyNode_ptr baseNode = fgGetNode( PROPERTY_ROOT, true );
  SGPropertyNode_ptr deviceNode = NULL;

  FGInputDevice *inputDevice = input_devices[index];
  if (inputDevice) {
    input_devices.erase(index);
    delete inputDevice;
    
  }
  deviceNode = baseNode->removeChild("device", index, false);
}

