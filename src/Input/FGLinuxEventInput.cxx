// FGEventInput.cxx -- handle event driven input devices for the Linux O/S
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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <cstring>
#include <sys/types.h>
#include <sys/stat.h>
#include "FGLinuxEventInput.hxx"

#include <libudev.h>
#include <poll.h>
#include <linux/input.h>
#include <fcntl.h>

#include <string.h>

struct TypeCode {
  unsigned type;
  unsigned code;

  inline unsigned long hashCode() const {
    return (unsigned long)type << 16 | (unsigned long)code;
  }

  bool operator < ( const TypeCode & other) const {
    return hashCode() < other.hashCode();
  }
};

// event to name translation table
// events are from include <linux/input.h>

static struct EventTypes {
  struct TypeCode typeCode;
  const char * name;
} EVENT_TYPES[] = {
  { { EV_SYN, SYN_REPORT },     "syn-report" },
  { { EV_SYN, SYN_CONFIG },     "syn-config" },

  // misc
  { { EV_KEY, BTN_0 }, "button-0" },
  { { EV_KEY, BTN_1 }, "button-1" },
  { { EV_KEY, BTN_2 }, "button-2" },
  { { EV_KEY, BTN_3 }, "button-3" },
  { { EV_KEY, BTN_4 }, "button-4" },
  { { EV_KEY, BTN_5 }, "button-5" },
  { { EV_KEY, BTN_6 }, "button-6" },
  { { EV_KEY, BTN_7 }, "button-7" },
  { { EV_KEY, BTN_8 }, "button-8" },
  { { EV_KEY, BTN_9 }, "button-9" },

  // mouse
  { { EV_KEY, BTN_LEFT },    "button-left" },
  { { EV_KEY, BTN_RIGHT },   "button-right" },
  { { EV_KEY, BTN_MIDDLE },  "button-middle" },
  { { EV_KEY, BTN_SIDE },    "button-side" },
  { { EV_KEY, BTN_EXTRA },   "button-extra" },
  { { EV_KEY, BTN_FORWARD }, "button-forward" },
  { { EV_KEY, BTN_BACK },    "button-back" },
  { { EV_KEY, BTN_TASK },    "button-task" },

  // joystick
  { { EV_KEY, BTN_TRIGGER }, "button-trigger" },
  { { EV_KEY, BTN_THUMB },   "button-thumb" },
  { { EV_KEY, BTN_THUMB2 },  "button-thumb2" },
  { { EV_KEY, BTN_TOP },     "button-top" },
  { { EV_KEY, BTN_TOP2 },    "button-top2" },
  { { EV_KEY, BTN_PINKIE },  "button-pinkie" },
  { { EV_KEY, BTN_BASE },    "button-base" },
  { { EV_KEY, BTN_BASE2 },   "button-base2" },
  { { EV_KEY, BTN_BASE3 },   "button-base3" },
  { { EV_KEY, BTN_BASE4 },   "button-base4" },
  { { EV_KEY, BTN_BASE5 },   "button-base5" },
  { { EV_KEY, BTN_BASE6 },   "button-base6" },
  { { EV_KEY, BTN_DEAD },    "button-dead" },

  // gamepad
  { { EV_KEY, BTN_A },      "button-a" },
  { { EV_KEY, BTN_B },      "button-b" },
  { { EV_KEY, BTN_C },      "button-c" },
  { { EV_KEY, BTN_X },      "button-x" },
  { { EV_KEY, BTN_Y },      "button-y" },
  { { EV_KEY, BTN_Z },      "button-z" },
  { { EV_KEY, BTN_TL },     "button-tl" },
  { { EV_KEY, BTN_TR },     "button-tr" },
  { { EV_KEY, BTN_TL2 },    "button-tl2" },
  { { EV_KEY, BTN_TR2 },    "button-tr2" },
  { { EV_KEY, BTN_SELECT }, "button-select" },
  { { EV_KEY, BTN_START },  "button-start" },
  { { EV_KEY, BTN_MODE },   "button-mode" },
  { { EV_KEY, BTN_THUMBL }, "button-thumbl" },
  { { EV_KEY, BTN_THUMBR }, "button-thumbr" },

  // digitizer
  { { EV_KEY, BTN_TOOL_PEN },       "button-pen" },
  { { EV_KEY, BTN_TOOL_RUBBER },    "button-rubber" },
  { { EV_KEY, BTN_TOOL_BRUSH },     "button-brush" },
  { { EV_KEY, BTN_TOOL_PENCIL },    "button-pencil" },
  { { EV_KEY, BTN_TOOL_AIRBRUSH },  "button-airbrush" },
  { { EV_KEY, BTN_TOOL_FINGER },    "button-finger" },
  { { EV_KEY, BTN_TOOL_MOUSE },     "button-mouse" },
  { { EV_KEY, BTN_TOOL_LENS },      "button-lens" },
  { { EV_KEY, BTN_TOUCH },          "button-touch" },
  { { EV_KEY, BTN_STYLUS },         "button-stylus" },
  { { EV_KEY, BTN_STYLUS2 },        "button-stylus2" },
  { { EV_KEY, BTN_TOOL_DOUBLETAP }, "button-doubletap" },
  { { EV_KEY, BTN_TOOL_TRIPLETAP }, "button-trippletap" },

  { { EV_KEY, BTN_WHEEL },          "button-wheel" },
  { { EV_KEY, BTN_GEAR_DOWN },      "button-gear-down" },
  { { EV_KEY, BTN_GEAR_UP },        "button-gear-up" },

  { { EV_REL, REL_X },     "rel-x-translate" },
  { { EV_REL, REL_Y},      "rel-y-translate" },
  { { EV_REL, REL_Z},      "rel-z-translate" },
  { { EV_REL, REL_RX},     "rel-x-rotate" },
  { { EV_REL, REL_RY},     "rel-y-rotate" },
  { { EV_REL, REL_RZ},     "rel-z-rotate" },
  { { EV_REL, REL_HWHEEL}, "rel-hwheel" },
  { { EV_REL, REL_DIAL},   "rel-dial" },
  { { EV_REL, REL_WHEEL},  "rel-wheel" },
  { { EV_REL, REL_MISC},   "rel-misc" },

  { { EV_ABS, ABS_X },          "abs-x-translate" },
  { { EV_ABS, ABS_Y },          "abs-y-translate" },
  { { EV_ABS, ABS_Z },          "abs-z-translate" },
  { { EV_ABS, ABS_RX },         "abs-x-rotate" },
  { { EV_ABS, ABS_RY },         "abs-y-rotate" },
  { { EV_ABS, ABS_RZ },         "abs-z-rotate" },
  { { EV_ABS, ABS_THROTTLE },   "abs-throttle" },
  { { EV_ABS, ABS_RUDDER },     "abs-rudder" },
  { { EV_ABS, ABS_WHEEL },      "abs-wheel" },
  { { EV_ABS, ABS_GAS },        "abs-gas" },
  { { EV_ABS, ABS_BRAKE },      "abs-brake" },
  { { EV_ABS, ABS_HAT0X },      "abs-hat0-x" },
  { { EV_ABS, ABS_HAT0Y },      "abs-hat0-y" },
  { { EV_ABS, ABS_HAT1X },      "abs-hat1-x" },
  { { EV_ABS, ABS_HAT1Y },      "abs-hat1-y" },
  { { EV_ABS, ABS_HAT2X },      "abs-hat2-x" },
  { { EV_ABS, ABS_HAT2Y },      "abs-hat2-y" },
  { { EV_ABS, ABS_HAT3X },      "abs-hat3-x" },
  { { EV_ABS, ABS_HAT3Y },      "abs-hat3-y" },
  { { EV_ABS, ABS_PRESSURE },   "abs-pressure" },
  { { EV_ABS, ABS_DISTANCE },   "abs-distance" },
  { { EV_ABS, ABS_TILT_X },     "abs-tilt-x" },
  { { EV_ABS, ABS_TILT_Y },     "abs-tilt-y" },
  { { EV_ABS, ABS_TOOL_WIDTH }, "abs-toold-width" },
  { { EV_ABS, ABS_VOLUME },     "abs-volume" },
  { { EV_ABS, ABS_MISC },       "abs-misc" },

  { { EV_MSC,  MSC_SERIAL },    "misc-serial" },
  { { EV_MSC,  MSC_PULSELED },  "misc-pulseled" },
  { { EV_MSC,  MSC_GESTURE },   "misc-gesture" },
  { { EV_MSC,  MSC_RAW },       "misc-raw" },
  { { EV_MSC,  MSC_SCAN },      "misc-scan" },

  // switch
  { { EV_SW, SW_LID },               "switch-lid" },
  { { EV_SW, SW_TABLET_MODE },       "switch-tablet-mode" },
  { { EV_SW, SW_HEADPHONE_INSERT },  "switch-headphone-insert" },
#ifdef SW_RFKILL_ALL 
  { { EV_SW, SW_RFKILL_ALL },        "switch-rfkill" },
#endif
#ifdef SW_MICROPHONE_INSERT 
  { { EV_SW, SW_MICROPHONE_INSERT }, "switch-microphone-insert" },
#endif
#ifdef SW_DOCK
  { { EV_SW, SW_DOCK },              "switch-dock" },
#endif

  { { EV_LED, LED_NUML},     "led-numlock" },
  { { EV_LED, LED_CAPSL},    "led-capslock" },
  { { EV_LED, LED_SCROLLL},  "led-scrolllock" },
  { { EV_LED, LED_COMPOSE},  "led-compose" },
  { { EV_LED, LED_KANA},     "led-kana" },
  { { EV_LED, LED_SLEEP},    "led-sleep" },
  { { EV_LED, LED_SUSPEND},  "led-suspend" },
  { { EV_LED, LED_MUTE},     "led-mute" },
  { { EV_LED, LED_MISC},     "led-misc" },
  { { EV_LED, LED_MAIL},     "led-mail" },
  { { EV_LED, LED_CHARGING}, "led-charging" }

};

