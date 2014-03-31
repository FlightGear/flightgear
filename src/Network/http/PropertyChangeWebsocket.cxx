// PropertyChangeWebsocket.cxx -- A websocket for propertychangelisteners
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

#include "PropertyChangeWebsocket.hxx"
#include "PropertyChangeObserver.hxx"
#include "jsonprops.hxx"
#include <simgear/debug/logstream.hxx>
#include <Main/fg_props.hxx>

#include <3rdparty/cjson/cJSON.h>

namespace flightgear {
namespace http {

using std::string;

static unsigned nextid = 0;

PropertyChangeWebsocket::PropertyChangeWebsocket(PropertyChangeObserver * propertyChangeObserver)
    : id(++nextid), _propertyChangeObserver(propertyChangeObserver)
{
}

PropertyChangeWebsocket::~PropertyChangeWebsocket()
{
}

void PropertyChangeWebsocket::close()
{
  SG_LOG(SG_NETWORK, SG_INFO, "closing PropertyChangeWebsocket #" << id);
  _watchedNodes.clear();
}

void PropertyChangeWebsocket::handleRequest(const HTTPRequest & request, WebsocketWriter &)
{
  if (request.Content.empty()) return;

  /*
   * allowed JSON is
   {
   command : 'addListener',
   nodes : [
   '/bar/baz',
   '/foo/bar'
   ],
   node: '/bax/foo'
   }
   */
  cJSON * json = cJSON_Parse(request.Content.c_str());
  if ( NULL != json) {
    string command;
    cJSON * j = cJSON_GetObjectItem(json, "command");
    if ( NULL != j && NULL != j->valuestring) {
      command = j->valuestring;
    }

    string nodeName;
    j = cJSON_GetObjectItem(json, "node");
    if ( NULL != j && NULL != j->valuestring) nodeName = j->valuestring;
    _watchedNodes.handleCommand(command, nodeName, _propertyChangeObserver);

    cJSON * nodes = cJSON_GetObjectItem(json, "nodes");
    if ( NULL != nodes) {
      for (int i = 0; i < cJSON_GetArraySize(nodes); i++) {
        cJSON * node = cJSON_GetArrayItem(nodes, i);
        if ( NULL == node) continue;
        if ( NULL == node->valuestring) continue;
        nodeName = node->valuestring;
        _watchedNodes.handleCommand(command, nodeName, _propertyChangeObserver);
      }
    }
    cJSON_Delete(json);
  }
}

void PropertyChangeWebsocket::poll(WebsocketWriter & writer)
{
  for (WatchedNodesList::iterator it = _watchedNodes.begin(); it != _watchedNodes.end(); ++it) {
    SGPropertyNode_ptr node = *it;

    string newValue;
    if (_propertyChangeObserver->isChangedValue(node)) {
      string out = JSON::toJsonString( false, node, 0, fgGetDouble("/sim/time/elapsed-sec") );
      SG_LOG(SG_NETWORK, SG_DEBUG, "PropertyChangeWebsocket::poll() new Value for " << node->getPath(true) << " '" << node->getStringValue() << "' #" << id << ": " << out );
      writer.writeText( out );
    }
  }
}

void PropertyChangeWebsocket::WatchedNodesList::handleCommand(const string & command, const string & node,
    PropertyChangeObserver * propertyChangeObserver)
{
  if (command == "addListener") {
    for (iterator it = begin(); it != end(); ++it) {
      if (node == (*it)->getPath(true)) {
        SG_LOG(SG_NETWORK, SG_WARN, "httpd: " << command << " '" << node << "' ignored (duplicate)");
        return; // dupliate
      }
    }
    SGPropertyNode_ptr n = propertyChangeObserver->addObservation(node);
    if (n.valid()) push_back(n);
    SG_LOG(SG_NETWORK, SG_INFO, "httpd: " << command << " '" << node << "' success");

  } else if (command == "removeListener") {
    for (iterator it = begin(); it != end(); ++it) {
      if (node == (*it)->getPath(true)) {
        this->erase(it);
        SG_LOG(SG_NETWORK, SG_INFO, "httpd: " << command << " '" << node << "' success");
        return;
      }
      SG_LOG(SG_NETWORK, SG_WARN, "httpd: " << command << " '" << node << "' ignored (not found)");
    }
  }
}

} // namespace http
} // namespace flightgear
