// fg_file.cxx -- File I/O routines
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


#include <Include/compiler.h>

#include STL_STRING

#include <Debug/logstream.hxx>
#include <Time/fg_time.hxx>

#include "fg_file.hxx"

FG_USING_STD(string);


FGFile::FGFile() {
}


FGFile::~FGFile() {
}


// open the file based on specified direction
bool FGFile::open( FGProtocol::fgProtocolDir dir ) {
    if ( dir == FGProtocol::out ) {
	fp = std::open( file_name.c_str(), O_WRONLY | O_CREAT | O_TRUNC,
			S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | 
			S_IROTH | S_IWOTH );
    } else if ( dir == FGProtocol::in ) {
	fp = std::open( file_name.c_str(), O_RDONLY );
    } else {
	FG_LOG( FG_IO, FG_ALERT, 
		"Error:  bidirection mode not available for files." );
	return false;
    }

    if ( fp == -1 ) {
	FG_LOG( FG_IO, FG_ALERT, "Error opening file: "	<< file_name );
	return false;
    }

    return true;
}


// read data from file
bool FGFile::read( char *buf, int *length ) {
    // save our current position
    int pos = lseek( fp, 0, SEEK_CUR );

    // read a chunk
    int result = std::read( fp, buf, FG_MAX_MSG_SIZE );

    // find the end of line and reset position
    int i;
    for ( i = 0; i < result && buf[i] != '\n'; ++i );
    if ( buf[i] == '\n' ) {
	*length = i + 1;
    } else {
	*length = i;
    }
    lseek( fp, pos + *length, SEEK_SET );
    
    // just in case ...
    buf[ *length ] = '\0';

    return true;
}


// write data to a file
bool FGFile::write( char *buf, int length ) {
    int result = std::write( fp, buf, length );
    if ( result != length ) {
	FG_LOG( FG_IO, FG_ALERT, "Error writing data: " << file_name );
	return false;
    }

    return true;
}


// close the port
bool FGFile::close() {
    if ( std::close( fp ) == -1 ) {
	return false;
    }

    return true;
}
