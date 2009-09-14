#include <string>
#include "Debug/logstream.hxx"

int
main( int argc, char* argv[] )
{
    fglog().setLogLevels( FG_ALL, FG_INFO );

    FG_LOG( FG_TERRAIN, FG_BULK,  "terrain::bulk" ); // shouldnt appear
    FG_LOG( FG_TERRAIN, FG_DEBUG, "terrain::debug" ); // shouldnt appear
    FG_LOG( FG_TERRAIN, FG_INFO,  "terrain::info" );
    FG_LOG( FG_TERRAIN, FG_WARN,  "terrain::warn" );
    FG_LOG( FG_TERRAIN, FG_ALERT, "terrain::alert" );

    int i = 12345;
    long l = 54321L;
    double d = 3.14159;
    string s = "Hello world!";

    FG_LOG( FG_EVENT, FG_INFO, "event::info "
				 << "i=" << i
				 << ", l=" << l
				 << ", d=" << d
	                         << ", d*l=" << d*l
				 << ", s=\"" << s << "\"" );

    // This shouldn't appear in log output:
    FG_LOG( FG_EVENT, FG_DEBUG, "event::debug "
	    << "- this should be seen - "
	    << "d=" << d
	    << ", s=\"" << s << "\"" );

    return 0;
}
