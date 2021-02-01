/*
     PLIB - A Suite of Portable Game Libraries
     Copyright (C) 1998,2002  Steve Baker

     This library is free software; you can redistribute it and/or
     modify it under the terms of the GNU Library General Public
     License as published by the Free Software Foundation; either
     version 2 of the License, or (at your option) any later version.

     This library is distributed in the hope that it will be useful,
     but WITHOUT ANY WARRANTY; without even the implied warranty of
     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
     Library General Public License for more details.

     You should have received a copy of the GNU Library General Public
     License along with this library; if not, write to the Free Software
     Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA

     For further information visit http://plib.sourceforge.net

     $Id: jsMacOSX.cxx 2165 2011-01-22 22:56:03Z fayjf $
*/

#include "FlightGear_js.h"

#include <mach/mach.h>
#include <IOKit/IOKitLib.h>
#include <IOKit/hid/IOHIDLib.h>
#include <mach/mach_error.h>
#include <IOKit/hid/IOHIDKeys.h>
#include <IOKit/IOCFPlugIn.h>
#include <CoreFoundation/CoreFoundation.h>

#ifdef MACOS_10_0_4
#	include <IOKit/hidsystem/IOHIDUsageTables.h>
#else
/* The header was moved here in MacOS X 10.1 */
#	include <Kernel/IOKit/hidsystem/IOHIDUsageTables.h>
#endif

#include <simgear/debug/logstream.hxx>

static const int kNumDevices = 32;
static int numDevices = -1;
static io_object_t ioDevices[kNumDevices];

static int NS_hat[8] = {1, 1, 0, -1, -1, -1, 0, 1};
static int WE_hat[8] = {0, 1, 1, 1, 0, -1, -1, -1};

struct os_specific_s {
  IOHIDDeviceInterface ** hidDev;
  IOHIDElementCookie buttonCookies[41];
  IOHIDElementCookie axisCookies[_JS_MAX_AXES];
  IOHIDElementCookie hatCookies[_JS_MAX_HATS];
  int num_hats;
  long hat_min[_JS_MAX_HATS];
  long hat_max[_JS_MAX_HATS];

  bool removed = false;

  void enumerateElements(jsJoystick* joy, CFTypeRef element);
  static void elementEnumerator( const void *element, void* vjs);
  /// callback for CFArrayApply
  void parseElement(jsJoystick* joy, CFDictionaryRef element);
  void addAxisElement(jsJoystick* joy, CFDictionaryRef axis);
  void addButtonElement(jsJoystick* joy, CFDictionaryRef button);
  void addHatElement(jsJoystick* joy, CFDictionaryRef hat);
};

static void findDevices(mach_port_t);
static CFDictionaryRef getCFProperties(io_object_t);


void jsInit()
{
	if (numDevices < 0) {
		numDevices = 0;

		mach_port_t masterPort;
		IOReturn rv = IOMasterPort(bootstrap_port, &masterPort);
		if (rv != kIOReturnSuccess) {
			jsSetError(SG_WARN, "error getting master Mach port");
			return;
		}

		findDevices(masterPort);
	}
}

void jsShutdown()
{
    numDevices = -1;
}

/** open the IOKit connection, enumerate all the HID devices, add their
interface references to the static array. We then use the array index
as the device number when we come to open() the joystick. */
static void findDevices(mach_port_t masterPort)
{
	CFMutableDictionaryRef hidMatch = NULL;
	IOReturn rv = kIOReturnSuccess;
	io_iterator_t hidIterator;

	// build a dictionary matching HID devices
	hidMatch = IOServiceMatching(kIOHIDDeviceKey);

	rv = IOServiceGetMatchingServices(masterPort, hidMatch, &hidIterator);
	if (rv != kIOReturnSuccess || !hidIterator) {
		jsSetError(SG_WARN, "no joystick (HID) devices found");
		return;
	}

	// iterate
	io_object_t ioDev;

	while ((ioDev = IOIteratorNext(hidIterator))) {
		// filter out keyboard and mouse devices
		CFDictionaryRef properties = getCFProperties(ioDev);
		long usage, page;

		CFTypeRef refPage = CFDictionaryGetValue (properties, CFSTR(kIOHIDPrimaryUsagePageKey));
		CFTypeRef refUsage = CFDictionaryGetValue (properties, CFSTR(kIOHIDPrimaryUsageKey));
		CFNumberGetValue((CFNumberRef) refUsage, kCFNumberLongType, &usage);
		CFNumberGetValue((CFNumberRef) refPage, kCFNumberLongType, &page);

		// keep only joystick devices
		if  ( (page == kHIDPage_GenericDesktop) &&
         ((usage == kHIDUsage_GD_Joystick) ||
          (usage == kHIDUsage_GD_GamePad)
    // || (usage == kHIDUsage_GD_MultiAxisController)
    // || (usage == kHIDUsage_GD_Hatswitch)
          )
        )
    {
			// add it to the array
			ioDevices[numDevices++] = ioDev;
    }
	}

	IOObjectRelease(hidIterator);
}

