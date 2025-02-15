// SPDX-FileCopyrightText: (C) 2025  Florent Rougon
// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileComment: Container class for related translation units

#include <initializer_list>
#include <string>
#include <utility>
#include <vector>

#include <simgear/debug/logstream.hxx>

#include "TranslationResource.hxx"

using std::string;
using std::vector;

namespace flightgear
{

void TranslationResource::addTranslationUnit(std::string name, int index,
                                             std::string sourceText,
                                             bool hasPlural)
{
    _map.emplace(KeyType(std::move(name), index),
                 TranslationUnit(std::move(sourceText), {}, hasPlural));
}

void TranslationResource::setTargetText_simple(
    std::string name, int index, std::string targetText)
{
    SG_LOG(SG_GENERAL, SG_DEBUG,
           "Setting target text for '" << name << ":" << index <<
           "' to '" << targetText << '\'');

    const auto key = std::make_pair(std::move(name), index);
    auto& translationUnit = _map[key];
    // XXX First plural form hardcoded
    translationUnit.setTargetText(0, std::move(targetText));
}

std::string TranslationResource::getTranslation(const std::string& name,
                                                int index,
                                                int pluralFormIndex) const
{
    std::string res;            // empty result by default

    auto it = _map.find(std::make_pair(name, index));
    if (it != _map.end()) {
        const auto transUnit = it->second;
        res = transUnit.getTargetText(pluralFormIndex);

        if (res.empty()) {
            res = transUnit.getSourceText();
        }
    }

    return res;
}

vector<string> TranslationResource::getTranslations(const string& name) const
{
    vector<string> result;
    decltype(_map)::const_iterator it;

    for (int index = 0;
         (it = _map.find(std::make_pair(name, index))) != _map.end();
         index++) {
        const auto& transUnit = it->second;
        // Plural form indices all hardcoded to 0
        const string targetText = transUnit.getTargetText(0);
        result.push_back(
            targetText.empty() ? transUnit.getSourceText() : targetText);
    }

    return result;
}

// Really useful?..
vector<string> TranslationResource::getTranslations(
    const string& name, const std::initializer_list<int> pluralFormIndices) const
{
    const int nbStrings = pluralFormIndices.size();
    auto pluralFormIndex = pluralFormIndices.begin();
    vector<string> result(nbStrings);

    for (int i = 0; i < nbStrings; i++) {
        result.push_back(getTranslation(name, i, *pluralFormIndex++));
    }

    return result;
}

int TranslationResource::getNumberOfStringsWithId(const string& name) const
{
    int index = 0;

    while (_map.find(std::make_pair(name, index)) != _map.end()) {
        index++;
    }

    return index;
}

} // namespace flightgear
