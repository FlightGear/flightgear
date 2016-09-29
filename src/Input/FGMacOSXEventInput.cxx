// FGMacOSXEventInput.cxx -- handle event driven input devices for Mac OS X
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

#include "FGMacOSXEventInput.hxx"

#include <simgear/sg_inlines.h>

#include <cstdlib>

#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/hid/IOHIDLib.h>
#include <IOKit/HID/IOHIDKeys.h>

#include <simgear/structure/exception.hxx>

static std::string nameForUsage(uint32_t usagePage, uint32_t usage)
{
    if (usagePage == kHIDPage_GenericDesktop) {
        switch (usage) {
        case kHIDUsage_GD_Joystick: return "joystick";
        case kHIDUsage_GD_Wheel:        return "wheel";
        case kHIDUsage_GD_Dial:         return "dial";
        case kHIDUsage_GD_Hatswitch:    return "hat";
        case kHIDUsage_GD_Slider:       return "slider";
        case kHIDUsage_GD_Rx:           return "x-rotate";
        case kHIDUsage_GD_Ry:           return "y-rotate";
        case kHIDUsage_GD_Rz:           return "z-rotate";
        case kHIDUsage_GD_X:            return "x-translate";
        case kHIDUsage_GD_Y:            return "y-translate";
        case kHIDUsage_GD_Z:            return "z-translate";
        default:
            SG_LOG(SG_INPUT, SG_WARN, "Unhandled HID generic desktop usage:" << usage);
        }
    } else if (usagePage == kHIDPage_Simulation) {
        switch (usage) {
        default:
            SG_LOG(SG_INPUT, SG_WARN, "Unhandled HID simulation usage:" << usage);
        }
    } else if (usagePage == kHIDPage_Consumer) {
        switch (usage) {
            default:
                SG_LOG(SG_INPUT, SG_WARN, "Unhandled HID consumer usage:" << usage);
        }
    } else if (usagePage == kHIDPage_AlphanumericDisplay) {
        switch (usage) {
        case kHIDUsage_AD_AlphanumericDisplay:  return "alphanumeric";
        case kHIDUsage_AD_CharacterReport:  return "character-report";
        case kHIDUsage_AD_DisplayData:  return "display-data";
        case 0x46: return "display-brightness";

        default:
            SG_LOG(SG_INPUT, SG_WARN, "Unhandled HID alphanumeric usage:" << usage);
        }
    } else if (usagePage == kHIDPage_LEDs) {
        switch (usage) {
        case kHIDUsage_LED_GenericIndicator: return "led-misc";
        case kHIDUsage_LED_Pause:       return "led-pause";
        default:
            SG_LOG(SG_INPUT, SG_WARN, "Unhandled HID LED usage:" << usage);

        }
    } else if (usagePage == kHIDPage_Button) {
        std::stringstream os;
        os << "button-" << usage;
        return os.str();
    } else if (usagePage >= kHIDPage_VendorDefinedStart) {
        return "vendor";
    } else {
        SG_LOG(SG_INPUT, SG_WARN, "Unhandled HID usage page:" << usagePage);
    }

    return "unknown";
}

class FGMacOSXEventInputPrivate
{
public:
    IOHIDManagerRef hidManager;
    FGMacOSXEventInput* p;
    double currentDt;
    int currentModifiers;

    void matchedDevice(IOHIDDeviceRef device);
    void removedDevice(IOHIDDeviceRef device);

    void iterateDevices(CFSetRef matchingSet);

    std::string getDeviceStringProperty(IOHIDDeviceRef device, CFStringRef hidProp);
    bool getDeviceIntProperty(IOHIDDeviceRef device, CFStringRef hidProp, int& value);
};


static void deviceMatchingCallback(
                                  void *          inContext,       // context from IOHIDManagerRegisterDeviceMatchingCallback
                                  IOReturn        inResult,        // the result of the matching operation
                                  void *          inSender,        // the IOHIDManagerRef for the new device
                                  IOHIDDeviceRef  inIOHIDDeviceRef // the new HID device
)
{
   // printf("%s(context: %p, result: %p, sender: %p, device: %p).\n",
    //       __PRETTY_FUNCTION__, inContext, (void *) inResult, inSender, (void*) inIOHIDDeviceRef);

    FGMacOSXEventInputPrivate* p = static_cast<FGMacOSXEventInputPrivate*>(inContext);
    p->matchedDevice(inIOHIDDeviceRef);
}   // Handle_DeviceMatchingCallback


