/* Copyright (c) 2007-2009 by Adalin B.V.
 * Copyright (c) 2007-2009 by Erik Hofman
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *      * Redistributions of source code must retain the above copyright
 *         notice, this list of conditions and the following disclaimer.
 *      * Redistributions in binary form must reproduce the above copyright
 *         notice, this list of conditions and the following disclaimer in the
 *         documentation and/or other materials provided with the distribution.
 *      * Neither the name of (any of) the copyrightholder(s) nor the
 *         names of its contributors may be used to endorse or promote products
 *         derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
 * THE COPYRIGHT HOLDER BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifdef WIN32
# define WIN32_LEAN_AND_MEAN
# include <windows.h>
# include <io.h>

typedef struct
{
    HANDLE m;
    void *p;
} SIMPLE_UNMMAP;

#else	/* !WIN32 */
# include <sys/mman.h>
# include <unistd.h>
#endif

#ifndef NDEBUG
# include <stdio.h>
#endif
#include <stdlib.h>     /* free, malloc */
#include <malloc.h>
#include <string.h>	/* memcmp */
#ifndef _MSC_VER
#include <strings.h>	/* strncasecmp */
#else
# define strncasecmp strnicmp
#endif
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <assert.h>
#include <ctype.h>

#ifndef XML_NONVALIDATING
#include "xml.h"

static const char *__xml_error_str[XML_MAX_ERROR];

struct _xml_error
{
    char *pos;
    int err_no;
};

static void __xmlErrorSet(const void *, const char *, unsigned int);
# define xmlErrorSet(a, b, c)	__xmlErrorSet(a, b, c)

# ifndef NDEBUG
# define PRINT_INFO(a) \
	assert((a) < XML_MAX_ERROR); \
	printf("at line %i: %s\n", __LINE__, __xml_error_str[(a)])
# else
# define PRINT_INFO(a)
# endif

# define SET_ERROR_AND_RETURN(a, b) \
       { *rlen = 0; *name = (a); *len = (b); PRINT_INFO(b); return 0; }

#else /* !XML_NONVALIDATING */
# define xmlErrorSet(a, b, c)
# define SET_ERROR_AND_RETURN(a, b)	return 0;
#endif

/*
 * It is required for both the rood node and the normal xml nodes to both
 * have 'char *name' defined as the first entry. The code tests whether
 * name == 0 to detect the root node.
 */
struct _root_id
{
    char *name;
    char *start;
    size_t len;
    int fd;
#ifndef XML_NONVALIDATING
    struct _xml_error *info;
#endif
# ifdef WIN32
    SIMPLE_UNMMAP un;
# endif
};

struct _xml_id
{
    char *name;
    char *start;
    size_t len;
    size_t name_len;
#ifndef XML_NONVALIDATING
    struct _root_id *root;
#endif
};

static char *__xmlNodeGetPath(const char *, size_t *, char **, size_t *);
static char *__xmlNodeGet(const char *, size_t *, char **, size_t *, size_t *);
static char *__xmlProcessCDATA(char **, size_t *);
static char *__xmlCommentSkip(const char *, size_t);
static char *__xmlInfoProcess(const char *, size_t);

static void *__xml_memncasecmp(const char *, size_t *, char **, size_t *);
static void __xmlPrepareData(char **, size_t *);

#ifdef WIN32
/*
 * map 'filename' and return a pointer to it.
 */
static void *simple_mmap(int, size_t, SIMPLE_UNMMAP *);
static void simple_unmmap(SIMPLE_UNMMAP *);

# define mmap(a,b,c,d,e,f)       simple_mmap((e), (b), &rid->un)
# define munmap(a,b)             simple_unmmap(&rid->un)
#endif

#ifndef NDEBUG
# define PRINT(a, b, c) { \
   if (a) { \
      size_t q, len = c; \
      if (b < c) len = b; \
      if (len < 50000) { \
         printf("(%i) '", len); \
         for (q=0; q<len; q++) printf("%c", ((char *)(a))[q]); \
         printf("'\n"); \
      } else printf("Length (%u) seems too large at line %i\n",len, __LINE__); \
   } else printf("NULL pointer at line %i\n", __LINE__); \
}
#endif

void *
xmlOpen(const char *filename)
{
    struct _root_id *rid = 0;

    if (filename)
    {
        int fd = open(filename, O_RDONLY);
        if (fd >= 0)
        {
            struct stat statbuf;
            void *mm;

            fstat(fd, &statbuf);
            mm = mmap(0, statbuf.st_size, PROT_READ, MAP_PRIVATE, fd, 0L);
            if (mm != (void *)-1)
            {
                rid = malloc(sizeof(struct _root_id));
                if (rid)
                {
                    rid->fd = fd;
                    rid->start = mm;
                    rid->len = statbuf.st_size;
                    rid->name = 0;
#ifndef XML_NONVALIDATING
                    rid->info = 0;
#endif
                }
            }
        }
    }

    return (void *)rid;
}

void *
xmlInitBuffer(const char *buffer, size_t size)
{
    struct _root_id *rid = 0;

    if (buffer && (size > 0))
    {
        rid = malloc(sizeof(struct _root_id));
        if (rid)
        {
            rid->fd = -1;
            rid->start = (char *)buffer;
            rid->len = size;
            rid->name = 0;
#ifndef XML_NONVALIDATING
            rid->info = 0;
#endif
        }
    }

    return (void *)rid;
}

