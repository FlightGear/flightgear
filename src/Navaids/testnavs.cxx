#include <simgear/misc/fgpath.hxx>

#include "ilslist.hxx"
#include "navlist.hxx"

int main() {
    double heading, dist;

    FGNavList navs;
    FGPath p_nav( "/home/curt/FlightGear/Navaids/default.nav" );
    navs.init( p_nav );
    FGNav n;
    if ( navs.query( -93.2, 45.14, 3000, 117.30,
		     &n, &heading, &dist) ) {
	cout << "Found a vor station in range" << endl;
	cout << " id = " << n.get_ident() << endl;
	cout << " heading = " << heading << " dist = " << dist << endl;
    } else {
	cout << "not picking up vor. :-(" << endl;
    }

    FGILSList ilslist;
    FGPath p_ils( "/home/curt/FlightGear/Navaids/default.ils" );
    ilslist.init( p_ils );
    FGILS i;
    if ( ilslist.query( -93.1, 45.24, 3000, 110.30,
			&i, &heading, &dist) ) {
	cout << "Found an ils station in range" << endl;
	cout << " apt = " << i.get_aptcode() << endl;
	cout << " rwy = " << i.get_rwyno() << endl;
	cout << " heading = " << heading << " dist = " << dist << endl;
    } else {
	cout << "not picking up ils. :-(" << endl;
    }
}
