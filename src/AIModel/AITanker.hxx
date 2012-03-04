// AITanker.hxx  Based on David Culp's AIModel code
// - Tanker specific code isolated from AI Aircraft code
// by Thomas Foerster, started June 2007
//
// 
// Original code written by David Culp, started October 2003.
// - davidculp2@comcast.net/
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

#ifndef FGAITANKER_HXX
#define FGAITANKER_HXX

#include "AIAircraft.hxx"

/**
 * An AI tanker for air-air refueling.
 *
 * This class is just a refactoring of the AA refueling related code in FGAIAircraft. The idea
 * is to have a clean generic AIAircraft class without any special functionality. In your
 * scenario specification use 'tanker' as the scenario type to use this class.
 * 
 * @author Thomas F�ster <t.foerster@biologie.hu-berlin.de>
*/

class FGAITanker : public FGAIAircraft {
public:
    FGAITanker(FGAISchedule* ref = 0);
    ~FGAITanker();

    virtual void readFromScenario(SGPropertyNode* scFileNode);
    virtual void bind();

    virtual const char* getTypeString(void) const { return "tanker"; }

    void setTACANChannelID(const string& id);
    
private:
    string TACAN_channel_id;     // The TACAN channel of this tanker
    bool contact;                // set if this tanker is within fuelling range

    virtual void Run(double dt);
    virtual void update (double dt);
};

#endif