static void deviceRemovalCallback(
                                   void *          inContext,       // context from IOHIDManagerRegisterDeviceMatchingCallback
                                   IOReturn        inResult,        // the result of the matching operation
                                   void *          inSender,        // the IOHIDManagerRef for the new device
                                   IOHIDDeviceRef  inIOHIDDeviceRef // the new HID device
)
{
    // printf("%s(context: %p, result: %p, sender: %p, device: %p).\n",
    //       __PRETTY_FUNCTION__, inContext, (void *) inResult, inSender, (void*) inIOHIDDeviceRef);
    FGMacOSXEventInputPrivate* p = static_cast<FGMacOSXEventInputPrivate*>(inContext);
    p->removedDevice(inIOHIDDeviceRef);
}   // Handle_DeviceMatchingCallback

//
// FGMacOSXInputDevice implementation
//

//
// FGMacOSXInputDevice
// Mac OS X specific FGInputDevice
//
class FGMacOSXInputDevice : public FGInputDevice {
public:

    FGMacOSXInputDevice(IOHIDDeviceRef hidRef,
                        FGMacOSXEventInputPrivate* subsys);

    virtual ~FGMacOSXInputDevice();

    void Open();
    void Close();
    
    virtual void update(double dt);
    virtual const char *TranslateEventName(FGEventData &eventData);
    virtual void Send( const char * eventName, double value );
    virtual void SendFeatureReport(unsigned int reportId, const std::string& data);
    virtual void AddHandledEvent( FGInputEvent_ptr handledEvent );

    void drainQueue();
    void handleValue(IOHIDValueRef value);

private:
    void buildElementNameDictionary();

    std::string nameForHIDElement(IOHIDElementRef element) const;

    IOHIDDeviceRef _hid;
    IOHIDQueueRef _queue;
    FGMacOSXEventInputPrivate* _subsystem;

    typedef std::map<std::string, IOHIDElementRef> NameElementDict;
    NameElementDict namedElements;
};

#if 0
static void valueAvailableCallback(void *   inContext, // context from IOHIDQueueRegisterValueAvailableCallback
                                   IOReturn inResult,  // the inResult
                                   void *   inSender  // IOHIDQueueRef of the queue
) {
    FGMacOSXInputDevice* dev = static_cast<FGMacOSXInputDevice*>(inContext);
    dev->drainQueue();
}   // Handle_ValueAvailableCallback
#endif

//
// FGMacOSXEventInput implementation
//


FGMacOSXEventInput::FGMacOSXEventInput() :
    FGEventInput(),
    d(new FGMacOSXEventInputPrivate)
{
    d->p = this; // store back pointer to outer object on pimpl
    SG_LOG(SG_INPUT, SG_DEBUG, "FGMacOSXEventInput created");
}

FGMacOSXEventInput::~FGMacOSXEventInput()
{
}

void FGMacOSXEventInput::init()
{
    // everything is deffered until postinit since we need Nasal
}

void FGMacOSXEventInput::postinit()
{
    d->hidManager = IOHIDManagerCreate(kCFAllocatorDefault, kIOHIDOptionsTypeNone);

    // set the HID device matching dictionary
    IOHIDManagerSetDeviceMatching( d->hidManager, NULL /* all devices */);

    IOHIDManagerRegisterDeviceMatchingCallback(d->hidManager, deviceMatchingCallback, d.get());
    IOHIDManagerRegisterDeviceRemovalCallback(d->hidManager, deviceRemovalCallback, d.get());

    IOHIDManagerOpen(d->hidManager, kIOHIDOptionsTypeNone);

    IOHIDManagerScheduleWithRunLoop(d->hidManager, CFRunLoopGetCurrent(), kCFRunLoopDefaultMode);
}

void FGMacOSXEventInput::shutdown()
{
    IOHIDManagerClose(d->hidManager, kIOHIDOptionsTypeNone);
    IOHIDManagerUnscheduleFromRunLoop(d->hidManager, CFRunLoopGetCurrent(), kCFRunLoopDefaultMode);
    CFRelease(d->hidManager);
}

//
// read all elements in each input device
//
void FGMacOSXEventInput::update(double dt)
{
    d->currentDt = dt;
    d->currentModifiers = fgGetKeyModifiers();
    FGEventInput::update(dt);
}

