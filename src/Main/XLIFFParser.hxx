// XLIFFParser.hxx -- parse an XLIFF 1.2 XML file
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

#include <simgear/xml/easyxml.hxx>

#include <string>

#include <simgear/props/propsfwd.hxx>

namespace flightgear
{

class XLIFFParser : public XMLVisitor
{
public:
    XLIFFParser(SGPropertyNode_ptr lroot);
    
protected:
    void startXML () override;
    void endXML   () override;
    void startElement (const char * name, const XMLAttributes &atts) override;
    void endElement (const char * name) override;
    void data (const char * s, int len) override;
    void pi (const char * target, const char * data) override;
    void warning (const char * message, int line, int column) override;
    
private:
    void finishTransUnit();
    
    SGPropertyNode_ptr _localeRoot;
    SGPropertyNode_ptr _resourceNode;
    
    std::string _text;
    std::string _unitId, _resource;
    std::string _source, _target;
    bool _approved = false;
};

}