static void joystickRemovalCallback(void* target, IOReturn result, void* refCon, void* sender)
{
    os_specific_s* ourJS = reinterpret_cast<os_specific_s*>(target);
    ourJS->removed = true;
}

jsJoystick::jsJoystick(int ident) :
	id(ident),
	os(NULL),
	error(JS_FALSE),
	num_axes(0),
	num_buttons(0)
{
	if (ident >= numDevices) {
		setError();
		return;
	}

    // since the JoystickINput code tries to re-open devices every few seconds,
    // we need to watch out for removed devices here.
    if (ioDevices[id] == 0) {
        setError();
        return;
    }

    os = new struct os_specific_s;
	os->num_hats = 0;
    os->hidDev = nullptr;

    // get the name now too
	CFDictionaryRef properties = getCFProperties(ioDevices[id]);
	CFTypeRef ref = CFDictionaryGetValue (properties, CFSTR(kIOHIDProductKey));
	if (!ref)
		ref = CFDictionaryGetValue (properties, CFSTR("USB Product Name"));

	if (!ref || !CFStringGetCString ((CFStringRef) ref, name, 128, CFStringGetSystemEncoding ())) {
		jsSetError(SG_WARN, "error getting device name");
		name[0] = '\0';
	}
	//printf("Joystick name: %s \n", name);
	open();
}

void jsJoystick::open()
{
#if 0		// test already done in the constructor
	if (id >= numDevices) {
		jsSetError(SG_WARN, "device index out of range in jsJoystick::open");
		return;
	}
#endif

	// create device interface
	IOReturn rv;
	SInt32 score;
	IOCFPlugInInterface **plugin;

	rv = IOCreatePlugInInterfaceForService(ioDevices[id],
		kIOHIDDeviceUserClientTypeID,
		kIOCFPlugInInterfaceID,
		&plugin, &score);

	if (rv != kIOReturnSuccess) {
		jsSetError(SG_WARN, "error creting plugin for io device");
		return;
	}

	HRESULT pluginResult = (*plugin)->QueryInterface(plugin,
		CFUUIDGetUUIDBytes(kIOHIDDeviceInterfaceID), (LPVOID*)&(os->hidDev) );

	if (pluginResult != S_OK)
		jsSetError(SG_WARN, "QI-ing IO plugin to HID Device interface failed");

	(*plugin)->Release(plugin); // don't leak a ref
	if (os->hidDev == NULL) return;

	// store the interface in this instance
	rv = (*(os->hidDev))->open(os->hidDev, 0);
	if (rv != kIOReturnSuccess) {
		jsSetError(SG_WARN, "error opening device interface");
		return;
	}

    rv = (*(os->hidDev))->setRemovalCallback(os->hidDev, &joystickRemovalCallback, os, nullptr);

    CFDictionaryRef props = getCFProperties(ioDevices[id]);
	if (!props) {
		// TODO ERROR REPORT
		return;
	}

	// recursively enumerate all the bits (buttons, axes, hats, ...)
	CFTypeRef topLevelElement =
		CFDictionaryGetValue (props, CFSTR(kIOHIDElementKey));
	os->enumerateElements(this, topLevelElement);
	CFRelease(props);

	// for hats to be implemented as axes: must be the last axes:
	for (int h = 0; h<2*os->num_hats; h++)
	{
		int index = num_axes++;
		dead_band [ index ] = 0.0f ;
		saturate  [ index ] = 1.0f ;
		center    [ index ] = 0.0f;
		max       [ index ] = 1.0f;
		min       [ index ] = -1.0f;
	}
}

