#include <string>

#include "serial.hxx"

main () {
    fgSERIAL port( "/dev/ttyS1", 4800);
    string value;

    port.write_port("ATDT 626-9800\n");

    while ( true ) {
	value = port.read_port();
	if ( value.length() ) {
	    cout << "-> " << value << endl;
	}
    }
}
