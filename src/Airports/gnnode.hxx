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

SG_USING_STD(string);
SG_USING_STD(vector);

class FGTaxiSegment;
typedef vector<FGTaxiSegment*>  FGTaxiSegmentVector;
typedef FGTaxiSegmentVector::iterator FGTaxiSegmentVectorIterator;

bool sortByHeadingDiff(FGTaxiSegment *a, FGTaxiSegment *b);
bool sortByLength     (FGTaxiSegment *a, FGTaxiSegment *b);
double processPosition(const string& pos);

class FGTaxiNode 
{
private:
  double lat;
  double lon;
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
      lat (0.0),
      lon (0.0),
      index(0),
      isOnRunway(false),
      holdType(0),
      pathScore(0),
      previousNode(0),
      previousSeg(0)
{
};

  FGTaxiNode(const FGTaxiNode &other) :
      lat(other.lat),
      lon(other.lon),
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
   lat          = other.lat;
   lon          = other.lon;
   index        = other.index;
   isOnRunway   = other.isOnRunway;
   holdType     = other.holdType;
   next         = other.next;
   pathScore    = other.pathScore;
   previousNode = other.previousNode;
   previousSeg  = other.previousSeg;
   return *this;
};

  void setIndex(int idx)                  { index = idx;                 };
  void setLatitude (double val)           { lat = val;                   };
  void setLongitude(double val)           { lon = val;                   };
  void setLatitude (const string& val)    { lat = processPosition(val);  };
  void setLongitude(const string& val)    { lon = processPosition(val);  };
  void addSegment(FGTaxiSegment *segment) { next.push_back(segment);     };
  void setHoldPointType(int val)          { holdType = val;              };
  void setOnRunway(bool val)              { isOnRunway = val;            };

  void setPathScore   (double val)         { pathScore    = val; };
  void setPreviousNode(FGTaxiNode *val)    { previousNode = val; };
  void setPreviousSeg (FGTaxiSegment *val) { previousSeg  = val; };

  FGTaxiNode    *getPreviousNode()    { return previousNode; };
  FGTaxiSegment *getPreviousSegment() { return previousSeg;  };

  double getPathScore() { return pathScore; };
  double getLatitude() { return lat;};
  double getLongitude(){ return lon;};

  int getIndex() { return index; };
  int getHoldPointType() { return holdType; };
  bool getIsOnRunway() { return isOnRunway; };

  FGTaxiNode *getAddress() { return this;};
  FGTaxiSegmentVectorIterator getBeginRoute() { return next.begin(); };
  FGTaxiSegmentVectorIterator getEndRoute()   { return next.end();   }; 
  bool operator<(const FGTaxiNode &other) const { return index < other.index; };

  void sortEndSegments(bool);

};

typedef vector<FGTaxiNode*> FGTaxiNodeVector;
typedef FGTaxiNodeVector::iterator FGTaxiNodeVectorIterator;

#endif
