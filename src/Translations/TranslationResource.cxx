// SPDX-FileCopyrightText: (C) 2025  Florent Rougon
// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileComment: Container class for related translation units

#include <algorithm>
#include <cassert>
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

void TranslationResource::setFirstTargetText(
    std::string name, int index, std::string targetText)
{
    SG_LOG(SG_GENERAL, SG_DEBUG,
           "Setting target text for '" << name << ":" << index <<
           "' to '" << targetText << '\'');

    const auto key = std::make_pair(std::move(name), index);
    auto& translationUnit = _map[key];
    // Set the first plural form
    translationUnit.setTargetText(0, std::move(targetText));
}

void TranslationResource::setTargetTexts(
    std::string name, int index, std::vector<std::string> targetTexts)
{
    SG_LOG(SG_GENERAL, SG_DEBUG,
           "Setting target texts for '" << name << ":" << index << ":\n\n");
    std::for_each(targetTexts.begin(), targetTexts.end(),
                  [](const std::string& t) {
                      SG_LOG(SG_GENERAL, SG_DEBUG, "\t" << t); });

    const auto key = std::make_pair(std::move(name), index);
    auto& translationUnit = _map[key];
    translationUnit.setTargetTexts(std::move(targetTexts));
}

std::string TranslationResource::getTranslation(
    const std::string& name, int index, std::size_t pluralFormIndex) const
{
    std::string res;            // empty result by default

    auto it = _map.find(std::make_pair(name, index));
    if (it != _map.end()) {
        const auto transUnit = it->second;
        const std::size_t nbTargetTexts = transUnit.getNumberOfTargetTexts();

        if (nbTargetTexts == 0) { // e.g., in the default translation
            res = transUnit.getSourceText();
        } else if (pluralFormIndex > 0 && !transUnit.getPluralStatus()) {
            SG_LOG(SG_GENERAL, SG_WARN,
                   "Requested plural form " << pluralFormIndex << " of the "
                   "translation of " << name << "[" << index << "] (source "
                   "text = “" << transUnit.getSourceText() << "”), "
                   "however this string wasn't declared with "
                   "has-plural=\"true\" in the default translation");
            res = transUnit.getSourceText();
        } else {
            assert(pluralFormIndex < nbTargetTexts);
            res = transUnit.getTargetText(pluralFormIndex);

            if (res.empty()) {
                res = transUnit.getSourceText();
            }
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
    const string& name,
    const std::initializer_list<std::size_t> pluralFormIndices) const
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
