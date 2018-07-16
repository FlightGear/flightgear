
#include "config.h"

#include "test_suite/dataStore.hxx"

#include "globals.hxx"

#if defined(HAVE_QT) && !defined(FG_TESTLIB)
#include <GUI/QtLauncher.hxx>
#endif

#include <Main/globals.hxx>
#include <Main/options.hxx>

#include <Navaids/NavDataCache.hxx>
#include <Time/TimeManager.hxx>

#include <simgear/debug/logstream.hxx>
#include <simgear/misc/sg_dir.hxx>
#include <simgear/timing/timestamp.hxx>

#include <iostream>

static SGPath tests_fgdata;

namespace FGTestApi {

namespace setUp {

    SGPath fgdataPath()
    {
        return tests_fgdata;
    }

  void initTestGlobals(const std::string& testName)
  {
    globals = new FGGlobals;

    DataStore &data = DataStore::get();
    if (!data.getFGRoot().exists()) {
        data.findFGRoot("");
    }
    globals->set_fg_root(data.getFGRoot());
    tests_fgdata = data.getFGRoot();

      // current dir
      SGPath homePath = SGPath::fromUtf8(FGBUILDDIR) / "test_home";
      if (!homePath.exists()) {
          (homePath / "dummyFile").create_dir(0755);
      }

      globals->set_fg_home(homePath);

      fgSetDefaults();

      flightgear::NavDataCache* cache = flightgear::NavDataCache::createInstance();
      if (cache->isRebuildRequired()) {
          std::cerr << "Navcache rebuild for testing" << std::flush;

          while (cache->rebuild() != flightgear::NavDataCache::REBUILD_DONE) {
            SGTimeStamp::sleepForMSec(1000);
            std::cerr << "." << std::flush;
          }
      }

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
#if defined(HAVE_QT) && !defined(FG_TESTLIB)
    flightgear::shutdownQtApp();
#endif

    delete globals;
  }

}  // End of namespace tearDown.

}  // End of namespace FGTestApi.
