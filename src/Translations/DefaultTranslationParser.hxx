// SPDX-FileCopyrightText: (C) 2025  Florent Rougon
// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileComment: Parse a FlightGear default translation file (e.g., menu.xml)

#pragma once

#include <string>

#include <simgear/xml/easyxml.hxx>

#include "TranslationResource.hxx"

namespace flightgear
{

class DefaultTranslationParser : public XMLVisitor
{
public:
    DefaultTranslationParser(TranslationResource* resource);

protected:
    void startXML () override;
    void endXML   () override;
    void startElement (const char * name, const XMLAttributes &atts) override;
    void endElement (const char * name) override;
    void data (const char * s, int len) override;
    void warning (const char * message, int line, int column) override;

private:
    bool asBoolean(const std::string& str);

    TranslationResource* _resource; // points to container for the transl. units
    std::string _name;
    std::string _sourceText;
    bool _hasPlural = false;
    // Number of elements found with a given name, at any time. Values in this
    // map indicate the index to assign to the next element whose name is the
    // key.
    std::map<std::string, int> _nextIndex;
    int _nestingLevel = 0;
};

} // namespace flightgear