void
xmlClose(void *id)
{
     struct _root_id *rid = (struct _root_id *)id;

     assert(rid != 0);
     assert(rid->name == 0);

     if (rid->fd != -1)
     {
         munmap(rid->start, rid->len);
         close(rid->fd);
     }

     if (rid->info) free(rid->info);
     free(rid);
     id = 0;
}

void *
xmlNodeGet(const void *id, const char *path)
{
    struct _xml_id *xid = (struct _xml_id *)id;
    struct _xml_id *xsid = 0;
    size_t len, slen;
    char *ptr, *node;

    assert(id != 0);
    assert(path != 0);

    node = (char *)path;
    len = xid->len;
    slen = strlen(path);
    ptr = __xmlNodeGetPath(xid->start, &len, &node, &slen);
    if (ptr)
    {
        xsid = malloc(sizeof(struct _xml_id));
        if (xsid)
        {
             xsid->len = len;
             xsid->start = ptr;
             xsid->name_len = slen;
             xsid->name = node;
#ifndef XML_NONVALIDATING
             if (xid->name)
                xsid->root = xid->root;
             else
                xsid->root = (struct _root_id *)xid;
#endif
        }
        else
        {
            xmlErrorSet(xid, 0, XML_OUT_OF_MEMORY);
        }
    }
    else if (slen == 0)
    {
        xmlErrorSet(xid, node, len);
    }

    return (void *)xsid;
}

void *
xmlNodeCopy(const void *id, const char *path)
{
    struct _xml_id *xid = (struct _xml_id *)id;
    struct _xml_id *xsid = 0;
    char *ptr, *node, *p;
    size_t slen, len;

    node = (char *)path;
    len = xid->len;
    slen = strlen(path);
    ptr = __xmlNodeGetPath(xid->start, &len, &node, &slen);
    if (ptr)
    {
        xsid = malloc(sizeof(struct _xml_id) + len);
        if (xsid)
        {
            p = (char *)xsid + sizeof(struct _xml_id);

            xsid->len = len;
            xsid->start = p;
            xsid->name_len = slen;
            xsid->name = node;
#ifndef XML_NONVALIDATING
            if (xid->name)
                xsid->root = xid->root;
            else
                xsid->root = (struct _root_id *)xid;
#endif

            memcpy(xsid->start, ptr, len);
        }
        else
        {
            xmlErrorSet(xid, 0, XML_OUT_OF_MEMORY);
        }
    }
    else if (slen == 0)
    {
        xmlErrorSet(xid, node, len);
    }

   return (void *)xsid;
}

char *
xmlNodeGetName(const void *id)
{
    struct _xml_id *xid = (struct _xml_id *)id;
    size_t len;
    char *ret;

    assert(xid != 0);

    len = xid->name_len;
    ret = malloc(len+1);
    if (ret)
    {
        memcpy(ret, xid->name, len);
        *(ret + len) = 0;
    }
    else
    {
        xmlErrorSet(xid, 0, XML_OUT_OF_MEMORY);
    }

    return ret;
}

size_t
xmlNodeCopyName(const void *id, char *buf, size_t buflen)
{
    struct _xml_id *xid = (struct _xml_id *)id;
    size_t slen = 0;
 
    assert(buf != 0);
    assert(buflen > 0);

    slen = xid->name_len;
    if (slen >= buflen)
    {
        slen = buflen-1;
        xmlErrorSet(xid, 0, XML_TRUNCATE_RESULT);
    }
    memcpy(buf, xid->name, slen);
    *(buf + slen) = 0;

    return slen;
}

unsigned int
xmlNodeGetNum(const void *id, const char *path)
{
    struct _xml_id *xid = (struct _xml_id *)id;
    size_t num = 0;

    assert(xid != 0);
    assert(path != 0);

    if (xid->len)
    {
        char *nodename, *pathname;
        size_t len, slen;
        char *p;

        nodename = (char *)path;
        if (*path == '/') nodename++;
        slen = strlen(nodename);

        pathname = strchr(nodename, '/');
        if (pathname)
        {
           char *node;

           len = xid->len;
           pathname++;
           slen -= pathname-nodename;
           node = pathname;
           p = __xmlNodeGetPath(xid->start, &len, &node, &slen);
           if (p == 0 && slen == 0)
           {
               xmlErrorSet(xid, node, len);
           }
        }
        else
        {
            p = xid->start;
            len = xid->len;
        }

        if (p)
        {
            char *ret, *node = nodename;
            ret = __xmlNodeGet(p, &len, &node, &slen, &num);
            if (ret == 0 && slen == 0)
            {
                xmlErrorSet(xid, node, len);
                num = 0;
            }
        }
    }

    return num;
}

void *
xmlNodeGetPos(const void *pid, void *id, const char *element, size_t num)
{
    struct _xml_id *xpid = (struct _xml_id *)pid;
    struct _xml_id *xid = (struct _xml_id *)id;
    size_t len, slen;
    char *ptr, *node;
    void *ret = 0;

    assert(xpid != 0);
    assert(xid != 0);
    assert(element != 0);

    len = xpid->len;
    slen = strlen(element);
    node = (char *)element;
    ptr = __xmlNodeGet(xpid->start, &len, &node, &slen, &num);
    if (ptr)
    {
         xid->len = len;
         xid->start = ptr;
         xid->name = node;
         xid->name_len = slen;
         ret = xid;
    }
    else if (slen == 0)
    {
        xmlErrorSet(xpid, node, len);
    }

    return ret;
}

