// Emesary notifications for AI system. 
//
// Richard Harrison; April 2020.
//
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

#ifndef _FG_AINOTIFICATIONS_HXX
#define _FG_AINOTIFICATIONS_HXX

#include <simgear/emesary/Emesary.hxx>
#include <simgear/math/SGMath.hxx>


class NearestCarrierToNotification : public simgear::Emesary::INotification
{
public:
    NearestCarrierToNotification(SGGeod _comparisonPosition) : position(),
                                                               comparisonPosition(_comparisonPosition),
                                                               distanceMeters(std::numeric_limits<double>::max()),
                                                               carrier(0),
                                                               deckheight(0),
                                                               heading(0),
                                                               vckts(0)
    {
    }

    /*virtual ~NearestCarrierToNotification()
    {
        if (position)
            delete position;
        position = 0;
    }*/
    const char*      GetType() override { return "NearestCarrierToNotification"; } 

    const SGGeod*             GetPosition() const { return position; }
    double                   GetHeading() const { return heading; }
    double                   GetVckts() const      { return vckts; }
    double                   GetDeckheight() const { return deckheight; }
    const class FGAICarrier* GetCarrier() const    { return carrier; }
    double                   GetDistanceMeters() const { return distanceMeters; }
    std::string              GetCarrierIdent() const { return carrierIdent; }
    double                   GetDistanceToMeters(const SGGeod& pos) const
    {
        if (carrier)
            return SGGeodesy::distanceM(comparisonPosition, pos);
        else
            return std::numeric_limits<double>::max()-1;
    }

    void SetPosition(SGGeod* _position) { position = _position; }

    void SetHeading(double _heading)          { heading = _heading; }
    void SetVckts(double _vckts)              { vckts = _vckts; }
    void SetDeckheight(double _deckheight)    { deckheight = _deckheight; }

    void SetCarrier(FGAICarrier* _carrier, SGGeod *_position)
    {
        carrier        = _carrier;
        distanceMeters = SGGeodesy::distanceM(comparisonPosition, *_position);
        position       = _position;
    }
    void SetDistanceMeters(double _distanceMeters)    { distanceMeters = _distanceMeters;   }
    void SetCarrierIdent(std::string _carrierIdent) { carrierIdent = _carrierIdent; }

    SGPropertyNode_ptr GetViewPositionLatNode() { return viewPositionLatDegNode; }
    SGPropertyNode_ptr GetViewPositionLonNode() { return viewPositionLonDegNode; }
    SGPropertyNode_ptr GetViewPositionAltNode() { return viewPositionAltFtNode; }

    void SetViewPositionLatNode(SGPropertyNode_ptr _viewPositionLatNode) { viewPositionLatDegNode = _viewPositionLatNode; }
    void SetViewPositionLonNode(SGPropertyNode_ptr _viewPositionLonNode) { viewPositionLonDegNode = _viewPositionLonNode; }
    void SetViewPositionAltNode(SGPropertyNode_ptr _viewPositionAltNode) { viewPositionAltFtNode = _viewPositionAltNode; }

    
protected:
    SGGeod             *position;
    SGGeod             comparisonPosition;

    SGPropertyNode_ptr viewPositionLatDegNode;
    SGPropertyNode_ptr viewPositionLonDegNode;
    SGPropertyNode_ptr viewPositionAltFtNode;

    double             heading;
    double             vckts;
    double             deckheight;
    double             distanceMeters;
    std::string        carrierIdent;
    class FGAICarrier* carrier;
};

#endif