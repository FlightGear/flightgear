#include <string>
#include <cstdlib>

#include <simgear/misc/sg_path.hxx>
#include <simgear/structure/exception.hxx>

#include <Main/locale.hxx>
#include <Main/options.hxx>
#include <Scripting/NasalSys.hxx>
#include <GUI/MessageBox.hxx>
#include <AIModel/AIAircraft.hxx>
#include <ATC/trafficcontrol.hxx>
#include <ATC/GroundController.hxx>
#include <Scenery/scenery.hxx>

// declared extern in main.hxx
std::string hostname;

string fgBasePackageVersion(const SGPath& base_path)
{
    return "";
}

void fgHiResDump()
{
}

void fgDumpSnapShot()
{

}

void fgCancelSnapShot()
{

}

void fgDumpSceneGraph()
{

}

void fgOSFullScreen()
{

}

void guiErrorMessage(const char *txt)
{

}

void guiErrorMessage(const char *txt, const sg_throwable &throwable)
{

}


void fgPrintVisibleSceneInfoCommand()
{

}


void openBrowser(const std::string& s)
{

}

void fgDumpTerrainBranch()
{

}

void fgOSExit(int code)
{
}

void postinitNasalGUI(naRef globals, naContext c)
{

}

void syncPausePopupState()
{

}
void
SGSetTextureFilter( int max)
{

}

int
SGGetTextureFilter()
{
  return 0;
}

bool FGScenery::get_elevation_m(const SGGeod& geod, double& alt,
                     const simgear::BVHMaterial** material,
                     const osg::Node* butNotFrom)
{
    return false;
}

namespace flightgear
{
  MessageBoxResult modalMessageBox(const std::string& caption,
      const std::string& msg,
      const std::string& moreText)
  {
	  return MSG_BOX_OK;
  }

  MessageBoxResult fatalMessageBox(const std::string& caption,
      const std::string& msg,
      const std::string& moreText)
  {
	  return MSG_BOX_OK;
  }
}

#ifdef __APPLE__
string_list
FGLocale::getUserLanguage()
{
    string_list result;
    const char* langEnv = ::getenv("LANG");
    if (langEnv) {
        result.push_back(langEnv);
    }

    return result;
}

namespace flightgear
{

SGPath Options::platformDefaultRoot() const
{
    SGPath dataDir;
    dataDir.append("data");
    return dataDir;
}

} // of namespace flightgear

#endif


FGATCController::FGATCController()
{

}

FGATCController::~FGATCController()
{

}

FGATCInstruction::FGATCInstruction()
{

}

////////////////////////////////////////////////


FGTowerController::FGTowerController(FGAirportDynamics*)
{
}

FGTowerController::~FGTowerController()
{
}

void FGTowerController::render(bool)
{

}

void FGTowerController::signOff(int id)
{
}

std::string FGTowerController::getName()
{
	return "tower";
}

void FGTowerController::update(double dt)
{

}

void FGTowerController::announcePosition(int id, FGAIFlightPlan *intendedRoute, int currentRoute, double lat, double lon, double hdg, double spd, double alt, double radius, int leg, FGAIAircraft *aircraft)
{

}

void FGTowerController::updateAircraftInformation(int id, double lat, double lon, double heading, double speed, double alt, double dt)
{

}

bool FGTowerController::hasInstruction(int id)
{
    return false;
}

FGATCInstruction FGTowerController::getInstruction(int id)
{
	return FGATCInstruction();
}

//////////////////////////////////////////////////////////////
/// \brief FGApproachController::FGApproachController
///
FGApproachController::FGApproachController(FGAirportDynamics*)
{
}

FGApproachController::~FGApproachController()
{
}


void FGApproachController::render(bool)
{

}

void FGApproachController::signOff(int id)
{
}

std::string FGApproachController::getName()
{
	return "approach";
}

void FGApproachController::update(double dt)
{

}

void FGApproachController::announcePosition(int id, FGAIFlightPlan *intendedRoute, int currentRoute, double lat, double lon, double hdg, double spd, double alt, double radius, int leg, FGAIAircraft *aircraft)
{

}

void FGApproachController::updateAircraftInformation(int id, double lat, double lon, double heading, double speed, double alt, double dt)
{

}

bool FGApproachController::hasInstruction(int id)
{
    return false;
}


FGATCInstruction FGApproachController::getInstruction(int id)
{
	return FGATCInstruction();
}

//////////////////////////////////////////////////////

FGStartupController::FGStartupController(FGAirportDynamics*)
{
}

FGStartupController::~FGStartupController()
{
}


void FGStartupController::render(bool)
{

}

void FGStartupController::signOff(int id)
{
}

std::string FGStartupController::getName()
{
	return "startup";
}

void FGStartupController::update(double dt)
{

}

void FGStartupController::announcePosition(int id, FGAIFlightPlan *intendedRoute, int currentRoute, double lat, double lon, double hdg, double spd, double alt, double radius, int leg, FGAIAircraft *aircraft)
{

}

void FGStartupController::updateAircraftInformation(int id, double lat, double lon, double heading, double speed, double alt, double dt)
{

}

bool FGStartupController::hasInstruction(int id)
{
    return false;
}


FGATCInstruction FGStartupController::getInstruction(int id)
{
	return FGATCInstruction();
}

//////////////////////////////////////////////////////

FGGroundController::FGGroundController()
{
}

FGGroundController::~FGGroundController()
{
}

void FGGroundController::init(FGAirportDynamics* pr)
{

}

void FGGroundController::render(bool)
{

}

void FGGroundController::signOff(int id)
{
}

std::string FGGroundController::getName()
{
	return "ground";
}

void FGGroundController::update(double dt)
{

}

void FGGroundController::announcePosition(int id, FGAIFlightPlan *intendedRoute, int currentRoute, double lat, double lon, double hdg, double spd, double alt, double radius, int leg, FGAIAircraft *aircraft)
{

}

void FGGroundController::updateAircraftInformation(int id, double lat, double lon, double heading, double speed, double alt, double dt)
{

}

bool FGGroundController::hasInstruction(int id)
{
    return false;
}


FGATCInstruction FGGroundController::getInstruction(int id)
{
	return FGATCInstruction();
}


#if defined(SG_WINDOWS)

#include <plib/sg.h>

// this stub is needed on Windows because sg.h decleares this function as extern
void sgdMakeCoordMat4(sgdMat4 m, const SGDfloat x, const SGDfloat y, const SGDfloat z, const SGDfloat h, const SGDfloat p, const SGDfloat r)
{
}
#endif

FGTrafficRecord::~FGTrafficRecord()
{

}
