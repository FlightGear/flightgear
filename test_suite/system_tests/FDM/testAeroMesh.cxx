#include "testAeroMesh.hxx"

#include "test_suite/FGTestApi/testGlobals.hxx"
#include "test_suite/FGTestApi/PrivateAccessorFDM.hxx"
#include "test_suite/FGTestApi/scene_graph.hxx"

#include <vector>
#include <map>
#include <iostream>

#include <simgear/constants.h>
#include <simgear/misc/test_macros.hxx>
#include <simgear/math/SGVec3.hxx>
#include <simgear/math/SGGeod.hxx>
#include <simgear/math/SGQuat.hxx>
#include <simgear/math/SGGeoc.hxx>
#include <simgear/props/props.hxx>

#include <AIModel/AIAircraft.hxx>
#include <AIModel/AIManager.hxx>
#include <AIModel/performancedata.hxx>
#include <AIModel/performancedb.hxx>
#include "FDM/AIWake/AircraftMesh.hxx"
#include "FDM/AIWake/AIWakeGroup.hxx"

extern "C" {
#include "src/FDM/ls_matrix.h"
}

#include "FDM/JSBSim/math/FGLocation.h"
#include "FDM/JSBSim/math/FGQuaternion.h"

#include <Main/globals.hxx>


using namespace std;
using namespace JSBSim;

double rho = 2.0E-3;


// Set up function for each test.
void AeroMeshTests::setUp()
{
    FGTestApi::setUp::initTestGlobals("aeromesh");
    FGTestApi::setUp::initScenery();
    globals->get_props()->getNode("environment/density-slugft3", true)
        ->setDoubleValue(rho);
}


// Clean up after each test.
void AeroMeshTests::tearDown()
{
    FGTestApi::tearDown::shutdownTestGlobals();
}


void AeroMeshTests::testLiftComputation()
{
    double b = 10.0;
    double c = 2.0;
    AircraftMesh_ptr mesh = new AircraftMesh(b, c, "test");
    SGGeod geodPos = SGGeod::fromDeg(0.0, 0.0);
    SGVec3d pos;
    double vel = 100.;
    double weight = 50.;

    SGGeodesy::SGGeodToCart(geodPos, pos);
    mesh->setPosition(pos, SGQuatd::unit());

    SGPropertyNode* props = globals->get_props()->getNode("ai/models", true);
    props->setDoubleValue("acceleration-kts-hour", 0.0);
    props->setDoubleValue("deceleration-kts-hour", 0.0);
    props->setDoubleValue("climbrate-fpm", 0.0);
    props->setDoubleValue("decentrate-fpm", 0.0);
    props->setDoubleValue("rotate-speed-kts", 0.0);
    props->setDoubleValue("takeoff-speed-kts", 0.0);
    props->setDoubleValue("climb-speed-kts", 0.0);
    props->setDoubleValue("cruise-speed-kts", 0.0);
    props->setDoubleValue("decent-speed-kts", 0.0);
    props->setDoubleValue("approach-speed-kts", 0.0);
    props->setDoubleValue("touchdown-speed-kts", 0.0);
    props->setDoubleValue("taxi-speed-kts", 0.0);
    props->setDoubleValue("geometry/wing/span-ft", b);
    props->setDoubleValue("geometry/wing/chord-ft", c);
    props->setDoubleValue("geometry/weight-lbs", weight);

    auto subsystem_mgr = globals->get_subsystem_mgr();
    subsystem_mgr->add<PerformanceDB>();
    subsystem_mgr->get_subsystem<PerformanceDB>()->bind();
    subsystem_mgr->get_subsystem<PerformanceDB>()->init();

    FGAIManager *aiManager = new FGAIManager;
    FGAIAircraft *ai = new FGAIAircraft;
    ai->setGeodPos(geodPos);
    ai->setSpeed(vel * SG_FPS_TO_KT);
    ai->setPerformance("", "jet_transport");
    ai->getPerformance()->initFromProps(props);
    aiManager->attach(ai);

    AIWakeGroup wg;
    wg.AddAI(ai);

    SGVec3d force = mesh->GetForce(wg, SGVec3d(vel, 0., 0.), rho);

    CPPUNIT_ASSERT_DOUBLES_EQUAL(force[1], 0.0, 1e-9);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(force[2], -weight, 1e-9);

    SGVec3d moment = mesh->GetMoment();
    CPPUNIT_ASSERT_DOUBLES_EQUAL(moment[0], 0.0, 1e-9);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(moment[1], -0.5*weight, 1e-9);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(moment[2], 0.0, 1e-9);

    auto accessor = FGTestApi::PrivateAccessor::FDM::Accessor();

    for (int i=1; i<= accessor.read_FDM_AIWake_WakeMesh_nelm(mesh); ++i)
        CPPUNIT_ASSERT_DOUBLES_EQUAL(accessor.read_FDM_AIWake_WakeMesh_Gamma(accessor.read_FDM_AIWake_AIWakeGroup_aiWakeData(&wg, 1))[i][1],
                          accessor.read_FDM_AIWake_WakeMesh_Gamma(mesh)[i][1], 1e-9);
}