static struct enbet {
  unsigned type;
  const char * name;
} EVENT_NAMES_BY_EVENT_TYPE[] = {
  { EV_SYN, "syn" },
  { EV_KEY, "button" },
  { EV_REL, "rel" },
  { EV_ABS, "abs" },
  { EV_MSC, "msc" },
  { EV_SW, "button" },
  { EV_LED, "led" },
  { EV_SND, "snd" },
  { EV_REP, "rep" },
  { EV_FF, "ff" },
  { EV_PWR, "pwr" },
  { EV_FF_STATUS, "ff-status" }
};


class EventNameByEventType : public std::map<unsigned,const char*> {
public:
  EventNameByEventType() {
    for( unsigned i = 0; i < sizeof(EVENT_NAMES_BY_EVENT_TYPE)/sizeof(EVENT_NAMES_BY_EVENT_TYPE[0]); i++ )
      (*this)[EVENT_NAMES_BY_EVENT_TYPE[i].type] = EVENT_NAMES_BY_EVENT_TYPE[i].name;
  }
};
static EventNameByEventType EVENT_NAME_BY_EVENT_TYPE;

class EventNameByType : public std::map<TypeCode,const char*> {
public:
  EventNameByType() {
    for( unsigned i = 0; i < sizeof(EVENT_TYPES)/sizeof(EVENT_TYPES[0]); i++ )
      (*this)[EVENT_TYPES[i].typeCode] = EVENT_TYPES[i].name;
  }
};
static EventNameByType EVENT_NAME_BY_TYPE;


