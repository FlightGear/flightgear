// dump out a gdbm version of the simple airport file

#include <simgear/compiler.h>

#include STL_IOSTREAM

#include <simgear/debug/logstream.hxx>

#include "runways.hxx"

#if !defined(SG_HAVE_NATIVE_SGI_COMPILERS)
SG_USING_STD(cout);
SG_USING_STD(endl);
#endif

int main( int argc, char **argv ) {
    FGRunwaysUtil runways;
    FGRunway r;

    sglog().setLogLevels( SG_ALL, SG_INFO );

    if ( argc == 3 ) {
	runways.load( argv[1] );
	runways.dump_mk4( argv[2] );    
    } else {
	cout << "usage: " << argv[0] << " <in> <out>" << endl;
	exit(-1);
    }

    cout << endl;

    FGRunways runway_db( argv[2] );

#if 0
    while ( runway_db.next( &r ) ) {
	cout << r.id << " " << r.rwy_no << endl;
    }
#endif

    runway_db.search( "KMSP", &r );
	
    while ( r.id == "KMSP" ) {
	cout << r.id << " " << r.rwy_no << endl;
	runway_db.next( &r );
    }


}
