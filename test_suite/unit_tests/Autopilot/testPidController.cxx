#include "testPidController.hxx"
#include "testPidControllerData.hxx"

#include "test_suite/FGTestApi/testGlobals.hxx"


#include <Autopilot/autopilot.hxx>
#include <Autopilot/pidcontroller.hxx>
#include <Main/fg_props.hxx>
#include <Main/globals.hxx>


#include <simgear/math/sg_random.hxx>
#include <simgear/props/props_io.hxx>

#include <unistd.h>

// Set up function for each test.
void PidControllerTests::setUp()
{
    FGTestApi::setUp::initTestGlobals("ap-pidcontroller");
}


// Clean up after each test.
void PidControllerTests::tearDown()
{
    FGTestApi::tearDown::shutdownTestGlobals();
}


SGPropertyNode_ptr PidControllerTests::configFromString(const std::string& s)
{
    SGPropertyNode_ptr config = new SGPropertyNode;

    std::istringstream iss(s);
    readProperties(iss, config);
    return config;
}

void PidControllerTests::test0()
{
    test(false /*startup_current*/);
}

void PidControllerTests::test1()
{
    test(true /*startup_current*/);
}

void PidControllerTests::test(bool startup_current)
{
    sg_srandom(999);

    // Define vertical-hold pid-controller (based on Harrier-GR3).
    //
    std::string config_text0 = 
        R"(<?xml version="1.0" encoding="UTF-8"?>
        <PropertyList>
          <pid-controller>
            <name>Vertical Speed Hold</name>
            <debug>false</debug>
            <startup-current></startup-current>
            <enable>true</enable>
            <input>
              <prop>/velocities/vertical-speed-fps</prop>
            </input>
            <reference>
                <prop>/autopilot/settings/vertical-speed-fpm</prop>
                <scale>0.01667</scale>
            </reference>
            <output>
              <prop>/controls/flight/elevator</prop>
            </output>
            <config>
              <Kp>-0.025</Kp> <!-- proportional gain -->
              <beta>1.0</beta>    <!-- input value weighing factor -->
              <alpha>0.1</alpha>  <!-- low pass filter weighing factor -->
              <gamma>0.0</gamma>  <!-- input value weighing factor for -->
                                  <!-- unfiltered derivative error -->
              <Ti>5</Ti>         <!-- integrator time -->
              <Td>0.01</Td>       <!-- derivator time -->

              <!-- Restrict elevator values to +/-0.35 at high speed, but allow full
              +/-1 range at low speed. -->
              <u_min>-1</u_min>
              <u_max>1</u_max>

            </config>
          </pid-controller>
        </PropertyList>
        )";
    
    // Set the <startup-current> element.
    std::string from = "<startup-current></startup-current>";
    std::string config_text = config_text0;
    config_text.replace(config_text.find(from), from.size(),
            (startup_current) ? "<startup-current>true</startup-current>" : "<startup-current>false</startup-current>"
            );
    assert(config_text != config_text0);
    std::cout << "config_text is:\n" << config_text << "\n";
    
    SGPropertyNode_ptr config = configFromString(config_text);

    auto ap = new FGXMLAutopilot::Autopilot(globals->get_props(), config);

    globals->add_subsystem("ap", ap, SGSubsystemMgr::FDM);
    ap->bind();
    ap->init();

    const std::vector<PidControllerOutput>& outputs = (startup_current) ? pidControllerOutputs1 : pidControllerOutputs0;
    assert(pidControllerInputs.size() == outputs.size());
    
    fgSetDouble("/controls/flight/elevator", 0);
    
    for (unsigned i=0; i<pidControllerInputs.size(); ++i) {
        const PidControllerInput& input = pidControllerInputs[i];
        const PidControllerOutput& output = outputs[i];
        fgSetDouble("/velocities/vertical-speed-fps", input.vspeed_fps);
        fgSetDouble("/autopilot/settings/vertical-speed-fpm", input.reference / 0.01667);
        ap->update(0.00833333 /*dt*/);
        double elevator = fgGetDouble("/controls/flight/elevator");
        CPPUNIT_ASSERT_DOUBLES_EQUAL(output.output, elevator, 0.0001);
        if (0) {
            // This generates C++ text for setting the <outputs> vector above.
            std::cout << "{ " << elevator << "},\n";
        }
    }
}