struct ltstr {
  bool operator()(const char * s1, const char * s2 ) const {
    return std::string(s1).compare( s2 ) < 0;
  }
};

class EventTypeByName : public std::map<const char *,TypeCode,ltstr> {
public:
  EventTypeByName() {
    for( unsigned i = 0; i < sizeof(EVENT_TYPES)/sizeof(EVENT_TYPES[0]); i++ )
      (*this)[EVENT_TYPES[i].name] = EVENT_TYPES[i].typeCode;
  }
};
static EventTypeByName EVENT_TYPE_BY_NAME;


FGLinuxInputDevice::FGLinuxInputDevice( std::string aName, std::string aDevname ) :
  FGInputDevice(aName),
  devname( aDevname ),
  fd(-1)
{
}

FGLinuxInputDevice::~FGLinuxInputDevice()
{
  try {
    Close();
  } 
  catch(...) {
  }
}

FGLinuxInputDevice::FGLinuxInputDevice() :
  fd(-1)
{
}

static inline bool bitSet( unsigned char * buf, unsigned bit )
{
  return (buf[bit/sizeof(unsigned char)/8] >> (bit%(sizeof(unsigned char)*8))) & 1;
}

void FGLinuxInputDevice::Open()
{
  if( fd != -1 ) return;
  if( (fd = ::open( devname.c_str(), O_RDWR )) == -1 ) { 
    throw std::exception();
  }

  if( GetGrab() && ioctl( fd, EVIOCGRAB, 2 ) == -1 ) {
    SG_LOG( SG_INPUT, SG_WARN, "Can't grab " << devname << " for exclusive access" );
  }

  {
    unsigned char buf[ABS_CNT/sizeof(unsigned char)/8];
    // get axes maximums
    if( ioctl( fd, EVIOCGBIT(EV_ABS,ABS_MAX), buf ) == -1 ) {
      SG_LOG( SG_INPUT, SG_WARN, "Can't get abs-axes for " << devname );
    } else {
      for( unsigned i = 0; i < ABS_MAX; i++ ) {
        if( bitSet( buf, i ) ) {
          struct input_absinfo ai;
          if( ioctl(fd, EVIOCGABS(i), &ai) == -1 ) {
            SG_LOG( SG_INPUT, SG_WARN, "Can't get abs-axes maximums for " << devname );
            continue;
          }
          absinfo[i] = ai;
/*
          SG_LOG( SG_INPUT, SG_INFO, "Axis #" << i <<
            ": value=" << ai.value << 
            ": minimum=" << ai.minimum << 
            ": maximum=" << ai.maximum << 
            ": fuzz=" << ai.fuzz << 
            ": flat=" << ai.flat << 
            ": resolution=" << ai.resolution );
*/

          // kick an initial event
          struct input_event event;
          event.type = EV_ABS;
          event.code = i;
          event.value = ai.value;
          FGLinuxEventData eventData( event, 0, 0 );
          eventData.value = Normalize( event );
          HandleEvent(eventData);
        }
      }
    }
  }
  {
    unsigned char mask[KEY_CNT/sizeof(unsigned char)/8];
    unsigned char flag[KEY_CNT/sizeof(unsigned char)/8];
    memset(mask,0,sizeof(mask));
    memset(flag,0,sizeof(flag));
    if( ioctl( fd, EVIOCGKEY(sizeof(flag)), flag ) == -1 ||
        ioctl( fd, EVIOCGBIT(EV_KEY, sizeof(mask)), mask ) == -1 ) {
      SG_LOG( SG_INPUT, SG_WARN, "Can't get keys for " << devname );
    } else {
      for( unsigned i = 0; i < KEY_MAX; i++ ) {
        if( bitSet( mask, i ) ) {
          struct input_event event;
          event.type = EV_KEY;
          event.code = i;
          event.value = bitSet(flag,i);
          FGLinuxEventData eventData( event, 0, 0 );
          HandleEvent(eventData);
        }
      }
    }
  }
  {
    unsigned char buf[SW_CNT/sizeof(unsigned char)/8];
    if( ioctl( fd, EVIOCGSW(sizeof(buf)), buf ) == -1 ) {
      SG_LOG( SG_INPUT, SG_WARN, "Can't get switches for " << devname );
    } else {
      for( unsigned i = 0; i < SW_MAX; i++ ) {
        if( bitSet( buf, i ) ) {
          struct input_event event;
          event.type = EV_SW;
          event.code = i;
          event.value = 1;
          FGLinuxEventData eventData( event, 0, 0 );
          HandleEvent(eventData);
        }
      }
    }
  }
}

