// fg_file.hxx -- File I/O routines
//
// Written by Curtis Olson, started November 1999.
//
// Copyright (C) 1999  Curtis L. Olson - curt@flightgear.org
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


#ifndef _FG_FILE_HXX
#define _FG_FILE_HXX


#ifndef __cplusplus
# error This library requires C++
#endif

#include "Include/compiler.h"

#include <string>

#include <sys/types.h>		// for open(), read(), write(), close()
#include <sys/stat.h>		// for open(), read(), write(), close()
#include <fcntl.h>		// for open(), read(), write(), close()
#include <unistd.h>		// for open(), read(), write(), close()

#include "iochannel.hxx"
#include "protocol.hxx"

FG_USING_STD(string);


class FGFile : public FGIOChannel {

    string file_name;
    int fp;

public:

    FGFile();
    ~FGFile();

    // open the file based on specified direction
    bool open( FGProtocol::fgProtocolDir dir );

    // read a block of data of specified size
    int read( char *buf, int length );

    // read a line of data, length is max size of input buffer
    int readline( char *buf, int length );

    // write data to a file
    int write( char *buf, int length );

    // close file
    bool close();

    inline string get_file_name() const { return file_name; }
    inline void set_file_name( const string& fn ) { file_name = fn; }
};


#endif // _FG_FILE_HXX


