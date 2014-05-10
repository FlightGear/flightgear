// Expose aircraft related data to Nasal
//
// Copyright (C) 2014 Thomas Geymayer
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

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include "NasalAircraft.hxx"
#include <Aircraft/FlightHistory.hxx>
#include <Main/globals.hxx>

#include <simgear/nasal/cppbind/NasalHash.hxx>
#include <simgear/nasal/cppbind/Ghost.hxx>

//------------------------------------------------------------------------------
static naRef f_getHistory(const nasal::CallContext& ctx)
{
  FGFlightHistory* history =
    static_cast<FGFlightHistory*>(globals->get_subsystem("history"));
  if( !history )
    naRuntimeError(ctx.c, "Failed to get 'history' subsystem");

  return ctx.to_nasal(history);
}

//------------------------------------------------------------------------------
void initNasalAircraft(naRef globals, naContext c)
{
  nasal::Ghost<SGSharedPtr<FGFlightHistory> >::init("FGFlightHistory")
    .method("pathForHistory", &FGFlightHistory::pathForHistory);

  nasal::Hash aircraft_module = nasal::Hash(globals, c).createHash("aircraft");
  aircraft_module.set("history", &f_getHistory);
}