double FGLinuxInputDevice::Normalize( struct input_event & event ) 
{
  if( absinfo.count(event.code) > 0 ) {
     const struct input_absinfo & ai = absinfo[(unsigned int)event.code];
     if( ai.maximum == ai.minimum )
       return 0.0;
     return ((double)event.value-(double)ai.minimum)/((double)ai.maximum-(double)ai.minimum);
  } else {
    return (double)event.value;
  }
}

void FGLinuxInputDevice::Close()
{
  if( fd != -1 ) {
    if( GetGrab() && ioctl( fd, EVIOCGRAB, 0 ) != 0 ) {
      SG_LOG( SG_INPUT, SG_WARN, "Can't ungrab " << devname );
    }
    ::close(fd);
  }
  fd = -1;
}

void FGLinuxInputDevice::Send( const char * eventName, double value )
{
  if( EVENT_TYPE_BY_NAME.count( eventName ) <= 0 ) {
    SG_LOG( SG_INPUT, SG_ALERT, "Can't send unknown event " << eventName );
    return;
  }

  TypeCode & typeCode = EVENT_TYPE_BY_NAME[ eventName ];

  if( fd == -1 )
    return;

  input_event evt;
  evt.type=typeCode.type;
  evt.code = typeCode.code;
  evt.value = (long)value;
  evt.time.tv_sec = 0;
  evt.time.tv_usec = 0;
  write( fd, &evt, sizeof(evt) );
  SG_LOG( SG_INPUT, SG_DEBUG, "Written event " << eventName 
          << " as type=" << evt.type << ", code=" << evt.code << " value=" << evt.value );
}

