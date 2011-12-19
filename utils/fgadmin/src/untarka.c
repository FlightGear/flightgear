#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#define DUMMY \
  WFLAGS="-ansi -pedantic -W -Wall -Wstrict-prototypes -Wtraditional -Wnested-externs -Winline -Wpointer-arith -Wbad-function-cast -Wcast-qual -Wmissing-prototypes -Wmissing-declarations -Wunused"; \
  set -ex; \
  gcc -s -O2 $WFLAGS -DHAVE_ZLIB -DHAVE_BZ2LIB untarka.c -o untarka -lz -lbz2; \
  exit 0
/*
 * untarka.c -- Display contents and/or extract file from a tar file (possibly
 * gzip'd, bzip'd or compress'd)
 * by pts@fazekas.hu Wed Jan 29 00:38:31 CET 2003
 * Z compiles fine at Wed Jan 29 14:20:32 CET 2003
 * all run file at Wed Jan 29 15:50:39 CET 2003
 * 0.34 Win32 runs fine at Wed Jan 29 16:02:57 GMT 2003
 *
 * Untarka extracts TAR (UNIX Tape ARchive) files without calling any
 * external programs. It supports and autodetects multiple compression
 * methods (.tar, .tar.Z, .tar.gz, .tar.bz2, .Z, .gz and .bz2). UNIX
 * devices, sockets, hard links, symlinks, owners and permissions are
 * ignored during extraction.
 *
 * written by "Pedro A. Aranda Guti\irrez" <paag@tid.es>
 * adaptation to Unix by Jean-loup Gailly <jloup@gzip.org>
 * adaptation to bzip2 by José Fonseca <em96115@fe.up.pt>
 * merged comressors by Szabó Péter <pts@fazekas.hu> at Tue Jan 28 23:40:23 CET 2003
 * compiles on Win32 under MinGW, Linux with gcc.
 *
 * To get gzip (zlib) support, pass to gcc: -DHAVE_ZLIB -lz
 * To get bzip2 (bzlib2) support, pass to gcc: -DHAVE_BZ2LIB -lbz2
 * To get compress (lz) support, relax, since it's the default.
 *
 * Compile memory-checking with checkergcc -g -ansi -pedantic -W -Wall -Wstrict-prototypes -Wtraditional -Wnested-externs -Winline -Wpointer-arith -Wbad-function-cast -Wcast-qual -Wmissing-prototypes -Wmissing-declarations -Wunused untarka.c -o untarka
 */
/* Imp: symlink()
 * Imp: mknod() device, socket
 * Imp: hard link()
 * Imp: chmod()
 * Imp: chown()
 * Imp: allow force non-tar gzipped file
 * Imp: command line options
 * Imp: uncompress ZIP files
 * Dat: zlib is able to read uncompressed files, libbz2 isn't
 * Dat: magic numbers:
 *  257     string          ustar\0         POSIX tar archive
 *  257     string          ustar\040\040\0 GNU tar archive
 *  0       string          BZh             bzip2 compressed data
 *  0       string          \037\213        gzip compressed data
 *  0       string          \037\235        compress'd data
 *  is_tar.c() from file(1)
 */

#undef DOSISH
#ifdef WIN32
#  include <windows.h>
#  define DOSISH 1
#  undef __STRICT_ANSI__
#endif
#ifdef MSDOS
#  define DOSISH 1
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <fcntl.h>

#ifdef DOSISH
# include <direct.h>
# include <io.h>
#else
# include <unistd.h>
#endif

#if DOSISH
#  ifndef F_OK
#    define F_OK (0)
#  endif
#  ifdef _MSC_VER
#    define mkdir(dirname,mode) _mkdir(dirname)
#    define unlink(fn)          _unlink(fn)
#    define access(path,mode)   _access(path,mode)
#  else
#    define mkdir(dirname,mode) _mkdir(dirname)
#  endif
#else
#  include <utime.h>
#  include <sys/stat.h> /* mkdir() */
#endif

#ifndef OF /* function prototypes */
#  if defined(__STDC__) || defined(__PROTOTYPES__) || defined(__cplusplus)
#    define OF(args)  args
#  else
#    define OF(args)  args
#  endif
#endif

#include "untarka.h"

static void error OF((const char *msg));

static unsigned long total_read;

/* #define TAR_GZ 1 */

typedef struct Readable {
  /*private*/ void *f;
  /** @return 0 on success, 1 on failure */
  int (*xOpen4Read)(struct Readable* self, char const* filename);
  /** @return 0 on success */
  int (*xClose)(struct Readable* self);
  /** @return (unsigned)-1 on error */
  unsigned (*xRead)(struct Readable* self, void* buf, unsigned len);
  char const* (*xError)(struct Readable* self, int *errnum_ret);
} Readable;

enum { FMT_U=1, FMT_Z=2, FMT_GZ=3, FMT_BZ2=4 };

/* #define xFILE FILE* */
static int xU_Open4Read(struct Readable* self, char const* filename) { total_read=0; return NULL==(self->f=fopen(filename,"rb")); }
static int xU_Close(struct Readable* self) { return fclose((FILE*)self->f); }
static unsigned xU_Read(struct Readable* self, void* buf, unsigned len) {
  unsigned got=fread(buf,1,len,(FILE*)self->f);
  total_read+=got;
  return got>0 ? got : ferror((FILE*)self->f) ? 0U-1 : 0;
}
static char const* xU_Error(struct Readable* self, int *errnum_ret) { return (*errnum_ret=ferror((FILE*)self->f))?"I/O error":"OK"; }

/* #define xFILE struct lz_reader_t* */
struct lz_reader_t;
static struct lz_reader_t* lz_read_new(FILE* inf);
static int lz_read_init(struct lz_reader_t* reader, FILE* inf);
static int lz_close(struct lz_reader_t* reader);
static unsigned lz_read(struct lz_reader_t* reader, char*buf, unsigned len);

