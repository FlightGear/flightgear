//
//  Written and (c) Torsten Dreyer - Torsten(at)t3r_dot_de
//
//  This program is free software; you can redistribute it and/or
//  modify it under the terms of the GNU General Public License as
//  published by the Free Software Foundation; either version 2 of the
//  License, or (at your option) any later version.
// 
//  This program is distributed in the hope that it will be useful, but
//  WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//  General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//
#ifndef __FGPANELPROTOCOL_HXX
#define __FGPANELPROTOCOL_HXX
#include <simgear/structure/subsystem_mgr.hxx>
#include <simgear/props/props.hxx>
#include <simgear/io/iochannel.hxx>
class PropertySetter;

typedef std::vector<PropertySetter*> PropertySetterVector;
class FGPanelProtocol : public SGSubsystem {
public:
  FGPanelProtocol( SGPropertyNode_ptr root );
  virtual ~FGPanelProtocol();
  virtual void init();
  virtual void reinit();
  virtual void update( double dt );

protected:

private:
  SGPropertyNode_ptr root;
  SGIOChannel * io;
  PropertySetterVector propertySetterVector;
};
#endif
