#include "config.h"

#include "test_suite/dataStore.hxx"

#include "globals.hxx"

#if defined(HAVE_QT)
#include <GUI/QtLauncher.hxx>
#endif

#include <Main/globals.hxx>
#include <Main/options.hxx>

#include <Time/TimeManager.hxx>

#include <simgear/structure/event_mgr.hxx>
#include <simgear/timing/timestamp.hxx>
#include <simgear/math/sg_geodesy.hxx>

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

    /**
     * Both the event manager and subsystem manager are initialised by the
     * FGGlobals ctor, but only the subsystem manager is destroyed by the dtor.
     * Here the event manager is added to the subsystem manager so it can be
     * destroyed via the subsystem manager.
     */
    globals->add_subsystem("events", globals->get_event_mgr(), SGSubsystemMgr::DISPLAY);
}

}  // End of namespace setUp.

void setPosition(const SGGeod& g)
{
    globals->get_props()->setDoubleValue("position/latitude-deg", g.getLatitudeDeg());
    globals->get_props()->setDoubleValue("position/longitude-deg", g.getLongitudeDeg());
    globals->get_props()->setDoubleValue("position/altitude-ft", g.getElevationFt());
}

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