char *
xmlGetString(const void *id)
{
    struct _xml_id *xid = (struct _xml_id *)id;
    char *str = 0;

    assert(xid != 0);

    if (xid->len)
    {
        size_t len;
        char *ps;

        ps = xid->start;
        len = xid->len;
        __xmlPrepareData(&ps, &len);

        if (len)
        {
            str = malloc(len+1);
            if (str)
            {
                memcpy(str, ps, len);
                *(str+len) = 0;
            }
            else
            {
                xmlErrorSet(xid, 0, XML_OUT_OF_MEMORY);
            }
        }
    }

    return str;
}

size_t
xmlCopyString(const void *id, char *buffer, size_t buflen)
{
    struct _xml_id *xid = (struct _xml_id *)id;
    size_t ret = 0;
 
    assert(xid != 0);
    assert(buffer != 0);
    assert(buflen > 0);

    *buffer = '\0';
    if (xid->len)
    {
        size_t len;
        char *p;

        p = xid->start;
        len = xid->len;
        __xmlPrepareData(&p, &len);
        if (len)
        {
            if (len >= buflen)
            {
                len = buflen-1;
                xmlErrorSet(xid, 0, XML_TRUNCATE_RESULT);
            }
            memcpy(buffer, p, len);
            *(buffer+len) = 0;
        }
        ret = len;
    }

    return ret;
}

int
xmlCompareString(const void *id, const char *s)
{
    struct _xml_id *xid = (struct _xml_id *)id;
    int ret = -1;

    assert(xid != 0);
    assert(s != 0);

    if (xid->len && (strlen(s) > 0))
    {
        size_t len;
        char *ps;

        ps = xid->start;
        len = xid->len-1;
        __xmlPrepareData(&ps, &len);
        ret = strncasecmp(ps, s, len);
    }

    return ret;
}

char *
xmlNodeGetString(const void *id, const char *path)
{
    struct _xml_id *xid = (struct _xml_id *)id;
    char *str = 0;

    assert(xid != 0);
    assert(path != 0);

    if (xid->len)
    {
        char *p, *node = (char *)path;
        size_t slen = strlen(node);
        size_t len = xid->len;

        slen = strlen(node);
        p = __xmlNodeGetPath(xid->start, &len, &node, &slen);
        if (p && len)
        {
            __xmlPrepareData(&p, &len);

            str = malloc(len+1);
            if (str)
            {
               memcpy(str, p, len);
               *(str+len) = '\0';
            }
            else
            {
                xmlErrorSet(xid, 0, XML_OUT_OF_MEMORY);
            }
        }
        else
        {
            xmlErrorSet(xid, node, len);
        }
    }

    return str;
}

size_t
xmlNodeCopyString(const void *id, const char *path, char *buffer, size_t buflen)
{
    struct _xml_id *xid = (struct _xml_id *)id;
    size_t ret = 0;

    assert(xid != 0);
    assert(path != 0);
    assert(buffer != 0);
    assert(buflen > 0);

    *buffer = '\0';
    if (xid->len)
    {
        char *p, *node = (char *)path;
        size_t slen = strlen(node);
        size_t len = xid->len;

        p = __xmlNodeGetPath(xid->start, &len, &node, &slen);
        if (p)
        {
            __xmlPrepareData(&p, &len);
            if (len)
            {
                if (len >= buflen)
                {
                    len = buflen-1;
                    xmlErrorSet(xid, 0, XML_TRUNCATE_RESULT);
                }

                memcpy(buffer, p, len);
                *(buffer+len) = '\0';
            }
            ret = 0;
        }
        else if (slen == 0)
        {
            xmlErrorSet(xid, node, len);
        }
    }

    return ret;
}

int
xmlNodeCompareString(const void *id, const char *path, const char *s)
{
    struct _xml_id *xid = (struct _xml_id *)id;
    int ret = -1;

    assert(xid != 0);
    assert(path != 0);
    assert(s != 0);

    if (xid->len && (strlen(s) > 0))
    {
        char *node, *str, *ps, *pe;
        size_t len, slen;

        len = xid->len;
        slen = strlen(path);
        node = (char *)path;
        str = __xmlNodeGetPath(xid->start, &len, &node, &slen);
        if (str)
        {
            ps = str;
            __xmlPrepareData(&ps, &len);
            ret = strncasecmp(ps, s, len);
        }
        else if (slen == 0)
        {
            xmlErrorSet(xid, node, len);
        }
    }

    return ret;
}

long int
xmlGetInt(const void *id)
{
    struct _xml_id *xid = (struct _xml_id *)id;
    long int li = 0;

    assert(xid != 0);

    if (xid->len)
    {
        char *end = xid->start + xid->len;
        li = strtol(xid->start, &end, 10);
    }

    return li;
}

long int
xmlNodeGetInt(const void *id, const char *path)
{
    struct _xml_id *xid = (struct _xml_id *)id;
    long int li = 0;

    assert(xid != 0);
    assert(path != 0);

    if (xid->len)
    {
        size_t len, slen;
        char *str, *node;

        len = xid->len;
        slen = strlen(path);
        node = (char *)path;
        str = __xmlNodeGetPath(xid->start, &len, &node, &slen);
        if (str)
        {
            char *end = str+len;
            li = strtol(str, &end, 10);
        }
        else if (slen == 0)
        {
            xmlErrorSet(xid, node, len);
        }
    }

    return li;
}

double
xmlGetDouble(const void *id)
{
    struct _xml_id *xid = (struct _xml_id *)id;
    double d = 0.0;

    assert(xid != 0);

    if (xid->len)
    {
        char *end = xid->start + xid->len;
        d = strtod(xid->start, &end);
    }

    return d;
}

