// dump out a gdbm version of the simple airport file

#include <simgear/compiler.h>

#include STL_IOSTREAM

#include <simgear/debug/logstream.hxx>

#include "simple.hxx"

#if !defined(SG_HAVE_NATIVE_SGI_COMPILERS)
SG_USING_STD(cout);
SG_USING_STD(endl);
#endif

int main( int argc, char **argv ) {
    FGAirportsUtil airports;
    FGAirport a;

    sglog().setLogLevels( SG_ALL, SG_INFO );

    if ( argc == 3 ) {
	airports.load( argv[1] );
	airports.dump_mk4( argv[2] );    
    } else {
	cout << "usage: " << argv[0] << " <in> <out>" << endl;
    }

    // FGAirports airport_db( argv[2] );
    // airport_db.search( "KANEZZZ", &a );

}
