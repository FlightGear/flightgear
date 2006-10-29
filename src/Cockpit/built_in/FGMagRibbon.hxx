//  FGMagRibbon.hxx - Built-in layer for the magnetic compass ribbon layer.
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
//  $Id$

#ifndef __FG_MAG_RIBBON_HXX
#define __FG_MAG_RIBBON_HXX

#include <Main/fg_props.hxx>
#include "../panel.hxx"

////////////////////////////////////////////////////////////////////////
// Built-in layer for the magnetic compass ribbon layer.
////////////////////////////////////////////////////////////////////////

class FGMagRibbon : public FGTexturedLayer
{
public:
    FGMagRibbon (int w, int h);
    virtual ~FGMagRibbon () {}

    virtual void draw (osg::State& state);

private:
    SGPropertyNode_ptr _magcompass_node;
};

#endif // __FG_MAG_RIBBON_HXX

// end of FGMagRibbon.hxx