double
xmlNodeGetDouble(const void *id, const char *path)
{
    struct _xml_id *xid = (struct _xml_id *)id;
    double d = 0.0;

    assert(xid != 0);
    assert(path != 0);

    if (xid->len)
    {
        size_t len, slen;
        char *str, *node;

        len = xid->len;
        slen = strlen(path);
        node = (char *)path;
        str = __xmlNodeGetPath(xid->start, &len, &node, &slen);
        if (str)
        {
            char *end = str+len;
            d = strtod(str, &end);
        }
        else if (slen == 0)
        {
            xmlErrorSet(xid, node, len);
        }
    }

    return d;
}

void *
xmlMarkId(const void *id)
{
    struct _xml_id *xmid = 0;

    assert(id != 0);

    xmid = malloc(sizeof(struct _xml_id));
    if (xmid)
    {
        struct _root_id *xrid = (struct _root_id *)id;
        if (xrid->name == 0)
        {
            xmid->name = "";
            xmid->start = xrid->start;
            xmid->len = xrid->len;
            xmid->name_len = 0;
#ifndef XML_NONVALIDATING
            xmid->root = xrid;
#endif
        }
        else
        {
            memcpy(xmid, id, sizeof(struct _xml_id));
        }
    }
    else
    {
        xmlErrorSet(id, 0, XML_OUT_OF_MEMORY);
    }

    return (void *)xmid;
}

double
xmlAttributeGetDouble(const void *id, const char *name)
{
    struct _xml_id *xid = (struct _xml_id *)id;
    double ret = 0.0;

    assert(xid != 0);
    assert(name != 0);

    if (xid->name_len)
    {
        size_t slen = strlen(name);
        char *ps, *pe;

        assert(xid->start > xid->name);

        ps = xid->name + xid->name_len + 1;
        pe = xid->start - 1;
        while (ps<pe)
        {
            while ((ps<pe) && isspace(*ps)) ps++;
            if (((size_t)(pe-ps) > slen) && (strncasecmp(ps, name, slen) == 0))
            {
                ps += slen;
                if ((ps<pe) && (*ps == '='))
                {
                    char *start;

                    ps++;
                    if (*ps == '"' || *ps == '\'') ps++;
                    else
                    {
                        xmlErrorSet(xid, ps, XML_ATTRIB_NO_OPENING_QUOTE);
                        return 0;
                    }

                    start = ps;
                    while ((ps<pe) && (*ps != '"') && (*ps != '\'')) ps++;
                    if (ps<pe)
                    {
                        ret = strtod(start, &ps);
                    }
                    else
                    {
                        xmlErrorSet(xid, ps, XML_ATTRIB_NO_CLOSING_QUOTE);
                        return 0;
                    }
                }
                else
                {
                    while ((ps<pe) && !isspace(*ps)) ps++;
                    continue;
                }

                break;
            }

            while ((ps<pe) && !isspace(*ps)) ps++;
        }
    }

    return ret;
}

long int
xmlAttributeGetInt(const void *id, const char *name)
{
    struct _xml_id *xid = (struct _xml_id *)id;
    long int ret = 0;

    assert(xid != 0);
    assert(name != 0);

    if (xid->name_len)
    {
        size_t slen = strlen(name);
        char *ps, *pe;

        assert(xid->start > xid->name);

        ps = xid->name + xid->name_len + 1;
        pe = xid->start - 1;
        while (ps<pe)
        {
            while ((ps<pe) && isspace(*ps)) ps++;
            if (((size_t)(pe-ps) > slen) && (strncasecmp(ps, name, slen) == 0))
            {
                ps += slen;
                if ((ps<pe) && (*ps == '='))
                {
                    char *start;

                    ps++;
                    if (*ps == '"' || *ps == '\'') ps++;
                    else
                    {
                        xmlErrorSet(xid, ps, XML_ATTRIB_NO_OPENING_QUOTE);
                        return 0;
                    }

                    start = ps;
                    while ((ps<pe) && (*ps != '"') && (*ps != '\'')) ps++;
                    if (ps<pe)
                    {
                        ret = strtol(start, &ps, 10);
                    }
                    else
                    {
                       xmlErrorSet(xid, ps, XML_ATTRIB_NO_CLOSING_QUOTE);
                       return 0;
                   }
               }
               else
               {
                   while ((ps<pe) && isspace(*ps)) ps++;
                   continue;
               }

               break;
           }

           while ((ps<pe) && !isspace(*ps)) ps++;
        }
    }

    return ret;
}

