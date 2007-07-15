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
  FGTaxiSegmentVector next; // a vector of pointers to all the segments leaving from this node
  
public:
  FGTaxiNode();
  FGTaxiNode(double, double, int);

  void setIndex(int idx)                  { index = idx;};
  void setLatitude (double val)           { lat = val;};
  void setLongitude(double val)           { lon = val;};
  void setLatitude (const string& val)           { lat = processPosition(val);  };
  void setLongitude(const string& val)           { lon = processPosition(val);  };
  void addSegment(FGTaxiSegment *segment) { next.push_back(segment); };
  
  double getLatitude() { return lat;};
  double getLongitude(){ return lon;};

  int getIndex() { return index; };
  FGTaxiNode *getAddress() { return this;};
  FGTaxiSegmentVectorIterator getBeginRoute() { return next.begin(); };
  FGTaxiSegmentVectorIterator getEndRoute()   { return next.end();   }; 
  bool operator<(const FGTaxiNode &other) const { return index < other.index; };

  void sortEndSegments(bool);

  // used in way finding
  double pathscore;
  FGTaxiNode* previousnode;
  FGTaxiSegment* previousseg;
  
};

typedef vector<FGTaxiNode*> FGTaxiNodeVector;
typedef FGTaxiNodeVector::iterator FGTaxiNodeVectorIterator;

#endif