static int xZ_Open4Read(struct Readable* self, char const* filename) {
  FILE *f=fopen(filename,"rb");
  if (NULL==f) return 1;
  return NULL==(self->f=lz_read_new(f));
}
static int xZ_Close(struct Readable* self) {
  int ret=lz_close((struct lz_reader_t*)self->f);
  free((struct lz_reader_t*)self->f);
  return ret;
}
static unsigned xZ_Read(struct Readable* self, void* buf, unsigned len) {
  return lz_read((struct lz_reader_t*)self->f, buf, len);
}
static char const* xZ_Error(struct Readable* self, int *errnum_ret) {
  (void)self;
  *errnum_ret=0;
  return "lzw error";
}

#if HAVE_ZLIB
#include "zlib.h"
/* #define xFILE gzFile */
static int xGZ_Open4Read(struct Readable* self, char const* filename) { total_read=0; return NULL==(self->f=gzopen(filename,"rb")); }
static int xGZ_Close(struct Readable* self) { return gzclose((gzFile)self->f); }
static unsigned xGZ_Read(struct Readable* self, void* buf, unsigned len) { unsigned l=gzread((gzFile)self->f,buf,len); total_read=((z_streamp)self->f)->total_in; return l; }
static char const* xGZ_Error(struct Readable* self, int *errnum_ret) { return gzerror((gzFile)self->f, errnum_ret); }
#endif

#if HAVE_BZ2LIB
#include "bzlib.h"
/* #define xFILE BZFILE* */
static int xBZ2_Open4Read(struct Readable* self, char const* filename) { return NULL==(self->f=BZ2_bzopen(filename,"rb")); }
static int xBZ2_Close(struct Readable* self) { BZ2_bzclose((BZFILE*)self->f); return 0; }
static unsigned xBZ2_Read(struct Readable* self, void* buf, unsigned len) { return BZ2_bzread((BZFILE*)self->f,buf,len); }
static char const* xBZ2_Error(struct Readable* self, int *errnum_ret) { return BZ2_bzerror((BZFILE*)self->f, errnum_ret); }
#endif

/* -- is_tar() */

/*
 * is_tar() -- figure out whether file is a tar archive.
 *
 * Stolen (by the author!) from the public domain tar program:
 * Public Domain version written 26 Aug 1985 John Gilmore (ihnp4!hoptoad!gnu).
 *
 * @(#)list.c 1.18 9/23/86 Public Domain - gnu
 * $Id$
 *
 * Comments changed and some code/comments reformatted
 * for file command by Ian Darwin.
 */

/* The magic field is filled with this if uname and gname are valid. */
#define	TMAGIC		"ustar  "	/* 7 chars and a null */

/*
 * Header block on tape.
 *
 * I'm going to use traditional DP naming conventions here.
 * A "block" is a big chunk of stuff that we do I/O on.
 * A "record" is a piece of info that we care about.
 * Typically many "record"s fit into a "block".
 */
#define	RECORDSIZE	512
#define	NAMSIZ	100
#define	TUNMLEN	32
#define	TGNMLEN	32

union tar_record {
	char		charptr[RECORDSIZE];
	struct header {
		char	name[NAMSIZ];
		char	mode[8];
		char	uid[8];
		char	gid[8];
		char	size[12];
		char	mtime[12];
		char	chksum[8];
		char	linkflag;
		char	linkname[NAMSIZ];
		char	magic[8];
		char	uname[TUNMLEN];
		char	gname[TGNMLEN];
		char	devmajor[8];
		char	devminor[8];
	} header;
};

#define	isodigit(c)	( ((c) >= '0') && ((c) <= '7') )

static int from_oct(int, char *);	/* Decode octal number */

/*
 * Return
 *	0 if the checksum is bad (i.e., probably not a tar archive),
 *	1 for old UNIX tar file,
 *	2 for Unix Std (POSIX) tar file.
 */
static int
is_tar(char *buf, unsigned nbytes)
{
	union tar_record *header = (union tar_record *)buf;
	int	i;
	int	sum, recsum;
	char	*p;

	if (nbytes < sizeof(union tar_record))
		return 0;

	recsum = from_oct(8,  header->header.chksum);

	sum = 0;
	p = header->charptr;
	for (i = sizeof(union tar_record); --i >= 0;) {
		/*
		 * We can't use unsigned char here because of old compilers,
		 * e.g. V7.
		 */
		sum += 0xFF & *p++;
	}

	/* Adjust checksum to count the "chksum" field as blanks. */
	for (i = sizeof(header->header.chksum); --i >= 0;)
		sum -= 0xFF & header->header.chksum[i];
	sum += ' '* sizeof header->header.chksum;

	if (sum != recsum)
		return 0;	/* Not a tar archive */

	if (0==strcmp(header->header.magic, TMAGIC))
		return 2;		/* Unix Standard tar archive */

	return 1;			/* Old fashioned tar archive */
}


/*
 * Quick and dirty octal conversion.
 *
 * Result is -1 if the field is invalid (all blank, or nonoctal).
 */
static int
from_oct(int digs, char *where)
{
	int	value;

	while (*where==' ' || *where=='\f' || *where=='\n' || *where=='\r' || *where=='\t' || *where=='\v') {
		where++;
		if (--digs <= 0)
			return -1;		/* All blank field */
	}
	value = 0;
	while (digs > 0 && isodigit(*where)) {	/* Scan til nonoctal */
		value = (value << 3) | (*where++ - '0');
		--digs;
	}

	if (digs > 0 && *where!='\0' && *where!=' ' && *where!='\f' && *where!='\n' && *where!='\r' && *where!='\t' && *where!='\v')
		return -1;			/* Ended on non-space/nul */

	return value;
}


/* --- */

/** Constructor.
 * @return 0 on success, positive for various failure reasons
 */
