#include <simgear/constants.h>
#include <simgear/structure/SGSharedPtr.hxx>
#include <simgear/math/SGVec3.hxx>

#include "FDM/AIWake/AeroElement.hxx"

#include "testAeroElement.hxx"


void AeroElementTests::testNormal()
{
    AeroElement_ptr el = new AeroElement(SGVec3d(-1., -0.5, 0.),
                                         SGVec3d(0., -0.5, 0.),
                                         SGVec3d(0., 0.5, 0.),
                                         SGVec3d(-1., 0.5, 0.));
    SGVec3d n = el->getNormal();
    CPPUNIT_ASSERT_DOUBLES_EQUAL(n[0], 0.0, 1e-9);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(n[1], 0.0, 1e-9);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(n[2], -1.0, 1e-9);
}

void AeroElementTests::testCollocationPoint()
{
    AeroElement_ptr el = new AeroElement(SGVec3d(-1., -0.5, 0.),
                                         SGVec3d(0., -0.5, 0.),
                                         SGVec3d(0., 0.5, 0.),
                                         SGVec3d(-1., 0.5, 0.));
    SGVec3d cp = el->getCollocationPoint();
    CPPUNIT_ASSERT_DOUBLES_EQUAL(cp[0], -0.75, 1e-9);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(cp[1], 0.0, 1e-9);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(cp[2], 0.0, 1e-9);
}

void AeroElementTests::testBoundVortexMidPoint()
{
    AeroElement_ptr el = new AeroElement(SGVec3d(-1., -0.5, 0.),
                                         SGVec3d(0., -0.5, 0.),
                                         SGVec3d(0., 0.5, 0.),
                                         SGVec3d(-1., 0.5, 0.));
    SGVec3d mp = el->getBoundVortexMidPoint();
    CPPUNIT_ASSERT_DOUBLES_EQUAL(mp[0], -0.25, 1e-9);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(mp[1], 0.0, 1e-9);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(mp[2], 0.0, 1e-9);
}

void AeroElementTests::testBoundVortex()
{
    AeroElement_ptr el = new AeroElement(SGVec3d(-1., -0.5, 0.),
                                         SGVec3d(0., -0.5, 0.),
                                         SGVec3d(0., 0.5, 0.),
                                         SGVec3d(-1., 0.5, 0.));
    SGVec3d v = el->getBoundVortex();
    CPPUNIT_ASSERT_DOUBLES_EQUAL(v[0], 0.0, 1e-9);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(v[1], 1.0, 1e-9);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(v[2], 0.0, 1e-9);
}

void AeroElementTests::testInducedVelocityOnBoundVortex()
{
    AeroElement_ptr el = new AeroElement(SGVec3d(-1., -0.5, 0.),
                                         SGVec3d(0., -0.5, 0.),
                                         SGVec3d(0., 0.5, 0.),
                                         SGVec3d(-1., 0.5, 0.));
    SGVec3d mp = el->getBoundVortexMidPoint();
    SGVec3d v = el->getInducedVelocity(mp);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(v[0], 0.0, 1e-9);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(v[1], 0.0, 1e-9);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(v[2], 1.0/M_PI, 1e-9);
}

void AeroElementTests::testInducedVelocityOnCollocationPoint()
{
    AeroElement_ptr el = new AeroElement(SGVec3d(-1., -0.5, 0.),
                                         SGVec3d(0., -0.5, 0.),
                                         SGVec3d(0., 0.5, 0.),
                                         SGVec3d(-1., 0.5, 0.));
    SGVec3d cp = el->getCollocationPoint();
    SGVec3d v = el->getInducedVelocity(cp);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(v[0], 0.0, 1e-9);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(v[1], 0.0, 1e-9);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(v[2], (1.0+sqrt(2.0)/M_PI), 1e-9);
}

void AeroElementTests::testInducedVelocityAtFarField()
{
    AeroElement_ptr el = new AeroElement(SGVec3d(-1., -0.5, 0.),
                                         SGVec3d(0., -0.5, 0.),
                                         SGVec3d(0., 0.5, 0.),
                                         SGVec3d(-1., 0.5, 0.));
    SGVec3d mp = el->getBoundVortexMidPoint();
    SGVec3d v = el->getInducedVelocity(mp+SGVec3d(-1000.,0.,0.));
    CPPUNIT_ASSERT_DOUBLES_EQUAL(v[0], 0.0, 1e-9);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(v[1], 0.0, 1e-9);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(v[2], 2.0/M_PI, 1e-7);
}

void AeroElementTests::testInducedVelocityAbove()
{
    AeroElement_ptr el = new AeroElement(SGVec3d(-1., -0.5, 0.),
                                         SGVec3d(0., -0.5, 0.),
                                         SGVec3d(0., 0.5, 0.),
                                         SGVec3d(-1., 0.5, 0.));
    SGVec3d mp = el->getBoundVortexMidPoint();
    SGVec3d v = el->getInducedVelocity(mp+SGVec3d(0.,0.,-0.5));
    CPPUNIT_ASSERT_DOUBLES_EQUAL(v[0], -1.0/(sqrt(2.0)*M_PI), 1e-9);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(v[1], 0.0, 1e-9);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(v[2], 0.5/M_PI, 1e-9);
}

void AeroElementTests::testInducedVelocityAboveWithOffset()
{
    AeroElement_ptr el = new AeroElement(SGVec3d(-1., -0.5, 0.),
                                         SGVec3d(0., -0.5, 0.),
                                         SGVec3d(0., 0.5, 0.),
                                         SGVec3d(-1., 0.5, 0.));
    SGVec3d mp = el->getBoundVortexMidPoint();
    SGVec3d v = el->getInducedVelocity(mp+SGVec3d(0.0, 0.5, -1.0));
    CPPUNIT_ASSERT_DOUBLES_EQUAL(v[0], -1.0/(4.0*M_PI*sqrt(2.0)), 1e-9);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(v[1], -0.125/M_PI, 1e-9);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(v[2], 0.125/M_PI, 1e-9);
}

void AeroElementTests::testInducedVelocityUpstream()
{
    AeroElement_ptr el = new AeroElement(SGVec3d(-1., -0.5, 0.),
                                         SGVec3d(0., -0.5, 0.),
                                         SGVec3d(0., 0.5, 0.),
                                         SGVec3d(-1., 0.5, 0.));
    SGVec3d mp = el->getBoundVortexMidPoint();
    SGVec3d v = el->getInducedVelocity(mp+SGVec3d(0.5, 0.0, 0.0));
    CPPUNIT_ASSERT_DOUBLES_EQUAL(v[0], 0.0, 1e-9);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(v[1], 0.0, 1e-9);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(v[2], (1.0-sqrt(2.0))/M_PI, 1e-9);
}
