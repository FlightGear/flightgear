// NasalFlightPlan.hxx -- expose FlightPlan classes to Nasal
//
// Written by James Turner, started 2020.
//
// Copyright (C) 2020 James Turner
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 2 of the
// License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

#pragma once

#include <simgear/nasal/nasal.h>

#include <Navaids/FlightPlan.hxx>
#include <Navaids/procedure.hxx>

flightgear::Waypt*           wayptGhost(naRef r);
flightgear::FlightPlan::Leg* fpLegGhost(naRef r);
flightgear::Procedure*       procedureGhost(naRef r);

naRef ghostForWaypt(naContext c, const flightgear::Waypt* wpt);
naRef ghostForLeg(naContext c, const flightgear::FlightPlan::Leg* leg);
naRef ghostForProcedure(naContext c, const flightgear::Procedure* proc);

naRef initNasalFlightPlan(naRef globals, naContext c);
void  shutdownNasalFlightPlan();
