
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

#include <stdio.h>
#include <sys/stat.h> // stat()
#include <unistd.h>   // stat()

#include <string>

#include <Bucket/bucketutils.h>



#ifdef WIN32
#ifndef TRUE
 #define FALSE 0
 #define TRUE 1
#endif

char* PathDivider()
{
	return "\\";
} // PathDivider

void ReplaceDivider( char* path )
{
	char	div = PathDivider()[0];
	int		i;

	if ( ! path )
		return;
	if ( div == '/' )
		return;

	for ( i = 0; path[i]; i++ )
		if ( path[i] == '/' )
			path[i] = div;

} // ReplaceDivider

int	Exists( char* path )
{
    struct stat statbuff;

	ReplaceDivider( path );
	if ( path[strlen( path ) - 1] == ':' )
		return TRUE;
	if ( _stat( path, &statbuff ) != 0 )
		return FALSE;
	return TRUE;
} // Exists


void CreateDir( char* path )
{
	if ( ! path  ||  ! strlen( path ) )
		return;
	ReplaceDivider( path );
								// see if the parent exists yet
	int		i;	// looping index
	string	parent;	// path to parent

	parent =  path;
	for ( i = strlen( parent.c_str() )-1; i >= 0; i-- )
		if ( parent[i] == PathDivider()[0] )
		{
			parent[i] = '\0';
			break;
		}
	
	if ( ! Exists( parent.c_str() ) )
	{
		CreateDir( parent.c_str() );
	}

	if ( ! Exists( path ) )
	{
		if (mkdir(path, S_IRWXU) != 0 )
		{
			cout << "Could not create directory " << path << endl;
		}else{
			cout << "CreateDir: " << path << endl;
		}
	}
	
} // CreateDir


int main(int argc, char **argv)
{
	string root;

	if(argc != 2)
	{
		cout << "Makedir failed needs one argument\n";
		return(10);
	}
	root = argv[1];
	
	CreateDir(root.c_str());
	
	return(0);
}
#endif // WIN32

