// list the contents of the specified tar file
// bool tarlist( char *tarfile, char *destdir, bool verbose );

// extract the specified tar file into the specified destination
// directory
#ifdef __cplusplus
extern "C" {
#endif

void tarextract( char *tarfile, char *destdir, int verbose, void (*step)(void*,int), void *data );

#ifdef __cplusplus
}
#endif

