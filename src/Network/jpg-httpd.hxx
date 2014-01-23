// jpg-httpd.hxx -- FGFS jpg-http interface
//
// Written by Curtis Olson, started June 2001.
//
// Copyright (C) 2001  Curtis L. Olson - http://www.flightgear.org/~curt
//
// Jpeg Image Support added August 2001
//  by Norman Vine - nhv@cape.com
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

#ifndef _FG_JPEG_HTTPD_HXX
#define _FG_JPEG_HTTPD_HXX

#include <memory> // for auto_ptr
#include <string>

#include "protocol.hxx"

// forward decls
class HttpdThread;

class FGJpegHttpd : public FGProtocol
{
    std::auto_ptr<HttpdThread> _imageServer;

public:
    FGJpegHttpd( int p, int hz, const std::string& type );
    ~FGJpegHttpd();

    bool open();
    bool process();
    bool close();
};

#endif // _FG_JPEG_HTTPD_HXX
