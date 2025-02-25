// SPDX-FileCopyrightText: (C) 2025  Florent Rougon
// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileComment: Parse a FlightGear default translation file (e.g., menu.xml)

#include "DefaultTranslationParser.hxx"

#include <cassert>
#include <string>
#include <utility>

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

[[noreturn]] void DefaultTranslationParser::parseError(const std::string& message)
{
    const sg_location location(getPath(), getLine(), getColumn());
    throw sg_io_exception(message, location, SG_ORIGIN, false);
}

void DefaultTranslationParser::startElement(const char* name,
                                            const XMLAttributes& attrs)
{
    bool allowedElement;
    const char* hasPluralStr = attrs.getValue("has-plural");

    switch (_state) {
    case State::LOOKING_FOR_RESOURCE_ELEMENT:
        if (!std::strcmp(name, "resource")) {
            _state = State::LOOKING_FOR_META_ELEMENT;
        } else {
            parseError("Expected the root element to be 'resource', "
                       "but found '" + string(name) + "' instead");
        }
        break;
    case State::LOOKING_FOR_META_ELEMENT:
        if (!std::strcmp(name, "meta")) {
            _state = State::READING_META_ELEMENT;
        } else {
            parseError("Expected a 'meta' element here, but found '"
                       + string(name) + "' instead");
        }
        break;
    case State::READING_META_ELEMENT:
        allowedElement = false;

        for (const string eltName : {"file-type", "format-version", "description",
                "language-description"}) {
            if (name == eltName) {
                startElementInsideMeta(eltName);
                allowedElement = true;
                break;
            }
        }

        if (!allowedElement) {
            parseError("Unexpected element '" + string(name) +
                       "' inside 'meta' element");
        }
        break;
    case State::LOOKING_FOR_STRINGS_ELEMENT:
        if (!std::strcmp(name, "strings")) {
            _state = State::READING_STRINGS_ELEMENT;
        } else {
            parseError("Expected a 'strings' element after 'meta', but found '"
                       + string(name) + "'");
        }
        break;
    case State::READING_STRINGS_ELEMENT:
        _stringTagName = name;
        _hasPlural = hasPluralStr && asBoolean(hasPluralStr);
        _text.clear(); // we'll gather the element's contents
        _state = State::READING_TRANSLATABLE_STRING;
        break;
    case State::READING_FILE_TYPE_ELEMENT:
        parseError("Unexpected element '" + string(name) +
                   "' inside <file-type>");
    case State::READING_FORMAT_VERSION_ELEMENT:
        parseError("Unexpected element '" + string(name) +
                   "' inside <format-version>");
    case State::READING_TRANSLATABLE_STRING:
        parseError("Unexpected element '" + string(name) +
                   "' inside translatable string '" + _stringTagName + "'");
    case State::AFTER_STRINGS_ELEMENT:
        parseError("Unexpected element '" + string(name) +
                   "' after the 'strings' element");
    }
}

void DefaultTranslationParser::startElementInsideMeta(const std::string& name)
{
    if (name == "file-type") {
        if (_foundFileType) {
            parseError("Only one 'file-type' element is allowed inside 'meta'.");
        }

        _foundFileType = true;
        _state = State::READING_FILE_TYPE_ELEMENT;
    } else if (name == "format-version") {
        if (_foundFormatVersion) {
            parseError("Only one 'format-version' element is allowed inside "
                       "'meta'.");
        }

        _foundFormatVersion = true;
        _state = State::READING_FORMAT_VERSION_ELEMENT;
    } // We ignore other legal elements here for now.

    _text.clear();              // we'll gather the element's contents
}

void DefaultTranslationParser::endElement(const char* name)
{
    const string expectedFileType = "FlightGear default translation file";
    const string expectedFormatVersion = "1";

    switch (_state) {
    case State::LOOKING_FOR_RESOURCE_ELEMENT:
        assert(false);
        break;
    case State::LOOKING_FOR_META_ELEMENT:
        parseError("Expected a 'meta' element as the first child of 'resource'");
    case State::READING_FILE_TYPE_ELEMENT:
        _fileType = std::move(_text);

        if (_fileType != expectedFileType) {
            parseError("Expected body of 'file-type' element to be '" +
                       expectedFileType + "', not '" + _fileType + "'");
        }

        _state = State::READING_META_ELEMENT;
        break;
    case State::READING_FORMAT_VERSION_ELEMENT:
        _formatVersion = std::move(_text);

        if (_formatVersion != expectedFormatVersion) {
            parseError("Expected body of 'format-version' element to be '" +
                       expectedFormatVersion + "', not '" + _formatVersion +
                       "'");
        }

        _state = State::READING_META_ELEMENT;
        break;
    case State::READING_META_ELEMENT:
        if (!std::strcmp(name, "meta")) {
            checkIfFormatIsSupported(); // 'meta' element now finished, go!
            _state = State::LOOKING_FOR_STRINGS_ELEMENT;
        } // else it should be an end tag for other supported <meta> children,
          // namely 'description' or 'language-description' for now.
        break;
    case State::LOOKING_FOR_STRINGS_ELEMENT:
        // No <strings> element: the file has no translatable strings, no big
        // deal
        break;
    case State::READING_TRANSLATABLE_STRING:
        _resource->addTranslationUnit(std::move(_stringTagName),
                                      _nextIndex[_stringTagName]++,
                                      std::move(_text), _hasPlural);
        _state = State::READING_STRINGS_ELEMENT;
        break;
    case State::READING_STRINGS_ELEMENT:
        _state = State::AFTER_STRINGS_ELEMENT;
        break;
    case State::AFTER_STRINGS_ELEMENT:
        assert(!std::strcmp(name, "resource"));
    }
}

void DefaultTranslationParser::data(const char * s, int len)
{
    _text += string(s, len);
}

void DefaultTranslationParser::warning(const char* message, int line,
                                       int column)
{
    SG_LOG(SG_GENERAL, SG_WARN, "Warning: " << message << " (line " << line
           << ", column " << column << ')');
}

void DefaultTranslationParser::checkIfFormatIsSupported()
{
    if (!_foundFileType) {
        parseError("'file-type' element is required inside the 'meta' element");
    } else if (!_foundFormatVersion) {
        parseError("'format-version' element is required inside the 'meta' "
                   "element");
    }

    // We've already checked the values of _fileType and _formatVersion as
    // soon as we found them (otherwise, in case of a problem, the line and
    // column info would be inaccurate). So, we're all good!

    return;
}

} // namespace flightgear
