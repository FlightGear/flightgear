// Stream based logging mechanism.
//
// Written by Bernie Bright, 1998
//
// Copyright (C) 1998  Bernie Bright - bbright@c031.aone.net.au
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
// (Log is kept at end of this file)

#include "logstream.hxx"

bool            logbuf::logging_enabled = true;
fgDebugClass    logbuf::logClass        = FG_NONE;
fgDebugPriority logbuf::logPriority     = FG_INFO;
streambuf*      logbuf::sbuf            = NULL;

logbuf::logbuf()
{
//     if ( sbuf == NULL )
// 	sbuf = cerr.rdbuf();
}

logbuf::~logbuf()
{
    if ( sbuf )
	    sync();
}

void
logbuf::set_sb( streambuf* sb )
{
    if ( sbuf )
	    sync();

    sbuf = sb;
}

void
logbuf::set_log_level( fgDebugClass c, fgDebugPriority p )
{
    logClass = c;
    logPriority = p;
}

void
logstream::setLogLevels( fgDebugClass c, fgDebugPriority p )
{
    logbuf::set_log_level( c, p );
}

// $Log$
// Revision 1.2  1999/01/19 20:53:34  curt
// Portability updates by Bernie Bright.
//
// Revision 1.1  1998/11/06 21:20:41  curt
// Initial revision.
//
