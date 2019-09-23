#ifndef FG_TEST_GLOBALS_HELPERS_HXX
#define FG_TEST_GLOBALS_HELPERS_HXX

#include <string>
#include <functional>
#include <vector>

#include <simgear/math/SGGeod.hxx>
#include <simgear/structure/SGSharedPtr.hxx>

namespace flightgear
{
    class FlightPlan;
    typedef SGSharedPtr<FlightPlan> FlightPlanRef;
    
    typedef std::vector<SGGeod> SGGeodVec;
}

namespace FGTestApi {

namespace setUp {

void initTestGlobals(const std::string& testName);

bool logPositionToKML(const std::string& testName);
    
void initStandardNasal();

void populateFPWithoutNasal(flightgear::FlightPlanRef f,
                         const std::string& depICAO, const std::string& depRunway,
                         const std::string& destICAO, const std::string& destRunway,
                         const std::string& waypoints);
    
void populateFPWithNasal(flightgear::FlightPlanRef f,
            const std::string& depICAO, const std::string& depRunway,
            const std::string& destICAO, const std::string& destRunway,
            const std::string& waypoints);
    
}  // End of namespace setUp.

void setPosition(const SGGeod& g);

void runForTime(double t);

    using RunCheck = std::function<bool(void)>;
    
bool runForTimeWithCheck(double t, RunCheck check);

void writeFlightPlanToKML(flightgear::FlightPlanRef fp);
    
void writeGeodsToKML(const std::string &label, const flightgear::SGGeodVec& geods);
    
namespace tearDown {

void shutdownTestGlobals();

}  // End of namespace tearDown.

}  // End of namespace FGTestApi.

#endif // of FG_TEST_GLOBALS_HELPERS_HXX
