#include "test_navaids2.hxx"

#include "test_suite/FGTestApi/testGlobals.hxx"
#include "test_suite/FGTestApi/NavDataCache.hxx"

#include <Navaids/NavDataCache.hxx>
#include <Navaids/navrecord.hxx>
#include <Navaids/navlist.hxx>


// Set up function for each test.
void NavaidsTests::setUp()
{
    FGTestApi::setUp::initTestGlobals("navaids2");
    FGTestApi::setUp::initNavDataCache();
}


// Clean up after each test.
void NavaidsTests::tearDown()
{
    FGTestApi::tearDown::shutdownTestGlobals();
}


void NavaidsTests::testBasic()
{
    SGGeod egccPos = SGGeod::fromDeg(-2.27, 53.35);
    FGNavRecordRef tla = FGNavList::findByFreq(115.7, egccPos);

    CPPUNIT_ASSERT_EQUAL(strcmp(tla->get_ident(), "TNT"), 0);
    CPPUNIT_ASSERT(tla->ident() == "TNT");
    CPPUNIT_ASSERT(tla->name() == "TRENT VOR-DME");
    CPPUNIT_ASSERT_EQUAL(tla->get_freq(), 11570);
    CPPUNIT_ASSERT_EQUAL(tla->get_range(), 130);
}
