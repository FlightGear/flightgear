// SPDX-FileCopyrightText: (C) 2025  Florent Rougon
// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileComment: Container class for related translation units

#pragma once

#include <initializer_list>
#include <map>
#include <string>
#include <utility>
#include <vector>

#include "TranslationUnit.hxx"

namespace flightgear
{

class TranslationResource
{
public:
    void addTranslationUnit(std::string name, int index, std::string sourceText,
                            bool hasPlural = false);
    void setFirstTargetText(std::string name, int index,
                            std::string targetText);
    void setTargetTexts(std::string name, int index,
                        std::vector<std::string> targetTexts);

    std::string getTranslation(const std::string& name, int index,
                               std::size_t pluralFormIndex) const;
    /**
     * Get translations for all strings with a given tag name.
     *
     * Iterate over all translation units that have the given name as the base
     * of their id (that is, the tag name in the default translation XML file).
     *
     * @return A vector containing the first target text (i.e., first plural
     *         form) of each translation unit.
     */
    std::vector<std::string> getTranslations(const std::string& name) const;

    /**
     * Get translations for strings that differ only by their index.
     *
     * The number of translations to fetch is the number of elements in
     * pluralFormIndices. For each i, pluralFormIndices[i] is the index of the
     * plural form to use when fetching the translation of the TranslationUnit
     * identified by the given name and index i.
     *
     * Not sure this will be very useful!
     */
    std::vector<std::string> getTranslations(
        const std::string& name,
        const std::initializer_list<std::size_t> pluralFormIndices) const;

    /**
     * Get the number of translated strings with the given tag name.
     */
     int getNumberOfStringsWithId(const std::string& name) const;

private:
    // In the default translation files, this corresponds to a node name and
    // its index.
    using KeyType = std::pair<std::string, int>;
    std::map<KeyType, TranslationUnit> _map;
};

} // namespace flightgear
