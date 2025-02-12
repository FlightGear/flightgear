/*
 * Copyright (C) 2018 Edward d'Auvergne
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


#ifndef _FG_NONINSTANCED_SUBSYSTEM_SYSTEM_TESTS_HXX
#define _FG_NONINSTANCED_SUBSYSTEM_SYSTEM_TESTS_HXX


#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/TestFixture.h>

#include <simgear/structure/subsystem_mgr.hxx>


// The system tests.
class NonInstancedSubsystemTests : public CppUnit::TestFixture
{
    // Set up the test suite.
    CPPUNIT_TEST_SUITE(NonInstancedSubsystemTests);
    CPPUNIT_TEST(testAirportDynamicsManager);
    //CPPUNIT_TEST(testAreaSampler);                // Not registered yet.
    //CPPUNIT_TEST(testAutopilot);                  // Not registered yet.
    CPPUNIT_TEST(testCanvasMgr);
    CPPUNIT_TEST(testDigitalFilter);
    CPPUNIT_TEST(testEphemeris);
    CPPUNIT_TEST(testErrorReporter);
    CPPUNIT_TEST(testFDMShell);
    //CPPUNIT_TEST(testFGACMS);                     // Not registered yet.
    //CPPUNIT_TEST(testFGADA);                      // Not registered yet.
    CPPUNIT_TEST(testFGAIManager);
    CPPUNIT_TEST(testFGAircraftModel);
    CPPUNIT_TEST(testFGATCManager);
    //CPPUNIT_TEST(testFGBalloonSim);               // Not registered yet.
    CPPUNIT_TEST(testFGCom);
    CPPUNIT_TEST(testFGControls);
    CPPUNIT_TEST(testFGDNSClient);
    //CPPUNIT_TEST(testFGElectricalSystem);         // Not registered yet.
    //CPPUNIT_TEST(testFGEnvironmentMgr);           // Segfault.
    //CPPUNIT_TEST(testFGExternalNet);              // Not registered yet.
    //CPPUNIT_TEST(testFGExternalPipe);             // Not registered yet.
    CPPUNIT_TEST(testFGFlightHistory);
    CPPUNIT_TEST(testFGHIDEventInput);
    CPPUNIT_TEST(testFGHTTPClient);
    //CPPUNIT_TEST(testFGHttpd);                    // Not registered yet.
    //CPPUNIT_TEST(testFGInput);                    // Segfault.
    CPPUNIT_TEST(testFGInstrumentMgr);
    CPPUNIT_TEST(testFGInterpolator);
    CPPUNIT_TEST(testFGIO);
    //CPPUNIT_TEST(testFGJoystickInput);            // Segfault.
    //CPPUNIT_TEST(testFGJSBsim);                   // Not registered yet.
    CPPUNIT_TEST(testFGKeyboardInput);
    //CPPUNIT_TEST(testFGLaRCsim);                  // Not registered yet.
    CPPUNIT_TEST(testFGLight);
    //CPPUNIT_TEST(testFGLinuxEventInput);          // Need to handle #define INPUTEVENT_CLASS FGLinuxEventInput.
    CPPUNIT_TEST(testFGLogger);
    //CPPUNIT_TEST(testFGMacOSXEventInput);         // Need to handle #define INPUTEVENT_CLASS FGMacOSXEventInput.
    //CPPUNIT_TEST(testFGMagicCarpet);              // Not registered yet.
    CPPUNIT_TEST(testFGMagVarManager);
    CPPUNIT_TEST(testFGModelMgr);
    CPPUNIT_TEST(testFGMouseInput);
    //CPPUNIT_TEST(testFGMultiplayMgr);             // double free or corruption (fasttop): 0x0000000006423860
    CPPUNIT_TEST(testFGNasalSys);
    //CPPUNIT_TEST(testFGNullFDM);                  // Not registered yet.
    //CPPUNIT_TEST(testFGPanel);                    // Not registered yet.
    //CPPUNIT_TEST(testFGPrecipitationMgr);         // Segfault.
    CPPUNIT_TEST(testFGProperties);
    //CPPUNIT_TEST(testFGReplay);                   // Segfault.
    CPPUNIT_TEST(testFGRidgeLift);
    //CPPUNIT_TEST(testFGRouteMgr);                 // Partially unstable (sometimes segfault or sometimes double free or corruption).
    //CPPUNIT_TEST(testFGScenery);                  // Segfault.
    CPPUNIT_TEST(testFGSoundManager);
    CPPUNIT_TEST(testFGSubmodelMgr);
    CPPUNIT_TEST(testFGSystemMgr);
    CPPUNIT_TEST(testFGTrafficManager);
    //CPPUNIT_TEST(testFGUFO);                      // Not registered yet.
    //CPPUNIT_TEST(testFGViewMgr);                  // Not registered yet.
    CPPUNIT_TEST(testFGVoiceMgr);
    //CPPUNIT_TEST(testFGXMLAutopilotGroup);        // Not registered yet.
    CPPUNIT_TEST(testFlipFlop);
    CPPUNIT_TEST(testGraphicsPresets);
    CPPUNIT_TEST(testGUIMgr);
    CPPUNIT_TEST(testHighlight);
    //CPPUNIT_TEST(testInstrumentGroup);            // Not registered yet.
    CPPUNIT_TEST(testLogic);
    //CPPUNIT_TEST(testNewGUI);                     // double free or corruption (fasttop): 0x000000000701d950
    //CPPUNIT_TEST(testNoaaMetarRealWxController);  // Not registered yet.
    CPPUNIT_TEST(testPerformanceDB);
    CPPUNIT_TEST(testPIDController);
    CPPUNIT_TEST(testPISimpleController);
    //CPPUNIT_TEST(testPitotSystem);                // Not registered yet.
    CPPUNIT_TEST(testPredictor);
    CPPUNIT_TEST(testSGEventMgr);
    //CPPUNIT_TEST(testSGPerformanceMonitor);       // Not registered yet.
    CPPUNIT_TEST(testSGSoundMgr);
    CPPUNIT_TEST(testSGTerraSync);
    //CPPUNIT_TEST(testStateMachineComponent);      // Not registered yet.
    //CPPUNIT_TEST(testStaticSystem);               // Not registered yet.
    //CPPUNIT_TEST(testTimeManager);                // Segfault.
    //CPPUNIT_TEST(testVacuumSystem);               // Not registered yet.
    //CPPUNIT_TEST(testView);                       // Not registered yet.
    //CPPUNIT_TEST(testYASim);                      // Not registered yet.
    CPPUNIT_TEST_SUITE_END();

public:
    // Set up function for each test.
    void setUp();

    // Clean up after each test.
    void tearDown();

    // The subsystem tests.
    void testAirportDynamicsManager();
    void testAreaSampler();
    void testAutopilot();
    void testCanvasMgr();
    void testDigitalFilter();
    void testEphemeris();
    void testErrorReporter();
    void testFDMShell();
    void testFGACMS();
    void testFGADA();
    void testFGAIManager();
    void testFGAircraftModel();
    void testFGATCManager();
    void testFGBalloonSim();
    void testFGCom();
    void testFGControls();
    void testFGDNSClient();
    void testFGElectricalSystem();
    void testFGEnvironmentMgr();
    void testFGExternalNet();
    void testFGExternalPipe();
    void testFGFlightHistory();
    void testFGHIDEventInput();
    void testFGHTTPClient();
    void testFGHttpd();
    void testFGInput();
    void testFGInstrumentMgr();
    void testFGInterpolator();
    void testFGIO();
    void testFGJoystickInput();
    void testFGJSBsim();
    void testFGKeyboardInput();
    void testFGLaRCsim();
    void testFGLight();
    void testFGLinuxEventInput();
    void testFGLogger();
    void testFGMacOSXEventInput();
    void testFGMagicCarpet();
    void testFGMagVarManager();
    void testFGModelMgr();
    void testFGMouseInput();
    void testFGMultiplayMgr();
    void testFGNasalSys();
    void testFGNullFDM();
    void testFGPanel();
    void testFGPrecipitationMgr();
    void testFGProperties();
    void testFGReplay();
    void testFGRidgeLift();
    void testFGRouteMgr();
    void testFGScenery();
    void testFGSoundManager();
    void testFGSubmodelMgr();
    void testFGSystemMgr();
    void testFGTrafficManager();
    void testFGUFO();
    void testFGViewMgr();
    void testFGVoiceMgr();
    void testFGXMLAutopilotGroup();
    void testFlipFlop();
    void testGraphicsPresets();
    void testGUIMgr();
    void testHighlight();
    void testInstrumentGroup();
    void testLogic();
    void testNavDisplay();
    void testNewGUI();
    void testNoaaMetarRealWxController();
    void testPerformanceDB();
    void testPIDController();
    void testPISimpleController();
    void testPitotSystem();
    void testPredictor();
    void testSGEventMgr();
    void testSGPerformanceMonitor();
    void testSGSoundMgr();
    void testSGTerraSync();
    void testStateMachineComponent();
    void testStaticSystem();
    void testTimeManager();
    void testVacuumSystem();
    void testView();
    void testYASim();

private:
    // The subsystem manager.
    std::unique_ptr<SGSubsystemMgr> _mgr;

    // Helper functions.
    void create(const char* name);
};

#endif  // _FG_NONINSTANCED_SUBSYSTEM_SYSTEM_TESTS_HXX
