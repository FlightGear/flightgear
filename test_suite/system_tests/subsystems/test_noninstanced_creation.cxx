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

#include "test_suite/FGTestApi/testGlobals.hxx"

#include "test_noninstanced_creation.hxx"


// Set up function for each test.
void NonInstancedSubsystemTests::setUp()
{
    // First set up the globals object.
    FGTestApi::setUp::initTestGlobals("NonInstancedSubsystemCreationTests");

    // The subsystem manager.
    _mgr.reset(new SGSubsystemMgr());
}


// Clean up after each test.
void NonInstancedSubsystemTests::tearDown()
{
    // Remove the manager.
    _mgr.reset();

    // Clean up globals.
    FGTestApi::tearDown::shutdownTestGlobals();
}


// Helper functions.
void NonInstancedSubsystemTests::create(const char* name)
{
    // Create the subsystem.
    _mgr->add(name);
    auto sub = _mgr->get_subsystem(name);

    // Check the subsystem.
    CPPUNIT_ASSERT(sub);
    CPPUNIT_ASSERT_EQUAL(sub->subsystemId(), std::string(name));
    CPPUNIT_ASSERT_EQUAL(sub->subsystemClassId(), std::string(name));
    CPPUNIT_ASSERT_EQUAL(sub->subsystemInstanceId(), std::string());
}