static int xOpen4Read(struct Readable *self, char const* filename) {
  unsigned i;
  char buf[RECORDSIZE];
  FILE *f=fopen(filename, "rb");
  if (f==NULL) return 4;
  i=sizeof(buf); while (i--!=0) buf[i]='\0';
  if (5>fread(buf, 1, sizeof(buf), f)) return 2;
  if (0) {
  } else if (buf[0]==0102 && buf[1]==0132 && buf[2]==0150) {
#if HAVE_BZ2LIB
    fclose(f);
    self->xOpen4Read=xBZ2_Open4Read;
    self->xClose=xBZ2_Close;
    self->xRead=xBZ2_Read;
    self->xError=xBZ2_Error;
    if (xBZ2_Open4Read(self, filename)!=0) return 1;
#else
    error("bzip2 compression not compiled in");
#endif
  } else if (buf[0]==0037 && (buf[1]&255)==0213 && (buf[2]&255)<=8) {
#if HAVE_ZLIB
    fclose(f);
    self->xOpen4Read=xGZ_Open4Read;
    self->xClose=xGZ_Close;
    self->xRead=xGZ_Read;
    self->xError=xGZ_Error;
    if (xGZ_Open4Read(self, filename)!=0) return 1;
#else
    error("gzip compression not compiled in");
#endif
  } else if (buf[0]==0037 && (buf[1]&255)==0235) {
    fclose(f);
    self->xOpen4Read=xZ_Open4Read;
    self->xClose=xZ_Close;
    self->xRead=xZ_Read;
    self->xError=xZ_Error;
    if (xZ_Open4Read(self, filename)!=0) return 1;
  } else if ((buf[257]==0165 && buf[258]==0163 && buf[259]==0164
           && buf[260]==0141 && buf[261]==0162
           && (buf[262]==0000 || (buf[262]==0040 && buf[263]==0040 && buf[264]==0))
             ) || is_tar(buf, sizeof(buf))
            ) {
    rewind(f);
    self->f=f; /* shortcut */
    self->xOpen4Read=xU_Open4Read;
    self->xClose=xU_Close;
    self->xRead=xU_Read;
    self->xError=xU_Error;
  } else return 3;
  return 0;
}

/* --- .Z LZ uncompression */

/* code derived from
 * (N)compress42.c - File compression ala IEEE Computer, Mar 1992.
 * Modified by pts@fazekas.hu at Wed Jul 18 17:25:38 CEST 2001.
 *
 * Authors:
 *   Spencer W. Thomas   (decvax!harpo!utah-cs!utah-gr!thomas)
 *   Jim McKie           (decvax!mcvax!jim)
 *   Steve Davies        (decvax!vax135!petsd!peora!srd)
 *   Ken Turkowski       (decvax!decwrl!turtlevax!ken)
 *   James A. Woods      (decvax!ihnp4!ames!jaw)
 *   Joe Orost           (decvax!vax135!petsd!joe)
 *   Dave Mack           (csu@alembic.acs.com)
 *   Peter Jannesen, Network Communication Systems
 *                       (peter@ncs.nl)
 *
 * Revision 4.2.3  92/03/14 peter@ncs.nl
 *   Optimise compress and decompress function and a lot of cleanups.
 *   New fast hash algoritme added (if more than 800Kb available).
 */
#define IBUFSIZ 4096

#define NOFUNCDEF
#undef DOS
#undef COMPATIBLE
#undef DONTPTS /* `#define DONTPTS 1' means original uncompress */
#undef DIRENT
#undef RECURSIVE
#undef SYSDIR
#undef UTIME_H
#undef BYTEORDER
#define BITS 16 /* _must_ be able to handle this many bits, anyway */
#define FAST
#if 1 /* !! */
#  define ASSERT(x) do { if (!(x)) abort(); } while(0)
#else
#  define ASSERT(x)
#endif

#include <stdio.h>
#include <string.h> /* memcpy(), memset() */
#include <stdlib.h> /* abort(), exit() */

/* #define fx_write(a,b,c) fwrite((b),1,(c),(a)) */
#define fx_read(a,b,c)  fread( (b),1,(c),(a))
#define fx_stdin        stdin
#define fx_stdout       stdout
#define fx_type         FILE*

#ifndef NOFUNCDEF
#define    LARGS(a)    ()    /* Relay on include files for libary func defs. */
    extern    void    *malloc LARGS((int));
    extern    void    free    LARGS((void *));
#ifndef _IBMR2
    extern    int        open    LARGS((char const *,int,...));
#endif
    extern    int        close    LARGS((int));
    extern    int        read    LARGS((int,void *,int));
    extern    int        write    LARGS((int,void const *,int));
    extern    int        chmod    LARGS((char const *,int));
    extern    int        unlink    LARGS((char const *));
    extern    int        chown    LARGS((char const *,int,int));
    extern    int        utime    LARGS((char const *,struct utimbuf const *));
    extern    char    *strcpy    LARGS((char *,char const *));
    extern    char    *strcat    LARGS((char *,char const *));
    extern    int        strcmp    LARGS((char const *,char const *));
    extern    unsigned strlen    LARGS((char const *));
    extern    void    *memset    LARGS((void *,char,unsigned int));
    extern    void    *memcpy    LARGS((void *,void const *,unsigned int));
    extern    int        atoi    LARGS((char const *));
    extern    void    exit    LARGS((int));
    extern    int        isatty    LARGS((int));
#endif

#ifdef    DEF_ERRNO
    extern int    errno;
#endif

#undef    min
#define   min(a,b)    ((a>b) ? b : a)

#define    MAGIC_1        (char_type)'\037'/* First byte of compressed file                */
#define    MAGIC_2        (char_type)'\235'/* Second byte of compressed file                */
#define BIT_MASK    0x1f            /* Mask for 'number of compresssion bits'        */
                                    /* Masks 0x20 and 0x40 are free.                  */
                                    /* I think 0x20 should mean that there is        */
                                    /* a fourth header byte (for expansion).        */
#define BLOCK_MODE    0x80          /* Block compresssion if table is full and        */
                                    /* compression rate is dropping flush tables    */

            /* the next two codes should not be changed lightly, as they must not   */
            /* lie within the contiguous general code space.                        */
#define FIRST    257                    /* first free entry                             */
#define CLEAR    256                    /* table clear output code                         */

#define INIT_BITS 9            /* initial number of bits/code */

#ifndef    BYTEORDER
#    define    BYTEORDER    0000
#endif

#ifndef    NOALLIGN
#    define    NOALLIGN    0
#endif

/*
 * machine variants which require cc -Dmachine:  pdp11, z8000, DOS
 */

#ifdef interdata    /* Perkin-Elmer                                                    */
#    define SIGNED_COMPARE_SLOW    /* signed compare is slower than unsigned             */
#endif

#ifdef MSDOS            /* PC/XT/AT (8088) processor                                    */
#    undef    BYTEORDER
#    define    BYTEORDER     4321
#    undef    NOALLIGN
#    define    NOALLIGN    1
#endif /* DOS */

