// MirrorPropertyTreeWebsocket.cxx -- A websocket for mirroring a property sub-tree
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

#include "MirrorPropertyTreeWebsocket.hxx"
#include "jsonprops.hxx"

#include <unordered_map>
#include <set>

#include <simgear/debug/logstream.hxx>
#include <simgear/props/props.hxx>
#include <simgear/structure/commands.hxx>

#include <simgear/props/props_io.hxx>
#include <Main/globals.hxx>
#include <Main/fg_props.hxx>

#include <3rdparty/cjson/cJSON.h>

namespace flightgear {
namespace http {

using std::string;

    typedef unsigned int PropertyId; // connection local property id

    struct PropertyValue
    {
        PropertyValue(SGPropertyNode* cur = nullptr) :
            type(simgear::props::NONE)
        {
            if (!cur) {
                return;
            }

            type = cur->getType();
            switch (type) {
            case simgear::props::INT:
                intValue = cur->getIntValue();
                break;

            case simgear::props::BOOL:
                intValue = cur->getBoolValue();
                break;

            case simgear::props::FLOAT:
            case simgear::props::DOUBLE:
                doubleValue = cur->getDoubleValue();
                break;

            case simgear::props::STRING:
            case simgear::props::UNSPECIFIED:
                stringValue = cur->getStringValue();
                break;

            case simgear::props::NONE:
                break;

            default:
                SG_LOG(SG_NETWORK, SG_INFO, "implement me!" << type);
                break;
            }
        }

        bool equals(SGPropertyNode* node, const PropertyValue& other) const
        {
            if (other.type != type) return false;
            switch (type) {
            case simgear::props::INT:
            case simgear::props::BOOL:
                return intValue == other.intValue;

            case simgear::props::FLOAT:
            case simgear::props::DOUBLE:
                return std::fabs(doubleValue - other.doubleValue) < 1e-4;

            case simgear::props::STRING:
            case simgear::props::UNSPECIFIED:
                return stringValue == other.stringValue;

            case simgear::props::NONE:
                return true;

            default:
                break;
            }

            return false;
        }

        simgear::props::Type type;
        union {
            int intValue;
            double doubleValue;
        };
        std::string stringValue;
    };


    struct RecentlyRemovedNode
    {
        RecentlyRemovedNode() : type(simgear::props::NONE), id(0) { }

        RecentlyRemovedNode(SGPropertyNode* node, unsigned int aId) :
            type(node->getType()),
            path(node->getPath()),
            id(aId)
        {}

        simgear::props::Type type;
        std::string path;
        unsigned int id;

        bool operator==(const RecentlyRemovedNode& other) const
        {
            return (type == other.type) && (path == other.path);
        }
    };

    class MirrorTreeListener : public SGPropertyChangeListener
    {
    public:
        MirrorTreeListener() : SGPropertyChangeListener(true /* recursive */)
        {
            previousValues.resize(2);
        }

        virtual ~MirrorTreeListener()
        {
            SG_LOG(SG_NETWORK, SG_INFO, "destroying MirrorTreeListener");
        }

        virtual void valueChanged(SGPropertyNode* node) override
        {
            auto it = idHash.find(node);
            if (it == idHash.end()) {
                // not new to the server, but new to the client
                newNodes.insert(node);
            } else {
                assert(previousValues.size() > it->second);
                PropertyValue newVal(node);
                if (!previousValues[it->second].equals(node, newVal)) {
                    previousValues[it->second] = newVal;
                    changedNodes.insert(node);
                }
            }
        }

        virtual void childAdded(SGPropertyNode* parent, SGPropertyNode* child) override
        {
            // this works becuase custom operator== overload on RecentlyRemovedNode
            // ignores the ID value.
            RecentlyRemovedNode m(child, 0);
            auto rrIt = std::find(recentlyRemoved.begin(), recentlyRemoved.end(), m);
            if (rrIt != recentlyRemoved.end()) {
                // recycle nodes which get thrashed from Nasal (deleted + re-created
                // each time a Nasal timer fires)
                removedNodes.erase(rrIt->id); // don't remove it!
                idHash.insert(std::make_pair(child, rrIt->id));
                changedNodes.insert(child);
                recentlyRemoved.erase(rrIt);
                return;
            }

            newNodes.insert(child);
        }

        virtual void childRemoved(SGPropertyNode* parent, SGPropertyNode* child) override
        {
            changedNodes.erase(child); // have to do this here with the pointer valid
            newNodes.erase(child);

            auto it = idHash.find(child);
            if (it != idHash.end()) {
                removedNodes.insert(it->second);
                idHash.erase(it);

                // record so we can map removed+add of the same property into
                // a simple value change (this happens commonly with the canvas
                // due to lazy Nasal scripting)
                recentlyRemoved.push_back(RecentlyRemovedNode(child, it->second));
            }
        }

        void registerSubtree(SGPropertyNode* node)
        {
            valueChanged(node);

            // and recurse
            int child = 0;
            for (; child < node->nChildren(); ++child) {
                registerSubtree(node->getChild(child));
            }
        }

        std::set<SGPropertyNode*> newNodes;
        std::set<SGPropertyNode*> changedNodes;
        std::set<PropertyId> removedNodes;

