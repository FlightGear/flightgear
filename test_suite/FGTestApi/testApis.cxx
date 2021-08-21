
#include "config.h"

// implement various test-only methods on classes

#include <Airports/groundnetwork.hxx>
#include <Airports/airport.hxx>
#include <Airports/xmlloader.hxx>
#include <Navaids/procedure.hxx>

void FGAirport::testSuiteInjectGroundnetXML(const SGPath& path)
{
    _groundNetwork.reset(new FGGroundNetwork(const_cast<FGAirport*>(this)));
    XMLLoader::loadFromPath(_groundNetwork.get(), path);
    _groundNetwork->init();
}

void FGAirport::testSuiteInjectProceduresXML(const SGPath& path)
{
    if (mProceduresLoaded) {
        SG_LOG(SG_GENERAL, SG_ALERT, "Procedures already loaded for" << ident());
        mSIDs.clear();
        mSTARs.clear();
        mApproaches.clear();
    }
    
    mProceduresLoaded = true;
    flightgear::RouteBase::loadAirportProcedures(path, const_cast<FGAirport*>(this));
}