void FGMacOSXEventInputPrivate::matchedDevice(IOHIDDeviceRef device)
{
    std::string productName = getDeviceStringProperty(device, CFSTR(kIOHIDProductKey));
    std::string manufacturer = getDeviceStringProperty(device, CFSTR(kIOHIDManufacturerKey));

    SG_LOG(SG_INPUT, SG_INFO, "matched device:" << productName << " from " << manufacturer);

    // allocate a Mac input device, and add to the base class to see if we have
    // a config

    FGMacOSXInputDevice* macInputDevice = new FGMacOSXInputDevice(device, this);
    macInputDevice->SetName(manufacturer + " " + productName);
    p->AddDevice(macInputDevice);
}

void FGMacOSXEventInputPrivate::removedDevice(IOHIDDeviceRef device)
{
    // see if we have an entry for the device
}

std::string FGMacOSXEventInputPrivate::getDeviceStringProperty(IOHIDDeviceRef device, CFStringRef hidProp)
{
    CFStringRef prop = (CFStringRef) IOHIDDeviceGetProperty(device, hidProp);
    if (CFGetTypeID(prop) != CFStringGetTypeID()) {
        return std::string();
    }

    size_t len = CFStringGetLength(prop);
    if (len == 0) {
        return std::string();
    }

    char* buffer = static_cast<char*>(malloc(len + 1)); // + 1 for the null byte
    Boolean ok = CFStringGetCString(prop, buffer, len + 1, kCFStringEncodingUTF8);
    if (!ok) {
        SG_LOG(SG_INPUT, SG_WARN, "string conversion failed");
    }

    std::string result(buffer, len);
    free(buffer);

    return result;

}

bool FGMacOSXEventInputPrivate::getDeviceIntProperty(IOHIDDeviceRef device, CFStringRef hidProp, int& value)
{
    CFTypeRef prop = IOHIDDeviceGetProperty(device, hidProp);
    if (CFGetTypeID(prop) != CFNumberGetTypeID()) {
        return false;
    }

    int32_t v;
    Boolean result = CFNumberGetValue((CFNumberRef) prop, kCFNumberSInt32Type, &v);
    value = v;
    return result;
}

void FGMacOSXEventInputPrivate::iterateDevices(CFSetRef matchingSet)
{
    size_t numDevices = CFSetGetCount(matchingSet);
    IOHIDDeviceRef* devs = static_cast<IOHIDDeviceRef*>(::malloc(numDevices * sizeof(IOHIDDeviceRef)));
    CFSetGetValues(matchingSet, (const void **) devs);

    for (size_t i=0; i < numDevices; ++i) {
        matchedDevice(devs[i]);
    }

    free(devs);
}


FGMacOSXInputDevice::FGMacOSXInputDevice(IOHIDDeviceRef hidRef,
                                         FGMacOSXEventInputPrivate* subsys)
{
    _hid = hidRef;
    CFRetain(_hid);
    _subsystem = subsys;

    CFIndex maxDepth = 128;
    _queue = IOHIDQueueCreate(kCFAllocatorDefault, _hid, maxDepth, kIOHIDOptionsTypeNone);
}

FGMacOSXInputDevice::~FGMacOSXInputDevice()
{
    NameElementDict::iterator it;
    for (it = namedElements.begin(); it != namedElements.end(); ++it) {
        CFRelease(it->second);
    }

    CFRelease(_queue);
    CFRelease(_hid);
}

void FGMacOSXInputDevice::Open()
{
    IOHIDDeviceOpen(_hid, kIOHIDOptionsTypeNone);
    IOHIDDeviceScheduleWithRunLoop(_hid, CFRunLoopGetCurrent(), kCFRunLoopDefaultMode);


    // IOHIDQueueRegisterValueAvailableCallback(_queue, valueAvailableCallback, this);

    IOHIDQueueScheduleWithRunLoop(_queue, CFRunLoopGetCurrent(), kCFRunLoopDefaultMode);
    IOHIDQueueStart(_queue);
}

