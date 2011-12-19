#include "gnnode.hxx"
#include "groundnetwork.hxx"

#include <iostream>
#include <algorithm>

#include <osg/Geode>
#include <Main/globals.hxx>
#include <Scenery/scenery.hxx>

using std::sort;

/*****************************************************************************
 * Helper function for parsing position string
 ****************************************************************************/
double processPosition(const string &pos)
{
  string prefix;
  string subs;
  string degree;
  string decimal;
  int sign = 1;
  double value;
  subs = pos;
  prefix= subs.substr(0,1);
  if (prefix == string("S") || (prefix == string("W")))
    sign = -1;
  subs    = subs.substr(1, subs.length());
  degree  = subs.substr(0, subs.find(" ",0));
  decimal = subs.substr(subs.find(" ",0), subs.length());
  
	      
  //cerr << sign << " "<< degree << " " << decimal << endl;
  value = sign * (atof(degree.c_str()) + atof(decimal.c_str())/60.0);
  //cerr << value <<endl;
  //exit(1);
  return value;
}

//bool sortByHeadingDiff(FGTaxiSegment *a, FGTaxiSegment *b) {
//  return a->hasSmallerHeadingDiff(*b);
//}

bool sortByLength(FGTaxiSegment *a, FGTaxiSegment *b) {
  return a->getLength() > b->getLength();
}

/**************************************************************************
 * FGTaxiNode
 *************************************************************************/
void FGTaxiNode::setElevation(double val)
{
    geod.setElevationM(val);
}

void FGTaxiNode::setLatitude (double val)
{
  geod.setLatitudeDeg(val);
}

void FGTaxiNode::setLongitude(double val)
{
  geod.setLongitudeDeg(val);
}

void FGTaxiNode::setLatitude (const string& val)
{
  geod.setLatitudeDeg(processPosition(val));
}

void FGTaxiNode::setLongitude(const string& val)
{
  geod.setLongitudeDeg(processPosition(val));
}
  
double FGTaxiNode::getElevationFt(double refelev)
{
    double elevF = geod.getElevationFt();
    double elevationEnd = 0;
    if ((elevF == 0) || (elevF == refelev)) {
        SGGeod center2 = geod;
        FGScenery * local_scenery = globals->get_scenery();
        center2.setElevationM(SG_MAX_ELEVATION_M);
        if (local_scenery->get_elevation_m( center2, elevationEnd, NULL )) {
            geod.setElevationM(elevationEnd);
        }
    }
    //cerr << "Returning elevation : " << geod.getElevationM() << ". Ref elev (feet) = " << refelev << endl;
    return geod.getElevationFt();
}

double FGTaxiNode::getElevationM(double refelev)
{
    //double refelevFt = refelev * SG_METER_TO_FEET;
    //double retval = getElevationFt(refelevFt);
    //cerr << "Returning elevation : " << geod.getElevationM() << ". Ref elev (meters) = " << refelev << endl;
    return geod.getElevationM();
}
