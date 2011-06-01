// gravity.hxx -- interface for earth gravitational model
//
// Written by Torsten Dreyer, June 2011
//
// Copyright (C) 2011  Torsten Dreyer - torsten (at) t3r _dot_ de
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

#ifndef __GRAVITY_HXX
#define __GRAVITY_HXX

#include <simgear/math/SGMath.hxx>

namespace Environment {

class Gravity 
{
public:
    virtual ~Gravity();
    virtual double getGravity( const SGGeod & position ) const = 0;

    const static Gravity * instance();

private:
    static Gravity * _instance;
    
};

} // namespace
#endif // __GRAVITY_HXX
