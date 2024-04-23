// FGEventInput.hxx -- handle event driven input devices for the Linux O/S
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

#ifndef __FGLINUXEVENTINPUT_HXX
#define __FGLINUXEVENTINPUT_HXX

#include "FGEventInput.hxx"
#include <linux/input.h>

struct FGLinuxEventData : public FGEventData {
    FGLinuxEventData( struct input_event & event, double dt, int modifiers ) :
        FGEventData( (double)event.value, dt, modifiers ),
        type(event.type),
        code(event.code) {
    }
    unsigned type;
    unsigned code;
};

/*
 * A implementation for linux event devices
 */
class FGLinuxInputDevice : public FGInputDevice
{
public:
    FGLinuxInputDevice();
    FGLinuxInputDevice(std::string aName, std::string aDevname, std::string aSerial, std::string aDevpath);
    virtual ~FGLinuxInputDevice();

    bool Open() override;
    void Close() override;
    void Send( const char * eventName, double value ) override;
    const char * TranslateEventName( FGEventData & eventData ) override;

    void SetDevname( const std::string & name );
    std::string GetDevFile() const { return devfile; }
    std::string GetDevPath() const { return devpath; }

    int GetFd() { return fd; }

    double Normalize( struct input_event & event );
private:
    std::string devfile;
    std::string devpath;
    int fd {-1};
    std::map<unsigned int,input_absinfo> absinfo;
};

class FGLinuxEventInput : public FGEventInput
{
public:
    FGLinuxEventInput();
    virtual ~ FGLinuxEventInput();

    // Subsystem API.
    void postinit() override;
    void update(double dt) override;

    // Subsystem identification.
    static const char* staticSubsystemClassId() { return "input-event"; }

protected:
};

#endif
