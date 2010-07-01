/**************************************************************************
 * cockpit.hxx -- cockpit defines and prototypes (initial draft)
 *
 * Written by Michele America, started September 1997.
 *
 * Copyright (C) 1997  Michele F. America  - nomimarketing@mail.telepac.pt
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 * $Id$
 **************************************************************************/
 

#ifndef _COCKPIT_HXX
#define _COCKPIT_HXX


#ifndef __cplusplus
# error This library requires C++
#endif

#include <osg/State>

bool fgCockpitInit();
void fgCockpitUpdate( osg::State* );

#endif /* _COCKPIT_HXX */
