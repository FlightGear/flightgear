#include <Debug/fg_debug.h>
#include <Include/general.h>
#include "airports.hxx"

fgGENERAL general;

main() {
    fgAIRPORTS a;
    fgAIRPORT air;

    general.root_dir = getenv("FG_ROOT");

    fgInitDebug();

    a.load("Airports");

    air = a.search("P13");

    printf("%s %lf %lf %lf\n", air.id, 
	   air.longitude, air.latitude, air.elevation);
}
