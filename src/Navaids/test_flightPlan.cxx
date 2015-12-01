
#include "config.h"

#include <iostream>

#include <Navaids/NavDataCache.hxx>
#include <Airports/airport.hxx>

#include <Autopilot/route_mgr.hxx>
#include <Navaids/FlightPlan.hxx>
#include <Navaids/waypoint.hxx>
#include <Navaids/navrecord.hxx>
#include <Navaids/airways.hxx>
#include <Navaids/routePath.hxx>

#include <Main/globals.hxx>
#include <Main/teststubs.hxx>

using namespace flightgear;

string_list stringsFromCharArray(const char* array[])
{
    string_list r;
    while (*array != NULL) {
        r.push_back(std::string(*array));
        ++array;
    }
    return r;
}

FlightPlanRef flightPlanFromString(const std::string& dep,
                                   const std::string& arr,
                                   const string_list& waypts)
{
    FGAirportRef depApt(FGAirport::getByIdent(dep));
    FGAirportRef arrApt(FGAirport::getByIdent(arr));

    FlightPlanRef fp(new FlightPlan);
    fp->setDeparture(depApt);
    fp->setDestination(arrApt);

    string_list::const_iterator it;
    for (it = waypts.begin(); it != waypts.end(); ++it) {
        fp->insertWayptAtIndex(fp->waypointFromString(*it), -1);
    }

    return fp;
}

void testRouting()
{
    FGAirportRef egcc(FGAirport::getByIdent("EGCC"));
    FGAirportRef eddm(FGAirport::getByIdent("EDDM"));

    FlightPlanRef fp(new FlightPlan);
    fp->setDeparture(egcc);
    fp->setDestination(eddm->getRunwayByIdent("26R"));

    WayptRef initialWP = new NavaidWaypoint(egcc, fp),
        endWP = new NavaidWaypoint(eddm, fp);

    WayptVec wps;
    Airway::highLevel()->route(initialWP, endWP, wps);


    std::cout << "airways route:" << wps.size() << std::endl;

    fp->insertWayptsAtIndex(wps, 0);

    for (int l=0; l<fp->numLegs(); ++l) {
        std::cout << "leg " << l;
        FlightPlan::Leg* leg = fp->legAtIndex(l);
        std::cout << " course:" << leg->courseDeg() << " for " << leg->distanceNm() << std::endl;
    }

    std::cout << "route distance:" << fp->totalDistanceNm() << std::endl;
}

void testVia()
{
    NavDataCache* cache = NavDataCache::instance();


    FGAirportRef eddh(FGAirport::getByIdent("EDDH"));
    FGAirportRef ekch(FGAirport::getByIdent("EKCH"));

    FlightPlanRef fp(new FlightPlan);
    fp->setDeparture(eddh->getRunwayByIdent("15"));
    fp->setDestination(ekch->getRunwayByIdent("22L"));


    FGPositioned::TypeFilter vorFilter(FGPositioned::VOR);
    FGPositionedRef ham = cache->findClosestWithIdent("HAM", eddh->geod(),&vorFilter);
    fp->insertWayptAtIndex(new NavaidWaypoint(ham, fp), 0);

    FGPositioned* intersection;
    fp->insertWayptAtIndex(new Via(fp, "airway", intersection), 0);

}

