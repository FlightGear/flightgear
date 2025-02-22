// SPDX-FileCopyrightText: (C) 2018  James Turner <james@flightgear.org>
// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileComment: parse an XLIFF 1.2 XML file

#pragma once

#include <cstddef>
#include <memory>
#include <stack>
#include <string>
#include <tuple>
#include <vector>

#include <simgear/props/propsfwd.hxx>
#include <simgear/xml/easyxml.hxx>

#include "TranslationDomain.hxx"
#include "TranslationResource.hxx"

namespace flightgear
{

class XLIFFParser : public XMLVisitor
{
public:
    XLIFFParser(const std::string& languageId, TranslationDomain* domain);

protected:
    void startXML () override;
    void endXML   () override;
    void startElement (const char * name, const XMLAttributes &atts) override;
    void endElement (const char * name) override;
    void data (const char * s, int len) override;
    void pi (const char * target, const char * data) override;
    void warning (const char * message, int line, int column) override;

private:
    void startTransUnitElement(const XMLAttributes &atts);
    void finishTransUnit(bool hasPlural);
    void startContextGroup(const char* resname_c);
    void endContextGroup();
    void startPluralGroup(const char* id_c);
    void endPluralGroup();

    // Return the context (i.e. “resource”), the basic id and the element index
    // (sibling elements with the same basic id differ by their indices).
    std::tuple<std::string, std::string, int>
    parseSimpleTransUnitId(const std::string& id);
    // If we are inside <group restype="x-gettext-plurals" id="food/banana:0">,
    // <trans-unit> elements inside this group are plural forms associated to
    // the same source text; expected values for their 'id' attributes are
    // 'food/banana:0[0]', 'food/banana:0[1]', etc.
    void checkIdOfPluralTransUnit(std::string id);
    void checkNumberOfPluralForms(std::size_t nbPluralFormsInTransUnit);

    const std::string _languageId; // string value of /sim/intl/locale[n]/id
    TranslationDomain* _domain;
    std::shared_ptr<TranslationResource> _currentResource;

    std::string _text;
    std::string _resource, _basicId, _pluralGroupId;
    int _index, _expectedPluralFormIndex;
    std::string _sourceText;
    std::vector<std::string> _targetTexts; // several elements = plural forms
    // Certain <file> elements must be completely skipped
    bool _skipElements = false;

    // We'll keep track of the <group> nesting state in a stack containing
    // std::unique_ptr<Group> instances.
    enum class GroupType {
        context,
        plural
    };

    class Group {
    public:
        Group(GroupType type);
        virtual ~Group() = default;

        GroupType type;

    protected:
        Group(const Group&) = default;
        Group(Group&&) = default;
        Group& operator=(const Group&) = default;
        Group& operator=(Group&&) = default;
    };

    struct ContextGroup final : Group {
        ContextGroup(const std::string& name);
        std::string name;
    };

    struct PluralGroup final : Group {
        PluralGroup(const std::string& id);
        std::string id;
    };

    std::stack<std::unique_ptr<Group>> _groupsStack;
};

} // namespace flightgear