void AeroMeshTests::testFourierLiftingLine()
{
    double b = 10.0;
    double c = 2.0;
    double vel = 100.;
    double weight = 50.;

    auto accessor = FGTestApi::PrivateAccessor::FDM::Accessor();

    WakeMesh_ptr mesh = new WakeMesh(b, c, "testMesh");
    int N = accessor.read_FDM_AIWake_WakeMesh_nelm(mesh);
    double **mtx = nr_matrix(1, N, 1, N);
    double **coef = nr_matrix(1, N, 1, 1);

    mesh->computeAoA(vel, rho, weight);

    for (int m=1; m<=N; ++m) {
        double vm = M_PI*m/(N+1);
        double sm = sin(vm);
        coef[m][1] = c*M_PI/(2*b);
        for (int n=1; n<=N; ++n)
            mtx[m][n] = sin(n*vm)*(1.0+coef[m][1]*n/sm);
    }

    nr_gaussj(mtx, N, coef, 1);

    double S = b*c;
    double AR = b*b/S;
    double lift = 0.5*rho*S*vel*vel*coef[1][1]*M_PI*AR;
    double sinAlpha = weight / lift;
    lift  *= sinAlpha;

    cout << "y, Lift (Fourier), Lift (VLM), Corrected lift (VLM)" << endl;

    for (int i=1; i<=N; ++i) {
        double y = accessor.read_FDM_AIWake_WakeMesh_elements(mesh)[i-1]->getBoundVortexMidPoint()[1];
        double theta = acos(2.0*y/b);
        double gamma = 0.0;
        for (int n=1; n<=N; ++n)
            gamma += coef[n][1]*sin(n*theta);

        gamma *= 2.0*b*vel*sinAlpha;

        cout << y << ", " << gamma << ", " << accessor.read_FDM_AIWake_WakeMesh_Gamma(mesh)[i][1] << ", "
             << accessor.read_FDM_AIWake_WakeMesh_Gamma(mesh)[i][1] / gamma - 1.0 << endl;
    }

    nr_free_matrix(mtx, 1, N, 1, N);
    nr_free_matrix(coef, 1, N, 1, 1);
}


void AeroMeshTests::testFrameTransformations()
{
    double b = 10.0;
    double c = 2.0;
    double yaw = 80. * SGD_DEGREES_TO_RADIANS;
    double pitch = 5. * SGD_DEGREES_TO_RADIANS;
    double roll = -10. * SGD_DEGREES_TO_RADIANS;
    SGQuatd orient = SGQuatd::fromYawPitchRoll(yaw, pitch, roll);
    SGGeod geodPos = SGGeod::fromDeg(45.0, 10.0);
    SGVec3d pos;

    SGGeodesy::SGGeodToCart(geodPos, pos);
    SGGeoc geoc = SGGeoc::fromCart(pos);

    FGLocation loc(geoc.getLongitudeRad(),
                   geoc.getLatitudeRad(),
                   geoc.getRadiusFt());

    CPPUNIT_ASSERT_DOUBLES_EQUAL(pos[0] * SG_METER_TO_FEET, loc(1), 1e-7);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(pos[1] * SG_METER_TO_FEET, loc(2), 1e-7);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(pos[2] * SG_METER_TO_FEET, loc(3), 1e-7);

    AircraftMesh_ptr mesh = new AircraftMesh(b, c, "test");
    mesh->setPosition(pos, orient);

    FGQuaternion qJ(roll, pitch, yaw);
    FGMatrix33 Tb2l = qJ.GetTInv();
    FGColumnVector3 refPos = loc.GetTec2l() * loc;

    for (int i=0; i < 4; ++i)
        CPPUNIT_ASSERT_DOUBLES_EQUAL(orient(i), qJ((i+1) % 4 + 1), 1e-9);

    auto accessor = FGTestApi::PrivateAccessor::FDM::Accessor();

    for (int i=0; i < accessor.read_FDM_AIWake_WakeMesh_nelm(mesh); ++i) {
        SGVec3d pt = accessor.read_FDM_AIWake_WakeMesh_elements(mesh)[i]->getBoundVortexMidPoint();
        FGColumnVector3 ptJ(pt[0], pt[1], pt[2]);
        FGColumnVector3 p = loc.GetTl2ec() * (refPos + Tb2l * ptJ);
        CPPUNIT_ASSERT_DOUBLES_EQUAL(accessor.read_FDM_AIWake_AircraftMesh_midPt(mesh)[i][0], p(1), 1e-7);
        CPPUNIT_ASSERT_DOUBLES_EQUAL(accessor.read_FDM_AIWake_AircraftMesh_midPt(mesh)[i][1], p(2), 1e-7);
        CPPUNIT_ASSERT_DOUBLES_EQUAL(accessor.read_FDM_AIWake_AircraftMesh_midPt(mesh)[i][2], p(3), 1e-7);

        pt = accessor.read_FDM_AIWake_WakeMesh_elements(mesh)[i]->getCollocationPoint();
        ptJ.InitMatrix(pt[0], pt[1], pt[2]);
        p = loc.GetTl2ec() * (refPos + Tb2l * ptJ);
        CPPUNIT_ASSERT_DOUBLES_EQUAL(accessor.read_FDM_AIWake_AircraftMesh_collPt(mesh)[i][0], p(1), 1e-7);
        CPPUNIT_ASSERT_DOUBLES_EQUAL(accessor.read_FDM_AIWake_AircraftMesh_collPt(mesh)[i][1], p(2), 1e-7);
        CPPUNIT_ASSERT_DOUBLES_EQUAL(accessor.read_FDM_AIWake_AircraftMesh_collPt(mesh)[i][2], p(3), 1e-7);
    }
}
