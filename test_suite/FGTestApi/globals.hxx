#ifndef FG_TEST_HELPERS_HXX
#define FG_TEST_HELPERS_HXX

#include <string>
#include <simgear/misc/sg_path.hxx>

namespace FGTestApi {

namespace setUp {

void initTestGlobals(const std::string& testName);

SGPath fgdataPath();

}  // End of namespace setUp.


namespace tearDown {

void shutdownTestGlobals();

}  // End of namespace tearDown.

}  // End of namespace FGTestApi.

#endif // of FG_TEST_HELPERS_HXX
