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

#ifndef _GN_NODE_HXX_
#define _GN_NODE_HXX_

#include <vector>
#include <string>

#include <simgear/compiler.h>
#include <simgear/math/sg_geodesy.hxx>

class FGTaxiSegment;
typedef std::vector<FGTaxiSegment*>  FGTaxiSegmentVector;
typedef FGTaxiSegmentVector::iterator FGTaxiSegmentVectorIterator;

bool sortByHeadingDiff(FGTaxiSegment *a, FGTaxiSegment *b);
bool sortByLength     (FGTaxiSegment *a, FGTaxiSegment *b);

class FGTaxiNode 
{
private:
  SGGeod geod;
  int index;

  bool isOnRunway;
  int  holdType;
  FGTaxiSegmentVector next; // a vector of pointers to all the segments leaving from this node

  // used in way finding
  double pathScore;
  FGTaxiNode* previousNode;
  FGTaxiSegment* previousSeg;


public:
  FGTaxiNode() :
      index(0),
      isOnRunway(false),
      holdType(0),
      pathScore(0),
      previousNode(0),
      previousSeg(0)
{
};

  FGTaxiNode(const FGTaxiNode &other) :
      geod(other.geod),
      index(other.index),
      isOnRunway(other.isOnRunway),
      holdType(other.holdType),
      next(other.next),
      pathScore(other.pathScore),
      previousNode(other.previousNode),
      previousSeg(other.previousSeg)
{
};

FGTaxiNode &operator =(const FGTaxiNode &other)
{
   geod                               = other.geod;
   index                              = other.index;
   isOnRunway                         = other.isOnRunway;
   holdType                           = other.holdType;
   next                               = other.next;
   pathScore                          = other.pathScore;
   previousNode                       = other.previousNode;
   previousSeg                        = other.previousSeg;
   return *this;
};

  void setIndex(int idx)                  { index = idx;                 };
  void setLatitude (double val);
  void setLongitude(double val);
  void setElevation(double val);
  void setLatitude (const std::string& val);
  void setLongitude(const std::string& val);
  void addSegment(FGTaxiSegment *segment) { next.push_back(segment);     };
  void setHoldPointType(int val)          { holdType = val;              };
  void setOnRunway(bool val)              { isOnRunway = val;            };

  void setPathScore   (double val)         { pathScore    = val; };
  void setPreviousNode(FGTaxiNode *val)    { previousNode = val; };
  void setPreviousSeg (FGTaxiSegment *val) { previousSeg  = val; };

  FGTaxiNode    *getPreviousNode()    { return previousNode; };
  FGTaxiSegment *getPreviousSegment() { return previousSeg;  };

  double getPathScore() { return pathScore; };
  double getLatitude() { return geod.getLatitudeDeg();};
  double getLongitude(){ return geod.getLongitudeDeg();};
  double getElevationM (double refelev=0);
  double getElevationFt(double refelev=0);

  const SGGeod& getGeod() const { return geod; }

  int getIndex() { return index; };
  int getHoldPointType() { return holdType; };
  bool getIsOnRunway() { return isOnRunway; };

  FGTaxiNode *getAddress() { return this;};
  FGTaxiSegmentVectorIterator getBeginRoute() { return next.begin(); };
  FGTaxiSegmentVectorIterator getEndRoute()   { return next.end();   }; 
  bool operator<(const FGTaxiNode &other) const { return index < other.index; };


};

typedef std::vector<FGTaxiNode*> FGTaxiNodeVector;
typedef FGTaxiNodeVector::iterator FGTaxiNodeVectorIterator;

#endif