CFDictionaryRef getCFProperties(io_object_t ioDev)
{
	IOReturn rv;
	CFMutableDictionaryRef cfProperties;

#if 0
	// comment copied from darwin/SDL_sysjoystick.c
	/* Mac OS X currently is not mirroring all USB properties to HID page so need to look at USB device page also
	 * get dictionary for usb properties: step up two levels and get CF dictionary for USB properties
	 */

	io_registry_entry_t parent1, parent2;

	rv = IORegistryEntryGetParentEntry (ioDev, kIOServicePlane, &parent1);
	if (rv != kIOReturnSuccess) {
		jsSetError(SG_WARN, "error getting device entry parent");
		return NULL;
	}

	rv = IORegistryEntryGetParentEntry (parent1, kIOServicePlane, &parent2);
	if (rv != kIOReturnSuccess) {
		jsSetError(SG_WARN, "error getting device entry parent 2");
		return NULL;
	}

#endif
	rv = IORegistryEntryCreateCFProperties( ioDev /*parent2*/,
		&cfProperties, kCFAllocatorDefault, kNilOptions);
	if (rv != kIOReturnSuccess || !cfProperties) {
		jsSetError(SG_WARN, "error getting device properties");
		return NULL;
	}

	return cfProperties;
}

void jsJoystick::close()
{
    // check for double-close
    if (!os)
		return;

    if (os->hidDev != NULL) (*(os->hidDev))->close(os->hidDev);

    if (os) {
        delete os;
        os = nullptr;
    }
}

/** element enumerator function : pass NULL for top-level*/
void os_specific_s::enumerateElements(jsJoystick* joy, CFTypeRef element)
{
	assert(CFGetTypeID(element) == CFArrayGetTypeID());

	CFRange range = {0, CFArrayGetCount ((CFArrayRef)element)};
	CFArrayApplyFunction((CFArrayRef) element, range,
		&elementEnumerator, joy);
}

void os_specific_s::elementEnumerator( const void *element, void* vjs)
{
	if (CFGetTypeID((CFTypeRef) element) != CFDictionaryGetTypeID()) {
		jsSetError(SG_WARN, "element enumerator passed non-dictionary value");
		return;
	}

	static_cast<jsJoystick*>(vjs)->
		os->parseElement( static_cast<jsJoystick*>(vjs), (CFDictionaryRef) element);
}

void os_specific_s::parseElement(jsJoystick* joy, CFDictionaryRef element)
{
	CFTypeRef refPage = CFDictionaryGetValue ((CFDictionaryRef) element, CFSTR(kIOHIDElementUsagePageKey));
	CFTypeRef refUsage = CFDictionaryGetValue ((CFDictionaryRef) element, CFSTR(kIOHIDElementUsageKey));

	long type, page, usage;

	CFNumberGetValue((CFNumberRef)
		CFDictionaryGetValue ((CFDictionaryRef) element, CFSTR(kIOHIDElementTypeKey)),
		kCFNumberLongType, &type);

	switch (type) {
	case kIOHIDElementTypeInput_Misc:
	case kIOHIDElementTypeInput_Axis:
	case kIOHIDElementTypeInput_Button:
		//printf("got input element...");
		CFNumberGetValue((CFNumberRef) refUsage, kCFNumberLongType, &usage);
		CFNumberGetValue((CFNumberRef) refPage, kCFNumberLongType, &page);

		if (page == kHIDPage_GenericDesktop) {
			switch (usage) /* look at usage to determine function */
			{
				case kHIDUsage_GD_X:
				case kHIDUsage_GD_Y:
				case kHIDUsage_GD_Z:
				case kHIDUsage_GD_Rx:
				case kHIDUsage_GD_Ry:
				case kHIDUsage_GD_Rz:
				case kHIDUsage_GD_Slider: // for throttle / trim controls
        case kHIDUsage_GD_Dial:
					//printf(" axis\n");
					/*joy->os->*/addAxisElement(joy, (CFDictionaryRef) element);
					break;

        case kHIDUsage_GD_Hatswitch:
					//printf(" hat\n");
					/*joy->os->*/addHatElement(joy, (CFDictionaryRef) element);
					break;
		default:
			SG_LOG(SG_INPUT, SG_INFO, "jsJoystick: input type element has unhandled usage:" << usage);
		break;
			}
		} else if (page == kHIDPage_Simulation) {
			switch (usage) /* look at usage to determine function */
			{
				case kHIDUsage_Sim_Rudder:
				case kHIDUsage_Sim_Throttle:
					//printf(" axis\n");
					/*joy->os->*/addAxisElement(joy, (CFDictionaryRef) element);
					break;
				default:
					SG_LOG(SG_INPUT, SG_WARN, "jsJoystick: Simulation page input type element has weird usage:" << usage);
			}
		} else if (page == kHIDPage_Button) {
			//printf(" button\n");
			/*joy->os->*/addButtonElement(joy, (CFDictionaryRef) element);
		} else if (page == kHIDPage_PID) {
			SG_LOG(SG_INPUT, SG_INFO, "jsJoystick: Force feedback and related data ignored");
		} else
			SG_LOG(SG_INPUT, SG_INFO, "jsJoystick: input type element has unhnadled HID page:" << page);
		break;

	case kIOHIDElementTypeCollection:
		/*joy->os->*/enumerateElements(joy,
			CFDictionaryGetValue(element, CFSTR(kIOHIDElementKey))
		);
		break;

	default:
		break;
	}
}

