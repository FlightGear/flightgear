// do some non-destructive read tests relating text files

#include <stdio.h>
#include <string.h>

static void readchars( FILE *fd ) {
    int c;
    while ( (c = fgetc(fd)) != EOF ) {
	printf("c = %d", c);
	if ( c == 10 ) {
	    printf(" LF \\n");
	}
	if ( c == 13 ) {
	    printf(" CR \\r");
	}
	printf("\n");
    }
}


static void readlines( FILE *fd ) {
    char line[256];
    while ( fgets(line, 255, fd) != NULL ) {
	int len = strlen(line);
	printf("line = %s", line);
	if ( len >= 2 ) {
	    printf("  char[n - 1] = %d  char[n] = %d\n",
		   line[len-2], line[len-1]);
	} else if ( len >= 1 ) {
	    printf("  char[n] = %d\n",line[len-1]);
	} else {
	    printf("empty string\n");
	}
    }
}


int main( int argc, char **argv ) {
    // check usage
    if ( argc != 2 ) {
	printf("usage: %s file\n", argv[0]);
	return -1;
    }

    char file[256];
    strcpy( file, argv[1] );
    FILE *fd;

    // open a file in (default) text mode
    printf("TEXT MODE (DEFAULT) by character\n\n");
    fd = fopen( file, "r" );
    if ( fd != NULL ) {
	readchars( fd );
	fclose(fd);
    } else {
	printf("Cannot open %s\n", file);
    }
    printf("\n");

    // open a file in (explicit) text mode
    printf("TEXT MODE (EXPLICIT) by character\n\n");
    fd = fopen( file, "rt" );
    if ( fd != NULL ) {
	readchars( fd );
	fclose(fd);
    } else {
	printf("Cannot open %s\n", file);
    }
    printf("\n");

    // open a file in (explicit) binary mode
    printf("BINARY MODE (EXPLICIT) by character\n\n");
    fd = fopen( file, "rb" );
    if ( fd != NULL ) {
	readchars( fd );
	fclose(fd);
    } else {
	printf("Cannot open %s\n", file);
    }
    printf("\n");

    // open a file in (default) text mode
    printf("TEXT MODE (DEFAULT) by line\n\n");
    fd = fopen( file, "r" );
    if ( fd != NULL ) {
	readlines( fd );
	fclose(fd);
    } else {
	printf("Cannot open %s\n", file);
    }
    printf("\n");

    // open a file in (explicit) text mode
    printf("TEXT MODE (EXPLICIT) by line\n\n");
    fd = fopen( file, "rt" );
    if ( fd != NULL ) {
	readlines( fd );
	fclose(fd);
    } else {
	printf("Cannot open %s\n", file);
    }
    printf("\n");

    // open a file in (explicit) binary mode
    printf("BINARY MODE (EXPLICIT) by line\n\n");
    fd = fopen( file, "rb" );
    if ( fd != NULL ) {
	readlines( fd );
	fclose(fd);
    } else {
	printf("Cannot open %s\n", file);
    }
    printf("\n");

    return 0;
}
