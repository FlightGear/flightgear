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

#include "fakeAIAircraft.hxx"
#include "FDM/AIWake/AircraftMesh.hxx"
#include "FDM/AIWake/AIWakeGroup.hxx"
extern "C" {
#include "src/FDM/LaRCsim/ls_matrix.h"
}

#include "FDM/JSBSim/math/FGLocation.h"
#include "FDM/JSBSim/math/FGQuaternion.h"

using namespace std;
using namespace JSBSim;

double rho = 2.0E-3;

void testLiftComputation()
{
    double b = 10.0;
    double c = 2.0;
    AircraftMesh_ptr mesh = new AircraftMesh(b, c);
    SGGeod geodPos = SGGeod::fromDeg(0.0, 0.0);
    SGVec3d pos;
    double vel = 100.;
    double weight = 50.;

    SGGeodesy::SGGeodToCart(geodPos, pos);
    mesh->setPosition(pos, SGQuatd::unit());

    FGAIAircraft ai(1);
    ai.setPosition(pos);
    ai.setGeom(b, c, weight);
    ai.setVelocity(vel);

    AIWakeGroup wg;
    wg.AddAI(&ai);

    SGVec3d force = mesh->GetForce(wg, SGVec3d(vel, 0., 0.), rho);

    SG_CHECK_EQUAL_EP(force[1], 0.0);
    SG_CHECK_EQUAL_EP(force[2], -weight);

    SGVec3d moment = mesh->GetMoment();
    SG_CHECK_EQUAL_EP(moment[0], 0.0);
    SG_CHECK_EQUAL_EP(moment[1], -0.5*weight);
    SG_CHECK_EQUAL_EP(moment[2], 0.0);

    for (int i=1; i<= mesh->nelm; ++i)
        SG_CHECK_EQUAL_EP(wg._aiWakeData[1].mesh->Gamma[i][1],
                          mesh->Gamma[i][1]);
}

void testFourierLiftingLine()
{
    double b = 10.0;
    double c = 2.0;
    double vel = 100.;
    double weight = 50.;

    WakeMesh_ptr mesh = new WakeMesh(b, c);
    int N = mesh->nelm;
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
        double y = mesh->elements[i-1]->getBoundVortexMidPoint()[1];
        double theta = acos(2.0*y/b);
        double gamma = 0.0;
        for (int n=1; n<=N; ++n)
            gamma += coef[n][1]*sin(n*theta);

        gamma *= 2.0*b*vel*sinAlpha;

        cout << y << ", " << gamma << ", " << mesh->Gamma[i][1] << ", "
             << mesh->Gamma[i][1] / gamma - 1.0 << endl;
    }

    nr_free_matrix(mtx, 1, N, 1, N);
    nr_free_matrix(coef, 1, N, 1, 1);
}

void testFrameTransformations()
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

    SG_CHECK_EQUAL_EP(pos[0] * SG_METER_TO_FEET, loc(1));
    SG_CHECK_EQUAL_EP(pos[1] * SG_METER_TO_FEET, loc(2));
    SG_CHECK_EQUAL_EP(pos[2] * SG_METER_TO_FEET, loc(3));

    AircraftMesh_ptr mesh = new AircraftMesh(b, c);
    mesh->setPosition(pos, orient);

    FGQuaternion qJ(roll, pitch, yaw);
    FGMatrix33 Tb2l = qJ.GetTInv();
    FGColumnVector3 refPos = loc.GetTec2l() * loc;

    for (int i=0; i < 4; ++i)
        SG_CHECK_EQUAL_EP(orient(i), qJ((i+1) % 4 + 1));

    for (int i=0; i < mesh->nelm; ++i) {
        SGVec3d pt = mesh->elements[i]->getBoundVortexMidPoint();
        FGColumnVector3 ptJ(pt[0], pt[1], pt[2]);
        FGColumnVector3 p = loc.GetTl2ec() * (refPos + Tb2l * ptJ);
        SG_CHECK_EQUAL_EP(mesh->midPt[i][0], p(1));
        SG_CHECK_EQUAL_EP(mesh->midPt[i][1], p(2));
        SG_CHECK_EQUAL_EP(mesh->midPt[i][2], p(3));

        pt = mesh->elements[i]->getCollocationPoint();
        ptJ.InitMatrix(pt[0], pt[1], pt[2]);
        p = loc.GetTl2ec() * (refPos + Tb2l * ptJ);
        SG_CHECK_EQUAL_EP(mesh->collPt[i][0], p(1));
        SG_CHECK_EQUAL_EP(mesh->collPt[i][1], p(2));
        SG_CHECK_EQUAL_EP(mesh->collPt[i][2], p(3));
    }
}

int main(int argc, char* argv[])
{
    globals->get_props()->getNode("environment/density-slugft3", false)
        ->setDoubleValue(rho);

    testFourierLiftingLine();
    testLiftComputation();
    testFrameTransformations();
}
