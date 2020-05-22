// NasalPositioned.hxx -- expose FGPositioned classes to Nasal
//
// Written by James Turner, started 2012.
//
// Copyright (C) 2012 James Turner
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

#ifndef SCRIPTING_NASAL_POSITIONED_HXX
#define SCRIPTING_NASAL_POSITIONED_HXX

#include <simgear/nasal/nasal.h>

#include <Navaids/positioned.hxx>


// forward decls
class SGGeod;
class FGRunway;
class FGAirport;

bool geodFromHash(naRef ref, SGGeod& result); 

FGAirport* airportGhost(naRef r);
FGRunway* runwayGhost(naRef r);

naRef ghostForPositioned(naContext c, FGPositionedRef pos);
naRef ghostForRunway(naContext c, const FGRunway* r);
naRef ghostForAirport(naContext c, const FGAirport* apt);

FGPositioned* positionedGhost(naRef r);
FGPositionedRef positionedFromArg(naRef ref);
int geodFromArgs(naRef* args, int offset, int argc, SGGeod& result);

naRef initNasalPositioned(naRef globals, naContext c);
naRef initNasalPositioned_cppbind(naRef globals, naContext c);
void postinitNasalPositioned(naRef globals, naContext c);
void shutdownNasalPositioned();

#endif // of SCRIPTING_NASAL_POSITIONED_HXX