        PropertyId idForProperty(SGPropertyNode* prop)
        {
            auto it = idHash.find(prop);
            if (it == idHash.end()) {
                it = idHash.insert(it, std::make_pair(prop, nextPropertyId++));
                previousValues.push_back(PropertyValue(prop));
            }
            return it->second;
        }

        cJSON* makeJSONData()
        {
            SGTimeStamp st;
            st.stamp();

            cJSON* result = cJSON_CreateObject();

            int newSize = newNodes.size();
            int changedSize = changedNodes.size();
            int removedSize = removedNodes.size();

            if (!newNodes.empty()) {
                cJSON * newNodesArray = cJSON_CreateArray();

                for (auto prop : newNodes) {
                    changedNodes.erase(prop); // avoid duplicate send
                    cJSON* newPropData = cJSON_CreateObject();
                    cJSON_AddItemToObject(newPropData, "path", cJSON_CreateString(prop->getPath(true).c_str()));
                    cJSON_AddItemToObject(newPropData, "type", cJSON_CreateString(JSON::getPropertyTypeString(prop->getType())));
                    cJSON_AddItemToObject(newPropData, "index", cJSON_CreateNumber(prop->getIndex()));
                    cJSON_AddItemToObject(newPropData, "position", cJSON_CreateNumber(prop->getPosition()));
                    cJSON_AddItemToObject(newPropData, "id", cJSON_CreateNumber(idForProperty(prop)));
                    if (prop->getType() != simgear::props::NONE) {
                        cJSON_AddItemToObject(newPropData, "value", JSON::valueToJson(prop));
                    }
                    cJSON_AddItemToArray(newNodesArray, newPropData);
                }

                newNodes.clear();
                cJSON_AddItemToObject(result, "created", newNodesArray);
            }


            if (!removedNodes.empty()) {
                cJSON * deletedNodesArray = cJSON_CreateArray();
                for (auto propId : removedNodes) {
                    cJSON_AddItemToArray(deletedNodesArray, cJSON_CreateNumber(propId));
                }
                cJSON_AddItemToObject(result, "removed", deletedNodesArray);
                removedNodes.clear();
            }

            if (!changedNodes.empty()) {
                cJSON * changedNodesArray = cJSON_CreateArray();

                for (auto prop : changedNodes) {
                    cJSON* propData = cJSON_CreateArray();
                    cJSON_AddItemToArray(propData, cJSON_CreateNumber(idForProperty(prop)));
                    cJSON_AddItemToArray(propData, JSON::valueToJson(prop));
                    cJSON_AddItemToArray(changedNodesArray, propData);
                }

                changedNodes.clear();
                cJSON_AddItemToObject(result, "changed", changedNodesArray);
            }

            SG_LOG(SG_NETWORK, SG_INFO, "making JSON data took:" << st.elapsedMSec() << " for " << newSize << "/" << changedSize << "/" << removedSize);
            recentlyRemoved.clear();

            return result;
        }

        bool haveChangesToSend() const
        {
            return !newNodes.empty() || !changedNodes.empty() || !removedNodes.empty();
        }
    private:
        PropertyId nextPropertyId = 1;
        std::unordered_map<SGPropertyNode*, PropertyId> idHash;
        std::vector<PropertyValue> previousValues;

        /// track recently removed nodes in case they are re-created imemdiately
        /// after with the same type, since we can make this much more efficient
        /// when sending over the wire.
        std::vector<RecentlyRemovedNode> recentlyRemoved;
    };

#if 0

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

  globals->get_commands()->execute(name->valuestring, arg);
}
#endif

MirrorPropertyTreeWebsocket::MirrorPropertyTreeWebsocket(const std::string& path) :
    _listener(new MirrorTreeListener),
    _minSendInterval(100)
{
    _subtreeRoot = globals->get_props()->getNode(path, true);
    _subtreeRoot->addChangeListener(_listener.get());
    _listener->registerSubtree(_subtreeRoot);
    _lastSendTime = SGTimeStamp::now();
}

MirrorPropertyTreeWebsocket::~MirrorPropertyTreeWebsocket()
{
    SG_LOG(SG_NETWORK, SG_INFO, "shutting down MirrorPropertyTreeWebsocket");
}

void MirrorPropertyTreeWebsocket::close()
{
    _subtreeRoot->removeChangeListener(_listener.get());

  #if 0
  SG_LOG(SG_NETWORK, SG_INFO, "closing PropertyChangeWebsocket #" << id);
  _watchedNodes.clear();
  #endif
}

void MirrorPropertyTreeWebsocket::handleRequest(const HTTPRequest & request, WebsocketWriter &writer)
{
  if (request.Content.empty()) return;
#if 0
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
  #endif
}

void MirrorPropertyTreeWebsocket::poll(WebsocketWriter & writer)
{
    if (!_listener->haveChangesToSend()) {
        return;
    }

    if (_lastSendTime.elapsedMSec() < _minSendInterval) {
        return;
    }

    // okay, we will send now, update the send stamp
    _lastSendTime.stamp();

    cJSON * json = _listener->makeJSONData();
    char * jsonString = cJSON_PrintUnformatted( json );
    writer.writeText( jsonString );
    free( jsonString );
    cJSON_Delete( json );
}

} // namespace http
} // namespace flightgear