#ifndef    O_BINARY
#    define    O_BINARY    0    /* System has no binary mode                            */
#endif

typedef long int code_int;

#ifdef SIGNED_COMPARE_SLOW
    typedef unsigned short int   count_short;
    typedef unsigned long int    cmp_code_int;    /* Cast to make compare faster    */
#else
    typedef long int             cmp_code_int;
#endif

typedef    unsigned char    char_type;

#define MAXCODE(n)    (1L << (n))

#ifndef    REGISTERS
#    define    REGISTERS    20
#endif
#define    REG1
#define    REG2
#define    REG3
#define    REG4
#define    REG5
#define    REG6
#define    REG7
#define    REG8
#define    REG9
#define    REG10
#define    REG11
#define    REG12
#define    REG13
#define    REG14
#define    REG15
#define    REG16
#if REGISTERS >= 1
#    undef    REG1
#    define    REG1    register
#endif
#if REGISTERS >= 2
#    undef    REG2
#    define    REG2    register
#endif
#if REGISTERS >= 3
#    undef    REG3
#    define    REG3    register
#endif
#if REGISTERS >= 4
#    undef    REG4
#    define    REG4    register
#endif
#if REGISTERS >= 5
#    undef    REG5
#    define    REG5    register
#endif
#if REGISTERS >= 6
#    undef    REG6
#    define    REG6    register
#endif
#if REGISTERS >= 7
#    undef    REG7
#    define    REG7    register
#endif
#if REGISTERS >= 8
#    undef    REG8
#    define    REG8    register
#endif
#if REGISTERS >= 9
#    undef    REG9
#    define    REG9    register
#endif
#if REGISTERS >= 10
#    undef    REG10
#    define    REG10    register
#endif
#if REGISTERS >= 11
#    undef    REG11
#    define    REG11    register
#endif
#if REGISTERS >= 12
#    undef    REG12
#    define    REG12    register
#endif
#if REGISTERS >= 13
#    undef    REG13
#    define    REG13    register
#endif
#if REGISTERS >= 14
#    undef    REG14
#    define    REG14    register
#endif
#if REGISTERS >= 15
#    undef    REG15
#    define    REG15    register
#endif
#if REGISTERS >= 16
#    undef    REG16
#    define    REG16    register
#endif

#if BYTEORDER == 4321 && NOALLIGN == 1
#define    input(b,o,c,n,m){                                                 \
                            (c) = (*(long *)(&(b)[(o)>>3])>>((o)&0x7))&(m);  \
                            (o) += (n);                                      \
                        }
#else
#define    input(b,o,c,n,m){    REG1 char_type         *p = &(b)[(o)>>3];    \
                            (c) = ((((long)(p[0]))|((long)(p[1])<<8)|        \
                                     ((long)(p[2])<<16))>>((o)&0x7))&(m);    \
                            (o) += (n);                                      \
                        }
#endif

/*
 * 8086 & 80286 Has a problem with array bigger than 64K so fake the array
 * For processors with a limited address space and segments.
 */
/*
 * To save much memory, we overlay the table used by compress() with those
 * used by lz_decompress().  The tab_prefix table is the same size and type
 * as the codetab.  The tab_suffix table needs 2**BITS characters.  We
 * get this from the beginning of htab.  The output stack uses the rest
 * of htab, and contains characters.  There is plenty of room for any
 * possible stack (stack used to be 8000 characters).
 */
#ifdef MAXSEG_64K
 #error removed MAXSEG_64K
#else    /* Normal machine */
#    define HSIZE (1<<16) /* (1<<15) is few, (1<<16) seems to be OK, (1<<17) is proven safe OK */
#if 0
    char_type lz_htab[2*HSIZE]; /* Dat: HSIZE for the codes and HSIZE for de_stack */
    unsigned short lz_codetab[HSIZE];
#endif
#    define    tab_prefixof(i)         codetab[i]
#    define    tab_suffixof(i)         htab[i]
#    define    de_stack                (htab+(sizeof(htab[0])*(2*HSIZE-1)))
#    define    clear_tab_prefixof()    memset(codetab, 0, 256*sizeof(codetab[0]));
#endif    /* MAXSEG_64K */

typedef struct lz_reader_t {
  /* constants (don't change during decompression) */
  fx_type fdin;
  char_type* inbuf; /* [IBUFSIZ+64];  Input buffer */
  char_type* htab; /* [2*HSIZE];  Dat: HSIZE for the codes and HSIZE for de_stack */
  unsigned short* codetab; /* [HSIZE]; */
  int blkmode;
  code_int maycode;

  /* variables that have to be saved */
  char_type        *stackp;
  code_int         code;
  int              finchar;
  code_int         oldcode;
  code_int         incode;
  int              inbits;
  int              posbits;
  int              insize;
  int              bitmask;
  code_int         freeent;
  code_int         maxcode;
  int              n_bits;
  int              rsize;
  int              maxbits;
} lz_reader_t;

static struct lz_reader_t* lz_read_new(FILE *inf) {
  lz_reader_t* reader=(lz_reader_t*)malloc(sizeof(lz_reader_t));
  reader->fdin=NULL;
  if (reader!=NULL) {
    if (lz_read_init(reader, inf)!=0) {
      lz_close(reader);
      free(reader); reader=NULL;
    }
  }
  return reader;
}
static int lz_close(struct lz_reader_t* reader) {
  FILE *fin=reader->fdin;
  reader->rsize=-1;
  if (reader->inbuf!=NULL)   { free(reader->inbuf  ); reader->inbuf  =NULL; }
  if (reader->htab!=NULL)    { free(reader->htab   ); reader->htab   =NULL; }
  if (reader->codetab!=NULL) { free(reader->codetab); reader->codetab=NULL; }
  if (fin==NULL) return 0;
  reader->fdin=NULL;
  return fclose(fin);
}
/** @param Zreader uninitialized
 * @return 0 on success
 */
