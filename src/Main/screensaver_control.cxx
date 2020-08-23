// screensaver_control.cxx -- disable the screensaver
//
// Written by Rebecca Palmer, December 2013.
//
// Copyright (C) 2013 Rebecca Palmer
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

#include <config.h>
#include <simgear/compiler.h>

#ifdef HAVE_DBUS
#include <dbus/dbus.h>//Uses the low-level libdbus rather than GDBus/QtDBus to avoid adding more dependencies than necessary.  http://dbus.freedesktop.org/doc/api/html/index.html
#endif
/** Attempt to disable the screensaver.
* 
* Screensavers/powersavers often do not monitor the joystick, and it is hence advisable to disable them while FlightGear is running.
* This function always exists, but currently only actually does anything on Linux, where it will disable gnome-screensaver/kscreensaver until FlightGear exits.
*
* The following might be useful to anyone wishing to add Windows support:
* http://msdn.microsoft.com/en-us/library/windows/desktop/aa373233%28v=vs.85%29.aspx
* http://msdn.microsoft.com/en-us/library/windows/desktop/aa373208%28v=vs.85%29.aspx (While this documentation says it only disables powersave, it is elsewhere reported to also disable screensaver)
* http://msdn.microsoft.com/en-us/library/windows/desktop/dd405534%28v=vs.85%29.aspx
*/
void fgOSDisableScreensaver()
{
#if defined(HAVE_DBUS) && defined(SG_UNIX) && !defined(SG_MAC)
    DBusConnection *dbus_connection;
    DBusMessage *dbus_inhibit_screenlock;
    unsigned int window_id=1000;//fake-it doesn't seem to care
    unsigned int inhibit_idle=8;//8=idle inhibit flag
    const char *app_name="org.flightgear";
    const char *inhibit_reason="Uses joystick input";
    
    // REVIEW: Memory Leak - 2,056 bytes in 1 blocks are still reachable
    dbus_connection=dbus_bus_get(DBUS_BUS_SESSION,NULL);
    dbus_connection_set_exit_on_disconnect(dbus_connection,FALSE);//Don't close us if we lose the DBus connection
    
    //Two possible interfaces; we send on both, as that is easier than trying to determine which will work
    //GNOME: https://people.gnome.org/~mccann/gnome-session/docs/gnome-session.html
    dbus_inhibit_screenlock=dbus_message_new_method_call("org.gnome.SessionManager","/org/gnome/SessionManager","org.gnome.SessionManager","Inhibit");
    // REVIEW: Memory Leak - 352 bytes in 1 blocks are indirectly lost
    dbus_message_append_args(dbus_inhibit_screenlock,DBUS_TYPE_STRING,&app_name,DBUS_TYPE_UINT32,&window_id,DBUS_TYPE_STRING,&inhibit_reason,DBUS_TYPE_UINT32,&inhibit_idle,DBUS_TYPE_INVALID);
    dbus_connection_send(dbus_connection,dbus_inhibit_screenlock,NULL);
    
    //KDE, GNOME 3.6+: http://standards.freedesktop.org/idle-inhibit-spec/0.1/re01.html
    dbus_inhibit_screenlock=dbus_message_new_method_call("org.freedesktop.ScreenSaver","/ScreenSaver","org.freedesktop.ScreenSaver","Inhibit");
    // REVIEW: Memory Leak - 320 bytes in 1 blocks are still reachable
    dbus_message_append_args(dbus_inhibit_screenlock,DBUS_TYPE_STRING,&app_name,DBUS_TYPE_STRING,&inhibit_reason,DBUS_TYPE_INVALID);
    dbus_connection_send(dbus_connection,dbus_inhibit_screenlock,NULL);
    dbus_connection_flush(dbus_connection);
    //Currently ignores the reply; it would need to read it if we wanted to determine whether we've succeeded and/or allow explicitly re-enabling the screensaver
    //Don't disconnect, that ends the inhibition
#endif
}
