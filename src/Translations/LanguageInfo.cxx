// SPDX-FileCopyrightText: (C) 2025  Florent Rougon
// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileComment: Information on plural forms for the supported languages

#include "LanguageInfo.hxx"

#include <cstdlib>
#include <map>
#include <string>

#include <GUI/MessageBox.hxx>

using std::map;
using std::string;

namespace flightgear {

const map<string, std::size_t> LanguageInfo::nbPluralFormsMap = {
    {"de",      2},
    {"default", 1},             // “engineering English” (default translation)
    {"en",      2},             // English (with singular and plural forms)
    {"es",      2},
    {"fr",      2},
    {"it",      2},
    {"ka",      2},
    {"nl",      2},
    {"pl",      3},
    {"pt",      2},
    {"ru",      3},
    {"tr",      1},
    {"zh_CN",   1}
};

// Static member functions
std::size_t LanguageInfo::pluralFormIndex_EngineeringEnglishStyle(uintType n)
{
    return 0;
}

std::size_t LanguageInfo::pluralFormIndex_EnglishStyle(uintType n)
{
    return (n != uintType(1));
}

std::size_t LanguageInfo::pluralFormIndex_FrenchStyle(uintType n)
{
    return (n > uintType(1));
}

std::size_t LanguageInfo::pluralFormIndex_PolishStyle(uintType n)
{
    return (n == uintType(1) ? 0 :
              n % uintType(10) >= uintType(2) &&
              n % uintType(10) <= uintType(4) &&
              (n % uintType(100) < uintType(10) ||
               n % uintType(100) >= uintType(20)) ? 1 : 2);
}

std::size_t LanguageInfo::pluralFormIndex_RussianStyle(uintType n)
{
    return (n % uintType(10) == uintType(1) &&
            n % uintType(100) != uintType(11)? 0 :
              n % uintType(10) >= uintType(2) &&
              n % uintType(10) <= uintType(4) &&
              (n % uintType(100) < uintType(10) ||
               n % uintType(100) >= uintType(20)) ? 1 : 2);
}

const map<string, LanguageInfo::funcType>
    LanguageInfo::pluralFormIndexFuncMap = {
        {"de",      pluralFormIndex_EnglishStyle},
        {"default", pluralFormIndex_EngineeringEnglishStyle},
        {"en",      pluralFormIndex_EnglishStyle},
        {"es",      pluralFormIndex_EnglishStyle},
        {"fr",      pluralFormIndex_FrenchStyle},
        {"it",      pluralFormIndex_EnglishStyle},
        {"ka",      pluralFormIndex_EnglishStyle},
        {"nl",      pluralFormIndex_EnglishStyle},
        {"pl",      pluralFormIndex_PolishStyle},
        {"pt",      pluralFormIndex_EnglishStyle},
        {"ru",      pluralFormIndex_RussianStyle},
        {"tr",      pluralFormIndex_EngineeringEnglishStyle},
        {"zh_CN",   pluralFormIndex_EngineeringEnglishStyle},
};

// Static member function
std::size_t LanguageInfo::getNumberOfPluralForms(const string& languageId)
{
    const auto it = nbPluralFormsMap.find(languageId);

    if (it == nbPluralFormsMap.end()) {
        flightgear::fatalMessageBoxThenExit(
          "Unknown language id",
          "Language id '" + languageId + "' unknown in " __FILE__ ".",
          "LanguageInfo::getNumberOfPluralForms() was called with language "
          "id '" + languageId + "', which is not a key of nbPluralFormsMap.");
    }

    return it->second;
}

// Static member function
std::size_t LanguageInfo::getPluralFormIndex(const string& languageId,
                                             intType number)
{
    const auto it = pluralFormIndexFuncMap.find(languageId);

    if (it == pluralFormIndexFuncMap.end()) {
        flightgear::fatalMessageBoxThenExit(
            "Unknown language id",
            "Language id '" + languageId + "' unknown in " __FILE__ ".",
            "LanguageInfo::getPluralFormIndex() was called with language "
            "id '" + languageId + "', which is not a key of "
            "pluralFormIndexFuncMap.");
    }

    return it->second(std::abs(number));
}

} // namespace flightgear
