#include <simgear/misc/fgpath.hxx>

#include "navlist.hxx"

int main() {
    FGNavList navs;

    FGPath p( "/export/data2/curt/FlightGear/Navaids/default.nav" );
   
    navs.init( p );

    FGNav n;
    double heading, dist;
    if ( navs.query( -93.2, 45.14, 3000, 117.30,
		     &n, &heading, &dist) ) {
	cout << "Found a station in range" << endl;
	cout << " id = " << n.get_ident() << endl;
	cout << " heading = " << heading << " dist = " << dist << endl;
    } else {
	cout << "not picking anything up. :-(" << endl;
    }
}
