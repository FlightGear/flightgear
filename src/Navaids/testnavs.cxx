#include <simgear/misc/fgpath.hxx>

#include "navaids.hxx"

int main() {
    FGNavAids navs;

    FGPath p( "/home/curt/FlightGear/Navaids/default.nav.gz" );
   
    navs.init( p );
}
