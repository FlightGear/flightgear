// FGMacOSXEventInput.hxx -- handle event driven input devices for Mac OS X
//
// Written by Tatsuhiro Nishioka, started Aug. 2009.
//
// Copyright (C) 2009 Tasuhiro Nishioka, tat <dot> fgmacosx <at> gmail <dot> com
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

#ifndef __FGMACOSXEVENTINPUT_HXX_
#define __FGMACOSXEVENTINPUT_HXX_

#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <sstream>
#include <mach/mach.h>
#include <mach/mach_error.h>
#include <IOKit/IOKitLib.h>
#include <IOKit/hid/IOHIDLib.h>
#include <IOKit/hid/IOHIDKeys.h>
#include <IOKit/IOCFPlugIn.h>
#include <CoreFoundation/CoreFoundation.h>
#include <Kernel/IOKit/hidsystem/IOHIDUsageTables.h>

#ifndef _TEST
#include "FGEventInput.hxx"
#endif

//
// HID Usage type
// HIDElementFactory will create a proper HIDElement subclass 
// depending on the usage type of a detected HID element
// See http://www.usb.org/developers/devclass_docs/Hut1_12.pdf for detail
//
typedef enum {
  kHIDUsageNotSupported = -1, // Debug use
  kHIDElementType = 0, // Debug use
  kHIDElementPage, // Debug use
  kHIDUsageSel, // Selector; Contained in a Named Array - not supported yet
  kHIDUsageSV,  // Static Value;  Axis?(Constant, Variable, Absolute) - not supported yet
  kHIDUsageSF,  // Static Flag;  Axis?(Constant, Variable, Absolute) - not supported yet
  kHIDUsageDV,  // Dynamic Value; Axis(Data, Variable, Absolute)
  kHIDUsageDF,  // Dynamic Flag;  Axis?(Data, Variable, Absolute) - not supported yet
  kHIDUsageOOC, // On/Off Control; Button?
  kHIDUsageMC,  // Momentary Control; Button
  kHIDUsageOSC, // One Shot Control; Button
  kHIDUsageRTC, // Re-trigger Control; Button?
  // FG specific types
  kHIDUsageHat, // HatSwitch ; a special usage of DV that generates two FGInputEvent instances
  kHIDUsageAxis, // Axis; a special usage of DV that has either abs- or rel- prefix 
} HIDUsageType;
  

class HIDElement;
struct FGMacOSXEventData : public FGEventData {
  FGMacOSXEventData(std::string name, double value, double dt, int modifiers) : 
    FGEventData(value, dt, modifiers), name(name) {}
  std::string name;
};

//
// For mapping HID element page/usage, usage type, and event names
//
struct HIDTypes {
  long key;
  HIDUsageType type;
  const char *eventName;
};

class FGMacOSXInputDevice;

//
// Generic HIDElement for DV, DF types
// 
class HIDElement {
public:
  HIDElement(CFDictionaryRef element, long page, long usage);
  virtual ~HIDElement() {}
  bool isUpdated();
  std::string getName() { return name; }
  virtual void generateEvent(FGMacOSXInputDevice *device, double dt, int modifiers);
  virtual long read(IOHIDDeviceInterface **interface);
  virtual void write(IOHIDDeviceInterface **interface, double value) { 
    SG_LOG(SG_INPUT, SG_WARN, "writing is not implemented on this device: " << name);
  }
protected:
  IOHIDElementCookie cookie;
  long type;
  long page;
  long usage;
  float value;
  float lastValue;

  std::string name;
};

class AxisElement : public HIDElement {
public:
  AxisElement(CFDictionaryRef element, long page, long usage);
  virtual ~AxisElement() {}
  virtual long read(IOHIDDeviceInterface **interface);
  virtual void generateEvent(FGMacOSXInputDevice *device, double dt, int modifiers);
private:
  long min;
  long max;
  float dead_band;
  float saturate;
  long center;
  bool isRelative;
  bool isWrapping;
  bool isNonLinear;
};

class ButtonElement : public HIDElement {
public:
  ButtonElement(CFDictionaryRef element, long page, long usage);
  virtual ~ButtonElement() {}
private:
};