static int lz_read_init(struct lz_reader_t* reader, FILE *inf) {
  char_type* htab;
  unsigned short* codetab;
  code_int codex;
  reader->fdin=inf;
  if (NULL==(reader->inbuf=malloc(IBUFSIZ+67))) return 1;
  if (NULL==(htab=reader->htab=malloc(2*HSIZE))) return 1; /* Dat: HSIZE for the codes and HSIZE for de_stack */
  if (NULL==(codetab=reader->codetab=malloc(sizeof(reader->codetab[0])*HSIZE))) return 1;
  reader->insize=0;
  reader->rsize=0;
  while (reader->insize < 3 && (reader->rsize = fx_read(reader->fdin, reader->inbuf+reader->insize, IBUFSIZ)) > 0)
      reader->insize += reader->rsize;

  if (reader->insize < 3
   || reader->inbuf[0] != (char_type)'\037' /* First byte of compressed file */
   || reader->inbuf[1] != (char_type)'\235' /* Second byte of compressed file */
  ) return reader->insize >= 0 ? 2 : 3;

  reader->maxbits = reader->inbuf[2] & BIT_MASK;
  reader->blkmode = reader->inbuf[2] & BLOCK_MODE;
  reader->maycode = MAXCODE(reader->maxbits);

  if (reader->maxbits > BITS) return 4;
#if 0
      fprintf(stderr,
              "%s: compressed with %d bits, can only handle %d bits\n",
              ("stdin"), maxbits, BITS);
      return 4;
#endif

  reader->maxcode = MAXCODE(reader->n_bits = INIT_BITS)-1;
  reader->bitmask = (1<<reader->n_bits)-1;
  reader->oldcode = -1;
  reader->finchar = 0;
  reader->posbits = 3<<3;
  reader->stackp  = NULL;
  reader->code    = -1;
  reader->incode  = -1; /* fake */
  reader->inbits  = 0;  /* fake */

  reader->freeent = ((reader->blkmode) ? FIRST : 256);

  clear_tab_prefixof();    /* As above, initialize the first 256 entries in the table. */

  for (codex = 255 ; codex >= 0 ; --codex)
      tab_suffixof(codex) = (char_type)codex;
  return 0; /* success */
}

/*
 * Decompress stdin to stdout.  This routine adapts to the codes in the
 * file building the "string" table on-the-fly; requiring no table to
 * be stored in the compressed file.  The tables used herein are shared
 * with those of the compress() routine.  See the definitions above.
 */
static unsigned lz_read(struct lz_reader_t* reader, char* outbuf, unsigned obufsize) {
/* static int lz_decompress(fx_type fdin, fx_type fdout) */
  REG2    char_type        *stackp;
  REG3    code_int         code;
  REG4    int              finchar;
  REG5    code_int         oldcode;
  REG6    code_int         incode;
  REG7    int              inbits;
  REG8    int              posbits;
  REG10   int              insize;
  REG11   int              bitmask;
  REG12   code_int         freeent;
  REG13   code_int         maxcode;
  REG14   code_int         maycode;
  REG15   int              n_bits;
  REG16   int              rsize;
          int              blkmode;
          int              maxbits;
          char_type* inbuf;
          char_type* htab;
          unsigned short* codetab;
          fx_type fdin;
  REG9    unsigned outpos=0; /* not restored */

#if 0
  fprintf(stderr, ":: call rsize=%d oldcode=%d\n", reader->rsize, (int)reader->oldcode);
#endif
  if (reader->rsize<1) return reader->rsize; /* already EOF or error */

  stackp =reader->stackp;
  code   =reader->code;
  finchar=reader->finchar;
  oldcode=reader->oldcode;
  incode =reader->incode;
  inbits =reader->inbits;
  posbits=reader->posbits;
  insize =reader->insize;
  bitmask=reader->bitmask;
  freeent=reader->freeent;
  maxcode=reader->maxcode;
  maycode=reader->maycode;
  n_bits =reader->n_bits;
  rsize  =reader->rsize;
  blkmode=reader->blkmode;
  maxbits=reader->maxbits;
  htab   =reader->htab;
  codetab=reader->codetab;
  fdin   =reader->fdin;
  inbuf  =reader->inbuf;
  if (oldcode!=-1) goto try_fit; /* lz_read() called again */

  do {
    resetbuf: ;
    {
      REG1     int    i;
      int                e;
      int                o;

      e = insize-(o = (posbits>>3));

      for (i = 0 ; i < e ; ++i)
          inbuf[i] = inbuf[i+o];

      insize = e;
      posbits = 0;
    }

    if ((unsigned)insize < /*sizeof(inbuf)-IBUFSIZ*/ 64) {
#if 0
      fprintf(stderr, ":: try read insize=%d\n", insize);
#endif
      if ((rsize = fx_read(fdin, inbuf+insize, IBUFSIZ)) < 0 || ferror(fdin)) return reader->rsize=-1;
#if 0
      fprintf(stderr, ":: got rsize=%d\n", rsize);
#endif
      insize += rsize;
      inbuf[insize]=inbuf[insize+1]=inbuf[insize+2]=0;
      /* ^^^ sentinel, so input() won't produce ``read uninitialized byte(s) in a block.'' */
    }

    inbits = ((rsize > 0) ? (insize - insize%n_bits)<<3 :
                            (insize<<3)-(n_bits-1));

    while (inbits > posbits) {
      if (freeent > maxcode) {
        posbits = ((posbits-1) + ((n_bits<<3) -
                         (posbits-1+(n_bits<<3))%(n_bits<<3)));
        ++n_bits;
        if (n_bits == maxbits)
            maxcode = maycode;
        else
            maxcode = MAXCODE(n_bits)-1;

        bitmask = (1<<n_bits)-1;
        goto resetbuf;
      }

#if 0
      fputc('#',stderr);
#endif
      input(inbuf,posbits,code,n_bits,bitmask);

      if (oldcode == -1) {
        outbuf[outpos++] = (char_type)(finchar = (int)(oldcode = code));
        continue;
      }
      /* Dat: oldcode will never be -1 again */

      if (code == CLEAR && blkmode) {
        clear_tab_prefixof();
        freeent = FIRST - 1;
        posbits = ((posbits-1) + ((n_bits<<3) -
                    (posbits-1+(n_bits<<3))%(n_bits<<3)));
        maxcode = MAXCODE(n_bits = INIT_BITS)-1;
        bitmask = (1<<n_bits)-1;
        goto resetbuf;
      }

      incode = code;
      stackp = de_stack;

      if (code >= freeent) {    /* Special case for KwKwK string.    */
        if (code > freeent) {
            posbits -= n_bits;
#if 0
            {
                REG1 char_type         *p;
                p = &inbuf[posbits>>3];

                fprintf(stderr, "uncompress: insize:%d posbits:%d inbuf:%02X %02X %02X %02X %02X (%d)\n", insize, posbits,
                        p[-1],p[0],p[1],p[2],p[3], (posbits&07));
                fprintf(stderr, "uncompress: corrupt input\n");
                abort();
            }
#else
            return reader->rsize=-1;
#endif
        }

        *--stackp = (char_type)finchar;
        code = oldcode;
      }

      while ((cmp_code_int)code >= (cmp_code_int)256) {
        /* Generate output characters in reverse order */
        *--stackp = tab_suffixof(code);
        code = tab_prefixof(code);
      }
#if 0
      fprintf(stderr, ":: to stack code=%d\n", (int)code);
#endif
      *--stackp =    (char_type)(finchar = tab_suffixof(code));
      ASSERT(outpos<=obufsize);

      /* And put them out in forward order */
      {
        REG1 int    i;

        try_fit: if (outpos+(i = (de_stack-stackp)) >= obufsize) {
          /* The entire stack doesn't fit into the outbuf */
          i=obufsize-outpos;
          memcpy(outbuf+outpos, stackp, i);
          stackp+=i;
          /* vvv Dat: blkmode, maycode, inbuf, htab, codetab, fdin need not be saved */
          reader->stackp =stackp;
          reader->code   =code;
          reader->finchar=finchar;
          reader->oldcode=oldcode;
          reader->incode =incode;
          reader->inbits =inbits;
          reader->posbits=posbits;
          reader->insize =insize;
          reader->bitmask=bitmask;
          reader->freeent=freeent;
          reader->maxcode=maxcode;
          reader->maycode=maycode;
          reader->n_bits =n_bits;
          reader->rsize  =rsize;
          reader->blkmode=blkmode;
          reader->maxbits=maxbits;
#if 0
          fprintf(stderr, ":: return obufsize=%d oldcode=%d\n", obufsize, (int)oldcode);
#endif
          return obufsize;
          /* Dat: co-routine return back to try_fit, with outpos==0, outbuf=... obufsize=... */
        } else {
          memcpy(outbuf+outpos, stackp, i);
          outpos += i;
        }
      }
      /* Dat: ignore stackp from now */

      if ((code = freeent) < maycode) { /* Generate the new entry. */
#if 0
        fprintf(stderr, ":: new entry code=(%d)\n", (int)code);
#endif
        tab_prefixof(code) = (unsigned short)oldcode;
        tab_suffixof(code) = (char_type)finchar;
        freeent = code+1;
      }
      oldcode = incode;    /* Remember previous code.    */
    }
  } while (rsize > 0);
  reader->rsize=0;
  return outpos;
}

