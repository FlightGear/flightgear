/*
 * fgcom - VoIP-Client for the FlightGear-Radio-Infrastructure
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA  02110-1301, USA.
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <sys/types.h>
#include <sys/stat.h>
#ifdef _MSC_VER
#include <Windows.h>
#else /* !_MSC_VER */
#include <unistd.h>
#include <stdint.h>
#endif /* _MSC_VER y/n */
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#ifdef __APPLE__
#  include <CoreFoundation/CoreFoundation.h>
#  include <mach-o/dyld.h> /* for _NSGetExecutablePath() */
#endif

#ifndef bcopy
#define bcopy(from, to, n) memcpy(to, from, n)
#endif

static int s_index;
static int s_file_handle;
static char *s_content;
static int s_size;

/**
 *
 * \fn void parser_init(void)
 *
 * \brief Starts parser initialization.
 *
 */
int parser_init(const char *filename)
{
	struct stat l_stat;
	ssize_t l_nbytes;
	int l_status;
    int oflag = O_RDONLY;
#ifdef _MSC_VER
    oflag |= _O_BINARY; /* if comparing to stat size then must be binary */
#endif

	s_index = 0;

	if((s_file_handle = open(filename, oflag)) < 0)
		return (s_file_handle);

	fstat(s_file_handle, &l_stat);

	l_status = -1;
	if((s_content = (char *)malloc((l_stat.st_size + 1) * sizeof(char))) != NULL)
	{
		if((l_nbytes = read(s_file_handle, s_content, l_stat.st_size)) == l_stat.st_size)
		{
			l_status = 0;
			s_size = l_stat.st_size;
		}
	}
	close(s_file_handle);

	return(l_status);
}

/**
 *
 * \fn void parser_exit(void)
 *
 * \brief Exits parser.
 *
 */
void parser_exit(void)
{
	free(s_content);
}

/**
 *
 * \fn int parser_get_next_value(const char *line, float *value)
 *
 * \brief Extract a numeric value.
 *
 * \param line	pointer on the line extracted from the input file.
 * \param value	pointer on the returned value.
 *
 * \return Returns 0 if value successfully extracted. Otherwhise, returns
 * a negative value meaning that an error occured.
 *
 */
int parser_get_next_value(double *value)
{
	int l_status = 0;
	unsigned int l_j;
	unsigned int l_size;
	char *l_buf;

	/* Check if we are already at the end of the string. */
	if(s_index >= s_size)
		return(-1);

	/* Enter main parser loop. */
	while((s_index < s_size) && (l_status == 0))
	{
		/* Search for something different than an espace or tab. */
		while(  (s_content[s_index] == ' ' || s_content[s_index] == '\t') &&
				(s_index < s_size) )
			s_index++;

		/* If we have reached end of file, we exit now. */
		if (s_index >= s_size)
			return(-1);

		/* If character is a CR, we restart for next line. */
		if(s_content[s_index] == '\n')
		{
			s_index++;
			continue;
		}

		/* Is it a comment ? */
		if(s_content[s_index] == '#')
		{
			/* Yes, go until end of line. */
			while((s_content[s_index] != '\n') && (s_index < s_size))
				s_index++;
		}
		else
		{
			/* We have found something that is not a comment. */
			while((s_content[s_index] < '0' || s_content[s_index] > '9') && (s_index < s_size))
				s_index++;

			if(s_index < s_size)
			{
				l_j = s_index + 1;
				while(  ((s_content[l_j] >= '0' && s_content[l_j] <= '9') ||
						 (s_content[l_j] == '.' || s_content[l_j] == ',')) &&
						((s_content[l_j] != '\n') && (l_j < (unsigned int)s_size)) )
					l_j++;

				l_size = l_j - s_index + 1;
				if((l_buf = (char *)malloc(l_size * sizeof(char))) != NULL)
				{
					/* Initialize buffer with O. */
					memset((void *)l_buf, 0, l_size);
					bcopy((const void *)(s_content + s_index), (void *)l_buf, l_size - 1);
					/* Convert string into double. */
					*value = atof(l_buf);
					/* Buffer is not needed any longer. */
					free(l_buf);
					/* Prepare for next value. */
					s_index = l_j + 1;
					break;
				}
			}
		}
	} /* while((s_index < s_size) && (l_status == 0)) */

	return(0);
}

/* cross-platform stat and check if directory or file
 * return 2 == directory
 * return 1 == file
 * return 0 == neither
 * NOTE: Think this fails in Windows if 
 *       the path has a trailing path separator
 *       and in some cases even if the path contains a forward (unix) separator
 * TODO: Should copy the path, and fix it for stat
 */

#ifdef _MSC_VER
#define M_ISDIR(a) (a & _S_IFDIR)
#else
#define M_ISDIR S_ISDIR
#endif

int is_file_or_directory( const char * path )
{
    struct stat buf;
    if ( stat(path,&buf) == 0 ) {
        if (M_ISDIR(buf.st_mode))
            return 2;
        return 1;
    }
    return 0;
}

/* trim to base binary path IN BUFFER,
 * essentially removing the executable name
 */
void trim_base_path_ib( char *path )
{
    size_t len = strlen(path);
    size_t i, off;
    int c;
    off = 0;
    for (i = 0; i < len; i++) {
        c = path[i];
        if (( c == '/' ) || ( c == '\\')) {
            off = i + 1;    // get after separator
#ifdef _MSC_VER
            if ( c == '/' )
                path[i] = '\\';
#endif // _MSC_VER
        }
    }
    path[off] = 0;
}

/*
 * get data path per OS 
 * In Windows and OSX the compiler only supplies a partial data file path,
 *  so this is to get the current binary installed path.
 * In *nix the full path is supplied, so this does nothing, except zero the path
 *
 */
int get_data_path_per_os( char *path, size_t len )
{
#if defined(MACOSX)
// if we're running as part of an application bundle, return the bundle's
// resources directory
// The following code looks for the base package inside the application
// bundle, in the standard Contents/Resources location.
    
  CFURLRef resourcesUrl = CFBundleCopyResourcesDirectoryURL(CFBundleGetMainBundle());
  if (resourcesUrl) {
      CFURLGetFileSystemRepresentation(resourcesUrl, true, (UInt8*) path, len);
      CFRelease(resourcesUrl);
      // append trailing seperator since CF doesn't
      len = strlen(path);
      path[len] = '/';
      path[len+1] = 0;
      return 0;
  }
  
// we're unbundled, simply return the executable path
    unsigned int size = (unsigned int) len;
    if (_NSGetExecutablePath(path, &size) == 0)  {
        // success
        trim_base_path_ib(path);
    } else {
        printf("ERROR: path buffer too small; need size %u\n", size);
        return 1;
    }
#elif defined(_MSC_VER)
    unsigned int size = GetModuleFileName(NULL,path, len);
    if (size && (size != len)) {
        // success
        trim_base_path_ib(path);
    } else {
        if (size) {
            printf("ERROR:GetModuleFileName: path buffer too small; need size more than %u\n", len);
        } else {
            printf("ERROR:GetModuleFileName: FAILED!\n");
        }
        return 1;
    }
#else
    // strcpy(path,"/usr/local/shared/");
    path[0] = 0;
#endif // MACOSX | _MSC_VER | others   
    return 0;
}


/* eof - utils.cpp */


