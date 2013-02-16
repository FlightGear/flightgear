// magvarmanager.hxx -- Wraps the SimGear SGMagVar in a subsystem
//
// Copyright (C) 2012  James Turner - zakalawe@mac.com
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

#ifndef FG_MAGVAR_MANAGER
#define FG_MAGVAR_MANAGER 1

#include <memory>

#include <simgear/structure/subsystem_mgr.hxx>
#include <simgear/props/propsfwd.hxx>

// forward decls
class SGMagVar;

class FGMagVarManager : public SGSubsystem
{
public:
    FGMagVarManager();
    virtual ~FGMagVarManager();
    
    virtual void init();
    virtual void bind();
    virtual void unbind();
    
    virtual void update(double dt);
    
private:
  std::auto_ptr<SGMagVar> _magVar;
  
  SGPropertyNode_ptr _magVarNode, _magDipNode;
};

#endif // of FG_MAGVAR_MANAGER
