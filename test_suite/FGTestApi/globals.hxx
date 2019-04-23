#ifndef FG_TEST_GLOBALS_HELPERS_HXX
#define FG_TEST_GLOBALS_HELPERS_HXX

#include <string>
#include <simgear/structure/SGSharedPtr.hxx>

class SGGeod;

namespace flightgear
{
    class FlightPlan;
    typedef SGSharedPtr<FlightPlan> FlightPlanRef;
}

namespace FGTestApi {

namespace setUp {

void initTestGlobals(const std::string& testName);

    void populateFP(flightgear::FlightPlanRef f,
                const std::string& depICAO, const std::string& depRunway,
                const std::string& destICAO, const std::string& destRunway,
                const std::string& waypoints);
    
}  // End of namespace setUp.

void setPosition(const SGGeod& g);

void runForTime(double t);

namespace tearDown {

void shutdownTestGlobals();

}  // End of namespace tearDown.

}  // End of namespace FGTestApi.

#endif // of FG_TEST_GLOBALS_HELPERS_HXX