class HatElement : public HIDElement {
public:
  HatElement(CFDictionaryRef element, long page, long usage, int id);
  virtual ~HatElement() {}
  virtual void generateEvent(FGMacOSXInputDevice *device, double dt, int modifiers);
private:
  int id;
  long min;
  long max;
};

class LEDElement : public HIDElement {
public:
  LEDElement(CFDictionaryRef element, long page, long usage);
  virtual ~LEDElement() {}
  virtual void write(IOHIDDeviceInterface **interface, double value);
};

class FeatureElement : public HIDElement {
public:
  FeatureElement(CFDictionaryRef element, long page, long usage, int count);
  virtual ~FeatureElement() {}
  virtual long read(IOHIDDeviceInterface **inerface);
};

//
// FGMacOSXInputDevice
// Mac OS X specific FGInputDevice
//
class FGMacOSXInputDevice : public FGInputDevice {
public:
  FGMacOSXInputDevice(io_object_t device);
  virtual ~FGMacOSXInputDevice() { Close(); }
  void Open();
  void Close();
  virtual void update(double dt);
  virtual const char *TranslateEventName(FGEventData &eventData);
  void Send( const char *eventName, double value);

  // Mac OS X specific methods
  CFDictionaryRef getProperties() {
    return FGMacOSXInputDevice::getProperties(device); 
  }
  static CFDictionaryRef getProperties(io_object_t device); 
  void addElement(HIDElement *element);

private:
  io_object_t device;
  IOHIDDeviceInterface **devInterface;
  std::map<std::string, HIDElement *> elements; // maps eventName and its relevant element for Send()
};

//
// HID element factory that iteratively parses and creates 
// HIDElement instances and add these to FGMacOSXDeviceInput
//
class HIDElementFactory {
public:
  static void create(CFTypeRef element, FGMacOSXInputDevice *inputDevice);
  static void elementEnumerator( const void *element, void *inputDevice); 
  static void parseElement(CFDictionaryRef element, FGMacOSXInputDevice *device);
};

//
// Mac OS X specific FGEventInput
//
class FGMacOSXEventInput : public FGEventInput {
public:
  FGMacOSXEventInput() : FGEventInput() { SG_LOG(SG_INPUT, SG_DEBUG, "FGMacOSXEventInput created"); }
  virtual ~FGMacOSXEventInput();
  virtual void update(double dt);
  virtual void init();

  // Mac OS X specific methods
  static void deviceAttached(void *device, io_iterator_t iterator) {
    static_cast<FGMacOSXEventInput *>(device)->attachDevice(iterator); 
  }

  static void deviceDetached(void *device, io_iterator_t iterator) { 
    static_cast<FGMacOSXEventInput *>(device)->detachDevice(iterator); 
  }

  void attachDevice(io_iterator_t iterator);
  void detachDevice(io_iterator_t iterator);

private:
  IONotificationPortRef notifyPort;
  CFRunLoopSourceRef runLoopSource;
  io_iterator_t addedIterator; 
  io_iterator_t removedIterator;

  // maps FG device property ID (i.e. /input/events/device[ID]) with io_object for detaching devices
  std::map<io_object_t, unsigned> deviceIndices; 
};


//
// For obtaining event name and type from both HID element page and usage
// 
class HIDTypeByID : public std::map<long, std::pair<HIDUsageType, const char *>*> {
public:
  HIDTypeByID(struct HIDTypes *table) {
    for( int i = 0; table[i].key!= -1; i++ )
      (*this)[table[i].key] = new std::pair<HIDUsageType, const char *>(table[i].type, table[i].eventName);
  }

  ~HIDTypeByID() {
    std::map<long, std::pair<HIDUsageType, const char *>*>::iterator it;
    for (it = this->begin(); it != this->end(); it++) {
      delete (*it).second;
    }
    clear();
  }

  // key = (HID_element_page) << 16 | HID_element_usage)
  const char *getName(long key) {
    std::pair<HIDUsageType, const char *> *usageType = (*this)[key];
    if (usageType == NULL) {
      return "";
    } else {
      return usageType->second;
    }
  }

  const HIDUsageType getType(long key) {
    std::pair<HIDUsageType, const char *> *usageType = (*this)[key];
    if (usageType == NULL) {
      return kHIDUsageNotSupported;
    } else {
      return usageType->first;
    }
  }
};

#endif
