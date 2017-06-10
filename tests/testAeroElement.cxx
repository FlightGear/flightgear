#include <simgear/constants.h>
#include <simgear/misc/test_macros.hxx>
#include <simgear/structure/SGSharedPtr.hxx>
#include <simgear/math/SGVec3.hxx>

#include "FDM/AIWake/AeroElement.hxx"

void testNormal()
{
    AeroElement_ptr el = new AeroElement(SGVec3d(-1., -0.5, 0.),
                                         SGVec3d(0., -0.5, 0.),
                                         SGVec3d(0., 0.5, 0.),
                                         SGVec3d(-1., 0.5, 0.));
    SGVec3d n = el->getNormal();
    SG_CHECK_EQUAL_EP(n[0], 0.0);
    SG_CHECK_EQUAL_EP(n[1], 0.0);
    SG_CHECK_EQUAL_EP(n[2], -1.0);
}

void testCollocationPoint()
{
    AeroElement_ptr el = new AeroElement(SGVec3d(-1., -0.5, 0.),
                                         SGVec3d(0., -0.5, 0.),
                                         SGVec3d(0., 0.5, 0.),
                                         SGVec3d(-1., 0.5, 0.));
    SGVec3d cp = el->getCollocationPoint();
    SG_CHECK_EQUAL_EP(cp[0], -0.75);
    SG_CHECK_EQUAL_EP(cp[1], 0.0);
    SG_CHECK_EQUAL_EP(cp[2], 0.0);
}

void testBoundVortexMidPoint()
{
    AeroElement_ptr el = new AeroElement(SGVec3d(-1., -0.5, 0.),
                                         SGVec3d(0., -0.5, 0.),
                                         SGVec3d(0., 0.5, 0.),
                                         SGVec3d(-1., 0.5, 0.));
    SGVec3d mp = el->getBoundVortexMidPoint();
    SG_CHECK_EQUAL_EP(mp[0], -0.25);
    SG_CHECK_EQUAL_EP(mp[1], 0.0);
    SG_CHECK_EQUAL_EP(mp[2], 0.0);
}

void testBoundVortex()
{
    AeroElement_ptr el = new AeroElement(SGVec3d(-1., -0.5, 0.),
                                         SGVec3d(0., -0.5, 0.),
                                         SGVec3d(0., 0.5, 0.),
                                         SGVec3d(-1., 0.5, 0.));
    SGVec3d v = el->getBoundVortex();
    SG_CHECK_EQUAL_EP(v[0], 0.0);
    SG_CHECK_EQUAL_EP(v[1], 1.0);
    SG_CHECK_EQUAL_EP(v[2], 0.0);
}

void testInducedVelocityOnBoundVortex()
{
    AeroElement_ptr el = new AeroElement(SGVec3d(-1., -0.5, 0.),
                                         SGVec3d(0., -0.5, 0.),
                                         SGVec3d(0., 0.5, 0.),
                                         SGVec3d(-1., 0.5, 0.));
    SGVec3d mp = el->getBoundVortexMidPoint();
    SGVec3d v = el->getInducedVelocity(mp);
    SG_CHECK_EQUAL_EP(v[0], 0.0);
    SG_CHECK_EQUAL_EP(v[1], 0.0);
    SG_CHECK_EQUAL_EP(v[2], 1.0/M_PI);
}

void testInducedVelocityOnCollocationPoint()
{
    AeroElement_ptr el = new AeroElement(SGVec3d(-1., -0.5, 0.),
                                         SGVec3d(0., -0.5, 0.),
                                         SGVec3d(0., 0.5, 0.),
                                         SGVec3d(-1., 0.5, 0.));
    SGVec3d cp = el->getCollocationPoint();
    SGVec3d v = el->getInducedVelocity(cp);
    SG_CHECK_EQUAL_EP(v[0], 0.0);
    SG_CHECK_EQUAL_EP(v[1], 0.0);
    SG_CHECK_EQUAL_EP(v[2], (1.0+sqrt(2.0)/M_PI));
}

void testInducedVelocityAtFarField()
{
    AeroElement_ptr el = new AeroElement(SGVec3d(-1., -0.5, 0.),
                                         SGVec3d(0., -0.5, 0.),
                                         SGVec3d(0., 0.5, 0.),
                                         SGVec3d(-1., 0.5, 0.));
    SGVec3d mp = el->getBoundVortexMidPoint();
    SGVec3d v = el->getInducedVelocity(mp+SGVec3d(-1000.,0.,0.));
    SG_CHECK_EQUAL_EP(v[0], 0.0);
    SG_CHECK_EQUAL_EP(v[1], 0.0);
    SG_CHECK_EQUAL_EP(v[2], 2.0/M_PI);
}

void testInducedVelocityAbove()
{
    AeroElement_ptr el = new AeroElement(SGVec3d(-1., -0.5, 0.),
                                         SGVec3d(0., -0.5, 0.),
                                         SGVec3d(0., 0.5, 0.),
                                         SGVec3d(-1., 0.5, 0.));
    SGVec3d mp = el->getBoundVortexMidPoint();
    SGVec3d v = el->getInducedVelocity(mp+SGVec3d(0.,0.,-0.5));
    SG_CHECK_EQUAL_EP(v[0], -1.0/(sqrt(2.0)*M_PI));
    SG_CHECK_EQUAL_EP(v[1], 0.0);
    SG_CHECK_EQUAL_EP(v[2], 0.5/M_PI);
}

void testInducedVelocityAboveWithOffset()
{
    AeroElement_ptr el = new AeroElement(SGVec3d(-1., -0.5, 0.),
                                         SGVec3d(0., -0.5, 0.),
                                         SGVec3d(0., 0.5, 0.),
                                         SGVec3d(-1., 0.5, 0.));
    SGVec3d mp = el->getBoundVortexMidPoint();
    SGVec3d v = el->getInducedVelocity(mp+SGVec3d(0.0, 0.5, -1.0));
    SG_CHECK_EQUAL_EP(v[0], -1.0/(4.0*M_PI*sqrt(2.0)));
    SG_CHECK_EQUAL_EP(v[1], -0.125/M_PI);
    SG_CHECK_EQUAL_EP(v[2], 0.125/M_PI);
}

void testInducedVelocityUpstream()
{
    AeroElement_ptr el = new AeroElement(SGVec3d(-1., -0.5, 0.),
                                         SGVec3d(0., -0.5, 0.),
                                         SGVec3d(0., 0.5, 0.),
                                         SGVec3d(-1., 0.5, 0.));
    SGVec3d mp = el->getBoundVortexMidPoint();
    SGVec3d v = el->getInducedVelocity(mp+SGVec3d(0.5, 0.0, 0.0));
    SG_CHECK_EQUAL_EP(v[0], 0.0);
    SG_CHECK_EQUAL_EP(v[1], 0.0);
    SG_CHECK_EQUAL_EP(v[2], (1.0-sqrt(2.0))/M_PI);
}

int main(int argc, char* argv[])
{
    testNormal();
    testCollocationPoint();
    testBoundVortexMidPoint();
    testBoundVortex();
    testInducedVelocityOnBoundVortex();
    testInducedVelocityAtFarField();
    testInducedVelocityAbove();
    testInducedVelocityAboveWithOffset();
    testInducedVelocityUpstream();
}