/* --- */

/* Values used in typeflag field.  */

#define REGTYPE	 '0'		/* regular file */
#define AREGTYPE '\0'		/* regular file */
#define LNKTYPE  '1'		/* link */
#define SYMTYPE  '2'		/* reserved */
#define CHRTYPE  '3'		/* character special */
#define BLKTYPE  '4'		/* block special */
#define DIRTYPE  '5'		/* directory */
#define FIFOTYPE '6'		/* FIFO special */
#define CONTTYPE '7'		/* reserved */

#define BLOCKSIZE 512

struct tar_header
{				/* byte offset */
  char name[100];		/*   0 */
  char mode[8];			/* 100 */
  char uid[8];			/* 108 */
  char gid[8];			/* 116 */
  char size[12];		/* 124 */
  char mtime[12];		/* 136 */
  char chksum[8];		/* 148 */
  char typeflag;		/* 156 */
  char linkname[100];		/* 157 */
  char magic[6];		/* 257 */
  char version[2];		/* 263 */
  char uname[32];		/* 265 */
  char gname[32];		/* 297 */
  char devmajor[8];		/* 329 */
  char devminor[8];		/* 337 */
  char prefix[155];		/* 345 */
				/* 500 */
};

union tar_buffer {
  char               buffer[BLOCKSIZE];
  struct tar_header  header;
};

enum { TGZ_EXTRACT = 0, TGZ_LIST };

#if 0
static char *TGZfname	OF((const char *));
void TGZnotfound	OF((const char *));

int getoct		OF((char *, int));
char *strtime		OF((time_t *));
int ExprMatch		OF((char *,char *));

int makedir		OF((char *));
int matchname		OF((int,int,char **,char *));

void error		OF((const char *));
int  tar		OF((gzFile, int, int, int, char **));

void help		OF((int));
int main		OF((int, char **));
#endif

static char *prog;

static void error(const char *msg) {
  fprintf(stderr, "%s: %s\n", prog, msg);
  exit(1);
}

#ifdef DOSISH
/* This will give a benign warning */
static char *TGZprefix[] = { "\0", ".tgz", ".tar.gz", ".tar.bz2", ".tar.Z", ".tar", NULL };

/* Return the real name of the TGZ archive */
/* or NULL if it does not exist. */
static char *TGZfname OF((const char *fname))
{
  static char buffer[1024];
  int origlen,i;

  strcpy(buffer,fname);
  origlen = strlen(buffer);

  for (i=0; TGZprefix[i]; i++)
    {
       strcpy(buffer+origlen,TGZprefix[i]);
       if (access(buffer,F_OK) == 0)
         return buffer;
    }
  return NULL;
}

/* error message for the filename */

static void TGZnotfound OF((const char *fname))
{
  int i;

  fprintf(stderr,"%s : couldn't find ",prog);
  for (i=0;TGZprefix[i];i++)
    fprintf(stderr,(TGZprefix[i+1]) ? "%s%s, " : "or %s%s\n",
            fname,
            TGZprefix[i]);
  exit(1);
}
#endif


/* help functions */

static int getoct(char *p,int width)
{
  int result = 0;
  char c;

  while (width --)
    {
      c = *p++;
      if (c == ' ')
	continue;
      if (c == 0)
	break;
      result = result * 8 + (c - '0');
    }
  return result;
}

