#include <string>

using std::string;

// list the contents of the specified tar file
bool tarlist( char *tarfile, char *destdir, bool verbose );

// extract the specified tar file into the specified destination
// directory
int tarextract( char *tarfile, char *destdir, bool verbose );
