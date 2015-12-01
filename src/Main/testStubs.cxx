
#ifdef HAVE_CONFIG_H
    #include "config.h"
#endif

#include <iostream>

#include <simgear/structure/exception.hxx>
#include <simgear/timing/sg_time.hxx>

#include <Main/testStubs.hxx>
#include <Scenery/scenery.hxx>
#include <Main/globals.hxx>
#include <Main/locale.hxx>
#include <Navaids/NavDataCache.hxx>

namespace flightgear
{

namespace test
{

void initHomeAndData(int argc, char* argv[])
{
    globals = new FGGlobals;

    // look for --test-home and --test-data options
    // look for

    SGPath rootPath, homePath;

    rootPath = SGPath("/Users/jmt/FGFS/fgdata", 0);
    homePath = SGPath(SGPath::desktop());
    homePath.append("fgtest");



    globals->set_fg_home(homePath.str());

    globals->set_fg_root(rootPath.str());

    // startup the cache
    NavDataCache* cache = NavDataCache::createInstance();
    if (cache->isRebuildRequired()) {
        std::cout << "cache rebuild required" << std::endl;
        NavDataCache::RebuildPhase phase = cache->rebuild();
        while (phase != NavDataCache::REBUILD_DONE) {
            // sleep to give the rebuild thread more time
            SGTimeStamp::sleepForMSec(50);
            phase = cache->rebuild();
        }

        std::cout << "cache rebuild complete" << std::endl;
    }

    SGPath zone(globals->get_fg_root());
    zone.append("Timezone");

    SGGeod basePosition;
    SGTime* time = new SGTime(basePosition, zone, 0);
    globals->set_time_params(time);
}

void bindAndInitSubsystems()
{
    globals->get_subsystem_mgr()->bind();
    globals->get_subsystem_mgr()->init();
    globals->get_subsystem_mgr()->postinit();
}

void verifyDoubleE(double a, double b, double epsilon)
{
    double d = fabs(b - a);
    if (d > epsilon) {
        std::ostringstream os;
        os << "expected '" << a << "' but got '" << b << "'";
        throw test_failure("verify double failed", os.str());
    }
}

void verify(const std::string& a, const std::string& b)
{
    if (a != b) {
        std::ostringstream os;
        os << "expected '" << a << "' but got '" << b << "'";
        throw test_failure("verify string failed", os.str());
    }
}

void verify(int a, int b)
{
    if (a != b) {
        std::ostringstream os;
        os << "expected '" << a << "' but got '" << b << "'";
        throw test_failure("verify int failed", os.str());
    }
}

void verifyDouble(double a, double b)
{
    verifyDoubleE(a, b, SG_EPSILON);
}

void verifyNullPtr(void* p)
{
    if (p != NULL) {
        throw test_failure("verify null ptr failed", "got non-NULL pointer");
    }
}

test_failure::test_failure(const std::string& msg, const std::string& description) :
    sg_exception(msg, description)
{

}


} // of namespace test

} // of namespace flightgear

void guiErrorMessage(const char *txt, const sg_throwable &throwable)
{
  std::cerr << "error:" << txt << std::endl;
}

#ifdef __APPLE__

string_list FGLocale::getUserLanguage()
{
    string_list r;
    r.push_back("en-US");
    return r;
}

#endif

