/*
 * FGInterpolator.hxx
 *
 *  Created on: 16.03.2013
 *      Author: tom
 */

#ifndef FG_INTERPOLATOR_HXX_
#define FG_INTERPOLATOR_HXX_

#include <simgear/props/PropertyInterpolationMgr.hxx>

class FGInterpolator:
  public simgear::PropertyInterpolationMgr
{
  public:
    FGInterpolator();
};


#endif /* FG_INTERPOLATOR_HXX_ */
