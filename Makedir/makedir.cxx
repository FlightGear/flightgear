#include <ctype.h>    // isspace()
#include <stdlib.h>   // atoi()
#include <math.h>     // rint()
#include <stdio.h>
#include <string.h>
#include <sys/stat.h> // stat()
#include <unistd.h>   // stat()

#include <string>

#include <Bucket/bucketutils.h>

#ifdef __CYGWIN32__
#  define MKDIR(a) mkdir(a,S_IRWXU)     // I am just guessing at this flag
(NHV)
#endif // __CYGWIN32__

#ifdef __CYGWIN32__

// return the file path name ( foo/bar/file.ext = foo/bar )
static void extract_path (char *in, char *base) {
    int len, i;

    len = strlen (in);
    strcpy (base, in);

    i = len - 1;
    while ( (i >= 0) && (in[i] != '/') ) {
	i--;
    }

    base[i] = '\0';
}


// Make a subdirectory
static int my_mkdir (char *dir) {
    struct stat stat_buf;
    int result;

    printf ("mk_dir() ");

    result = stat (dir, &stat_buf);

    if (result != 0) {
	MKDIR (dir);
	result = stat (dir, &stat_buf);
	if (result != 0) {
	    printf ("problem creating %s\n", dir);
	} else {
	    printf ("%s created\n", dir);
	}
    } else {
	printf ("%s already exists\n", dir);
    }

    return (result);
}

#endif // __CYGWIN32__


void scenery_dir( const string& dir ) {
    struct stat stat_buf;
    char base_path[256], file[256], exfile[256];
#ifdef __CYGWIN32__
    char tmp_path[256];
#endif
    string command;
    FILE *fd;
	fgBUCKET p;
	int result;

    cout << "Dir = " + dir + "\n";

    // stat() directory and create if needed
    result = stat(dir.c_str(), &stat_buf);
    if ( result != 0 ) {
	cout << "Stat error need to create directory\n";

#ifndef __CYGWIN32__

	command = "mkdir -p " + dir + "\n";
	system( command.c_str() );

#else // __CYGWIN32__

	// Cygwin crashes when trying to output to node file
	// explicitly making directory structure seems OK on Win95

	extract_path (base_path, tmp_path);

	dir = tmp_path;
	if (my_mkdir ( dir.c_str() )) {	exit (-1); }

	dir = base_path;
	if (my_mkdir ( dir.c_str() )) {	exit (-1); }

#endif // __CYGWIN32__

    } else {
	// assume directory exists
		cout << "  Allready Exists !\n";
    }
}

int main(int argc, char **argv)
{
	string root;

	if(argc != 2)
	{
		cout << "Makedir failed\n";
		return(10);
	}
	root = argv[1];
	scenery_dir(root);

	return(0);
}
