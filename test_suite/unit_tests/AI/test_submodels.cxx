/*
 * Copyright (C) 2021 Colin Geniet
 *
 * This file is part of the program FlightGear.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "test_submodels.hxx"

#include "test_suite/FGTestApi/NavDataCache.hxx"
#include "test_suite/FGTestApi/TestPilot.hxx"
#include "test_suite/FGTestApi/testGlobals.hxx"

#include <AIModel/AIAircraft.hxx>
#include <AIModel/AIManager.hxx>
#include <AIModel/submodel.hxx>

#include <Airports/airport.hxx>
#include <Main/fg_props.hxx>
#include <Main/globals.hxx>

#include <cmath>

using std::string;

/////////////////////////////////////////////////////////////////////////////

// Set up function for each test.
void SubmodelsTests::setUp()
{
    FGTestApi::setUp::initTestGlobals("Submodels");
    FGTestApi::setUp::initNavDataCache();

    globals->append_aircraft_path(SGPath::fromUtf8(FG_TEST_SUITE_DATA) / "Aircraft");

    auto props = globals->get_props();
    props->setBoolValue("sim/ai/enabled", true);
    props->setStringValue("sim/submodels/path", "Aircraft/Test/submodels.xml");

    globals->add_new_subsystem<FGAIManager>();
    globals->add_new_subsystem<FGSubmodelMgr>();

    globals->get_subsystem_mgr()->bind();
    globals->get_subsystem_mgr()->init();
    globals->get_subsystem_mgr()->postinit();
}

// Clean up after each test.
void SubmodelsTests::tearDown()
{
    FGTestApi::tearDown::shutdownTestGlobals();
}

void SubmodelsTests::testLoadXML()
{
    auto props = globals->get_props();
    auto sm_node = props->getNode("ai/submodels");
    CPPUNIT_ASSERT(sm_node->hasChild("submodel", 0));
    sm_node = sm_node->getChild("submodel", 0);
    CPPUNIT_ASSERT_EQUAL(string{"testLoadXML"}, static_cast<string>(sm_node->getStringValue("name")));
    CPPUNIT_ASSERT_EQUAL(0, sm_node->getIntValue("id"));
    CPPUNIT_ASSERT_EQUAL(42, sm_node->getIntValue("count"));
    CPPUNIT_ASSERT(sm_node->getBoolValue("serviceable"));
}

FGAIBase* SubmodelsTests::findAIModel(std::string &name) {
    auto ai_list = globals->get_subsystem<FGAIManager>()->get_ai_list();
    auto filter = [name](FGAIBase *model) { return model->_getName() == name; };
    return *std::find_if(ai_list.begin(), ai_list.end(), filter);
}

int SubmodelsTests::countAIModels(std::string &name) {
    auto ai_list = globals->get_subsystem<FGAIManager>()->get_ai_list();
    auto filter = [name](FGAIBase *model) { return model->_getName() == name; };
    return static_cast<int>(std::count_if(ai_list.begin(), ai_list.end(), filter));
}

void SubmodelsTests::testRelease()
{
    auto props = globals->get_props();
    auto sm_node = props->getNode("ai/submodels/submodel[1]");
    std::string name = sm_node->getStringValue("name");

    // Setup reasonable flight conditions.
    auto bikf = FGAirport::findByIdent("BIKF");
    auto pilot = SGSharedPtr<FGTestApi::TestPilot>(new FGTestApi::TestPilot);
    FGTestApi::setPosition(bikf->geod());
    pilot->resetAtPosition(bikf->geod());
    pilot->setSpeedKts(0);
    pilot->setCourseTrue(0.0);
    pilot->setTargetAltitudeFtMSL(0);

    // Sanity check
    CPPUNIT_ASSERT_EQUAL(0, countAIModels(name));

    // Don't release anything
    FGTestApi::runForTime(10.0);
    CPPUNIT_ASSERT_EQUAL(0, countAIModels(name));

    // Release submodel
    sm_node->setBoolValue("trigger", true);
    FGTestApi::runForTime(1);
    sm_node->setBoolValue("trigger", false);
    FGTestApi::runForTime(1);
    CPPUNIT_ASSERT_EQUAL(1, countAIModels(name));

    // Check validity
    auto sm = findAIModel(name);
    CPPUNIT_ASSERT(sm->isValid());
    CPPUNIT_ASSERT(!sm->getDie());

    // Second time
    sm_node->setBoolValue("trigger", true);
    FGTestApi::runForTime(5);
    sm_node->setBoolValue("trigger", false);
    FGTestApi::runForTime(1);
    CPPUNIT_ASSERT_EQUAL(2, countAIModels(name));

    // Let submodels expire
    FGTestApi::runForTime(20);
    CPPUNIT_ASSERT_EQUAL(0, countAIModels(name));

    // Switch to repeat release
    sm_node->setBoolValue("repeat", true);
    sm_node->setBoolValue("trigger", true);
    FGTestApi::runForTime(4.2);     // release interval is 1s
    sm_node->setBoolValue("trigger", false);
    CPPUNIT_ASSERT_EQUAL(4, countAIModels(name));

    // Let submodels expire
    FGTestApi::runForTime(20);
    CPPUNIT_ASSERT_EQUAL(0, countAIModels(name));

    // In repeat mode, release timer persists when trigger is false.
    // Currently it is at ~0.2s, so this must not release anything.
    sm_node->setBoolValue("trigger", true);
    FGTestApi::runForTime(0.5);
    sm_node->setBoolValue("trigger", false);
    CPPUNIT_ASSERT_EQUAL(0, countAIModels(name));

    // Set limited count
    sm_node->setIntValue("count", 3);
    sm_node->setBoolValue("trigger", true);
    FGTestApi::runForTime(1);
    CPPUNIT_ASSERT_EQUAL(2, sm_node->getIntValue("count"));
    CPPUNIT_ASSERT_EQUAL(1, countAIModels(name));
    FGTestApi::runForTime(5);
    CPPUNIT_ASSERT_EQUAL(0, sm_node->getIntValue("count"));
    CPPUNIT_ASSERT_EQUAL(3, countAIModels(name));
}

void SubmodelsTests::testInitialState()
{
    auto props = globals->get_props();
    auto sm_node = props->getNode("ai/submodels/submodel[2]");
    std::string name = sm_node->getStringValue("name");

    // Submodel parameters
    double x_offset = 10, y_offset = 6, z_offset = 1, yaw_offset = 30, pitch_offset = 50, speed = 100;

    // Setup reasonable flight conditions.
    auto bikf = FGAirport::findByIdent("BIKF");
    auto pilot = SGSharedPtr<FGTestApi::TestPilot>(new FGTestApi::TestPilot);
    FGTestApi::setPosition(bikf->geod());
    pilot->resetAtPosition(bikf->geod());
    pilot->setSpeedKts(0);
    pilot->setCourseTrue(90);
    pilot->setTargetAltitudeFtMSL(0);
    props->setDoubleValue("/orientation/pitch-deg", 0);
    props->setDoubleValue("/orientation/roll-deg", 0);
    FGTestApi::runForTime(1);

    sm_node->setBoolValue("trigger", true);
    // Run update loop with dt=0 to capture initial state.
    globals->get_subsystem<FGSubmodelMgr>()->update(0);
    globals->get_subsystem<FGAIManager>()->update(0);
    sm_node->setBoolValue("trigger", false);

    CPPUNIT_ASSERT_EQUAL(1, countAIModels(name));
    auto sm = findAIModel(name);
    auto sm_pos = sm->getCartPos();

    auto ac_pos = globals->get_aircraft_position_cart(); //ac->getCartPosAt(SGVec3d(0, 0, 0));
    double heading, pitch, roll;
    globals->get_aircraft_orientation(heading, pitch, roll);

    CPPUNIT_ASSERT_DOUBLES_EQUAL(90, heading, 0.1);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(0, pitch, 0.1);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(0, roll, 0.1);

    // Submodel release point
    // Submodels offsets are in x-back,y-right,z-up frame.
    // Computation is in x-forward,y-irght,z-down frame.
    SGVec3d offset(-x_offset, y_offset, -z_offset);
    SGQuatd local_frame_rotation = SGQuatd::fromLonLat(globals->get_aircraft_position());
    local_frame_rotation *= SGQuatd::fromYawPitchRollDeg(heading, pitch, roll);
    offset = local_frame_rotation.backTransform(offset);
    auto release_pos = ac_pos + offset;

    CPPUNIT_ASSERT_DOUBLES_EQUAL(release_pos.x(), sm_pos.x(), 0.01);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(release_pos.y(), sm_pos.y(), 0.01);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(release_pos.z(), sm_pos.z(), 0.01);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(remainder(heading + yaw_offset, 360), remainder(sm->getTrueHeadingDeg(), 360), 0.1);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(pitch + pitch_offset, sm->_getPitch(), 0.1);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(speed, sm->_getSpeed() * SG_KT_TO_FPS, 0.1);

    // Let submodels expire
    FGTestApi::runForTime(20);
    CPPUNIT_ASSERT_EQUAL(0, countAIModels(name));

    // Second test for velocities
    double speed_east = 100, wind_north = 20;
    pilot->setSpeedKts(100 * SG_FPS_TO_KT);
    props->setDoubleValue("/orientation/pitch-deg", 0);
    props->setDoubleValue("/orientation/roll-deg", 0);
    props->setDoubleValue("/velocities/speed-north-fps", 0);
    props->setDoubleValue("/velocities/speed-east-fps", speed_east);
    props->setDoubleValue("/velocities/speed-down-fps", 0);
    props->setDoubleValue("/environment/wind-from-north-fps", wind_north);
    props->setDoubleValue("/environment/wind-from-east-fps", 0);
    FGTestApi::runForTime(1);

    sm_node->setBoolValue("trigger", true);
    // Run update loop with dt=0 to capture initial state.
    globals->get_subsystem<FGSubmodelMgr>()->update(0);
    globals->get_subsystem<FGAIManager>()->update(0);
    sm_node->setBoolValue("trigger", false);

    CPPUNIT_ASSERT_EQUAL(1, countAIModels(name));
    sm = findAIModel(name);

    // Check initial speeds.
    // _get_speed_*_fps gives airspeed for submodels, which initially must be the vector
    // parent_ground_velocity + opposed_wind_velocity + submodel_launch_velocity
    CPPUNIT_ASSERT_DOUBLES_EQUAL(cos((heading + yaw_offset) * SG_DEGREES_TO_RADIANS)
                                 * cos((pitch + pitch_offset) * SG_DEGREES_TO_RADIANS)
                                 * speed
                                 + wind_north,
                                 sm->_get_speed_north_fps(), 0.1);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(sin((heading + yaw_offset) * SG_DEGREES_TO_RADIANS)
                                 * cos((pitch + pitch_offset) * SG_DEGREES_TO_RADIANS)
                                 * speed
                                 + speed_east,
                                 sm->_get_speed_east_fps(), 0.1);
    // In AIBase, attribute 'vs' is in fpm, but AIBallistic uses it as fps?!
    CPPUNIT_ASSERT_DOUBLES_EQUAL(sin((pitch + pitch_offset) * SG_DEGREES_TO_RADIANS) * speed,
                                 sm->_getVS_fps() * 60, 0.1);
}
