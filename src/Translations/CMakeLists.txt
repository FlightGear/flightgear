# SPDX-FileCopyrightText: (C) 2025  Florent Rougon
# SPDX-License-Identifier: GPL-2.0-or-later

include(FlightGearComponent)

set(SOURCES XLIFFParser.cxx
            )

set(HEADERS XLIFFParser.hxx
            )

flightgear_component(Translations "${SOURCES}" "${HEADERS}")
