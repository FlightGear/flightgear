// RemoteXMLRequest.hxx -- xml http requests
//
// Written by James Turner
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

#ifndef REMOTEXMLREQUEST_HXXH
#define REMOTEXMLREQUEST_HXXH

#include <simgear/structure/exception.hxx>
#include <simgear/io/HTTPMemoryRequest.hxx>
#include <simgear/props/props_io.hxx>

class RemoteXMLRequest:
  public simgear::HTTP::MemoryRequest
{
public:
    SGPropertyNode_ptr _complete;
    SGPropertyNode_ptr _status;
    SGPropertyNode_ptr _failed;
    SGPropertyNode_ptr _target;

    RemoteXMLRequest(const std::string& url, SGPropertyNode* targetNode) :
      simgear::HTTP::MemoryRequest(url),
      _target(targetNode)
    {
    }
    
    void setCompletionProp(SGPropertyNode_ptr p)
    {
        _complete = p;
    }
    
    void setStatusProp(SGPropertyNode_ptr p)
    {
        _status = p;
    }
    
    void setFailedProp(SGPropertyNode_ptr p)
    {
        _failed = p;
    }
    
protected:
    
    virtual void onFail()
    {
        SG_LOG(SG_IO, SG_INFO, "network level failure in RemoteXMLRequest");
        if (_failed) {
            _failed->setBoolValue(true);
        }
    }
    
    virtual void onDone()
    {
        int response = responseCode();
        bool failed = false;
        if (response == 200) {
            try {
                const char* buffer = responseBody().c_str();
                readProperties(buffer, responseBody().size(), _target, true);
            } catch (const sg_exception &e) {
                SG_LOG(SG_IO, SG_WARN, "parsing XML from remote, failed: " << e.getFormattedMessage());
                failed = true;
                response = 406; // 'not acceptable', anything better?
            }
        } else {
            failed = true;
        }
    // now the response data is output, signal Nasal / listeners
        if (_complete) _complete->setBoolValue(true);
        if (_status) _status->setIntValue(response);
        if (_failed && failed) _failed->setBoolValue(true);
    }
};


#endif //REMOTEXMLREQUEST_HXXH
