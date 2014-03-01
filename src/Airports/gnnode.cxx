#include "gnnode.hxx"

#include <boost/foreach.hpp>

#include "groundnetwork.hxx"

#include <Navaids/NavDataCache.hxx>
#include <Main/globals.hxx>
#include <Scenery/scenery.hxx>

using namespace flightgear;

/**************************************************************************
 * FGTaxiNode
 *************************************************************************/

FGTaxiNode::FGTaxiNode(PositionedID aGuid, const SGGeod& pos, bool aOnRunway, int aHoldType) :
  FGPositioned(aGuid, FGPositioned::PARKING, "", pos),
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

double FGTaxiNode::getElevationFt()
{
  const SGGeod& pos = geod();
  if( pos.getElevationFt() == 0.0)
  {
    SGGeod center2 = pos;
    FGScenery* local_scenery = globals->get_scenery();
    center2.setElevationM(SG_MAX_ELEVATION_M);
    double elevationEnd = -100;
    if (local_scenery->get_elevation_m( center2, elevationEnd, NULL ))
    {
      SGGeod newPos = pos;
      newPos.setElevationM(elevationEnd);
      // this will call modifyPosition to update mPosition
      NavDataCache* cache = NavDataCache::instance();
      NavDataCache::Transaction txn(cache);
      cache->updatePosition(guid(), newPos);
      txn.commit();
    }
  }
  
  return pos.getElevationFt();
}

double FGTaxiNode::getElevationM()
{
  return getElevationFt() * SG_FEET_TO_METER;
}
