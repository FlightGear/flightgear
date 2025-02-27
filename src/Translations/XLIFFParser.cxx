// SPDX-FileCopyrightText: (C) 2018  James Turner <james@flightgear.org>
// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileComment: parse an XLIFF 1.2 XML file

#include "config.h"

#include "XLIFFParser.hxx"

#include <cstring>
#include <regex>
#include <string>
#include <tuple>
#include <utility>

// simgear
#include <simgear/debug/logstream.hxx>
#include <simgear/props/props.hxx>
#include <simgear/misc/strutils.hxx>

#include <GUI/MessageBox.hxx>
#include "LanguageInfo.hxx"

using namespace flightgear;
namespace strutils = simgear::strutils;

XLIFFParser::XLIFFParser(const std::string& languageId,
                         TranslationDomain* domain) :
    _languageId(languageId), _domain(domain)
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

    if (_skipElements) {
        return;
    } else if (tag == "file") {
        const char* origName_c = atts.getValue("original");

        if (origName_c && !std::strcmp(origName_c, "Obsolete_PO_entries")) {
            _skipElements = true; // skip all the contents of this <file> element
        }
    } else if (tag == "trans-unit") {
        startTransUnitElement(atts);
    } else if (tag == "group") {
        const char* resType_c = atts.getValue("restype");

        if (resType_c && !std::strcmp(resType_c,
                                      "x-trolltech-linguist-context")) {
            startContextGroup(atts.getValue("resname"));
        } else if (resType_c && !std::strcmp(resType_c, "x-gettext-plurals")) {
            startPluralGroup(atts.getValue("id"));
        }
    }
}

void XLIFFParser::startTransUnitElement(const XMLAttributes &atts)
{
    const char* id_c = atts.getValue("id");
    if (!id_c) {
        flightgear::fatalMessageBoxThenExit(
            "Error while parsing a .xlf file",
            "<trans-unit> element with no 'id' attribute.",
            "Illegal <trans-unit> element with no 'id' attribute at " +
                std::to_string(getLine()) + " of " + getPath() + ".");
    }

    if (!_pluralGroupId.empty()) { // if inside a plural group
        checkIdOfPluralTransUnit(id_c);
        _expectedPluralFormIndex++;
    } else {                    // non-plural translation unit
        std::tie(_resource, _basicId, _index) = parseSimpleTransUnitId(id_c);
    }
}

void XLIFFParser::checkIdOfPluralTransUnit(std::string transUnitId)
{
    const std::string expectedId =
        _pluralGroupId + "[" + std::to_string(_expectedPluralFormIndex) + "]";

    if (transUnitId != expectedId) {
        flightgear::fatalMessageBoxThenExit(
            "Error while parsing a .xlf file",
            "Unexpected value '" + transUnitId + "' for 'id' attribute of "
            "<trans-unit> element found inside plural group with id='" +
            _pluralGroupId + "' (expected: '" + expectedId + "') at " +
            std::to_string(getLine()) + " of " + getPath() + ".");
    }
}

std::tuple<std::string, std::string, int>
XLIFFParser::parseSimpleTransUnitId(const std::string& id)
{
    std::regex simpleIdRegexp(R"(^([^/:]+)/([^/:]+):(\d+)$)");
    std::smatch results;

    if (std::regex_match(id, results, simpleIdRegexp)) {
        const int elementIdx = strutils::readNonNegativeInt<int>(results.str(3));
        return std::make_tuple(results.str(1), results.str(2), elementIdx);
    } else {
        flightgear::fatalMessageBoxThenExit(
            "Error while parsing a .xlf file",
            "Unexpected 'id' attribute value in a <trans-unit> or <group>.",
            "Unexpected syntax for a <trans-unit> or "
            "<group restype=\"x-gettext-plurals\" ...> 'id' attribute: '" +
            id + "' at " + std::to_string(getLine()) + " of " + getPath() + ".");
    }
}

void XLIFFParser::endElement(const char* name)
{
    std::string tag(name);

    if (tag == "file") {
        _skipElements = false;
    } else if (_skipElements) {
        return;
    } else if (tag == "source") {
        _sourceText = _text;
    } else if (tag == "target") {
        _targetTexts.push_back(std::move(_text));
    } else if (tag == "trans-unit") {
        if (_pluralGroupId.empty()) { // not inside a plural group
            finishTransUnit(false /* hasPlural */);
        }
    } else if (tag == "group") {
        assert(_groupsStack.size() > 0);

        switch (_groupsStack.top()->type) {
        case GroupType::context:
            endContextGroup();
            break;
        case GroupType::plural:
            endPluralGroup();
            break;
        default:
            std::abort();
        }
    }
}

