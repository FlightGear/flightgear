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
#include <simgear/structure/SGSharedPtr.hxx>

#include <Navaids/positioned.hxx>

class FGTaxiSegment;

typedef std::vector<FGTaxiSegment*>  FGTaxiSegmentVector;
typedef FGTaxiSegmentVector::iterator FGTaxiSegmentVectorIterator;

bool sortByHeadingDiff(FGTaxiSegment *a, FGTaxiSegment *b);
bool sortByLength     (FGTaxiSegment *a, FGTaxiSegment *b);

class FGTaxiNode : public FGPositioned
{
protected:
  int index;

  bool isOnRunway;
  int  holdType;
  FGTaxiSegmentVector next; // a vector of pointers to all the segments leaving from this node

  // used in way finding - should really move to a dynamic struct
  double pathScore;
  FGTaxiNode* previousNode;
  FGTaxiSegment* previousSeg;


public:    
  FGTaxiNode(PositionedID aGuid, int index, const SGGeod& pos, bool aOnRunway, int aHoldType);
  virtual ~FGTaxiNode();
  
  void setElevation(double val);
  void addSegment(FGTaxiSegment *segment) { next.push_back(segment);     };

  void setPathScore   (double val)         { pathScore    = val; };
  void setPreviousNode(FGTaxiNode *val)    { previousNode = val; };
  void setPreviousSeg (FGTaxiSegment *val) { previousSeg  = val; };

  FGTaxiNode    *getPreviousNode()    { return previousNode; };
  FGTaxiSegment *getPreviousSegment() { return previousSeg;  };

  double getPathScore() { return pathScore; };

  double getElevationM (double refelev);
  double getElevationFt(double refelev);
  
  int getIndex() const { return index; };
  int getHoldPointType() const { return holdType; };
  bool getIsOnRunway() const { return isOnRunway; };

  const FGTaxiSegmentVector& arcs() const
  { return next; }
  
  /// find the arg which leads from this node to another.
  /// returns NULL if no such arc exists.
  FGTaxiSegment* getArcTo(FGTaxiNode* aEnd) const;
  
  bool operator<(const FGTaxiNode &other) const { return index < other.index; };


};

typedef SGSharedPtr<FGTaxiNode> FGTaxiNode_ptr;
typedef std::vector<FGTaxiNode_ptr> FGTaxiNodeVector;
typedef FGTaxiNodeVector::iterator FGTaxiNodeVectorIterator;

#endif
