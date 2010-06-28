// logic.hxx - Base class for logic components
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
#ifndef __LOGICCOMPONENT_HXX
#define __LOGICCOMPONENT_HXX 1

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include "digitalcomponent.hxx"

namespace FGXMLAutopilot {

/**
 * @brief A simple logic class writing &lt;condition&gt; to a property
 */
class Logic : public DigitalComponent {
public:
    bool get_input() const;
    void set_output( bool value );
    bool get_output() const;
protected:
    void update( bool firstTime, double dt );
};

}
#endif // LOGICCOMPONENT_HXX

