// HDNSClient.cxx -- Singleton DNS client object
//
// Written by James Turner, started April 2012.
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

#include "config.h"

#include "DNSClient.hxx"

#include <cassert>

#include <Main/fg_props.hxx>

#include <simgear/sg_inlines.h>


using namespace simgear;

FGDNSClient::FGDNSClient() :
    _inited(false)
{
}

FGDNSClient::~FGDNSClient()
{
}

void FGDNSClient::init()
{
    if (_inited) {
        return;
    }

    _dns.reset( new simgear::DNS::Client() );

    _inited = true;
}

void FGDNSClient::postinit()
{
}

void FGDNSClient::shutdown()
{
    _dns.reset();
    _inited = false;
}

void FGDNSClient::update(double)
{
  _dns->update();
}

void FGDNSClient::makeRequest(const simgear::DNS::Request_ptr& req)
{
  _dns->makeRequest(req);
}


// Register the subsystem.
SGSubsystemMgr::Registrant<FGDNSClient> registrantFGDNSClient;
