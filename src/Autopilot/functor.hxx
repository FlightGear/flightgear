// functor.hxx - a utility to create object based on names
//
// Written by Torsten Dreyer
//
// Copyright (C) 2010  Torsten Dreyer - Torsten (at) t3r (dot) de
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
#ifndef __FUNCTOR_HXX
#define __FUNCTOR_HXX 1

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <simgear/props/props.hxx>

namespace FGXMLAutopilot {

template <class TBase> class FunctorBase {
public:
  virtual TBase * operator()( SGPropertyNode_ptr configNode ) = 0;
};

template <class TClass,class TBase> class CreateAndConfigureFunctor : 
  public FunctorBase<TBase> {
public:
  virtual TBase * operator()( SGPropertyNode_ptr configNode ) { 
    TBase * base = new TClass();
    base->configure( configNode );
    return base;
  }
};

}

#endif // __FUNCTOR_HXX 1
