// dump out a gdbm version of the simple airport file

#include "simple.hxx"

int main( int argc, char **argv ) {
    FGAirportsUtil airports;
    FGAirport a;

    if ( argc == 3 ) {
	airports.load( argv[1] );
	airports.dump_gdbm( argv[2] );    
    } else {
	cout << "usage: " << argv[0] << " <in> <out>" << endl;
    }
}
