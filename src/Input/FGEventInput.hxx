// FGEventInput.hxx -- handle event driven input devices
//
// Written by Torsten Dreyer, started July 2009
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

#ifndef __FGEVENTINPUT_HXX
#define __FGEVENTINPUT_HXX

#include "FGCommonInput.hxx"
#include "FGButton.hxx"
#include "FGDeviceConfigurationMap.hxx"
#include <simgear/structure/subsystem_mgr.hxx>

/*
 * A base structure for event data. 
 * To be extended for O/S specific implementation data
 */
struct FGEventData {
  FGEventData( double aValue, double aDt, int aModifiers ) : value(aValue), dt(aDt), modifiers(aModifiers) {}
  int modifiers;
  double value;
  double dt;
};

class FGEventSetting : public SGReferenced {
public:
  FGEventSetting( SGPropertyNode_ptr base );

  bool Test();

  /* 
   * access for the value property
   */
  double GetValue();

protected:
  double value;
  SGPropertyNode_ptr valueNode;
  SGSharedPtr<const SGCondition> condition;
};

typedef SGSharedPtr<FGEventSetting> FGEventSetting_ptr;
typedef vector<FGEventSetting_ptr> setting_list_t;

/*
 * A wrapper class for a configured event. 
 * 
 * <event>
 *   <desc>Change the view pitch</desc>
 *   <name>rel-x-rotate</name>
 *   <binding>
 *     <command>property-adjust</command>
 *     <property>sim/current-view/pitch-offset-deg</property>
 *     <factor type="double">0.01</factor>
 *     <min type="double">-90.0</min>
 *     <max type="double">90.0</max>
 *     <wrap type="bool">false</wrap>
 *   </binding>
 *   <mod-xyz>
 *    <binding>
 *      ...
 *    </binding>
 *   </mod-xyz>
 * </event>
 */
class FGInputDevice;
class FGInputEvent : public SGReferenced,FGCommonInput {
public:
  /*
   * Constructor for the class. The arg node shall point
   * to the property corresponding to the <event>  node
   */
  FGInputEvent( FGInputDevice * device, SGPropertyNode_ptr node );
  virtual ~FGInputEvent();

  /*
   * dispatch the event value through all bindings 
   */
  virtual void fire( FGEventData & eventData );

  /*
   * access for the name property
   */
  string GetName() const { return name; }

  /*
   * access for the description property
   */
  string GetDescription() const { return desc; }

  virtual void update( double dt );

  static FGInputEvent * NewObject( FGInputDevice * device, SGPropertyNode_ptr node );

protected:
  /* A more or less meaningfull description of the event */
  string desc;

  /* One of the predefined names of the event */
  string name;

  /* A list of SGBinding objects */
  binding_list_t bindings[KEYMOD_MAX];

  /* A list of FGEventSetting objects */
  setting_list_t settings;

  /* A pointer to the associated device */
  FGInputDevice * device;

  double lastDt;
  double intervalSec;
  double lastSettingValue;
};

class FGButtonEvent : public FGInputEvent {
public:
  FGButtonEvent( FGInputDevice * device, SGPropertyNode_ptr node );
  virtual void fire( FGEventData & eventData );

protected:
  bool repeatable;
  bool lastState;
};

class FGAxisEvent : public FGInputEvent {
public:
  FGAxisEvent( FGInputDevice * device, SGPropertyNode_ptr node );
protected:
  virtual void fire( FGEventData & eventData );
  double tolerance;
  double minRange;
  double maxRange;
  double center;
  double deadband;
  double lowThreshold;
  double highThreshold;
  double lastValue;
};

typedef class SGSharedPtr<FGInputEvent> FGInputEvent_ptr;

/*
 * A abstract class implementing basic functionality of input devices for
 * all operating systems. This is the base class for the O/S-specific
 * implementation of input device handlers
 */
class FGInputDevice : public SGReferenced {
public:
  FGInputDevice() : debugEvents(false) {}
  FGInputDevice( string aName ) : name(aName) {}
    
  virtual ~FGInputDevice();

  virtual void Open() = 0;
  virtual void Close() = 0;

  virtual void Send( const char * eventName, double value ) = 0;

  inline void Send( const string & eventName, double value ) {
    Send( eventName.c_str(), value );
  }

  virtual const char * TranslateEventName( FGEventData & eventData ) = 0;


  void SetName( string name );
  string & GetName() { return name; }

  void HandleEvent( FGEventData & eventData );

  void AddHandledEvent( FGInputEvent_ptr handledEvent ) {
    if( handledEvents.count( handledEvent->GetName() ) == 0 )
      handledEvents[handledEvent->GetName()] = handledEvent;
  }

  virtual void update( double dt );

  bool GetDebugEvents () const { return debugEvents; }
  void SetDebugEvents( bool value ) { debugEvents = value; }

private:
  // A map of events, this device handles
  map<string,FGInputEvent_ptr> handledEvents;

  // the device has a name to be recognized
  string name;

  // print out events comming in from the device
  // if true
  bool   debugEvents;
};

typedef SGSharedPtr<FGInputDevice> FGInputDevice_ptr;


/*
 * The Subsystem for the event input device 
 */
class FGEventInput : public SGSubsystem,FGCommonInput {
public:
  FGEventInput();
  virtual ~FGEventInput();
  virtual void init();
  virtual void postinit();
  virtual void update( double dt );

protected:
  static const char * PROPERTY_ROOT;

  void AddDevice( FGInputDevice * inputDevice );
  map<int,FGInputDevice*> input_devices;
  FGDeviceConfigurationMap configMap;
};

#endif