void FGMacOSXInputDevice::buildElementNameDictionary()
{
    // copy all elements from the device
    CFArrayRef elements = IOHIDDeviceCopyMatchingElements(_hid, NULL,
                                                          kIOHIDOptionsTypeNone);

    CFIndex count = CFArrayGetCount(elements);
    typedef std::map<std::string, unsigned int> NameCountMap;
    NameCountMap nameCounts;

    std::set<IOHIDElementCookie> seenCookies;

    for (int i = 0; i < count; ++i) {
        IOHIDElementRef element = (IOHIDElementRef) CFArrayGetValueAtIndex(elements, i);
        IOHIDElementCookie cookie = IOHIDElementGetCookie(element);
        if (seenCookies.find(cookie) != seenCookies.end()) {
            // skip duplicate match of this element;
            continue;
        }

        seenCookies.insert(cookie);


       // bool isWrapping = IOHIDElementIsWrapping(element);
        IOHIDElementType ty = IOHIDElementGetType(element);
        if (ty == kIOHIDElementTypeCollection)
        {
           continue;
        }

        uint32_t page = IOHIDElementGetUsagePage(element);
        uint32_t usage = IOHIDElementGetUsage(element);
        bool isRelative = IOHIDElementIsRelative(element);

        // compute the name for the element
        std::string name = nameForUsage(page, usage);
        if (isRelative) {
            name = "rel-" + name; // prefix relative elements
        }

        NameCountMap::iterator it = nameCounts.find(name);
        unsigned int nameCount;
        std::string finalName = name;

        if (it == nameCounts.end()) {
            nameCounts[name] = nameCount = 1;
        } else {
            // check if we have a collison but different element types, eg
            // input & feature. In which case, prefix the feature one since
            // we assume it's the input one which is more interesting.

            IOHIDElementRef other = namedElements[name];
            IOHIDElementType otherTy = IOHIDElementGetType(other);
            if (otherTy != ty) {
                // types mismatch
                if (otherTy == kIOHIDElementTypeFeature) {
                    namedElements[name] = element;
                    element = other;
                    finalName = "feature-" + name;
                } else if (ty == kIOHIDElementTypeFeature) {
                    finalName = "feature-" + name;
                }
                nameCount = 1;
            } else {
                // duplicate element, append ordinal suffix
                std::stringstream os;
                os << name << "-" << it->second;
                finalName = os.str();
                nameCount = ++(it->second);
            }
        }

        CFRetain(element);
        namedElements[finalName] = element;

        if (nameCount == 2) {
            // we have more than one entry for this name, so ensures
            // the first item is availabe with the -0 suffix
            std::stringstream os;
            os << name << "-0";
            namedElements[os.str()] = namedElements[name];
            CFRetain(namedElements[os.str()]);
        }
    }
#if 1
    NameElementDict::const_iterator it;
    for (it = namedElements.begin(); it != namedElements.end(); ++it) {
        int report = IOHIDElementGetReportID(it->second);
        int reportCount = IOHIDElementGetReportCount(it->second);
        int reportSize = IOHIDElementGetReportSize(it->second);
        CFTypeRef sizeProp = IOHIDElementGetProperty(it->second, CFSTR(kIOHIDElementDuplicateIndexKey));
        if (sizeProp) {
            CFShow(sizeProp);
        }

        bool isArray = IOHIDElementIsArray(it->second);
        if (isArray) {
            SG_LOG(SG_INPUT, SG_INFO, "YYYYYYYYYYYYYYYYYY");
        }
        SG_LOG(SG_INPUT, SG_INFO, "\t" << it->first << " report ID " << report << " /count=" << reportCount
               << " ;sz=" << reportSize);
    }
#endif

    CFRelease(elements);
}

void FGMacOSXInputDevice::Close()
{
    IOHIDQueueStop(_queue);
    IOHIDDeviceUnscheduleFromRunLoop(_hid, CFRunLoopGetCurrent(), kCFRunLoopDefaultMode);
    IOHIDDeviceClose(_hid, kIOHIDOptionsTypeNone);
}

void FGMacOSXInputDevice::AddHandledEvent( FGInputEvent_ptr handledEvent )
{
    SG_LOG(SG_INPUT, SG_INFO, "adding event:" << handledEvent->GetName());

    if (namedElements.empty()) {
        buildElementNameDictionary();
    }

    NameElementDict::iterator it = namedElements.find(handledEvent->GetName());
    if (it == namedElements.end()) {
        SG_LOG(SG_INPUT, SG_WARN, "device does not have any element with name:" << handledEvent->GetName());
        return;
    }

    IOHIDQueueAddElement(_queue, it->second);
    FGInputDevice::AddHandledEvent(handledEvent);
}

