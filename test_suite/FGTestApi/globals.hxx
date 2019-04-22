#ifndef FG_TEST_GLOBALS_HELPERS_HXX
#define FG_TEST_GLOBALS_HELPERS_HXX

#include <string>

class SGGeod;

namespace FGTestApi {

namespace setUp {

void initTestGlobals(const std::string& testName);

}  // End of namespace setUp.

void setPosition(const SGGeod& g);

void runForTime(double t);

namespace tearDown {

void shutdownTestGlobals();

}  // End of namespace tearDown.

}  // End of namespace FGTestApi.

#endif // of FG_TEST_GLOBALS_HELPERS_HXX
