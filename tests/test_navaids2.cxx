#include "unitTestHelpers.hxx"

#include <simgear/misc/test_macros.hxx>

#include <Navaids/NavDataCache.hxx>
#include <Navaids/navrecord.hxx>
#include <Navaids/navlist.hxx>

void testBasic()
{
    SGGeod egccPos = SGGeod::fromDeg(-2.27, 53.35);
    FGNavRecordRef tla = FGNavList::findByFreq(115.7, egccPos);

    SG_CHECK_EQUAL(strcmp(tla->get_ident(), "TNT"), 0);
    SG_CHECK_EQUAL(tla->ident(), "TNT");
    SG_CHECK_EQUAL(tla->name(), "TRENT VOR-DME");
    SG_CHECK_EQUAL(tla->get_freq(), 11570);
    SG_CHECK_EQUAL(tla->get_range(), 130);
    
}

int main(int argc, char* argv[])
{
  fgtest::initTestGlobals("navaids2");

  testBasic();

  fgtest::shutdownTestGlobals();
}