char *
xmlAttributeGetString(const void *id, const char *name)
{
    struct _xml_id *xid = (struct _xml_id *)id;
    char *ret = 0;

    assert(xid != 0);
    assert(name != 0);

    if (xid->name_len)
    {
        size_t slen = strlen(name);
        char *ps, *pe;

        assert(xid->start > xid->name);

        ps = xid->name + xid->name_len + 1;
        pe = xid->start - 1;
        while (ps<pe)
        {
             while ((ps<pe) && isspace(*ps)) ps++;
             if (((size_t)(pe-ps) > slen) && (strncasecmp(ps, name, slen) == 0))
             {
                 ps += slen;
                 if ((ps<pe) && (*ps == '='))
                 {
                     char *start;

                     ps++;
                     if (*ps == '"' || *ps == '\'') ps++;
                     else
                     {
                         xmlErrorSet(xid, ps, XML_ATTRIB_NO_OPENING_QUOTE);
                         return 0;
                     }

                     start = ps;
                     while ((ps<pe) && (*ps != '"') && (*ps != '\'')) ps++;
                     if (ps<pe)
                     {
                         ret = malloc(ps-start);
                         if (ret)
                         {
                             memcpy(ret, start, (ps-start));
                             *(ret+(ps-start)) = '\0';
                         }
                         else
                         {
                             xmlErrorSet(xid, 0, XML_OUT_OF_MEMORY);
                         }
                     }
                     else
                     {
                         xmlErrorSet(xid, ps, XML_ATTRIB_NO_CLOSING_QUOTE);
                        return 0;
                     }
                 }
                 else
                 {
                     while ((ps<pe) && !isspace(*ps)) ps++;
                     continue;
                 }


                 break;
             }

             while ((ps<pe) && !isspace(*ps)) ps++;
        }
    }

    return ret;
}

size_t
xmlAttributeCopyString(const void *id, const char *name,
                                       char *buffer, size_t buflen)
{
    struct _xml_id *xid = (struct _xml_id *)id;
    size_t ret = 0;

    assert(xid != 0);
    assert(name != 0);
    assert(buffer != 0);
    assert(buflen > 0);

    if (xid->name_len)
    {
        size_t slen = strlen(name);
        char *ps, *pe;

        assert(xid->start > xid->name);

        *buffer = '\0';
        ps = xid->name + xid->name_len + 1;
        pe = xid->start - 1;
        while (ps<pe)
        {
            while ((ps<pe) && isspace(*ps)) ps++;
            if (((size_t)(pe-ps) > slen) && (strncasecmp(ps, name, slen) == 0))
            {
                ps += slen;
                if ((ps<pe) && (*ps == '='))
                {
                    char *start;

                    ps++;
                    if (*ps == '"' || *ps == '\'') ps++;
                    else
                    {
                        xmlErrorSet(xid, ps, XML_ATTRIB_NO_OPENING_QUOTE);
                        return 0;
                    }

                    start = ps;
                    while ((ps<pe) && (*ps != '"') && (*ps != '\'')) ps++;
                    if (ps<pe)
                    {
                        size_t restlen = ps-start;
                        if (restlen >= buflen)
                        {
                            restlen = buflen-1;
                            xmlErrorSet(xid, ps, XML_TRUNCATE_RESULT);
                        }

                        memcpy(buffer, start, restlen);
                        *(buffer+restlen) = 0;
                        ret = restlen;
                    }
                    else
                    {
                        xmlErrorSet(xid, ps, XML_ATTRIB_NO_CLOSING_QUOTE);
                        return 0;
                    }
                }
                else
                {
                    while ((ps<pe) && isspace(*ps)) ps++;
                    continue;
                }

                break;
            }

            while ((ps<pe) && !isspace(*ps)) ps++;
        }
    }

    return ret;
}

int
xmlAttributeCompareString(const void *id, const char *name, const char *s)
{
    struct _xml_id *xid = (struct _xml_id *)id;
    int ret = -1;
 
    assert(xid != 0);
    assert(name != 0);
    assert(s != 0);

    if (xid->name_len && strlen(s))
    {
        size_t slen = strlen(name);
        char *ps, *pe;

        assert(xid->start > xid->name);

        ps = xid->name + xid->name_len + 1;
        pe = xid->start - 1;
        while (ps<pe)
        {
            while ((ps<pe) && isspace(*ps)) ps++;
            if (((size_t)(pe-ps) > slen) && (strncasecmp(ps, name, slen) == 0))
            {
                ps += slen;
                if ((ps<pe) && (*ps == '='))
                {
                    char *start;

                    ps++;
                    if (*ps == '"' || *ps == '\'') ps++;
                    else
                    {
                        xmlErrorSet(xid, ps, XML_ATTRIB_NO_OPENING_QUOTE);
                        return 0;
                    }

                    start = ps;
                    while ((ps<pe) && (*ps != '"') && (*ps != '\'')) ps++;
                    if (ps<pe)
                    {
                        ret = strncasecmp(start, s, ps-start);
                    }
                    else
                    {
                        xmlErrorSet(xid, ps, XML_ATTRIB_NO_CLOSING_QUOTE);
                        return 0;
                    }
                }
                else
                {
                    while ((ps<pe) && !isspace(*ps)) ps++;
                    continue;
                }

                break;
            }

            while ((ps<pe) && !isspace(*ps)) ps++;
        }
    }

    return ret;
}


#ifndef XML_NONVALIDATING
int
xmlErrorGetNo(const void *id, int clear)
{
    int ret = 0;

    if (id)
    {
        struct _xml_id *xid = (struct _xml_id *)id;
        struct _root_id *rid;

        if (xid->name) rid = xid->root;
        else rid = (struct _root_id *)xid;

        assert(rid != 0);

        if (rid->info)
        {
            struct _xml_error *err = rid->info;

            ret = err->err_no;
            if (clear) err->err_no = 0;
        }
    }

    return ret;
}

size_t
xmlErrorGetLineNo(const void *id, int clear)
{
    size_t ret = 0;

    if (id)
    {
        struct _xml_id *xid = (struct _xml_id *)id;
        struct _root_id *rid;

        if (xid->name) rid = xid->root;
        else rid = (struct _root_id *)xid;

        assert(rid != 0);

        if (rid->info)
        {
            struct _xml_error *err = rid->info;
            char *ps = rid->start;
            char *pe = err->pos;
            char *new;

            ret++;
            while (ps<pe)
            {
               new = memchr(ps, '\n', pe-ps);
               if (new) ret++;
               else break;
               ps = new+1;
            }       

            if (clear) err->err_no = 0;
        }
    }

    return ret;
}

