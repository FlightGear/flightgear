// terrainsampler.hxx -- 
//
// Written by Torsten Dreyer, started July 2010
// Based on local weather implementation in nasal from 
// Thorsten Renk
//
// Copyright (C) 2010  Curtis Olson
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
//
#ifndef _TERRAIN_SAMPLER_HXX
#define _TERRAIN_SAMPLER_HXX

#include <simgear/structure/subsystem_mgr.hxx>

namespace Environment {
class TerrainSampler : public SGSubsystemGroup
{
public:
	static TerrainSampler * createInstance( SGPropertyNode_ptr rootNode );
};

} // namespace
#endif
