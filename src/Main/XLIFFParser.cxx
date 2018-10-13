// XLIFFParser.cxx -- parse an XLIFF 1.2 XML file
////
// Copyright (C) 2018  James Turner <james@flightgear.org>
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

#include "XLIFFParser.hxx"

#include <cstring>
#include <string>

// simgear
#include <simgear/debug/logstream.hxx>
#include <simgear/props/props.hxx>
#include <simgear/misc/strutils.hxx>

using namespace flightgear;

XLIFFParser::XLIFFParser(SGPropertyNode_ptr lroot) :
    _localeRoot(lroot)
{
    
}

void XLIFFParser::startXML()
{
    
}

void XLIFFParser::endXML()
{
    
}

void XLIFFParser::startElement(const char *name, const XMLAttributes &atts)
{
    _text.clear();
    std::string tag(name);
    if (tag == "trans-unit") {
        _unitId = atts.getValue("id");
        if (_unitId.empty()) {
            SG_LOG(SG_GENERAL, SG_WARN, "XLIFF trans-unit with missing ID: line "
                   << getLine() << " of " << getPath());
        }

        _source.clear();
        _target.clear();
        const char* ac = atts.getValue("approved");
        if (!ac || !strcmp(ac, "")) {
            _approved = false;
        } else {
            _approved = simgear::strutils::to_bool(std::string{ac});
        }
    } else if (tag == "group") {
        _resource = atts.getValue("resname");
        if (_resource.empty()) {
            SG_LOG(SG_GENERAL, SG_WARN, "XLIFF group with missing resname: line "
                   << getLine() << " of " << getPath());
        } else {
            _resourceNode = _localeRoot->getChild(_resource, 0, true /* create */);
        }
    }
}

void XLIFFParser::endElement(const char* name)
{
    std::string tag(name);
    if (tag == "source") {
        _source = _text;
    } else if (tag == "target") {
        _target = _text;
    } else if (tag == "trans-unit") {
        finishTransUnit();
    }
    
}

void XLIFFParser::finishTransUnit()
{
    if (!_resourceNode) {
        SG_LOG(SG_GENERAL, SG_WARN, "XLIFF trans-unit without enclosing resource group: line "
               << getLine() << " of " << getPath());
        return;
    }

    if (!_approved || _target.empty()) {
        // skip un-approved or missing translations
        return;
    }
    
    const auto slashPos = _unitId.find('/');
    const auto indexPos = _unitId.find(':');

    if (slashPos == std::string::npos)  {
        SG_LOG(SG_GENERAL, SG_WARN, "XLIFF trans-unit id without resource: '" <<
               _unitId << "' at line " << getLine() << " of " << getPath());
        return;
    }
    
    const auto res = _unitId.substr(0, slashPos);
    if (res != _resource) {
        // this implies the <group> node resname doesn't match the
        // id resource prefix. For now just warn and skip, we could decide
        // that one or the other takes precedence here?
        SG_LOG(SG_GENERAL, SG_WARN, "XLIFF trans-unit with inconsistent resource: line "
               << getLine() << " of " << getPath());
        return;
    }
    
    const auto id = _unitId.substr(slashPos + 1, indexPos - (slashPos + 1));
    const int index = std::stoi(_unitId.substr(indexPos+1));
    _resourceNode->getNode(id, index, true)->setStringValue(_target);
}

void XLIFFParser::data (const char * s, int len)
{
    _text += std::string(s, static_cast<size_t>(len));
}


void XLIFFParser::pi (const char * target, const char * data)
{
    SG_UNUSED(target);
    SG_UNUSED(data);
    //cout << "Processing instruction " << target << ' ' << data << endl;
}

void XLIFFParser::warning (const char * message, int line, int column) {
    SG_LOG(SG_GENERAL, SG_WARN, "Warning: " << message << " (" << line << ',' << column << ')');
}