void testBasicFlightplan()
{
    NavDataCache* cache = NavDataCache::instance();

    FGAirportRef egph(FGAirport::getByIdent("EGPH"));
    FGAirportRef eham(FGAirport::getByIdent("EHAM"));

    FlightPlanRef fp0(new FlightPlan);
    fp0->setDeparture(egph->getRunwayByIdent("24"));
    fp0->setDestination(eham->getRunwayByIdent("18C"));

    FGPositioned::TypeFilter vorFilter(FGPositioned::VOR);
    FGPositionedRef tla = cache->findClosestWithIdent("TLA", egph->geod(),&vorFilter);
    fp0->insertWayptAtIndex(new NavaidWaypoint(tla, fp0), 0);

    FGPositionedRef newVor = cache->findClosestWithIdent("NEW", egph->geod(),&vorFilter);
    fp0->insertWayptAtIndex(new NavaidWaypoint(newVor, fp0), 1);

    FGPositionedRef pam = cache->findClosestWithIdent("PAM", eham->geod(),&vorFilter);
    fp0->insertWayptAtIndex(new NavaidWaypoint(pam, fp0), 2);

    test::verify(fp0->numLegs(), 3);

    double d = SGGeodesy::distanceNm(newVor->geod(), pam->geod());
    test::verifyDoubleE(fp0->legAtIndex(2)->distanceNm(), d, 0.01);
    double h = SGGeodesy::courseDeg(newVor->geod(), pam->geod());
    test::verifyDoubleE(fp0->legAtIndex(2)->courseDeg(), h, 0.1);

    // insert OTR waypoint in between

    // OTR 113.9
    fp0->insertWayptAtIndex(fp0->waypointFromString("OTR"), 2);
    fp0->insertWayptAtIndex(fp0->waypointFromString("SPY"), 3);

    test::verify(fp0->legAtIndex(3)->waypoint()->source()->ident(), "SPY");

    FGRouteMgr* routeMgr = globals->get_subsystem<FGRouteMgr>();

    test::verifyNullPtr(fp0->currentLeg());
    test::verify(fp0->currentIndex(), -1);

    double td = 0.0;
    for (int l=0; l<fp0->numLegs(); ++l) {
        FlightPlan::Leg* leg = fp0->legAtIndex(l);
        td += leg->distanceNm();
    }

    test::verifyDoubleE(fp0->totalDistanceNm(), td, 0.1);

    routeMgr->setFlightPlan(fp0);

    routeMgr->activate();

    test::verify(fp0->currentIndex(), 0);
    test::verifyNullPtr(fp0->previousLeg());
    test::verifyDouble(fp0->currentLeg()->distanceAlongRoute(), 0.0);

    double c = fp0->currentLeg()->distanceNm() +
    fp0->legAtIndex(1)->distanceNm() +
    fp0->legAtIndex(2)->distanceNm();

    fp0->setCurrentIndex(2); // leg 2
    test::verify(fp0->currentIndex(), 2);
    test::verify(fp0->currentLeg()->waypoint()->ident(), "OTR");
    test::verifyDoubleE(fp0->currentLeg()->distanceAlongRoute(), c, 0.1);

    SGPath savePath = globals->get_fg_home();
    savePath.append("fp-save.xml");
    fp0->save(savePath);

    // test loading

    FlightPlanRef fp1(new FlightPlan);
    fp1->load(savePath);

    test::verify(fp0->numLegs(), fp1->numLegs());
    test::verifyDoubleE(fp0->totalDistanceNm(), fp1->totalDistanceNm(), 0.01);

    fp1->activate();
    test::verify(fp1->currentIndex(), 0);
    fp1->setCurrentIndex(2);
    fp1->deleteIndex(2); // delete current index
    test::verify(fp1->currentIndex(), 2);
    test::verify(fp1->numLegs(), 4);
    test::verify(fp1->currentLeg()->waypoint()->ident(), "SPY");

    // test clearing
    fp1->clear();
    test::verify(fp1->numLegs(), 0);

    // test delete waypoint before the current; we should adjust current
    // index accordingly
    FlightPlanRef fp2(new FlightPlan);
    fp2->load(savePath);
    fp2->activate();
    fp2->setCurrentIndex(2);
    fp2->deleteIndex(1); // delete prior to active
    test::verify(fp2->currentIndex(), 1);
    test::verify(fp2->currentLeg()->waypoint()->ident(), "OTR");

    // test insert waypoint before current; againwe should adjust index
    fp2->insertWayptAtIndex(fp2->waypointFromString("DCS"), 1);
    fp2->insertWayptAtIndex(fp2->waypointFromString("POL"), 2);

    test::verify(fp2->currentIndex(), 3);
    test::verify(fp2->currentLeg()->waypoint()->ident(), "OTR");

    // test insert waypoint after current; again shouldn't cause any problem
    fp2->insertWayptAtIndex(fp2->waypointFromString("SPL"), -1);

    test::verify(fp2->currentIndex(), 3);
    test::verify(fp2->currentLeg()->waypoint()->ident(), "OTR");
}

void testRoutePath()
{
    const char* waypts[] = {
        "ACT",
        "TPL",
        "CWK",
        0
    };
    FlightPlanRef fp = flightPlanFromString("KDFW", "KAUS",
                                            stringsFromCharArray(waypts));

    RoutePath path(fp);

    const double leg0Dist = path.distanceBetweenIndices(0, 1),
        leg1Dist = path.distanceBetweenIndices(1, 2);

    test::verifyDouble(path.distanceBetweenIndices(0, 3),
                       leg0Dist +
                       leg1Dist +
                       path.distanceBetweenIndices(2, 3));

    // mid-point of leg 1, hopefully
    SGGeod ptOnPath = path.positionForDistanceFrom(0, leg0Dist + (0.5 * leg1Dist));

    SGGeod pt = fp->legAtIndex(1)->waypoint()->position();
    double bearing = SGGeodesy::courseDeg(pt, ptOnPath);

    test::verifyDoubleE(bearing, path.trackForIndex(2), 1e-3);
    test::verifyDoubleE(bearing, fp->legAtIndex(2)->courseDeg(), 1e-3);
    double dist = SGGeodesy::distanceM(pt, ptOnPath);
    test::verifyDoubleE(dist, leg1Dist * 0.5, 1.0);


}

int main(int argc, char* argv[])
{

    test::initHomeAndData(argc, argv);

    globals->add_new_subsystem<FGRouteMgr>();

    test::bindAndInitSubsystems();

    testBasicFlightplan();

    testRoutePath();

    testRouting();

    testVia();

    return 0;
}
