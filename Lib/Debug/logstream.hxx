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

#ifndef _LOGSTREAM_H
#define _LOGSTREAM_H

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif


#include <Include/compiler.h>

#ifdef FG_HAVE_STD_INCLUDES
# include <streambuf>
# include <iostream>
#else
# include <iostream.h>
# include "Include/fg_traits.hxx"
#endif

#include "debug_types.h"

#ifndef FG_HAVE_NATIVE_SGI_COMPILERS
FG_USING_STD(streambuf);
FG_USING_STD(ostream);
FG_USING_STD(cerr);
FG_USING_STD(endl);
#endif

//
// TODO:
//
// 1. Change output destination. Done.
// 2. Make logbuf thread safe.
// 3. Read environment for default debugClass and debugPriority.
//

//-----------------------------------------------------------------------------
//
// logbuf is an output-only streambuf with the ability to disable sets of
// messages at runtime. Only messages with priority >= logbuf::logPriority
// and debugClass == logbuf::logClass are output.
//
class logbuf : public streambuf
{
public:

#ifndef FG_HAVE_STD_INCLUDES
    typedef char_traits<char>           traits_type;
    typedef char_traits<char>::int_type int_type;
    typedef char_traits<char>::pos_type pos_type;
    typedef char_traits<char>::off_type off_type;
#endif
//     logbuf( streambuf* sb ) : sbuf(sb) {}
    logbuf();
    ~logbuf();

    // Is logging enabled?
    bool enabled() { return logging_enabled; }

    // Set the logging level of subsequent messages.
    void set_log_state( fgDebugClass c, fgDebugPriority p );

    // Set the global logging level.
    static void set_log_level( fgDebugClass c, fgDebugPriority p );

    //
    void set_sb( streambuf* sb );

protected:

    inline virtual int sync();
    int_type overflow( int ch );
//     int xsputn( const char* s, istreamsize n );

private:

    // The streambuf used for actual output. Defaults to cerr.rdbuf().
    static streambuf* sbuf;

    static bool logging_enabled;
    static fgDebugClass logClass;
    static fgDebugPriority logPriority;

private:

    // Not defined.
    logbuf( const logbuf& );
    void operator= ( const logbuf& );
};

inline int
logbuf::sync()
{
#ifdef FG_HAVE_STD_INCLUDES
	return sbuf->pubsync();
#else
	return sbuf->sync();
#endif
}

inline void
logbuf::set_log_state( fgDebugClass c, fgDebugPriority p )
{
    logging_enabled = ((c & logClass) != 0 && p >= logPriority);
}

inline logbuf::int_type
logbuf::overflow( int c )
{
    return logging_enabled ? sbuf->sputc(c) : (EOF == 0 ? 1: 0);
}

//-----------------------------------------------------------------------------
//
// logstream manipulator for setting the log level of a message.
//
struct loglevel
{
    loglevel( fgDebugClass c, fgDebugPriority p )
	: logClass(c), logPriority(p) {}

    fgDebugClass logClass;
    fgDebugPriority logPriority;
};

//-----------------------------------------------------------------------------
//
// A helper class that ensures a streambuf and ostream are constructed and
// destroyed in the correct order.  The streambuf must be created before the
// ostream but bases are constructed before members.  Thus, making this class
// a private base of logstream, declared to the left of ostream, we ensure the
// correct order of construction and destruction.
//
struct logstream_base
{
//     logstream_base( streambuf* sb ) : lbuf(sb) {}
    logstream_base() {}

    logbuf lbuf;
};

//-----------------------------------------------------------------------------
//
// 
//
class logstream : private logstream_base, public ostream
{
public:
    // The default is to send messages to cerr.
    logstream( ostream& out )
// 	: logstream_base(out.rdbuf()),
	: logstream_base(),
	  ostream(&lbuf) { lbuf.set_sb(out.rdbuf());}

    void set_output( ostream& out ) { lbuf.set_sb( out.rdbuf() ); }

    // Set the global log class and priority level.
     void setLogLevels( fgDebugClass c, fgDebugPriority p );

    // Output operator to capture the debug level and priority of a message.
    inline ostream& operator<< ( const loglevel& l );
};

inline ostream&
logstream::operator<< ( const loglevel& l )
{
    lbuf.set_log_state( l.logClass, l.logPriority );
    return *this;
}

//-----------------------------------------------------------------------------
//
// Return the one and only logstream instance.
// We use a function instead of a global object so we are assured that cerr
// has been initialised.
//
inline logstream&
fglog()
{
    static logstream logstrm( cerr );
    return logstrm;
}

#ifdef FG_NDEBUG
# define FG_LOG(C,P,M)
#else
# define FG_LOG(C,P,M) fglog() << loglevel(C,P) << M << endl
#endif

#endif // _LOGSTREAM_H

// $Log$
// Revision 1.1  1999/04/05 21:32:33  curt
// Initial revision
//
// Revision 1.4  1999/03/02 01:01:47  curt
// Tweaks for compiling with native SGI compilers.
//
// Revision 1.3  1999/01/19 20:53:35  curt
// Portability updates by Bernie Bright.
//
// Revision 1.2  1998/11/07 19:07:02  curt
// Enable release builds using the --without-logging option to the configure
// script.  Also a couple log message cleanups, plus some C to C++ comment
// conversion.
//
// Revision 1.1  1998/11/06 21:20:42  curt
// Initial revision.
//
