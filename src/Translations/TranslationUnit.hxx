// SPDX-FileCopyrightText: (C) 2025  Florent Rougon
// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileComment: Container class for a string and its translation

#pragma once

#include <string>
#include <vector>

namespace flightgear
{

class TranslationUnit
{
public:
    explicit TranslationUnit(const std::string sourceText = {},
                             const std::vector<std::string> targetTexts = {},
                             bool hasPlural = false);

    std::string getSourceText() const;
    void setSourceText(std::string text);

    std::string getTargetText(int pluralFormIndex = 0) const;
    // For sanity checks when a caller is about to use a plural form index
    std::size_t getNumberOfTargetTexts() const;
    void setTargetText(int pluralFormIndex, std::string text);
    void setTargetTexts(std::vector<std::string> texts);
    bool getPluralStatus() const;
    void setPluralStatus(int hasPlural);

private:
    std::string _sourceText;
    std::vector<std::string> _targetTexts;
    bool _hasPlural;
};

} // namespace flightgear
