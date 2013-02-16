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

/**
 * @brief Interface for a flip flop implementation. Can be configured from a property node and
 * returns a state depending on input lines.
 */
class FlipFlopImplementation : public SGReferenced {
protected:
 /**
  * @brief configure this component from a property node. Iterates through all nodes found
  *        as childs under configNode and calls configure of the derived class for each child.
  * @param configNode the property node containing the configuration 
  */
  virtual bool configure( const std::string & nodeName, SGPropertyNode_ptr configNode ) { return false; }
public:
  virtual ~FlipFlopImplementation() {}
  /**
   * @brief evaluates the output state from the input lines
   * @param dt the elapsed time in seconds from since the last call
   * @param input a map of named input lines
   * @param q a reference to a boolean variable to receive the output state
   * @return true if the state has changed, false otherwise
   */
  virtual bool getState( double dt, DigitalComponent::InputMap input, bool & q ) { return false; }

 /**
  * @brief configure this component from a property node. Iterates through all nodes found
  *        as childs under configNode and calls configure of the derived class for each child.
  * @param configNode the property node containing the configuration 
  */
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

   /** 
    * @brief Implementation of the pure virtual function of the Component class. Gets called from
    * the update method if it's not disabled with the firstTime parameter set to true if this
    * is the first call after being enabled 
    * @param firstTime set to true if this is the first update call since this component has
             been enabled. Set to false for every subsequent call.
    * @param dt  the elapsed time since the last call
    */
    void update( bool firstTime, double dt );

private:
   /** 
    * @brief Pointer to the actual flip flop implementation
    */
    SGSharedPtr<FlipFlopImplementation> _implementation;

};

}
#endif // FLIPFLOPCOMPONENT_HXX
