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

#include "FGEventInput.hxx"
#include <Main/fg_props.hxx>
#include <simgear/io/sg_file.hxx>
#include <poll.h>
#include <linux/input.h>

static inline bool StartsWith( string & s, const char * cp )
{
  return s.compare( 0, strlen(cp), cp ) == 0;
}

FGInputEvent * FGInputEvent::NewObject( SGPropertyNode_ptr node )
{
  string name = node->getStringValue( "name" );
  if( StartsWith( name, "button-" ) )
    return new FGButtonEvent( node );

  if( StartsWith( name, "rel-" ) )
    return new FGAxisEvent( node );

  if( StartsWith( name, "abs-" ) )
    return new FGAxisEvent( node );

  return NULL;
}

FGInputEvent::FGInputEvent( SGPropertyNode_ptr node ) :
  lastDt(0.0)
{
  name = node->getStringValue( "name" );
  desc = node->getStringValue( "desc" );
  intervalSec = node->getDoubleValue("interval-sec",0.0);
  string module = "event";
  read_bindings( node, bindings, KEYMOD_NONE, module );
}

FGInputEvent::~FGInputEvent()
{
}

void FGInputEvent::fire( FGEventData & eventData )
{
  lastDt += eventData.dt;
  if( lastDt >= intervalSec ) {

    for( binding_list_t::iterator it = bindings[KEYMOD_NONE].begin(); it != bindings[KEYMOD_NONE].end(); it++ )
      (*it)->fire( eventData.value, 1.0 );

    lastDt -= intervalSec;
  }
}

FGAxisEvent::FGAxisEvent( SGPropertyNode_ptr node ) :
  FGInputEvent( node )
{
  tolerance = node->getDoubleValue("tolerance", 0.002);
  minRange = node->getDoubleValue("min-range", -1024.0);
  maxRange = node->getDoubleValue("max-range", 1024.0);
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
  FGInputEvent::fire( eventData );
}

FGButtonEvent::FGButtonEvent( SGPropertyNode_ptr node ) :
  FGInputEvent( node )
{
}

void FGButtonEvent::fire( FGEventData & eventData )
{
  FGInputEvent::fire( eventData );
}

FGInputDevice::~FGInputDevice()
{
} 

void FGInputDevice::HandleEvent( FGEventData & eventData )
{
  string eventName = TranslateEventName( eventData );  
  cout << GetName() << " has event " << eventName << endl;
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
  for( map<int,FGInputDevice*>::iterator it = input_devices.begin(); it != input_devices.end(); it++ )
    delete (*it).second;
  input_devices.clear();
}

void FGEventInput::init( )
{
  SG_LOG(SG_INPUT, SG_DEBUG, "Initializing event bindings");
  SGPropertyNode * base = fgGetNode("/input/event", true);

}

void FGEventInput::postinit ()
{
}

void FGEventInput::AddDevice( FGInputDevice * inputDevice )
{
  SGPropertyNode_ptr baseNode = fgGetNode( PROPERTY_ROOT, true );
  SGPropertyNode_ptr deviceNode = NULL;

  // look for configuration in the device map
  if( configMap.count( inputDevice->GetName() ) > 0 ) {
    // found - copy to /input/event/device[n]

    // find a free index
    unsigned index;
    for( index = 0; index < 1000; index++ )
      if( (deviceNode = baseNode->getNode( "device", index, false ) ) == NULL )
        break;

    if( index == 1000 ) {
      SG_LOG(SG_INPUT, SG_WARN, "To many event devices - ignoring " << inputDevice->GetName() );
      return;
    }

    // create this node 
    deviceNode = baseNode->getNode( "device", index, true );

    // and copy the properties from the configuration tree
    copyProperties( configMap[ inputDevice->GetName() ], deviceNode );
  }

  if( deviceNode == NULL ) {
    SG_LOG(SG_INPUT, SG_WARN, "No configuration found for device " << inputDevice->GetName() );
    return;
  }

  vector<SGPropertyNode_ptr> eventNodes = deviceNode->getChildren( "event" );
  for( vector<SGPropertyNode_ptr>::iterator it = eventNodes.begin(); it != eventNodes.end(); it++ ) {
    FGInputEvent * p = FGInputEvent::NewObject( *it );
    if( p == NULL ) {
      SG_LOG(SG_INPUT, SG_WARN, "Unhandled event/name in " << inputDevice->GetName()  );
      continue;
    }
    inputDevice->AddHandledEvent( p );
  }

  // TODO:
  // add nodes for the last event:
  // last-event/name [string]
  // last-event/value [double]
  
  try {	
    inputDevice->Open();
    input_devices[ deviceNode->getIndex() ] = inputDevice;
  }
  catch( ... ) {
    SG_LOG(SG_INPUT, SG_WARN, "can't open InputDevice " << inputDevice->GetName()  );
  }
}