static char *strtime (time_t *t)
{
  struct tm   *local;
  static char result[32];

  local = localtime(t);
  sprintf(result,"%2d/%02d/%4d %02d:%02d:%02d",
	  local->tm_mday, local->tm_mon+1, local->tm_year+1900,
	  local->tm_hour, local->tm_min,   local->tm_sec);
  return result;
}


/* regular expression matching */

#define ISSPECIAL(c) (((c) == '*') || ((c) == '/'))

static int ExprMatch(char *string,char *expr)
{
  while (1)
    {
      if (ISSPECIAL(*expr))
	{
	  if (*expr == '/')
	    {
	      if (*string != '\\' && *string != '/')
		return 0;
	      string ++; expr++;
	    }
	  else if (*expr == '*')
	    {
	      if (*expr ++ == 0)
		return 1;
	      while (*++string != *expr)
		if (*string == 0)
		  return 0;
	    }
	}
      else
	{
	  if (*string != *expr)
	    return 0;
	  if (*expr++ == 0)
	    return 1;
	  string++;
	}
    }
}

/* recursive make directory */
/* abort if you get an ENOENT errno somewhere in the middle */
/* e.g. ignore error "mkdir on existing directory" */
/* */
/* return 1 if OK */
/*        0 on error */

static int makedir (char *newdir) {
  /* avoid calling strdup, since it's not ansi C */
  int  len = strlen(newdir);
  char *p, *buffer = malloc(len+1);
  if (buffer==NULL) error("out of memory");
  memcpy(buffer,newdir,len+1);

  if (len <= 0) {
    free(buffer);
    return 0;
  }
  if (buffer[len-1] == '/') {
    buffer[len-1] = '\0';
  }
  if (mkdir(buffer, 0775) == 0)
    {
      free(buffer);
      return 1;
    }

  p = buffer+1;
  while (1)
    {
      char hold;

      while(*p && *p != '\\' && *p != '/')
	p++;
      hold = *p;
      *p = 0;
      if ((mkdir(buffer, 0775) == -1) && (errno == ENOENT))
	{
	  fprintf(stderr,"%s: couldn't create directory %s\n",prog,buffer);
	  free(buffer);
	  return 0;
	}
      if (hold == 0)
	break;
      *p++ = hold;
    }
  free(buffer);
  return 1;
}

static int matchname (int arg,int argc,char **argv,char *fname)
{
  if (arg == argc)		/* no arguments given (untgz tgzarchive) */
    return 1;

  while (arg < argc)
    if (ExprMatch(fname,argv[arg++]))
      return 1;

  return 0; /* ignore this for the moment being */
}


/* Tar file list or extract */

static int tar (Readable* rin,int action,int arg,int argc,char **argv, char const* TGZfile, int verbose, void (*step)(void *,int), void *data) {
  union  tar_buffer buffer;
  int    is_tar_ok=0;
  int    len;
  int    err;
  int    getheader = 1;
  int    remaining = 0;
  FILE   *outfile = NULL;
  char   fname[BLOCKSIZE];
  time_t tartime = 0;
  unsigned long last_read;

#if 0
  while (0<(len=rin->xRead(rin, &buffer, BLOCKSIZE))) {
    fwrite(buffer.buffer, 1, len, stdout);
  }
  exit(0);
#endif

  last_read = 0;
  if (action == TGZ_LIST)
    printf("     day      time     size                       file\n"
	   " ---------- -------- --------- -------------------------------------\n");
  while (1) {
      len = rin->xRead(rin, &buffer, BLOCKSIZE);
      if (len+1 == 0)
	error (rin->xError(rin, &err));
      if (step) { step(data,total_read - last_read); last_read = total_read; }
      if (!is_tar_ok && !(is_tar_ok=is_tar(buffer.buffer, len))) {
        fprintf(stderr, "%s: compressed file not tared: %s\n", prog, TGZfile);
        if (action == TGZ_EXTRACT) {
          FILE *of;
          unsigned len0=strlen(TGZfile), len1=len0;
          char *ofn=malloc(len1+5);
          if (ofn==NULL) return 1;
          memcpy(ofn, TGZfile, len1+1);
          while (len1>0 && ofn[len1]!='/' && ofn[len1]!='\\' && ofn[len1]!='.') len1--;
          if (ofn[len1]=='.') ofn[len1]='\0'; else strcpy(ofn+len0, ".unc");
          /* ^^^ remove last extension, or add `.unc' */
          fprintf(stderr, "%s: extracting as: %s\n", prog, ofn);
          if (NULL==(of=fopen(ofn, "wb"))) error("fopen write error");
          do {
            fwrite(buffer.buffer, 1, len, of);
            if (ferror(of)) error("write error");
          } while (0<(len=rin->xRead(rin, &buffer, BLOCKSIZE))); /* Imp: larger blocks */
          len=ferror(of);
          if (0!=(len|fclose(of))) error("fclose write error");
          free(ofn);
          return 0;
        }
        return 1;
        /* !! */
      }
      /*
       * Always expect complete blocks to process
       * the tar information.
       */
      if (len != BLOCKSIZE)
	error("read: incomplete block read");

      /*
       * If we have to get a tar header
       */
      if (getheader == 1)
	{
	  /*
	   * if we met the end of the tar
	   * or the end-of-tar block,
	   * we are done
	   */
	  if ((len == 0)  || (buffer.header.name[0]== 0)) break;

	  tartime = (time_t)getoct(buffer.header.mtime,12);
	  strcpy(fname,buffer.header.name);

	  switch (buffer.header.typeflag)
	    {
	    case DIRTYPE:
	      if (action == TGZ_LIST)
		printf(" %s     <dir> %s\n",strtime(&tartime),fname);
	      if (action == TGZ_EXTRACT)
		makedir(fname);
	      break;
	    case REGTYPE:
	    case AREGTYPE:
	      remaining = getoct(buffer.header.size,12);
	      if (action == TGZ_LIST)
		printf(" %s %9d %s\n",strtime(&tartime),remaining,fname);
	      if (action == TGZ_EXTRACT)
		{
		  if ((remaining) && (matchname(arg,argc,argv,fname)))
		    {
		      outfile = fopen(fname,"wb");
		      if (outfile == NULL) {
			/* try creating directory */
			char *p = strrchr(fname, '/');
			if (p != NULL) {
			  *p = '\0';
			  makedir(fname);
			  *p = '/';
			  outfile = fopen(fname,"wb");
			}
		      }
                      if (verbose)
		        fprintf(stderr,
			        "%s %s\n",
			        (outfile) ? "Extracting" : "Couldn't create",
			        fname);
		    }
		  else
		    outfile = NULL;
		}
	      /*
	       * could have no contents
	       */
	      getheader = (remaining) ? 0 : 1;
	      break;
	    default:
	      if (action == TGZ_LIST)
		printf(" %s     <---> %s\n",strtime(&tartime),fname);
	      break;
	    }
	}
      else
	{
	  unsigned int bytes = (remaining > BLOCKSIZE) ? BLOCKSIZE : remaining;

	  if ((action == TGZ_EXTRACT) && (outfile != NULL))
	    {
	      if (fwrite(&buffer,sizeof(char),bytes,outfile) != bytes)
		{
		  fprintf(stderr,"%s : error writing %s skipping...\n",prog,fname);
		  fclose(outfile);
		  unlink(fname);
		}
	    }
	  remaining -= bytes;
	  if (remaining == 0)
	    {
	      getheader = 1;
	      if ((action == TGZ_EXTRACT) && (outfile != NULL))
		{
#ifdef WIN32
		  HANDLE hFile;
		  FILETIME ftm,ftLocal;
		  SYSTEMTIME st;
		  struct tm localt;

		  fclose(outfile);

		  localt = *localtime(&tartime);

		  hFile = CreateFile(fname, GENERIC_READ | GENERIC_WRITE,
				     0, NULL, OPEN_EXISTING, 0, NULL);

		  st.wYear = (WORD)localt.tm_year+1900;
		  st.wMonth = (WORD)localt.tm_mon;
		  st.wDayOfWeek = (WORD)localt.tm_wday;
		  st.wDay = (WORD)localt.tm_mday;
		  st.wHour = (WORD)localt.tm_hour;
		  st.wMinute = (WORD)localt.tm_min;
		  st.wSecond = (WORD)localt.tm_sec;
		  st.wMilliseconds = 0;
		  SystemTimeToFileTime(&st,&ftLocal);
		  LocalFileTimeToFileTime(&ftLocal,&ftm);
		  SetFileTime(hFile,&ftm,NULL,&ftm);
		  CloseHandle(hFile);

		  outfile = NULL;
#else
		  struct utimbuf settime;

		  settime.actime = settime.modtime = tartime;

		  fclose(outfile);
		  outfile = NULL;
		  utime(fname,&settime);
#endif
		}
	    }
	}
    }

  if (rin->xClose(rin))
    error("failed gzclose");

  return 0;
}


