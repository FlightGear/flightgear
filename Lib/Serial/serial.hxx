// serial.hxx -- Unix serial I/O support
//
// Written by Curtis Olson, started November 1998.
//
// Copyright (C) 1998  Curtis L. Olson - curt@flightgear.org
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
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//
// $Id$


#ifndef _SERIAL_HXX
#define _SERIAL_HXX


#ifndef __cplusplus
# error This library requires C++
#endif

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#if defined( WIN32 ) && !defined( __CYGWIN__) && !defined( __CYGWIN32__ )
#  include <windows.h>
#endif

#include <Include/compiler.h>
#include STL_STRING
FG_USING_STD(string);

// if someone know how to do this all with C++ streams let me know
// #include <stdio.h>


class fgSERIAL
{
#if defined( WIN32 ) && !defined( __CYGWIN__) && !defined( __CYGWIN32__ )
    typedef HANDLE fd_type;
#else
    typedef int fd_type;
#endif

private:

    fd_type fd;
    bool dev_open;

public:

    fgSERIAL();
    fgSERIAL(const string& device, int baud);

    ~fgSERIAL();

    bool open_port(const string& device);
    bool close_port();
    bool set_baud(int baud);
    string read_port();
    int write_port(const string& value);

    inline bool is_enabled() { return dev_open; }
};


#endif // _SERIAL_HXX


