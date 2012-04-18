// HTTPClient.cxx -- Singleton HTTP client object
//
// Written by James Turner, started April 2012.
//
// Copyright (C) 2012  James Turner
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

#include "HTTPClient.hxx"

#include <Main/fg_props.hxx>

static FGHTTPClient* static_instance = NULL;

FGHTTPClient* FGHTTPClient::instance()
{
	if (!static_instance) {
		static_instance = new FGHTTPClient;
	}
	
	return static_instance;
}

bool FGHTTPClient::haveInstance()
{
    return (static_instance != NULL);
}

FGHTTPClient::FGHTTPClient()
{
    std::string proxyHost(fgGetString("/sim/presets/proxy/host"));
    int proxyPort(fgGetInt("/sim/presets/proxy/port"));
    std::string proxyAuth(fgGetString("/sim/presets/proxy/auth"));
    
    if (!proxyHost.empty()) {
        setProxy(proxyHost, proxyPort, proxyAuth);
    }
}
