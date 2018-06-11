#include "config.h"

#include "test_suite/dataStore.hxx"

#include "globals.hxx"

#if defined(HAVE_QT)
#include <GUI/QtLauncher.hxx>
#endif

#include <Main/globals.hxx>
#include <Main/options.hxx>

#include <Time/TimeManager.hxx>

#include <simgear/timing/timestamp.hxx>


namespace FGTestApi {

namespace setUp {

void initTestGlobals(const std::string& testName)
{
    globals = new FGGlobals;

    DataStore &data = DataStore::get();
    if (!data.getFGRoot().exists()) {
        data.findFGRoot("");
    }
    globals->set_fg_root(data.getFGRoot());

    // current dir
    SGPath homePath = SGPath::fromUtf8(FGBUILDDIR) / "test_home";
    if (!homePath.exists()) {
        (homePath / "dummyFile").create_dir(0755);
    }

    globals->set_fg_home(homePath);

    // Activate headless mode.
    globals->set_headless(true);

    fgSetDefaults();

    std::unique_ptr<TimeManager> t;
    t.reset(new TimeManager);
    t->init(); // establish mag-var data
}

}  // End of namespace setUp.


namespace tearDown {

void shutdownTestGlobals()
{
    // The QApplication instance must be destroyed before exit() begins, see
    // <https://bugreports.qt.io/browse/QTBUG-48709> (otherwise, segfault).
#if defined(HAVE_QT)
    flightgear::shutdownQtApp();
#endif

    delete globals;
}

}  // End of namespace tearDown.

}  // End of namespace FGTestApi.
