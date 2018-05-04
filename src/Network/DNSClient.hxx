// DNSClient.hxx -- Singleton DNS client object
//
// Written by Torsten Dreyer, started November 2016.
// Based on HTTPClient from James Turner
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

#ifndef FG_DNS_CLIENT_HXX
#define FG_DNS_CLIENT_HXX

#include <simgear/structure/subsystem_mgr.hxx>
#include <simgear/io/DNSClient.hxx>
#include <memory>

class FGDNSClient : public SGSubsystem
{
public:
    FGDNSClient();
    virtual ~FGDNSClient();

    // Subsystem API.
    void init() override;
    void postinit() override;
    void shutdown() override;
    void update(double) override;

    // Subsystem identification.
    static const char* staticSubsystemClassId() { return "dns"; }

    void makeRequest(const simgear::DNS::Request_ptr& req);

//    simgear::HTTP::Client* client() { return _http.get(); }
//    simgear::HTTP::Client const* client() const { return _http.get(); }

private:
    bool _inited;
    std::unique_ptr<simgear::DNS::Client> _dns;
};

#endif // FG_DNS_CLIENT_HXX


