// SPDX-FileCopyrightText: (C) 2025  Florent Rougon
// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileComment: Parse a FlightGear default translation file (e.g., menu.xml)

#include "DefaultTranslationParser.hxx"

#include <cassert>
#include <string>

#include <simgear/structure/exception.hxx>

using std::string;

namespace flightgear
{

DefaultTranslationParser::DefaultTranslationParser(TranslationResource* resource)
    : _resource(resource)
{ }

void DefaultTranslationParser::startXML()
{ }

void DefaultTranslationParser::endXML()
{ }

bool DefaultTranslationParser::asBoolean(const string& str)
{
    if (str == "true") {
        return true;
    } else if (str == "false") {
        return false;
    }

    const string message = ("invalid boolean value '" + str +
                            "' (expected 'true' or 'false')");
    const sg_location location(getPath(), getLine(), getColumn());
    throw sg_io_exception(message, location, SG_ORIGIN, false);
}

void DefaultTranslationParser::startElement(const char* name,
                                            const XMLAttributes& attrs)
{
    _nestingLevel++;

    if (_nestingLevel == 1) {   // we are processing the root element
        return;
    } else if (_nestingLevel > 2) {
        const string message = "nesting elements is not supported";
        const sg_location location(getPath(), getLine(), getColumn());
        throw sg_io_exception(message, location, SG_ORIGIN, false);
    }

    _name = name;
    const char* hasPluralStr = attrs.getValue("has-plural");
    _hasPlural = hasPluralStr && asBoolean(hasPluralStr);
    _sourceText.clear();
}

void DefaultTranslationParser::endElement(const char* name)
{
    if (_nestingLevel == 1) {
        return;
    }

    assert (_nestingLevel == 2);
    assert(name == _name);

    _resource->addTranslationUnit(std::move(_name), _nextIndex[_name]++,
                                  std::move(_sourceText), _hasPlural);
    _nestingLevel--;
}

void DefaultTranslationParser::data(const char * s, int len)
{
    _sourceText += string(s, len);
}

void DefaultTranslationParser::warning(const char* message, int line,
                                       int column)
{
    SG_LOG(SG_GENERAL, SG_WARN, "Warning: " << message << " (line " << line
           << ", column " << column << ')');
}

} // namespace flightgear
