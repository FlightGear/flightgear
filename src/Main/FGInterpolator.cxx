/*
 * FGInterpolator.cxx
 *
 *  Created on: 16.03.2013
 *      Author: tom
 */

#include "FGInterpolator.hxx"
#include <simgear/scene/util/ColorInterpolator.hxx>

//------------------------------------------------------------------------------
FGInterpolator::FGInterpolator()
{
  addInterpolatorFactory<simgear::ColorInterpolator>("color");
}