size_t
xmlErrorGetColumnNo(const void *id, int clear)
{
    size_t ret = 0;

    if (id)
    {
        struct _xml_id *xid = (struct _xml_id *)id;
        struct _root_id *rid;

        if (xid->name) rid = xid->root;
        else rid = (struct _root_id *)xid;

        assert(rid != 0);

        if (rid->info)
        {
            struct _xml_error *err = rid->info;
            char *ps = rid->start;
            char *pe = err->pos;
            char *new;

            while (ps<pe)
            {
               new = memchr(ps, '\n', pe-ps);
               new = memchr(ps, '\n', pe-ps);
               if (new) ret++;
               else break;
               ps = new+1;
            }
            ret = pe-ps;

            if (clear) err->err_no = 0;
        }
    }

    return ret;
}

const char *
xmlErrorGetString(const void *id, int clear)
{
    char *ret = 0;

    if (id)
    {
        struct _xml_id *xid = (struct _xml_id *)id;
        struct _root_id *rid;

        if (xid->name) rid = xid->root;
        else rid = (struct _root_id *)xid;

        assert(rid != 0);

        if (rid->info)
        {
            struct _xml_error *err = rid->info;
            if (XML_NO_ERROR <= err->err_no && err->err_no < XML_MAX_ERROR)
            {
               ret = (char *)__xml_error_str[err->err_no];
            }
            else
            {
               ret = "incorrect error number.";
            }

            if (clear) err->err_no = 0;
        }
    }

    return ret;
}
#endif

/* -------------------------------------------------------------------------- */

#ifndef XML_NONVALIDATING
static const char *__xml_error_str[XML_MAX_ERROR] =
{
    "no error.",
    "unable to allocate enough memory.",
    "unable to open file for reading.",
    "requested node name is invalid.",
    "unexpected end of section.",
    "buffer too small to hold all data, truncating.",
    "incorrect comment section.",
    "bad information block.",
    "incompatible opening tag for element.",
    "missing or invalid closing tag for element.",
    "missing or invalid opening quote for attribute.",
    "missing or invalid closing quote for attribute."
};
#endif

char *
__xmlNodeGetPath(const char *start, size_t *len, char **name, size_t *nlen)
{
    char *path;
    char *ret = 0;

    assert(start != 0);
    assert(len != 0);
    assert(*len != 0);
    assert(name != 0);
    assert(*name != 0);
    assert(nlen != 0);
    assert(*nlen != 0);

    if (*nlen > *len) return 0;

    path = *name;
    if (*path == '/') path++;
    if (*path != '\0')
    {
        size_t num, blocklen, pathlen, nodelen;
        char *node;

        node = path;
        pathlen = strlen(path);
        path = strchr(node, '/');

        if (!path) nodelen = pathlen;
        else nodelen = path++ - node;

        num = 0;
        blocklen = *len;
        ret = __xmlNodeGet(start, &blocklen, &node, &nodelen, &num);
        if (ret)
        {
            if (path)
            {
                ret = __xmlNodeGetPath(ret, &blocklen, &path, &pathlen);
                *name = path;
                *len = blocklen;
                *nlen = pathlen;
            }
            else
            {
               *name = node;
               *nlen = nodelen;
               *len = blocklen;
            }
        }
        else
        {
            *len = 0;
            *nlen = 0;
        }
    }

    return ret;
}

