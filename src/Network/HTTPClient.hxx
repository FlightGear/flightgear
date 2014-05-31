// HTTPClient.hxx -- Singleton HTTP client object
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

#ifndef FG_HTTP_CLIENT_HXX
#define FG_HTTP_CLIENT_HXX

#include <simgear/structure/subsystem_mgr.hxx>
#include <simgear/io/HTTPClient.hxx>
#include <memory>

class FGHTTPClient : public SGSubsystem
{
public:
    FGHTTPClient();
    virtual ~FGHTTPClient();
    
    void makeRequest(const simgear::HTTP::Request_ptr& req);

    simgear::HTTP::Client* client() { return _http.get(); }
    simgear::HTTP::Client const* client() const { return _http.get(); }
    
    virtual void init();
    virtual void postinit();
    virtual void shutdown();
    virtual void update(double);

private:
    std::auto_ptr<simgear::HTTP::Client> _http;
};

#endif // FG_HTTP_CLIENT_HXX


