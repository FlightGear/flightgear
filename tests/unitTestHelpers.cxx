
#include "config.h"

#include "unitTestHelpers.hxx"

#include <Main/globals.hxx>
#include <Navaids/NavDataCache.hxx>
#include <Time/TimeManager.hxx>

#include <simgear/debug/logstream.hxx>
#include <simgear/misc/sg_dir.hxx>
#include <simgear/timing/timestamp.hxx>

#include <iostream>

namespace fgtest
{

    bool looksLikeFGData(const SGPath& path)
    {
        return (path / "defaults.xml").exists();
    }

  void initTestGlobals(const std::string& testName)
  {
    sglog().setLogLevels( SG_ALL, SG_WARN );
    sglog().setDeveloperMode(true);

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
            exit(EXIT_FAILURE);
        }


      // current dir
      SGPath homePath = simgear::Dir::current().path() / "test_home";
      if (!homePath.exists()) {
          (homePath / "dummyFile").create_dir(0755);
      }

      globals->set_fg_home(homePath);

      flightgear::NavDataCache* cache = flightgear::NavDataCache::createInstance();
      if (cache->isRebuildRequired()) {
          std::cerr << "Navcache rebuild for testing" << std::flush;

          while (cache->rebuild() != flightgear::NavDataCache::REBUILD_DONE) {
			  SGTimeStamp::sleepForMSec(1000);
              std::cerr << "." << std::flush;
          }
      }

      TimeManager* t = new TimeManager;
      t->init(); // establish mag-var data
  }

  void shutdownTestGlobals()
  {
    delete globals;
  }
} // of namespace fgtest
