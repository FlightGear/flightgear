// SPDX-FileCopyrightText: (C) 2018  James Turner <james@flightgear.org>
// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileComment: parse an XLIFF 1.2 XML file

#pragma once

#include <memory>
#include <string>

#include <simgear/props/propsfwd.hxx>
#include <simgear/xml/easyxml.hxx>

#include "TranslationDomain.hxx"
#include "TranslationResource.hxx"

namespace flightgear
{

class XLIFFParser : public XMLVisitor
{
public:
    XLIFFParser(TranslationDomain* domain);

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

    TranslationDomain* _domain;
    std::shared_ptr<TranslationResource> _currentResource;

    std::string _text;
    std::string _unitId, _resource;
    std::string _source, _target;
    bool _approved = false;
};

} // namespace flightgear