char *
__xmlNodeGet(const char *start, size_t *len, char **name, size_t *rlen, size_t *nodenum)
{
    char *cdata, *open_element = *name;
    char *element, *start_tag=0;
    char *new, *cur, *ne, *ret = 0;
    size_t restlen, elementlen;
    size_t open_len = *rlen;
    size_t return_len = 0;
    int found, num;

    assert(start != 0);
    assert(len != 0);
    assert(name != 0);
    assert(rlen != 0);
    assert(nodenum != 0);

    if (open_len == 0 || *name == 0)
        SET_ERROR_AND_RETURN((char *)start, XML_INVALID_NODE_NAME);

    cdata = (char *)start;
    if (*rlen > *len)
        SET_ERROR_AND_RETURN((char *)start, XML_UNEXPECTED_EOF);

    found = 0;
    num = *nodenum;
    restlen = *len;
    cur = (char *)start;
    ne = cur + restlen;

    /* search for an opening tag */
    while ((new = memchr(cur, '<', restlen)) != 0)
    {
        size_t len_remaining;
        char *rptr;

        if (*(new+1) == '/')		/* end of section */
        {
            *len -= restlen;
             break;
        }

        new++;
        restlen -= new-cur;
        cur = new;

        if (*cur == '!') /* comment */
        {
            char *start = cur;
            size_t blocklen = restlen;
            new = __xmlProcessCDATA(&start, &blocklen);
            if (!new && start && open_len)                       /* CDATA */
                SET_ERROR_AND_RETURN(cur, XML_INVALID_COMMENT);

            restlen -= new-cur;
            cur = new;
            continue;
        }
        else if (*cur == '?') /* info block */
        {
            new = __xmlInfoProcess(cur, restlen);
            if (!new)
                SET_ERROR_AND_RETURN(cur, XML_INVALID_INFO_BLOCK);

            restlen -= new-cur;
            cur = new;
            continue;
        }

        /*
         * get element name and a pointer to after the opening tag
         */
        element = *name;
        elementlen = *rlen;
        len_remaining = restlen;
        new = rptr = __xml_memncasecmp(cur, &restlen, &element, &elementlen);
        if (rptr) 			/* requested element was found */
        {
            return_len = elementlen;
            if (found == num)
            {
                ret = new;
                open_len = elementlen;
                start_tag = element;
            }
            else start_tag = 0;
        }
        else				/* different element name was foud */
        {
            new = cur + (len_remaining - restlen);
            if (new >= ne)
                SET_ERROR_AND_RETURN(cur, XML_UNEXPECTED_EOF);

            element = *name;
        }

        if (*(new-2) == '/')				/* e.g. <test/> */
        {
            cur = new;
            if (rptr)
            {
                if (found == num)
                {
                    open_element = start_tag;
                    *len = 0;
                }
                found++;
            }
            continue;
        }

        /*
         * get the next xml tag
         */
        /* restlen -= new-cur; not necessary because of __xml_memncasecmp */
        cur = new;
        new = memchr(cur, '<', restlen);
        if (!new)
            SET_ERROR_AND_RETURN(cur, XML_ELEMENT_NO_CLOSING_TAG);

        new++;
        restlen -= new-cur;
        cur = new;
        if (*cur == '!')				/* comment, CDATA */
        {
            char *start = cur;
            size_t blocklen = restlen;
            new = __xmlProcessCDATA(&start, &blocklen);
            if (new && start && open_len)			/* CDATA */
            {
                cdata = ret;
            }
            else if (!new)
                SET_ERROR_AND_RETURN(cur, XML_INVALID_COMMENT);

            restlen -= new-cur;
            cur = new;

            /*
             * look for the closing tag of the cascading block
             */
            new = memchr(cur, '<', restlen);
            if (!new)
                SET_ERROR_AND_RETURN(cur, XML_ELEMENT_NO_CLOSING_TAG);

            new++;
            restlen -= new-cur;
            cur = new;
        }

        if (*cur != '/')			/* cascading tag found */
        {
            char *node = "*";
            size_t slen = restlen+1; /* due to cur-1 below*/
            size_t nlen = 1;
            size_t pos = -1;

            /*
             * recursively walk the xml tree from here
             */
            new = __xmlNodeGet(cur-1, &slen, &node, &nlen, &pos);
            if (!new)
            {
                if (nlen == 0)		/* error upstream */
                {
                    *rlen = nlen;
                    *name = node;
                    *len = slen;
                    return 0;
                }

                if (slen == restlen)
                    SET_ERROR_AND_RETURN(cur, XML_UNEXPECTED_EOF);

                slen--;
                new = cur + slen;
                restlen -= slen;
            }
            else restlen -= slen;

            /* 
             * look for the closing tag of the cascading block
             */
            cur = new;
            new = memchr(cur, '<', restlen);
            if (!new)
                SET_ERROR_AND_RETURN(cur, XML_ELEMENT_NO_CLOSING_TAG);

            new++;
            restlen -= new-cur;
            cur = new;
        }

        if (*cur == '/')				/* closing tag found */
        {
            if (!strncasecmp(new+1, element, elementlen))
            {
                if (*(new+elementlen+1) != '>')
                    SET_ERROR_AND_RETURN(new+1, XML_ELEMENT_NO_CLOSING_TAG);

                if (found == num)
                {
                    if (start_tag)
                    {
                        *len = new-ret-1;
                        open_element = start_tag;
                        cdata = (char *)start;
                        start_tag = 0;
                    }
                    else /* report error */
                        SET_ERROR_AND_RETURN(new, XML_ELEMENT_NO_OPENING_TAG);
                }
                found++;
            }

            new = memchr(cur, '>', restlen);
            if (!new)
                SET_ERROR_AND_RETURN(cur, XML_ELEMENT_NO_CLOSING_TAG);

            restlen -= new-cur;
            cur = new;
        }
        else
            SET_ERROR_AND_RETURN(cur, XML_ELEMENT_NO_CLOSING_TAG);

    } /* while */

    if (found == 0)
    {
        ret = 0;
        *rlen = 0;
        *name = start_tag;
        *len = XML_NO_ERROR;	/* element not found, no real error */
    }
    else
    {
        *rlen = open_len;
        *name = open_element;
        *nodenum = found;
    }

    return ret;
}

char *
__xmlProcessCDATA(char **start, size_t *len)
{
    char *cur, *new;
    size_t restlen = *len;

    cur = *start;
    if ((restlen > 6) && (*(cur+1) == '-'))             /* comment */
    {
        new = __xmlCommentSkip(cur, restlen);
        if (new)
        {
            *start += 3;				/* !-- */
            *len = (*start-cur)-3;			/* --> */
        }
        return new;
    }

    if (restlen < 12) return 0;                         /* ![CDATA[ ]]> */

    cur = *start;
    new = 0;

    if (memcmp(cur, "![CDATA[", 8) == 0)
    {
        *start = cur+8;
        cur += 8;
        restlen -= 8;
        do
        {
            new = memchr(cur, ']', restlen);
            if (new)
            {
                if ((restlen > 3) && (memcmp(new, "]]>", 3) == 0))
                {
                    *len = new-*start;
                    restlen -= 3;
                    new += 3;
                    break;
                }
                cur = new+1;
            }
            else
            {
                *len = 0;
                break;
            }
        }
        while (new && (restlen > 2));
    }

    return new;
}