// The non-instanced subsystems.
void NonInstancedSubsystemTests::testAirportDynamicsManager()     { create("airport-dynamics"); }
void NonInstancedSubsystemTests::testAreaSampler()                { create("area"); }
void NonInstancedSubsystemTests::testAutopilot()                  { create("autopilot"); }
void NonInstancedSubsystemTests::testCanvasMgr()                  { create("Canvas"); }
void NonInstancedSubsystemTests::testDigitalFilter()              { create("filter"); }
void NonInstancedSubsystemTests::testEphemeris()                  { create("ephemeris"); }
void NonInstancedSubsystemTests::testErrorReporter()              { create("error-reporting"); }
void NonInstancedSubsystemTests::testFDMShell()                   { create("flight"); }
void NonInstancedSubsystemTests::testFGACMS()                     { create("acms"); }
void NonInstancedSubsystemTests::testFGADA()                      { create("ada"); }
void NonInstancedSubsystemTests::testFGAIManager()                { create("ai-model"); }
void NonInstancedSubsystemTests::testFGAircraftModel()            { create("aircraft-model"); }
void NonInstancedSubsystemTests::testFGATCManager()               { create("ATC"); }
void NonInstancedSubsystemTests::testFGBalloonSim()               { create("balloon"); }
void NonInstancedSubsystemTests::testFGCom()                      { create("fgcom"); }
void NonInstancedSubsystemTests::testFGControls()                 { create("controls"); }
void NonInstancedSubsystemTests::testFGDNSClient()                { create("dns"); }
void NonInstancedSubsystemTests::testFGElectricalSystem()         { create("electrical"); }
void NonInstancedSubsystemTests::testFGEnvironmentMgr()           { create("environment"); }
void NonInstancedSubsystemTests::testFGExternalNet()              { create("network"); }
void NonInstancedSubsystemTests::testFGExternalPipe()             { create("pipe"); }
void NonInstancedSubsystemTests::testFGFlightHistory()            { create("history"); }
void NonInstancedSubsystemTests::testFGHIDEventInput()            { create("input-event-hid"); }
void NonInstancedSubsystemTests::testFGHTTPClient()               { create("http"); }
void NonInstancedSubsystemTests::testFGHttpd()                    { create("httpd"); }
void NonInstancedSubsystemTests::testFGInput()                    { create("input"); }
void NonInstancedSubsystemTests::testFGInstrumentMgr()            { create("instrumentation"); }
void NonInstancedSubsystemTests::testFGInterpolator()             { create("prop-interpolator"); }
void NonInstancedSubsystemTests::testFGIO()                       { create("io"); }
void NonInstancedSubsystemTests::testFGJoystickInput()            { create("input-joystick"); }
void NonInstancedSubsystemTests::testFGJSBsim()                   { create("jsb"); }
void NonInstancedSubsystemTests::testFGKeyboardInput()            { create("input-keyboard"); }
void NonInstancedSubsystemTests::testFGLight()                    { create("lighting"); }
void NonInstancedSubsystemTests::testFGLinuxEventInput()          { create("input-event"); }
void NonInstancedSubsystemTests::testFGLogger()                   { create("logger"); }
void NonInstancedSubsystemTests::testFGMacOSXEventInput()         { create("input-event"); }
void NonInstancedSubsystemTests::testFGMagicCarpet()              { create("magic"); }
void NonInstancedSubsystemTests::testFGMagVarManager()            { create("magvar"); }
void NonInstancedSubsystemTests::testFGModelMgr()                 { create("model-manager"); }
void NonInstancedSubsystemTests::testFGMouseInput()               { create("input-mouse"); }
void NonInstancedSubsystemTests::testFGMultiplayMgr()             { create("mp"); }
void NonInstancedSubsystemTests::testFGNasalSys()                 { create("nasal"); }
void NonInstancedSubsystemTests::testFGNullFDM()                  { create("null"); }
void NonInstancedSubsystemTests::testFGPanel()                    { create("panel"); }
void NonInstancedSubsystemTests::testFGPrecipitationMgr()         { create("precipitation"); }
void NonInstancedSubsystemTests::testFGProperties()               { create("properties"); }
void NonInstancedSubsystemTests::testFGReplay()                   { create("replay"); }
void NonInstancedSubsystemTests::testFGRidgeLift()                { create("ridgelift"); }
void NonInstancedSubsystemTests::testFGRouteMgr()                 { create("route-manager"); }
void NonInstancedSubsystemTests::testFGScenery()                  { create("scenery"); }
void NonInstancedSubsystemTests::testFGSoundManager()             { create("sound"); }
void NonInstancedSubsystemTests::testFGSubmodelMgr()              { create("submodel-mgr"); }
void NonInstancedSubsystemTests::testFGSystemMgr()                { create("systems"); }
void NonInstancedSubsystemTests::testFGTrafficManager()           { create("traffic-manager"); }
void NonInstancedSubsystemTests::testFGUFO()                      { create("ufo"); }
void NonInstancedSubsystemTests::testFGViewMgr()                  { create("view-manager"); }
void NonInstancedSubsystemTests::testFGVoiceMgr()                 { create("voice"); }
void NonInstancedSubsystemTests::testFGXMLAutopilotGroup()        { create("xml-rules"); }
void NonInstancedSubsystemTests::testFlipFlop()                   { create("flipflop"); }
void NonInstancedSubsystemTests::testGraphicsPresets()            { create("graphics-presets"); }
void NonInstancedSubsystemTests::testGUIMgr()                     { create("CanvasGUI"); }
void NonInstancedSubsystemTests::testHighlight()                  { create("reflect"); }
void NonInstancedSubsystemTests::testInstrumentGroup()            { create("instruments"); }
void NonInstancedSubsystemTests::testLogic()                      { create("logic"); }
void NonInstancedSubsystemTests::testNewGUI()                     { create("gui"); }
void NonInstancedSubsystemTests::testNoaaMetarRealWxController()  { create("noaa-metar"); }
void NonInstancedSubsystemTests::testPerformanceDB()              { create("aircraft-performance-db"); }
void NonInstancedSubsystemTests::testPIDController()              { create("pid-controller"); }
void NonInstancedSubsystemTests::testPISimpleController()         { create("pi-simple-controller"); }
void NonInstancedSubsystemTests::testPitotSystem()                { create("pitot"); }
void NonInstancedSubsystemTests::testPredictor()                  { create("predict-simple"); }
void NonInstancedSubsystemTests::testSGEventMgr()                 { create("events"); }
void NonInstancedSubsystemTests::testSGPerformanceMonitor()       { create("performance-mon"); }
void NonInstancedSubsystemTests::testSGSoundMgr()                 { create("sound"); }
void NonInstancedSubsystemTests::testSGTerraSync()                { create("terrasync"); }
void NonInstancedSubsystemTests::testStateMachineComponent()      { create("state-machine"); }
void NonInstancedSubsystemTests::testStaticSystem()               { create("static"); }
void NonInstancedSubsystemTests::testTimeManager()                { create("time"); }
void NonInstancedSubsystemTests::testVacuumSystem()               { create("vacuum"); }
void NonInstancedSubsystemTests::testView()                       { create("view"); }
void NonInstancedSubsystemTests::testYASim()                      { create("yasim"); }
