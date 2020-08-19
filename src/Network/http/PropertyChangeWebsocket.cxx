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
#include <simgear/props/props.hxx>
#include <simgear/structure/commands.hxx>

#include <simgear/props/props_io.hxx>
#include <Main/globals.hxx>
#include <Main/fg_props.hxx>

#include <cJSON.h>

namespace flightgear {
namespace http {

using std::string;

static unsigned nextid = 0;

static void setPropertyFromJson(SGPropertyNode_ptr prop, cJSON * json)
{
  if (!prop) return;
  if ( NULL == json ) return;
  switch ( json->type ) {
  case cJSON_String:
    prop->setStringValue(json->valuestring);
    break;
    
  case cJSON_Number:
    prop->setDoubleValue(json->valuedouble);
    break;
    
  case cJSON_True:
    prop->setBoolValue(true);
    break;
      
  case cJSON_False:
    prop->setBoolValue(false);
    break;
      
  default:
    return;
  }
}
  
static void handleSetCommand(const string_list& nodes, cJSON* json, WebsocketWriter &writer)
{
  cJSON * value = cJSON_GetObjectItem(json, "value");
  if ( NULL != value ) {
    if (nodes.size() > 1) {
      SG_LOG(SG_NETWORK, SG_WARN, "httpd: WS set: insufficent values for nodes:" << nodes.size());
      return;
    }
    
    SGPropertyNode_ptr n = fgGetNode(nodes.front());
    if (!n) {
      SG_LOG(SG_NETWORK, SG_WARN, "httpd: set '" << nodes.front() << "'  not found");
      return;
    }
    
    setPropertyFromJson(n, value);
    return;
  }
  
  cJSON * values = cJSON_GetObjectItem(json, "values");
  if ( ( NULL == values ) || ( static_cast<size_t>(cJSON_GetArraySize(values)) != nodes.size()) ) {
    SG_LOG(SG_NETWORK, SG_WARN, "httpd: WS set: mismatched nodes/values sizes:" << nodes.size());
    return;
  }
  
  string_list::const_iterator it;
  int i=0;
  for (it = nodes.begin(); it != nodes.end(); ++it, ++i) {
    SGPropertyNode_ptr n = fgGetNode(*it);
    if (!n) {
      SG_LOG(SG_NETWORK, SG_WARN, "httpd: get '" << *it << "'  not found");
      return;
    }

    setPropertyFromJson(n, cJSON_GetArrayItem(values, i));
  } // of nodes iteration
}
  
static void handleExecCommand(cJSON* json)
{
  cJSON* name = cJSON_GetObjectItem(json, "fgcommand");
  if ((NULL == name )|| (NULL == name->valuestring)) {
    SG_LOG(SG_NETWORK, SG_WARN, "httpd: exec: no fgcommand name");
    return;
  }
  
  SGPropertyNode_ptr arg(new SGPropertyNode);
  JSON::addChildrenToProp( json, arg );
  
  globals->get_commands()->execute(name->valuestring, arg, nullptr);
}
  
PropertyChangeWebsocket::PropertyChangeWebsocket(PropertyChangeObserver * propertyChangeObserver)
    : id(++nextid),
      _propertyChangeObserver(propertyChangeObserver),
      _minTriggerInterval(fgGetDouble("/sim/http/property-websocket/update-interval-secs", 0.05)), // default 20Hz
      _lastTrigger(-1000)
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

void PropertyChangeWebsocket::handleGetCommand(const string_list& nodes, WebsocketWriter &writer)
{
  double t = fgGetDouble("/sim/time/elapsed-sec");
  string_list::const_iterator it;
  for (it = nodes.begin(); it != nodes.end(); ++it) {
    SGPropertyNode_ptr n = fgGetNode(*it);
    if (!n) {
      SG_LOG(SG_NETWORK, SG_WARN, "httpd: get '" << *it << "'  not found");
      return;
    }
    
    writer.writeText( JSON::toJsonString( false, n, 0, t ) );
  } // of nodes iteration
}
  
void PropertyChangeWebsocket::handleRequest(const HTTPRequest & request, WebsocketWriter &writer)
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

    // handle a single node name, or an array of them
    string_list nodeNames;
    j = cJSON_GetObjectItem(json, "node");
    if ( NULL != j && NULL != j->valuestring) {
        nodeNames.push_back(simgear::strutils::strip(string(j->valuestring)));
    }

    cJSON * nodes = cJSON_GetObjectItem(json, "nodes");
    if ( NULL != nodes) {
      for (int i = 0; i < cJSON_GetArraySize(nodes); i++) {
        cJSON * node = cJSON_GetArrayItem(nodes, i);
        if ( NULL == node) continue;
        if ( NULL == node->valuestring) continue;
        nodeNames.push_back(simgear::strutils::strip(string(node->valuestring)));
      }
    }
    
    if (command == "get") {
      handleGetCommand(nodeNames, writer);
    } else if (command == "set") {
      handleSetCommand(nodeNames, json, writer);
    } else if (command == "exec") {
      handleExecCommand(json);
    } else {
      string_list::const_iterator it;
      for (it = nodeNames.begin(); it != nodeNames.end(); ++it) {
        _watchedNodes.handleCommand(command, *it, _propertyChangeObserver);
      }
    }
    
    cJSON_Delete(json);
  }
}

void PropertyChangeWebsocket::poll(WebsocketWriter & writer)
{
  double now = fgGetDouble("/sim/time/elapsed-sec");

  if( _minTriggerInterval > .0 ) {
    if( now - _lastTrigger <= _minTriggerInterval )
      return;

    _lastTrigger = now;
  }

  for (WatchedNodesList::iterator it = _watchedNodes.begin(); it != _watchedNodes.end(); ++it) {
    SGPropertyNode_ptr node = *it;

    string newValue;
    if (_propertyChangeObserver->isChangedValue(node)) {
      string out = JSON::toJsonString( false, node, 0, now );
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
    }
    SG_LOG(SG_NETWORK, SG_WARN, "httpd: " << command << " '" << node << "' ignored (not found)");
  }
}

} // namespace http
} // namespace flightgear
