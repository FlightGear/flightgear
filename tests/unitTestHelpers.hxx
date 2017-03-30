#ifndef FG_TEST_HELPERS_HXX
#define FG_TEST_HELPERS_HXX

#include <string>
#include <simgear/misc/sg_path.hxx>

namespace fgtest
{

  void initTestGlobals(const std::string& testName);

  void shutdownTestGlobals();

    SGPath fgdataPath();
}

#endif // of FG_TEST_HELPERS_HXX
