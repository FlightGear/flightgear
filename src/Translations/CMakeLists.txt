# SPDX-FileCopyrightText: (C) 2025  Florent Rougon
# SPDX-License-Identifier: GPL-2.0-or-later

include(FlightGearComponent)

set(SOURCES DefaultTranslationParser.cxx
            GetLocalizedStringsSetup.cxx
            LanguageInfo.cxx
            TranslationDomain.cxx
            TranslationResource.cxx
            TranslationUnit.cxx
            XLIFFParser.cxx
            )

set(HEADERS DefaultTranslationParser.hxx
            GetLocalizedStringsSetup.hxx
            LanguageInfo.hxx
            TranslationDomain.hxx
            TranslationResource.hxx
            TranslationUnit.hxx
            XLIFFParser.hxx
            )

flightgear_component(Translations "${SOURCES}" "${HEADERS}")