void XLIFFParser::startContextGroup(const char* resname_c)
{
    if (resname_c == nullptr) {
        SG_LOG(SG_GENERAL, SG_WARN,
               "XLIFF group with restype=\"x-trolltech-linguist-context\" has "
               "no 'resname' attribute: line " << getLine() << " of " <<
               getPath());
        return;
    }

    const std::string resname{resname_c};

    if (resname.empty()) {
        SG_LOG(SG_GENERAL, SG_WARN,
               "XLIFF group with restype=\"x-trolltech-linguist-context\" has "
               "an empty 'resname' attribute: line " << getLine() << " of " <<
               getPath());
        return;
    }

    _resource = resname;
    // This is where the strings will be stored. getResourceCreate()
    // creates the TranslationResource if necessary.
    _currentResource = _domain->getResourceCreate(resname);
    _groupsStack.push(std::make_unique<ContextGroup>(resname));
}

void XLIFFParser::startPluralGroup(const char* id_c)
{
    if (id_c == nullptr) {
        SG_LOG(SG_GENERAL, SG_WARN,
               "XLIFF group with restype=\"x-gettext-plurals\" has "
               "no 'id' attribute: at line " << getLine() << " of " <<
               getPath());
        return;
    }

    // Instance member _resource was set when the context group was started.
    std::string resource;
    std::tie(resource, _basicId, _index) = parseSimpleTransUnitId(id_c);

    if (resource != _resource) {
        flightgear::fatalMessageBoxThenExit(
            "Error while parsing a .xlf file",
            "Unexpected 'id' attribute value in a <group "
            "restype=\"x-gettext-plurals\" ...> element.",
            "Attribute 'id' of a <group restype=\"x-gettext-plurals\" ...> "
            "element inside plural group '" + _pluralGroupId +
            "' specifies resource '" + resource + "' whereas "
            "the current context group declares resname='" + _resource + "' "
            "(attribute id='" + std::string(id_c) + "' at line " +
            std::to_string(getLine()) + " of " + getPath() + ").");
    }

    _pluralGroupId = id_c;
    _expectedPluralFormIndex = 0; // next <trans-unit> is for plural form 0
    _groupsStack.push(std::make_unique<PluralGroup>(id_c));
}

void XLIFFParser::endContextGroup()
{
    assert(dynamic_cast<ContextGroup*>(_groupsStack.top().get())->name
           == _resource);

    _groupsStack.pop();
    _resource.clear();
    _currentResource.reset();
}

void XLIFFParser::endPluralGroup()
{
    assert(dynamic_cast<PluralGroup*>(_groupsStack.top().get())->id
           == _pluralGroupId);

    finishTransUnit(true /* hasPlural */);

    _groupsStack.pop();
    _pluralGroupId.clear();
}

void XLIFFParser::finishTransUnit(bool hasPlural)
{
    if (!_currentResource) {
        SG_LOG(SG_GENERAL, SG_WARN,
               "XLIFF trans-unit without enclosing resource <group>: at line "
               << getLine() << " of " << getPath());
    } else if (hasPlural) {
        checkNumberOfPluralForms(_targetTexts.size());
        _currentResource->setTargetTexts(std::move(_basicId), _index,
                                         std::move(_targetTexts));
    } else {
        assert(_targetTexts.size() > 0);

        _currentResource->setFirstTargetText(std::move(_basicId), _index,
                                             std::move(_targetTexts[0]));
    }

    _sourceText.clear();
    _targetTexts.clear();
}

void XLIFFParser::checkNumberOfPluralForms(std::size_t nbPluralFormsInTransUnit)
{
    const std::size_t nbPluralFormsInCode =
        LanguageInfo::getNumberOfPluralForms(_languageId);

    if (nbPluralFormsInTransUnit != nbPluralFormsInCode) {
        flightgear::fatalMessageBoxThenExit(
            "Error while parsing a .xlf file",
            "Mismatch between the number of plural forms found in a "
            "group with restype=\"x-gettext-plurals\" and the number of "
            "plural forms declared in LanguageInfo.cxx for language '" +
            _languageId + "'.",
            "Found a plural group with " +
            std::to_string(nbPluralFormsInTransUnit) + " plural forms, however "
            "the number of plural forms for this language as set in "
            "LanguageInfo.cxx is " + std::to_string(nbPluralFormsInCode) +
            " (at " + std::to_string(getLine()) + " of " + getPath() + ").");
    }
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

// For the elements of the <group> stack
XLIFFParser::Group::Group(GroupType type_) : type(type_)
{ }

XLIFFParser::ContextGroup::ContextGroup(const std::string& name_)
    : Group(GroupType::context), name(name_)
{ }

XLIFFParser::PluralGroup::PluralGroup(const std::string& id_)
    : Group(GroupType::plural), id(id_)
{ }
