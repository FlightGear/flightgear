//  panel_io.cxx - I/O for 2D panel.
//
//  Written by David Megginson, started January 2000.
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
//  $Id: panel_io.hxx,v 1.6 2006/04/17 13:03:43 mfranz Exp $

#ifndef __PANEL_IO_HXX
#define __PANEL_IO_HXX

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#ifdef HAVE_WINDOWS_H
#  include <windows.h>
#endif

#include "panel.hxx"

class FGReadablePanel : public FGPanel {
public:
  static SGSharedPtr<FGPanel> read(SGPropertyNode_ptr root);
};


#endif // __PANEL_IO_HXX
