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
#include <fcntl.h>
#include <stdlib.h>	/* free, malloc */
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <assert.h>
#include <ctype.h>
#ifndef _MSC_VER
# include <strings.h>  /* strncasecmp */
#else
# define strncasecmp strnicmp
#endif

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
#else /* !XML_NONVALIDATING */
# define xmlErrorSet(a, b, c)
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

static char *__xmlNodeCopy(const char *, size_t *, const char **);
static char *__xmlNodeGetPath(const char *, size_t *, char **, size_t *);
static char *__xmlNodeGet(const char *, size_t *, char **, size_t *, size_t *);
static char *__xmlCommentSkip(const char *, size_t);
static char *__xmlInfoProcess(const char *, size_t);

static void *__xml_memncasecmp(const char *, size_t *, char **, size_t *);

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
        if (fd > 0)
        {
            rid = malloc(sizeof(struct _root_id));
            if (rid)
            {
                struct stat statbuf;

                fstat(fd, &statbuf);

                rid->fd = fd;
                rid->len = statbuf.st_size;
                rid->start = mmap(0, rid->len, PROT_READ, MAP_PRIVATE, fd, 0L);
                rid->name = 0;
#ifndef XML_NONVALIDATING
                rid->info = 0;
#endif
            }
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

     munmap(rid->start, rid->len);
     close(rid->fd);

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

    slen = buflen-1;
    if (slen > xid->name_len)
    {
        slen = xid->name_len;
    }
    else
    {
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
        char *ps, *pe, *pend;
        int nlen;

        ps = xid->start;
        nlen = xid->len;
        pend = ps+nlen;
        pe = pend;

        while ((ps<pe) && isspace(*ps)) ps++;
        while ((pe>ps) && isspace(*pe)) pe--;
        nlen = (pe-ps);
        if (nlen)
        {
            str = malloc(nlen+1);
            if (str)
            {
                memcpy(str, ps, nlen);
                *(str+nlen) = 0;
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

    if (xid->len)
    {
        char *ps, *pe, *pend;
        size_t slen, nlen;

        *buffer = '\0';
        nlen = buflen-1;
        ps = xid->start;
        slen = xid->len;
        pend = ps+slen;
        pe = pend;

        while ((ps<pe) && isspace(*ps)) ps++;
        while ((pe>ps) && isspace(*pe)) pe--;
        nlen = (pe-ps);
        if (nlen > slen) nlen = slen;

        if (nlen)
        {
            if (nlen >= buflen)
            {
                nlen = buflen-1;
                xmlErrorSet(xid, 0, XML_TRUNCATE_RESULT);
            }
            memcpy(buffer, ps, nlen);
            *(buffer+nlen) = 0;
            ret = nlen;
        }
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
        char *ps, *pe;

        ps = xid->start;
        pe = ps + xid->len;
        pe--;

        while ((ps<pe) && isspace(*ps)) ps++;
        while ((pe>ps) && isspace(*pe)) pe--;
        pe++;

        ret = strncasecmp(ps, s, pe-ps);
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
        size_t len = xid->len;
        char *node = (char *)path;

        str = __xmlNodeCopy(xid->start, &len, &path);
        if (str)
        {
            char *ps, *pe, *pend;
            int slen;

            ps = str;
            slen = strlen(str);
            pend = ps+slen;
            pe = pend-1;

            while ((ps<pe) && isspace(*ps)) ps++;
            while ((pe>ps) && isspace(*pe)) pe--;

            *++pe = 0;
            slen = (pe-ps);
            if (slen && (ps>str)) memmove(str, ps, slen);
            else if (!slen) *str = 0;
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

    if (xid->len)
    {
        char *str, *node;
        size_t slen, nlen;

        *buffer = '\0';
        nlen = xid->len;
        slen = strlen(path);
        node = (char *)path;
        str = __xmlNodeGetPath(xid->start, &nlen, &node, &slen);
        if (str)
        {
            char *ps, *pe;

            ps = str;
            pe = ps+nlen-1;

            while ((ps<pe) && isspace(*ps)) ps++;
            while ((pe>ps) && isspace(*pe)) pe--;

            nlen = (pe-ps)+1;
            if (nlen >= buflen)
            {
                nlen = buflen-1;
                xmlErrorSet(xid, 0, XML_TRUNCATE_RESULT);
            }

            memcpy(buffer, ps, nlen);
            *(buffer + nlen) = '\0';
            ret = nlen;
        }
        else if (slen == 0)
        {
            xmlErrorSet(xid, node, nlen);
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
            pe = ps + len;
            pe--;

            while ((ps<pe) && isspace(*ps)) ps++;
            while ((pe>ps) && isspace(*pe)) pe--;
            pe++;

            ret = strncasecmp(ps, s, pe-ps);
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

    if (xid->len && xid->name_len)
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

    if (xid->len && xid->name_len)
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

    if (xid->len && xid->name_len)
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

    if (xid->len && xid->name_len)
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

    if (xid->len && xid->name_len && strlen(s))
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
               if (new)
               {
                  ps = new;
                  ret++;
               }
               ps++;
            }       

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
    "buffer too small to hold all data, truncating.",
    "incorrect comment section.",
    "bad information block.",
    "unexpected end of section.",
    "incompatible opening tag for element.",
    "missing or invalid closing tag for element.",
    "missing or invalid opening quote for attribute.",
    "missing or invalid closing quote for attribute."
};
#endif

char *
__xmlNodeCopy(const char *start, size_t *len, const char **path)
{
    char *node, *p, *ret = 0;
    size_t rlen, slen;

    rlen = *len;
    slen = strlen(*path);
    node = (char *)*path;
    p = __xmlNodeGetPath(start, &rlen, &node, &slen);
    if (p && rlen)
    {
        ret = malloc(rlen+1);
        if (ret)
        {
            memcpy(ret, p, rlen);
            *(ret+rlen+1) = '\0';
        }
        else
        {
            xmlErrorSet(0, 0, XML_OUT_OF_MEMORY);
        }
    }
    else if (slen == 0)
    {
        *path = node;
        *len = rlen;
    }

    return ret;
}

char *
__xmlNodeGetPath(const char *start, size_t *len, char **name, size_t *plen)
{
    char *node;
    char *ret = 0;

    assert(start != 0);
    assert(len != 0);
    assert(name != 0);
    assert(*name != 0);
    assert(plen != 0);
    assert(*plen != 0);

    if ((*len == 0) || (*plen == 0) || (*plen > *len))
        return 0;

    node = *name;
    if (*node == '/') node++;
    if (*node != 0)
    {
        size_t plen, slen;
        char *path;
        size_t num;

        slen = strlen(node);
        path = strchr(node, '/');

        if (!path) plen = slen;
        else plen = path++ - node;

        if (path)
        {
            num = 0;
            ret = __xmlNodeGet(start, len, &node, &plen, &num);
            if (ret)
            {
                plen = slen - (path - *name);
                ret = __xmlNodeGetPath(ret, len, &path, &plen);
                *name = path;
            }
            else if (plen == 0)
            {
               *name = node;
            }
        }
    }

    return ret;
}

char *
__xmlNodeGet(const char *start, size_t *len, char **name, size_t *rlen, size_t *nodenum)
{
    char *new, *cur, *ne, *ret = 0;
    char *element, *start_tag=0;
    size_t restlen, elementlen;
    size_t retlen = 0;
    int found, num;

    assert(start != 0);
    assert(len != 0);
    assert(*len != 0);
    assert(name != 0);
    assert(*name != 0);
    assert(rlen != 0);
    assert(*rlen != 0);
    assert(nodenum != 0);

    if (*rlen > *len)
    {
        *rlen = 0;
        *name = start;
        *len = XML_UNEXPECTED_EOF;
        return 0;
    }

    found = 0;
    num = *nodenum;
    restlen = *len;
    cur = (char *)start;
    ne = cur + restlen;

    while ((new = memchr(cur, '<', restlen)) != 0)
    {
        if (*(new+1) == '/')		/* cascading closing tag found */
        {
            *len -= restlen;
             break;
        }

        new++;
        restlen -= new-cur;
        cur = new;

        if (*cur == '!')
        {
            new = __xmlCommentSkip(cur, restlen);
            if (!new)
            {
                *rlen = 0;
                *name = cur;
                *len = XML_INVALID_COMMENT;
                return 0;
            }
            restlen -= new-cur;
            cur = new;
            continue;
        }
        else if (*cur == '?')
        {
            new = __xmlInfoProcess(cur, restlen);
            if (!new)
            {
                *rlen = 0;
                *name = cur;
                *len = XML_INVALID_INFO_BLOCK;
                return 0;
            }

            restlen -= new-cur;
            cur = new;
            continue;
        }

        element = *name;
        elementlen = *rlen;
        new = __xml_memncasecmp(cur, &restlen, &element, &elementlen);
        if (new)
        {
            retlen = elementlen;
            if (found == num)
            {
                ret = new+1;
                start_tag = element;
                *rlen = elementlen;
            }
            else start_tag = 0;
        }
        else
        {
            new = cur+elementlen;
            if (new >= ne)
            {
                *rlen = 0;
                *name = cur;
                *len = XML_UNEXPECTED_EOF;
                return 0;
            }
            element = *name;
        }

        if (*(new-1) == '/')				/* e.g. <test/> */
        {
            if (found == num)
            {
               *len = 0;
               *name = start_tag;
            }
            found++;

            if ((restlen < 1) || (*new != '>'))
            {
                *rlen = 0;
                *name = cur;
                *len = XML_ELEMENT_NO_CLOSING_TAG;
                return 0;
            }

            restlen -= new+1-cur;
            cur = new+1;
            continue;
        }

        /* restlen -= new-cur; not necessary because of __xml_memncasecmp */
        cur = new;
        new = memchr(cur, '<', restlen);
        if (!new)
        {
            *rlen = 0;
            *name = cur;
            *len = XML_ELEMENT_NO_CLOSING_TAG;
            return 0;
        }       

        restlen -= new-cur;
        cur = new;
        if (*(cur+1) != '/')                            /* new node found */
        {
            char *node = "*";
            size_t slen = restlen;
            size_t nlen = 1;
            size_t pos = -1;

            new = __xmlNodeGet(cur, &slen, &node, &nlen, &pos);
            if (!new)
            {
                if (slen == restlen)
                {
                    *rlen = 0;
                    *name = cur;
                    *len = XML_UNEXPECTED_EOF;
                    return 0;
                }
                else new = cur + slen;
            }

            restlen -= slen;
            cur = new;

            new = memchr(cur, '<', restlen);
            if (!new)
            {
                *rlen = 0;
                *name = cur;
                *len = XML_ELEMENT_NO_CLOSING_TAG;
                return 0;
            }

            restlen -= new-cur;
            cur = new;
        }

        if (*(cur+1) == '/')				/* closing tag found */
        {
            if (!strncasecmp(new+2, element, elementlen))
            {
                if (found == num)
                {
                   if (start_tag)
                   {
                       *len = new-ret;
                       *name = start_tag;
                   }
                   else /* report error */
                   {
                       *rlen = 0;
                       *name = new;
                       *len = XML_ELEMENT_NO_OPENING_TAG;
                       return 0;
                   }
                }
                found++;
            }

            new = memchr(cur, '>', restlen);
            if (!new)
            {
                *rlen = 0;
                *name = cur;
                *len = XML_ELEMENT_NO_CLOSING_TAG;
                return 0;
            }

            restlen -= new-cur;
            cur = new;
        }
        else
        {
            *rlen = 0;
            *name = cur;
            return 0;
        }
    }

    if ((ret == 0) && (start_tag == 0) && (*rlen > 1))
    {
        ret = 0;
        *rlen = 0;
        *name = start_tag;
        *len = XML_NO_ERROR;	/* element not found */
    }
    else
    {
        *rlen = retlen;
        *nodenum = found;
    }

    return ret;
}

char *
__xmlCommentSkip(const char *start, size_t len)
{
    char *cur, *new;

    cur = (char *)start;
    new = 0;

    if (memcmp(cur, "!--", 3) == 0)
    {
        if (len < 6) return 0;				/* <!-- --> */

        cur += 3;
        len -= 3;
        do
        {
            new = memchr(cur, '-', len);
            if (new)
            {
                len -= new - cur;
                if ((len > 3) && (memcmp(new, "-->", 3) == 0))
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


#define NOCASECMP(a,b)  ( ((a)^(b)) & 0xdf )
void *
__xml_memncasecmp(const char *haystack, size_t *haystacklen,
                  char **needle, size_t *needlelen)
{
    void *rptr = 0;

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
           *needle = (char *)haystack;
           *needlelen = hs - haystack;
           while ((hs < he) && (*hs != '>')) hs++;
           rptr = hs;
        }
        else
        {
            size_t nlen = *needlelen;
            char *he = hs + *haystacklen;

            for (i=0; i<nlen; i++)
            {
                if (NOCASECMP(*hs,*ns) && (*ns != '?')) break;
                if (isspace(*hs) || (*hs == '>')) break;
                hs++;
                ns++;
            }

            if (i != nlen)
            {
                while((hs < he) && !isspace(*hs) && (*hs != '>')) hs++;
                *needle = (char *)haystack;
                *needlelen = hs - haystack;
            }
            else
            {
                int found = (isspace(*hs) || (*hs == '>'));

                *needle = (char *)haystack;
                *needlelen = hs - haystack;

                while ((hs < he) && (*hs != '>')) hs++;

                if (!found) *needlelen = hs - haystack;
                else rptr = hs;
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
    if (!f) return NULL;

    m = CreateFileMapping(f, NULL, PAGE_READONLY, 0, 0, NULL);
    if (!m) return NULL;

    p = MapViewOfFile(m, FILE_MAP_READ, 0,0,0);
    if (!p)
    {
        CloseHandle(m);
        return NULL;
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
