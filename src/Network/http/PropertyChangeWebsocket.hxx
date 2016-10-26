// PropertyChangeWebsocket.hxx -- A websocket for propertychangelisteners
//
// Written by Torsten Dreyer, started April 2014.
//
// Copyright (C) 2014  Torsten Dreyer
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

#ifndef PROPERTYCHANGEWEBSOCKET_HXX_
#define PROPERTYCHANGEWEBSOCKET_HXX_

#include "Websocket.hxx"
#include <simgear/props/props.hxx>

#include <vector>

namespace flightgear {
namespace http {

class PropertyChangeObserver;

class PropertyChangeWebsocket: public Websocket {
public:
  PropertyChangeWebsocket(PropertyChangeObserver * propertyChangeObserver);
  virtual ~PropertyChangeWebsocket();
  virtual void close();
  virtual void handleRequest(const HTTPRequest & request, WebsocketWriter & writer);
  virtual void poll(WebsocketWriter & writer);

private:
  unsigned id;
  PropertyChangeObserver * _propertyChangeObserver;

  void handleGetCommand(const string_list& nodes, WebsocketWriter &writer);
  
  class WatchedNodesList: public std::vector<SGPropertyNode_ptr> {
  public:
    void handleCommand(const std::string & command, const std::string & node, PropertyChangeObserver * propertyChangeObserver);
  };

  WatchedNodesList _watchedNodes;
  double _minTriggerInterval;
  double _lastTrigger;
};

}
}

#endif /* PROPERTYCHANGEWEBSOCKET_HXX_ */
