// SPDX-FileCopyrightText: (C) 2025  Florent Rougon
// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileComment: Container class for a string and its translation

#include <string>
#include <utility>
#include <vector>

#include "TranslationUnit.hxx"

namespace flightgear
{

TranslationUnit::TranslationUnit(const std::string sourceText,
                                 const std::vector<std::string> targetTexts,
                                 bool hasPlural)
    : _sourceText(std::move(sourceText)),
      _targetTexts(std::move(targetTexts)),
      _hasPlural(hasPlural)
{ }

std::string TranslationUnit::getSourceText() const
{
    return _sourceText;
}

void TranslationUnit::setSourceText(std::string text)
{
    _sourceText = std::move(text);
}

std::string TranslationUnit::getTargetText(int pluralFormIndex) const
{
    if (static_cast<std::size_t>(pluralFormIndex) >= _targetTexts.size()) {
        return {};
    }

    return _targetTexts[pluralFormIndex];
}

std::size_t TranslationUnit::getNumberOfTargetTexts() const
{
    return _targetTexts.size();
}

void TranslationUnit::setTargetText(int pluralFormIndex, std::string text)
{
    if (static_cast<std::size_t>(pluralFormIndex) >= _targetTexts.size()) {
        _targetTexts.resize(pluralFormIndex + 1);
    }

    _targetTexts[pluralFormIndex] = std::move(text);
}

void TranslationUnit::setTargetTexts(std::vector<std::string> texts)
{
    _targetTexts = std::move(texts);
}

bool TranslationUnit::getPluralStatus() const
{
    return _hasPlural;
}

void TranslationUnit::setPluralStatus(int hasPlural)
{
    _hasPlural = hasPlural;
}

} // namespace flightgear
