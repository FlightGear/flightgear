// AITanker.cxx  Based on David Culp's AIModel code
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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include "AITanker.hxx"

FGAITanker::FGAITanker(FGAISchedule* ref): FGAIAircraft(ref){
}

FGAITanker::~FGAITanker() {}

void FGAITanker::readFromScenario(SGPropertyNode* scFileNode) {
    if (!scFileNode)
        return;

    FGAIAircraft::readFromScenario(scFileNode);
    setTACANChannelID(scFileNode->getStringValue("TACAN-channel-ID",""));
    setName(scFileNode->getStringValue("name", "Tanker"));

}

void FGAITanker::bind() {
    FGAIAircraft::bind();

    tie("refuel/contact", SGRawValuePointer<bool>(&contact));
    tie("position/altitude-agl-ft",SGRawValuePointer<double>(&altitude_agl_ft));

    props->setStringValue("navaids/tacan/channel-ID", TACAN_channel_id.c_str());
    props->setStringValue("name", _name.c_str());
    props->setBoolValue("tanker", true);
}

void FGAITanker::setTACANChannelID(const string& id) {
    TACAN_channel_id = id;
}

void FGAITanker::Run(double dt) {
    //FGAIAircraft::Run(dt);

    double start = pos.getElevationFt() + 1000;
    altitude_agl_ft = _getAltitudeAGL(pos, start);

    //###########################//
    // do calculations for radar //
    //###########################//
    double range_ft2 = UpdateRadar(manager);

    // check if radar contact
    if ( (range_ft2 < 250.0 * 250.0) && (y_shift > 0.0)
              && (elevation > 0.0) ) {
        //refuel_node->setBoolValue(true);
        contact = true;
    } else {
        //refuel_node->setBoolValue(false);
        contact = false;
    }
}


void FGAITanker::update(double dt) {
     FGAIAircraft::update(dt);
     Run(dt);
     Transform();
}
