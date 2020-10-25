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

#include <simgear/io/HTTPClient.hxx>
#include <simgear/package/Catalog.hxx>
#include <simgear/structure/subsystem_mgr.hxx>

#include <memory>

class FGHTTPClient : public SGSubsystem
{
public:
    FGHTTPClient();
    virtual ~FGHTTPClient();

    // Subsystem API.
    void init() override;
    void postinit() override;
    void shutdown() override;
    void update(double) override;

    // Subsystem identification.
    static const char* staticSubsystemClassId() { return "http"; }

    void makeRequest(const simgear::HTTP::Request_ptr& req);

    simgear::HTTP::Client* client() { return _http.get(); }
    simgear::HTTP::Client const* client() const { return _http.get(); }

    bool isDefaultCatalogInstalled() const;
    simgear::pkg::CatalogRef addDefaultCatalog();

    std::string getDefaultCatalogId() const;
    std::string getDefaultCatalogUrl() const;
    std::string getDefaultCatalogFallbackUrl() const;

private:
    bool _inited;
    std::unique_ptr<simgear::HTTP::Client> _http;
};

#endif // FG_HTTP_CLIENT_HXX


