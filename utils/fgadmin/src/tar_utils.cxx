#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <tar.h>

#include <libtar.h>
#include <zlib.h>

#include "tar_utils.hxx"


int gzopen_frontend( char *pathname, int oflags, int mode ) {
    char *gzoflags;
    gzFile gzf;
    int fd;

    switch (oflags & O_ACCMODE)
        {
        case O_WRONLY:
            gzoflags = "wb";
            break;
        case O_RDONLY:
            gzoflags = "rb";
            break;
        default:
        case O_RDWR:
            errno = EINVAL;
            return -1;
        }

    fd = open(pathname, oflags, mode);
    if (fd == -1)
        return -1;

    if ((oflags & O_CREAT) && fchmod(fd, mode))
        return -1;

    gzf = gzdopen(fd, gzoflags);
    if (!gzf) {
        errno = ENOMEM;
        return -1;
    }

    return (int)gzf;
}


tartype_t gztype = {
    (openfunc_t) gzopen_frontend,
    (closefunc_t) gzclose,
    (readfunc_t) gzread,
    (writefunc_t) gzwrite
};


// list the contents of the specified tar file
bool tarlist( char *tarfile, char *destdir, bool verbose ) {
    TAR *t;
    int i;

    int tarflags = TAR_GNU;
    if ( verbose ) {
        tarflags |= TAR_VERBOSE;
    }

    if ( tar_open( &t, tarfile, &gztype, O_RDONLY, 0, tarflags ) == -1) {
        fprintf(stderr, "tar_open(): %s\n", strerror(errno));
        return -1;
    }

    while ((i = th_read(t)) == 0) {
        th_print_long_ls(t);
#ifdef DEBUG
        th_print(t);
#endif
        if (TH_ISREG(t) && tar_skip_regfile(t) != 0) {
            fprintf(stderr, "tar_skip_regfile(): %s\n",
                    strerror(errno));
            return -1;
        }
    }

#ifdef DEBUG
    printf( "th_read() returned %d\n", i);
    printf( "EOF mark encountered after %ld bytes\n",
            gzseek((gzFile) t->fd, 0, SEEK_CUR)
            );
#endif

    if (tar_close(t) != 0) {
        fprintf(stderr, "tar_close(): %s\n", strerror(errno));
        return -1;
    }

    return 0;
}


// extract the specified tar file into the specified destination
// directory
int tarextract( char *tarfile, char *destdir, bool verbose ) {
    TAR *t;

#ifdef DEBUG
    puts("opening tarfile...");
#endif

    int tarflags = TAR_GNU;
    if ( verbose ) {
        tarflags |= TAR_VERBOSE;
    }

    if ( tar_open(&t, tarfile, &gztype, O_RDONLY, 0, tarflags) == -1 ) {
        fprintf(stderr, "tar_open(): %s\n", strerror(errno));
        return -1;
    }

#ifdef DEBUG
    puts("extracting tarfile...");
#endif
    if (tar_extract_all(t, destdir) != 0) {
        fprintf(stderr, "tar_extract_all(): %s\n", strerror(errno));
        return -1;
    }

#ifdef DEBUG
    puts("closing tarfile...");
#endif
    if (tar_close(t) != 0) {
        fprintf(stderr, "tar_close(): %s\n", strerror(errno));
        return -1;
    }

    return 0;
}
