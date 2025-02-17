// SPDX-FileCopyrightText: (C) 2018  James Turner <james@flightgear.org>
// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileComment: parse an XLIFF 1.2 XML file

#include "config.h"

#include "XLIFFParser.hxx"

#include <cstring>
#include <string>

// simgear
#include <simgear/debug/logstream.hxx>
#include <simgear/props/props.hxx>
#include <simgear/misc/strutils.hxx>

using namespace flightgear;

XLIFFParser::XLIFFParser(TranslationDomain* domain) :
    _domain(domain)
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
        if (!ac || !std::strcmp(ac, "")) {
            _approved = false;
        } else {
            _approved = simgear::strutils::to_bool(std::string{ac});
        }
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

void XLIFFParser::endElement(const char* name)
{
    std::string tag(name);
    if (tag == "source") {
        _source = _text;
    } else if (tag == "target") {
        _target = _text;
    } else if (tag == "trans-unit") {
        finishTransUnit();
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
               "no 'id' attribute: line " << getLine() << " of " <<
               getPath());
        return;
    }

    const std::string id{id_c};

    if (id.empty()) {
        SG_LOG(SG_GENERAL, SG_WARN,
               "XLIFF group with restype=\"x-gettext-plurals\" has "
               "an empty 'id' attribute: line " << getLine() << " of " <<
               getPath());
        return;
    }

    _pluralGroupId = id;
    _groupsStack.push(std::make_unique<PluralGroup>(id));
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

    _groupsStack.pop();
    _pluralGroupId.clear();
}

void XLIFFParser::finishTransUnit()
{
    if (!_currentResource) {
        SG_LOG(SG_GENERAL, SG_WARN, "XLIFF trans-unit without enclosing resource group: line "
               << getLine() << " of " << getPath());
        return;
    }

    if (_target.empty()) {
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
    _currentResource->setTargetText_simple(id, index, _target);
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
    : Group(GroupType::context), id(id_)
{ }
