// SPDX-FileCopyrightText: (C) 2025  Florent Rougon
// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileComment: Information on plural forms for the supported languages

#pragma once

#include <functional>
#include <map>
#include <string>
#include <type_traits>

namespace flightgear
{

class LanguageInfo
{
public:
    using intType = long long;

    static std::size_t getNumberOfPluralForms(const std::string& languageId);
    static std::size_t getPluralFormIndex(const std::string& languageId,
                                          intType number);

private:
    // Important: std::abs(i) is not UB and fits in an uintType as long as i
    // has type intType.
    using uintType = std::make_unsigned_t<intType>;

    static const std::map<std::string, std::size_t> nbPluralFormsMap;

    using funcType = std::function<std::size_t(intType)>;
    /// Map from language ids to functions that return the index of the plural
    /// form to use for a given cardinal number.
    static const std::map<std::string, funcType> pluralFormIndexFuncMap;

    static std::size_t pluralFormIndex_EngineeringEnglishStyle(uintType n);
    static std::size_t pluralFormIndex_EnglishStyle(uintType n);
    static std::size_t pluralFormIndex_FrenchStyle(uintType n);
    static std::size_t pluralFormIndex_PolishStyle(uintType n);
    static std::size_t pluralFormIndex_RussianStyle(uintType n);
};

}