void os_specific_s::addAxisElement(jsJoystick* joy, CFDictionaryRef axis)
{
	long cookie, lmin, lmax;
	CFNumberGetValue ((CFNumberRef)
		CFDictionaryGetValue (axis, CFSTR(kIOHIDElementCookieKey)),
		kCFNumberLongType, &cookie);

	int index = joy->num_axes++;

	/*joy->os->*/axisCookies[index] = (IOHIDElementCookie) cookie;

	CFNumberGetValue ((CFNumberRef)
		CFDictionaryGetValue (axis, CFSTR(kIOHIDElementMinKey)),
		kCFNumberLongType, &lmin);

	CFNumberGetValue ((CFNumberRef)
		CFDictionaryGetValue (axis, CFSTR(kIOHIDElementMaxKey)),
		kCFNumberLongType, &lmax);

	joy->min[index] = lmin;
	joy->max[index] = lmax;
	joy->dead_band[index] = 0.0;
	joy->saturate[index] = 1.0;
	joy->center[index] = (lmax - lmin) * 0.5 + lmin;
}

void os_specific_s::addButtonElement(jsJoystick* joy, CFDictionaryRef button)
{
	long cookie;
	CFNumberGetValue ((CFNumberRef)
		CFDictionaryGetValue (button, CFSTR(kIOHIDElementCookieKey)),
		kCFNumberLongType, &cookie);

	/*joy->os->*/buttonCookies[joy->num_buttons++] = (IOHIDElementCookie) cookie;
	// anything else for buttons?
}

void os_specific_s::addHatElement(jsJoystick* joy, CFDictionaryRef hat)
{
	long cookie, lmin, lmax;
	CFNumberGetValue ((CFNumberRef)
		CFDictionaryGetValue (hat, CFSTR(kIOHIDElementCookieKey)),
		kCFNumberLongType, &cookie);

	int index = /*joy->*/num_hats++;

	/*joy->os->*/hatCookies[index] = (IOHIDElementCookie) cookie;

	CFNumberGetValue ((CFNumberRef)
		CFDictionaryGetValue (hat, CFSTR(kIOHIDElementMinKey)),
		kCFNumberLongType, &lmin);

	CFNumberGetValue ((CFNumberRef)
		CFDictionaryGetValue (hat, CFSTR(kIOHIDElementMaxKey)),
		kCFNumberLongType, &lmax);

	hat_min[index] = lmin;
	hat_max[index] = lmax;
	// do we map hats to axes or buttons?
	// axes; there is room for that: Buttons are limited to 32.
	// (a joystick with 2 hats will use 16 buttons!)
}

void jsJoystick::rawRead(int *buttons, float *axes)
{
    if (!os)
        return;

    if (buttons)
        *buttons = 0;

    if (os->removed) {
        setError();
        close();

        // clear out from the static array, in case someone tries to open us
        // again
        ioDevices[id] = 0;

        return;
    }

     IOHIDEventStruct hidEvent;

	for (int b=0; b<num_buttons; ++b) {
		(*(os->hidDev))->getElementValue(os->hidDev, os->buttonCookies[b], &hidEvent);
		if (hidEvent.value && buttons)
			*buttons |= 1 << b;
	}

	// real axes:
	int real_num_axes = num_axes - 2*os->num_hats;
	for (int a=0; a<real_num_axes; ++a) {
		(*(os->hidDev))->getElementValue(os->hidDev, os->axisCookies[a], &hidEvent);
		axes[a] = hidEvent.value;
	}

	// hats:
	for (int h=0; h < os->num_hats; ++h) {
		(*(os->hidDev))->getElementValue(os->hidDev, os->hatCookies[h], &hidEvent);
		 long result = ( hidEvent.value - os->hat_min[h] ) * 8;
		 result /= ( os->hat_max[h] - os->hat_min[h] + 1 );
		 if ( (result>=0) && (result<8) )
		 {
			 axes[h+real_num_axes+1] = NS_hat[result];
			 axes[h+real_num_axes] = WE_hat[result];
		 }
		 else
		 {
			 axes[h+real_num_axes] = 0;
			 axes[h+real_num_axes+1] = 0;
		 }
	}
}