static char ugly_buffer[128];
const char * FGLinuxInputDevice::TranslateEventName( FGEventData & eventData ) 
{
  FGLinuxEventData & linuxEventData = (FGLinuxEventData&)eventData;
  TypeCode typeCode;
  typeCode.type = linuxEventData.type;
  typeCode.code = linuxEventData.code;
  if( EVENT_NAME_BY_TYPE.count(typeCode) == 0 ) {
    // event not known in translation tables
    if( EVENT_NAME_BY_EVENT_TYPE.count(linuxEventData.type) == 0 ) {
      // event type not known in translation tables
      sprintf( ugly_buffer, "unknown-%u-%u", (unsigned)linuxEventData.type, (unsigned)linuxEventData.code );
      return ugly_buffer;
    }
    sprintf( ugly_buffer, "%s-%u", EVENT_NAME_BY_EVENT_TYPE[linuxEventData.type], (unsigned)linuxEventData.code );
    return ugly_buffer;
  }

  return EVENT_NAME_BY_TYPE[typeCode];
}

void FGLinuxInputDevice::SetDevname( const std::string & name )
{
  this->devname = name; 
}

FGLinuxEventInput::FGLinuxEventInput()
{
}

FGLinuxEventInput::~FGLinuxEventInput()
{
}

void FGLinuxEventInput::postinit()
{
  FGEventInput::postinit();

  struct udev * udev = udev_new();

  struct udev_enumerate *enumerate = udev_enumerate_new(udev);
  udev_enumerate_add_match_subsystem(enumerate, "input");
  udev_enumerate_scan_devices(enumerate);
  struct udev_list_entry *devices = udev_enumerate_get_list_entry(enumerate);
  struct udev_list_entry *dev_list_entry;

  udev_list_entry_foreach(dev_list_entry, devices) {
    const char * path = udev_list_entry_get_name(dev_list_entry);
    struct udev_device *dev = udev_device_new_from_syspath(udev, path);
    const char * node = udev_device_get_devnode(dev);
 
    dev = udev_device_get_parent( dev );
    const char * name = udev_device_get_sysattr_value(dev,"name");

    SG_LOG(SG_INPUT,SG_DEBUG, "name=" << (name?name:"<null>") << ", node=" << (node?node:"<null>"));
    if( name && node )
      AddDevice( new FGLinuxInputDevice(name, node) );

    udev_device_unref(dev);
  }

  udev_unref(udev);

}

void FGLinuxEventInput::update( double dt )
{
  FGEventInput::update( dt );
  // index the input devices by the associated fd and prepare
  // the pollfd array by filling in the file descriptor
  struct pollfd fds[input_devices.size()];
  std::map<int,FGLinuxInputDevice*> devicesByFd;
  std::map<int,FGInputDevice*>::const_iterator it;
  int i;
  for( i=0, it = input_devices.begin(); it != input_devices.end(); ++it, i++ ) {
    FGInputDevice* p = (*it).second;
    int fd = ((FGLinuxInputDevice*)p)->GetFd();
    fds[i].fd = fd;
    fds[i].events = POLLIN;
    devicesByFd[fd] = (FGLinuxInputDevice*)p;
  }

  int modifiers = fgGetKeyModifiers();
  // poll all devices until no more events are in the queue
  // do no more than maxpolls in a single loop to prevent locking
  int maxpolls = 100;
  while( maxpolls-- > 0 && ::poll( fds, i, 0 ) > 0 ) {
    for( unsigned i = 0; i < sizeof(fds)/sizeof(fds[0]); i++ ) {
      if( fds[i].revents & POLLIN ) {

        // if this device reports data ready, it should be a input_event struct
        struct input_event event;

        if( read( fds[i].fd, &event, sizeof(event) ) != sizeof(event) )
          continue;

        FGLinuxEventData eventData( event, dt, modifiers );

        if( event.type == EV_ABS )
          eventData.value = devicesByFd[fds[i].fd]->Normalize( event );

        // let the FGInputDevice handle the data
        devicesByFd[fds[i].fd]->HandleEvent( eventData );
      }
    }
  }
}

