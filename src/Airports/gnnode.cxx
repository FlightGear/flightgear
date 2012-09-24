#include "gnnode.hxx"

#include <boost/foreach.hpp>

#include "groundnetwork.hxx"

#include <Main/globals.hxx>
#include <Scenery/scenery.hxx>

/**************************************************************************
 * FGTaxiNode
 *************************************************************************/

FGTaxiNode::FGTaxiNode(PositionedID aGuid, int index, const SGGeod& pos, bool aOnRunway, int aHoldType) :
  FGPositioned(aGuid, FGPositioned::PARKING, "", pos),
  index(index),
  isOnRunway(aOnRunway),
  holdType(aHoldType)
{
  
}

FGTaxiNode::~FGTaxiNode()
{
}

void FGTaxiNode::setElevation(double val)
{
  // ignored for the moment
}

double FGTaxiNode::getElevationFt(double refelev)
{
    double elevF = elevation();
#if 0
    double elevationEnd = 0;
    if ((elevF == 0) || (elevF == refelev)) {
        SGGeod center2 = mPosition;
        FGScenery * local_scenery = globals->get_scenery();
        center2.setElevationM(SG_MAX_ELEVATION_M);
        if (local_scenery->get_elevation_m( center2, elevationEnd, NULL )) {
            geod.setElevationM(elevationEnd);
        }
    }
#endif
  return mPosition.getElevationFt();
}

double FGTaxiNode::getElevationM(double refelev)
{
    return geod().getElevationM();
}

FGTaxiSegment* FGTaxiNode::getArcTo(FGTaxiNode* aEnd) const
{
  BOOST_FOREACH(FGTaxiSegment* arc, next) {
    if (arc->getEnd() == aEnd) {
      return arc;
    }
  }
  
  return NULL;
}
