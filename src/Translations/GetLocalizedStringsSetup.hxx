// SPDX-FileCopyrightText: (C) 2025  Florent Rougon
// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileComment: Class for retrieving translated strings

#pragma once

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

#include "LanguageInfo.hxx"
#include "TranslationDomain.hxx"

namespace flightgear
{

class GetLocalizedStringsSetup
{
public:
    // I did this to avoid making both LanguageInfo and
    // GetLocalizedStringsSetup class templates...
    using intType = LanguageInfo::intType;

    GetLocalizedStringsSetup& setDomain(std::string domain);
    GetLocalizedStringsSetup& setIndex(int index);
    GetLocalizedStringsSetup& setCardinalNumber(intType number);

    std::string getString(const std::string& resource,
                          const std::string& basicId);
    std::string getStringWithDefault(const std::string& resource,
                                     const std::string& basicId,
                                     const std::string& defaultValue);
    std::vector<std::string> getStrings(const std::string& resource,
                                        const std::string& basicId);
    std::size_t getStringCount(const std::string& resource,
                               const std::string& basicId);

private:
    TranslationDomain::ResourceRef getResource(const std::string& resourceName)
        const;

    std::string _domain = "core";
    int _elementIndex = 0; ///< for sibling elements with the same name
    /// Determines which plural form will be used
    std::optional<intType> _cardinalNumber; // has no value initially
};

} // namespace flightgear