#ifdef DOSISH
/* =========================================================== */

static void help(int exitval)
{
  fprintf(stderr,
	  "untarka v0.34: super untar + untar.Z"
#ifdef HAVE_ZLIB
          " + untar.gz"
#endif
#ifdef HAVE_BZ2LIB
          " + untar.bz2"
#endif
	  "\ncontains code (C) by pts@fazekas.hu since 2003.01.28\n"
	  "This program is under GPL >=2.0. There is NO WARRANTY. Use at your own risk!\n\n"
          "Usage: untarka [-x] tarfile          to extract all files\n"
          "       untarka -x tarfile fname ...  to extract selected files\n"
          "       untarka -l tarfile            to list archive contents\n"
          "       untarka -h                    to display this help\n");
  exit(exitval);
}

/* ====================================================================== */

extern int _CRT_glob;
int _CRT_glob = 0;	/* disable globbing of the arguments */
#endif
#ifdef COMPILE_MAIN
int main(int argc,char **argv) {
    int 	action = TGZ_EXTRACT;
    int 	arg = 1;
    char	*TGZfile;
    Readable    r;

#if DOSISH
    setmode(0, O_BINARY);
#endif
    prog = strrchr(argv[0],'\\');
    if (prog == NULL)
      {
	prog = strrchr(argv[0],'/');
	if (prog == NULL)
	  {
	    prog = strrchr(argv[0],':');
	    if (prog == NULL)
	      prog = argv[0];
	    else
	      prog++;
	  }
	else
	  prog++;
      }
    else
      prog++;

    if (argc == 1)
      help(0);

    if (strcmp(argv[arg],"-l") == 0 || strcmp(argv[arg],"-v") == 0) {
      action = TGZ_LIST;
      if (argc == ++arg) help(0);
    } else if (strcmp(argv[arg],"-x") == 0) {
      action = TGZ_EXTRACT;
      if (argc == ++arg) help(0);
    } else if (strcmp(argv[arg],"-h") == 0) help(0);

    if ((TGZfile = TGZfname(argv[arg])) == NULL)
      TGZnotfound(argv[arg]);

    ++arg;
    if ((action == TGZ_LIST) && (arg != argc))
      help(1);

    /*
     *  Process the TGZ file
     */
    switch (action) {
     case TGZ_LIST:
     case TGZ_EXTRACT:
      switch (xOpen4Read(&r,TGZfile)) {
       case 0: break;
       case 1: fprintf(stderr, "%s: cannot decode compressed tar file: %s\n", prog, TGZfile); return 1;
       case 2: fprintf(stderr, "%s: tar file too short: %s\n", prog, TGZfile); return 1;
       case 3: fprintf(stderr, "%s: not a tar file: %s\n", prog, TGZfile); return 1;
       case 4: fprintf(stderr, "%s: cannot open tar file: %s\n", prog, TGZfile); return 1;
      }
      return tar(&r, action, arg, argc, argv, TGZfile, 1, 0, 0);
      default: error("Unknown option!");
    }
    return 0;
}
#endif

void
tarextract(char *TGZfile,char *dest,int verbose, void (*step)(void *,int), void *data)
{
   Readable    r;
   if (xOpen4Read(&r,TGZfile) == 0)
      {
      int arg = 2,
          argc = 2;
      char *argv[2];
      chdir( dest );
      argv[0] = "fgadmin";
      argv[1] = TGZfile;
      tar(&r, TGZ_EXTRACT, arg, argc, argv, TGZfile, 0, step, data);
      }
}
