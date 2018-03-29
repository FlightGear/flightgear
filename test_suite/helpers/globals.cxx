
#include "config.h"

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

namespace fgtest
{

    bool looksLikeFGData(const SGPath& path)
    {
        return (path / "defaults.xml").exists();
    }


    SGPath fgdataPath()
    {
        return tests_fgdata;
    }

  void initTestGlobals(const std::string& testName)
  {
    globals = new FGGlobals;

      bool foundRoot = false;
    if (std::getenv("FG_ROOT")) {
      SGPath fg_root = SGPath::fromEnv("FG_ROOT");
        if (looksLikeFGData(fg_root)) {
            globals->set_fg_root(fg_root);
            foundRoot = true;
        }
    }

      if (!foundRoot) {
        SGPath pkgLibDir = SGPath::fromUtf8(PKGLIBDIR);
        if (looksLikeFGData(pkgLibDir)) {
          globals->set_fg_root(pkgLibDir);
            foundRoot = true;
        }
      }

      if (!foundRoot) {
      SGPath dataDir = SGPath::fromUtf8(FGSRCDIR) / ".." / "fgdata";
        if (looksLikeFGData(dataDir)) {
            globals->set_fg_root(dataDir);
            foundRoot = true;
        }
      }

      if (!foundRoot) {
            std::cerr << "FGData not found" << std::endl;
        }

      tests_fgdata = globals->get_fg_root();

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

  void shutdownTestGlobals()
  {
    // The QApplication instance must be destroyed before exit() begins, see
    // <https://bugreports.qt.io/browse/QTBUG-48709> (otherwise, segfault).
#if defined(HAVE_QT) && !defined(FG_TESTLIB)
    flightgear::shutdownQtApp();
#endif

    delete globals;
  }
} // of namespace fgtest
