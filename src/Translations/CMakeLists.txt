# SPDX-FileCopyrightText: (C) 2025  Florent Rougon
# SPDX-License-Identifier: GPL-2.0-or-later

include(FlightGearComponent)

set(SOURCES DefaultTranslationParser.cxx
            TranslationDomain.cxx
            TranslationResource.cxx
            TranslationUnit.cxx
            XLIFFParser.cxx
            )

set(HEADERS DefaultTranslationParser.hxx
            TranslationDomain.hxx
            TranslationResource.hxx
            TranslationUnit.hxx
            XLIFFParser.hxx
            )

flightgear_component(Translations "${SOURCES}" "${HEADERS}")
