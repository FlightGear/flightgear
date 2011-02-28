// realwx_ctrl.cxx -- Process real weather data
//
// Written by David Megginson, started May 2002.
// Rewritten by Torsten Dreyer, August 2010
//
// Copyright (C) 2002  David Megginson - david@megginson.com
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

#ifndef _REALWX_CTRL_HXX
#define _REALWX_CTRL_HXX

#include <simgear/structure/subsystem_mgr.hxx>
#include <simgear/props/props.hxx>
namespace Environment {
class RealWxController : public SGSubsystem
{
public:
	static RealWxController * createInstance( SGPropertyNode_ptr rootNode );
};

} // namespace
#endif // _REALWX_CTRL_HXX