char *
__xmlCommentSkip(const char *start, size_t len)
{
    char *cur, *new;

    if (len < 7) return 0;				 /* !-- --> */

    cur = (char *)start;
    new = 0;

    if (memcmp(cur, "!--", 3) == 0)
    {
        cur += 3;
        len -= 3;
        do
        {
            new = memchr(cur, '-', len);
            if (new)
            {
                len -= new - cur;
                if ((len >= 3) && (memcmp(new, "-->", 3) == 0))
                {
                    new += 3;
                    len -= 3;
                    break;
                }
                cur = new+1;
            }
            else break;
        }
        while (new && (len > 2));
    }

    return new;
}

char *
__xmlInfoProcess(const char *start, size_t len)
{
    char *cur, *new;

    cur = (char *)start;
    new = 0;

    if (*cur == '?')
    {
        if (len < 3) return 0;				/* <? ?> */

        cur++;
        len--;
        new = memchr(cur, '?', len);
        if (!new || *(new+1) != '>') return 0;

        new += 2;
    }

    return new;
}


static void __xmlPrepareData(char **start, size_t *blocklen)
{
    size_t len = *blocklen;
    char *pe, *ps = *start;

    pe = ps + len;
    while ((ps<pe) && isspace(*ps)) ps++;
    while ((pe>ps) && isspace(*pe)) pe--;
    len = (pe-ps);

    /* CDATA or comment */
    if ((len >= 2) && !strncmp(ps, "<!", 2))
    {
        char *start = ps+1;
        size_t blocklen = len-1;
        if (blocklen >= 6)                  /* !-- --> */
        {
            char *new = __xmlProcessCDATA(&start, &blocklen);
            if (new)
            {
                ps = start;
                len = blocklen;
                pe = ps + len;

                while ((ps<pe) && isspace(*ps)) ps++;
                while ((pe>ps) && isspace(*pe)) pe--;
                len = (pe-ps);
            }
        }
    }

    *start = ps;
    *blocklen = len;
}

#define NOCASECMP(a,b)  ( ((a)^(b)) & 0xdf )
void *
__xml_memncasecmp(const char *haystack, size_t *haystacklen,
                  char **needle, size_t *needlelen)
{
    char *rptr = 0;

    if (haystack && needle && needlelen && (*needlelen > 0)
        && (*haystacklen >= *needlelen))
    {
        char *hs = (char *)haystack;
        char *ns;
        size_t i;

        ns = *needle;

        /* search for everything */
        if ((*ns == '*') && (*needlelen == 1))
        {
           char *he = hs + *haystacklen;

           while ((hs < he) && !isspace(*hs) && (*hs != '>')) hs++;
           if (*(hs-1) == '/') hs--;

           *needle = (char *)haystack;
           *needlelen = hs - haystack;

           ns = memchr(hs, '>', he-hs);
           if (ns) hs = ns+1;
           else hs = he;
     
           rptr = hs;
        }
        else
        {
            size_t nlen = *needlelen;
            char *he = hs + *haystacklen;

            for (i=0; i<nlen; i++)
            {
                if (NOCASECMP(*hs,*ns) && (*ns != '?')) break;
                if (isspace(*hs) || (*hs == '/') || (*hs == '>')) break;
                hs++;
                ns++;
            }

            if (i == nlen)
            {
                *needle = (char *)haystack;
                *needlelen = hs - haystack;

                ns = memchr(hs, '>', he-hs);
                if (ns) hs = ns+1;
                else hs = he;

                rptr = hs;
            }
            else /* not found */
            {
                while((hs < he) && !isspace(*hs) && (*hs != '>')) hs++;
                if (*(hs-1) == '/') hs--;

                *needle = (char *)haystack;
                *needlelen = hs - haystack;

                ns = memchr(hs, '>', he-hs);
                if (ns) hs = ns+1;
                else hs = he;
            }
        }

        *haystacklen -= hs - haystack;
    }

    return rptr;
}

#ifndef XML_NONVALIDATING
void
__xmlErrorSet(const void *id, const char *pos, unsigned int err_no)
{
   struct _xml_id *xid = (struct _xml_id *)id;
   struct _root_id *rid;

   assert(xid != 0);

   if (xid->name) rid = xid->root;
   else rid = (struct _root_id *)xid;

   assert(rid != 0);
   if (rid->info == 0)
   {
      rid->info = malloc(sizeof(struct _xml_error));
   }

   if (rid->info)
   {
      struct _xml_error *err = rid->info;

      err->pos = (char *)pos;
      err->err_no = err_no;
   }
}
#endif

#ifdef WIN32
/* Source:
 * https://mollyrocket.com/forums/viewtopic.php?p=2529
 */

void *
simple_mmap(int fd, size_t length, SIMPLE_UNMMAP *un)
{
    HANDLE f;
    HANDLE m;
    void *p;

    f = (HANDLE)_get_osfhandle(fd);
    if (!f) return (void *)-1;

    m = CreateFileMapping(f, NULL, PAGE_READONLY, 0, 0, NULL);
    if (!m) return (void *)-1;

    p = MapViewOfFile(m, FILE_MAP_READ, 0,0,0);
    if (!p)
    {
        CloseHandle(m);
        return (void *)-1;
    }

    if (un)
    {
        un->m = m;
        un->p = p;
    }

    return p;
}

void
simple_unmmap(SIMPLE_UNMMAP *un)
{
    UnmapViewOfFile(un->p);
    CloseHandle(un->m);
}
#endif
