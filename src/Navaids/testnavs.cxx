#include <simgear/misc/fgpath.hxx>

#include "fixlist.hxx"
#include "ilslist.hxx"
#include "navlist.hxx"

int main() {
    double heading, dist;

    current_navlist = new FGNavList;
    FGPath p_nav( "/home/curt/FlightGear/Navaids/default.nav" );
    current_navlist->init( p_nav );
    FGNav n;
    if ( current_navlist->query( -93.2, 45.14, 3000, 117.30, &n) ) {
	cout << "Found a vor station in range" << endl;
	cout << " id = " << n.get_ident() << endl;
    } else {
	cout << "not picking up vor. :-(" << endl;
    }

    current_ilslist = new FGILSList;
    FGPath p_ils( "/home/curt/FlightGear/Navaids/default.ils" );
    current_ilslist->init( p_ils );
    FGILS i;
    if ( current_ilslist->query( -93.1, 45.24, 3000, 110.30, &i) ) {
	cout << "Found an ils station in range" << endl;
	cout << " apt = " << i.get_aptcode() << endl;
	cout << " rwy = " << i.get_rwyno() << endl;
    } else {
	cout << "not picking up ils. :-(" << endl;
    }

    current_fixlist = new FGFixList;
    FGPath p_fix( "/home/curt/FlightGear/Navaids/default.fix" );
    current_fixlist->init( p_fix );
    FGFix fix;
    if ( current_fixlist->query( "SHELL", -82, 41, 3000,
				 &fix, &heading, &dist) ) {
	cout << "Found a matching fix" << endl;
	cout << " id = " << fix.get_ident() << endl;
	cout << " heading = " << heading << " dist = " << dist << endl;
    } else {
	cout << "did not find fix. :-(" << endl;
    }
}
