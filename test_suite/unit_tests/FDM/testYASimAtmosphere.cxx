#include "test_suite/FGTestApi/PrivateAccessorFDM.hxx"

#include "testYASimAtmosphere.hxx"

#include <FDM/YASim/Math.hpp>

#include <simgear/debug/logstream.hxx>


using namespace yasim;

void YASimAtmosphereTests::setUp()
{
    a.reset(new Atmosphere());
}


void YASimAtmosphereTests::tearDown()
{
    a.reset();
}


void YASimAtmosphereTests::testAtmosphere()
{
    auto accessor = FGTestApi::PrivateAccessor::FDM::Accessor();

    int numColumns = accessor.read_FDM_YASim_Atmosphere_numColumns(a);
    int maxTableIndex = a->maxTableIndex();
    int rows = maxTableIndex + 1;
    const float maxDeviation = 0.0002f;

    SG_LOG(SG_GENERAL, SG_INFO, "Columns = " << numColumns);
    SG_LOG(SG_GENERAL, SG_INFO, "Rows = " << rows);

    for (int alt = 0; alt <= maxTableIndex; alt++) {
        float density = a->calcStdDensity(accessor.read_FDM_YASim_Atmosphere_data(a, alt, a->PRESSURE), accessor.read_FDM_YASim_Atmosphere_data(a, alt, a->TEMPERATURE));
        float delta = accessor.read_FDM_YASim_Atmosphere_data(a, alt, a->DENSITY) - density;
        SG_LOG(SG_GENERAL, SG_INFO, "alt: " << alt << ", delta: " << delta);
        if (Math::abs(delta) > maxDeviation)
            CPPUNIT_FAIL("Deviation above limit of 0.0002");
    }
    SG_LOG(SG_GENERAL, SG_INFO, "Deviation below " << maxDeviation << " for all rows.");
}
