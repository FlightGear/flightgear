// flipflop.hxx - implementation of multiple flip flop types
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
#ifndef __FLIPFLOPCOMPONENT_HXX
#define __FLIPFLOPCOMPONENT_HXX 1

#include "logic.hxx"

namespace FGXMLAutopilot {

class FlipFlopImplementation : public SGReferenced {
protected:
    virtual bool configure( const std::string & nodeName, SGPropertyNode_ptr configNode ) { return false; }
public:
  virtual bool getState( double dt, DigitalComponent::InputMap input, bool & q ) { return false; }
  bool configure( SGPropertyNode_ptr configNode );
};

/**
 * @brief A simple flipflop implementation
 */
class FlipFlop : public Logic {
public:
protected:
    /**
     * @brief Over-rideable hook method to allow derived classes to refine top-level
     * node parsing. 
     * @param aName
     * @param aNode
     * @return true if the node was handled, false otherwise.
     */
    virtual bool configure( const std::string & nodeName, SGPropertyNode_ptr configNode );
    void update( bool firstTime, double dt );

private:
    SGSharedPtr<FlipFlopImplementation> _implementation;

};

}
#endif // FLIPFLOPCOMPONENT_HXX
