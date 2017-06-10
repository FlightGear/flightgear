// AIWakeGroup.hxx -- Group of AI wake meshes for the computation of the induced
// wake.
//
// Written by Bertrand Coconnier, started April 2017.
//
// Copyright (C) 2017  Bertrand Coconnier  - bcoconni@users.sf.net
//
// This program is free software; you can redistribute it and/or modify it under
// the terms of the GNU General Public License as published by the Free Software
// Foundation; either version 2 of the License, or (at your option) any later
// version.
//
// This program is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
// FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
// details.
//
// You should have received a copy of the GNU General Public License along with
// this program; if not, write to the Free Software Foundation, Inc., 51
// Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
//
// $Id$

#include <math.h>
#include <vector>
#include <simgear/structure/SGSharedPtr.hxx>
#include <simgear/math/SGVec3.hxx>
#include <simgear/math/SGGeod.hxx>
#include <simgear/math/SGGeoc.hxx>
#include <simgear/math/SGQuat.hxx>
#ifdef FG_TESTLIB
#include <map>
#include <simgear/props/props.hxx>
#include "tests/fakeAIAircraft.hxx"
Globals g;
Globals *globals = &g;
#else
#include "Main/globals.hxx"
#include "AIModel/performancedata.hxx"
#include "AIModel/AIAircraft.hxx"
#endif
#include "FDM/AIWake/AIWakeGroup.hxx"

AIWakeGroup::AIWakeGroup(void)
{
    SGPropertyNode* _props = globals->get_props();
    _density_slugft = _props->getNode("environment/density-slugft3", true);
}

void AIWakeGroup::AddAI(FGAIAircraft* ai)
{
    int id = ai->getID();
    PerformanceData* perfData = ai->getPerformance();

    if (_aiWakeData.find(id) == _aiWakeData.end()) {
        double span = perfData->wingSpan();
        double chord = perfData->wingChord();
        _aiWakeData[id] = AIWakeData(new WakeMesh(span, chord));

        SG_LOG(SG_FLIGHT, SG_DEV_ALERT,
               "Created mesh for " << ai->_getName() << " ID: #" << id
               // << "Type:" << ai->getAcType() << endl
               // << "Name:" << ai->_getName() << endl
               // << "Speed:" << ai->getSpeed() << endl
               // << "Vertical speed:" << ai->getVerticalSpeed() << endl
               // << "Altitude:" << ai->getAltitude() << endl
               // << "Heading:" << ai->_getHeading() << endl
               // << "Roll:" << ai->getRoll() << endl
               // << "Pitch:" << ai->getPitch() << endl
               // << "Wing span (ft):" << span << endl
               // << "Wing chord (ft):" << chord << endl
               // << "Weight (lbs):" << perfData->weight() << endl
               );
    }

    AIWakeData& data = _aiWakeData[id];
    data.visited = true;

    data.position = ai->getCartPos() * SG_METER_TO_FEET;
    SGGeoc geoc = SGGeoc::fromCart(data.position);
    SGQuatd Te2l = SGQuatd::fromLonLatRad(geoc.getLongitudeRad(),
                                          geoc.getLatitudeRad());
    data.Te2b = Te2l * SGQuatd::fromYawPitchRollDeg(ai->_getHeading(),
                                                    ai->getPitch(), 0.0);

    double hVel = ai->getSpeed()*SG_KT_TO_FPS;
    double vVel = ai->getVerticalSpeed()/60;
    double gamma = atan2(vVel, hVel);
    double vel = sqrt(hVel*hVel + vVel*vVel);
    double weight = perfData->weight();
    _aiWakeData[id].mesh->computeAoA(vel, _density_slugft->getDoubleValue(),
                                     weight*cos(gamma));
}

SGVec3d AIWakeGroup::getInducedVelocityAt(const SGVec3d& pt) const
{
    SGVec3d vi(0.,0.,0.);
    for (auto item : _aiWakeData) {
        AIWakeData& data = item.second;
        if (!data.visited) continue;

        SGVec3d at = data.Te2b.transform(pt - data.position);
        vi += data.Te2b.backTransform(data.mesh->getInducedVelocityAt(at));
    }
    return vi;
}

void AIWakeGroup::gc(void)
{
    for (auto it=_aiWakeData.begin(); it != _aiWakeData.end(); ++it) {
        if (!(*it).second.visited) {
            SG_LOG(SG_FLIGHT, SG_DEV_ALERT, "Deleted mesh for aircraft #"
                   << (*it).first);
            _aiWakeData.erase(it);
            break; // erase has invalidated the iterator, other dead meshes (if
            // any) will be erased at the next time steps.
        }
    }

    for (auto &item : _aiWakeData) item.second.visited = false;
}
