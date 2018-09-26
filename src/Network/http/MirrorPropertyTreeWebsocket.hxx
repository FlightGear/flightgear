// MirrorPropertyTreeWebsocket.hxx -- A websocket for mirroring a property sub-tree
//
// Written by James Turner, started November 2016.
//
// Copyright (C) 2016  James Turner
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

#ifndef MIRROR_PROP_TREE_WEBSOCKET_HXX_
#define MIRROR_PROP_TREE_WEBSOCKET_HXX_

#include "Websocket.hxx"

#include <simgear/props/props.hxx>
#include <simgear/timing/timestamp.hxx>

#include <vector>
#include <memory>

namespace flightgear {
namespace http {

    class MirrorTreeListener;
    
class MirrorPropertyTreeWebsocket : public Websocket
{
public:
    MirrorPropertyTreeWebsocket(const std::string& path);
    ~MirrorPropertyTreeWebsocket() override;

    void close() override;
    void handleRequest(const HTTPRequest & request, WebsocketWriter & writer) override;
    void poll(WebsocketWriter & writer) override;

private:
    void checkNodeExists();

    friend class MirrorTreeListener;

    std::string _rootPath;
    SGPropertyNode_ptr _subtreeRoot;
    std::unique_ptr<MirrorTreeListener> _listener;
    int _minSendInterval;
    SGTimeStamp _lastSendTime;
};

}
}

#endif /* MIRROR_PROP_TREE_WEBSOCKET_HXX_ */