void FGMacOSXInputDevice::update(double dt)
{
    SG_UNUSED(dt);
    drainQueue();

    FGInputDevice::update(dt);
}

void FGMacOSXInputDevice::drainQueue()
{
    do {
        IOHIDValueRef valueRef = IOHIDQueueCopyNextValueWithTimeout( _queue, 0. );
        if ( !valueRef ) break;
        handleValue(valueRef);
        CFRelease( valueRef );
    } while ( 1 ) ;
}

void FGMacOSXInputDevice::handleValue(IOHIDValueRef value)
{
    IOHIDElementRef element = IOHIDValueGetElement(value);
    CFIndex val;

// need report count to know if we have to handle this specially
    int reportCount = IOHIDElementGetReportCount(element);
    if (reportCount > 1) {
        // for a repeated element, we need to read the final value of
        // the data bytes (even though this will be the lowest numbered name
        // for this element.
        int bitSize = IOHIDElementGetReportSize(element);
        const uint8_t* bytes= IOHIDValueGetBytePtr(value);
        size_t byteLen = IOHIDValueGetLength(value);
        uint8_t finalByte = bytes[byteLen - 1];

        if (bitSize == 8) {
            val = (int8_t) finalByte; // force sign extension
        } else if (bitSize == 4) {
            int8_t b = finalByte >> 4; // high nibble
            if (b & 0x08) {
                // manually sign extend
                b |= 0xf0; // set all bits except the low nibble
            }
            val = b;
        } else {
            throw sg_io_exception("Unhandled bit size in MacoSXHID");
        }
    } else {
        val = IOHIDValueGetIntegerValue(value);
    }

// supress spurious 0-valued relative events
    std::string name = nameForHIDElement(element);
    if ((name.find("rel-") == 0) && (val == 0)) {
        return;
    }

    FGMacOSXEventData eventData(name, val,
                                _subsystem->currentDt,
                                _subsystem->currentModifiers);
    HandleEvent(eventData);
    
}

std::string FGMacOSXInputDevice::nameForHIDElement(IOHIDElementRef element) const
{
    NameElementDict::const_iterator it;
    for (it = namedElements.begin(); it != namedElements.end(); ++it) {
        if (it->second == element) {
            return it->first;
        }
    }

    throw sg_exception("Unknown HID element");
}

const char *FGMacOSXInputDevice::TranslateEventName(FGEventData &eventData)
{
    FGMacOSXEventData &macEvent = (FGMacOSXEventData &)eventData;
    return macEvent.name.c_str();
}

//
// Outputs value to an writable element (like LEDElement)
//
void FGMacOSXInputDevice::Send(const char *eventName, double value)
{
    NameElementDict::const_iterator it = namedElements.find(eventName);
    if (it == namedElements.end()) {
        SG_LOG(SG_INPUT, SG_WARN, "FGMacOSXInputDevice::Send: unknown element:" << eventName);
        return;
    }

    CFIndex cfVal = value;
    uint64_t timestamp = 0;
    IOHIDValueRef valueRef = IOHIDValueCreateWithIntegerValue(kCFAllocatorDefault,
                                                              it->second, timestamp, cfVal);
    IOHIDDeviceSetValue(_hid, it->second, valueRef);
    CFRelease(valueRef);
}

void FGMacOSXInputDevice::SendFeatureReport(unsigned int reportId, const std::string& data)
{

    string d(data);
    if (reportId > 0) {
        // prefix with report number
        d.insert(d.begin(), static_cast<char>(reportId));
    }

    size_t len = d.size();
    const uint8_t* bytes = (const uint8_t*) d.data();

    std::stringstream ss;
    for (int i=0; i<len; ++i) {
        ss << static_cast<int>(bytes[i]);
        ss << ":";
    }

    SG_LOG(SG_INPUT, SG_INFO, "report " << reportId << " sending " << ss.str());

    IOReturn res = IOHIDDeviceSetReport(_hid,
                               kIOHIDReportTypeFeature,
                               reportId, /* Report ID*/
                               bytes, len);
    
    if (res != kIOReturnSuccess) {
        SG_LOG(SG_INPUT, SG_WARN, "failed to send feature report" << reportId);
    }

}
