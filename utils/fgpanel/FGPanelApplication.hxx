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
#ifndef __FGPANELAPPLICATION_HXX
#define __FGPANELAPPLICATION_HXX

#include "FGGLApplication.hxx"
#include "FGPanelProtocol.hxx"

#include <simgear/structure/subsystem_mgr.hxx>
#include <simgear/props/props.hxx>

#include <string>

#include "panel.hxx"

class FGPanelApplication : public FGGLApplication {
public:
  FGPanelApplication( int argc, char ** argv );
  ~FGPanelApplication();

  void Run();

protected:
  virtual void Key( unsigned char key, int x, int y );
  virtual void Idle();
//  virtual void Display();
  virtual void Reshape( int width, int height );

  virtual void Init();

  double Sleep();

  SGSharedPtr<FGPanel> panel;
  SGSharedPtr<FGPanelProtocol> protocol;

  int width;
  int height;
};

#endif
