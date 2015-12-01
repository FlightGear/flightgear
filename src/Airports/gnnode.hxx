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

#include <simgear/compiler.h>
#include <simgear/structure/SGSharedPtr.hxx>

#include <Navaids/positioned.hxx>

class FGTaxiNode : public FGPositioned
{
protected:
    const int m_index;

  bool isOnRunway;
  int  holdType;
  bool m_isPushback;

public:    
  FGTaxiNode(int index, const SGGeod& pos, bool aOnRunway, int aHoldType);
  virtual ~FGTaxiNode();
  
  void setElevation(double val);

  double getElevationM ();
  double getElevationFt();
  
  int getIndex() const;

  int getHoldPointType() const { return holdType; };
  bool getIsOnRunway() const { return isOnRunway; };
  bool isPushback() const { return m_isPushback; }

  void setIsPushback();
};

#endif
