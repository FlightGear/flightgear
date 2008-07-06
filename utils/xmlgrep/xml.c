/* Copyright (c) 2007, 2008 by Adalin B.V.
 * Copyright (c) 2007, 2008 by Erik Hofman
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
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

typedef struct
{
   HANDLE m;
   void *p;
} SIMPLE_UNMMAP;

static SIMPLE_UNMMAP un;

/*
 * map 'filename' and return a pointer to it.
 */
void *simple_mmap(int, unsigned int, SIMPLE_UNMMAP *);
void simple_unmmap(SIMPLE_UNMMAP *);

#define mmap(a,b,c,d,e,f)	simple_mmap((e), (b), &un)
#define munmap(a,b)		simple_unmmap(&un)

#else	/* !WIN32 */
# include <sys/mman.h>
# include <fcntl.h>
#endif

#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <ctype.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


struct _xml_id
{
    char *start;
    size_t len;
    int fd;
};

static char *__xmlCopyNode(char *, size_t, const char *);
static char *__xmlGetNode(char *, size_t, const char *, size_t *);
static void *__xml_memmem(const void *, size_t, const void *, size_t);
static void *__xml_memncasecmp(void *, size_t, void **, size_t *);

#define PRINT(a, b, c) { \
   size_t q, len=(((b)>(c)) ? (c) : (b)); \
   if (a) { \
      printf("(%i) '", len); \
      for (q=0; q<len; q++) printf("%c", (a)[q]); \
      printf("'\n"); \
   } else printf("NULL pointer at line %i\n", __LINE__); \
}

void *
xmlOpen(char *fn)
{
    struct _xml_id *id = 0;

    if (fn)
    {
        int fd = open(fn, O_RDONLY);
        if (fd > 0)
        {
            id = malloc(sizeof(struct _xml_id));
            if (id)
            {
                struct stat statbuf;

                fstat(fd, &statbuf);

                id->fd = fd;
                id->len = statbuf.st_size;
                id->start = mmap(0, id->len, PROT_READ, MAP_PRIVATE, fd, 0L);
            }
        }
    }

    return (void*)id;
}

void
xmlClose(void *id)
{
    if (id)
    {
        struct _xml_id *xid = (struct _xml_id *)id;
        munmap(xid->start, xid->len);
        close(xid->fd);
        free(id);
        id = 0;
    }
}

void *
xmlCopyNode(void *id, char *path)
{
    struct _xml_id *xsid = 0;

    if (id && path)
    {
        struct _xml_id *xid = (struct _xml_id *)id;
        char *ptr, *p;
        size_t rlen;

        rlen = strlen(path);
        ptr = __xmlGetNode(xid->start, xid->len, path, &rlen);
        if (ptr)
        {
            xsid = malloc(sizeof(struct _xml_id) + rlen);
            if (xsid)
            {
                p = (char *)xsid + sizeof(struct _xml_id);

                xsid->len = rlen;
                xsid->start = p;
                xsid->fd = 0;

                memcpy(xsid->start, ptr, rlen);
           }
       }
   }

   return (void *)xsid;
}

void *
xmlGetNode(void *id, char *path)
{
   struct _xml_id *xsid = 0;

   if (id && path)
   {
      struct _xml_id *xid = (struct _xml_id *)id;
      size_t rlen;
      char *ptr;

      rlen = strlen(path);
      ptr = __xmlGetNode(xid->start, xid->len, path, &rlen);
      if (ptr)
      {
         xsid = malloc(sizeof(struct _xml_id));
         xsid->len = rlen;
         xsid->start = ptr;
         xsid->fd = 0;
      }
   }

   return (void *)xsid;
}

const char *
xmlGetNextElement(const void *pid, void *id, char *path)
{
    struct _xml_id *xpid = (struct _xml_id *)pid;
    const char *ret;

    if (id && path)
    {
        struct _xml_id *xid = (struct _xml_id *)id;
        size_t rlen, nlen;
        char *ptr;

        if (xid->len < xpid->len) xid->start += xid->len;
        nlen = xpid->len - (xid->start - xpid->start);

        rlen = strlen(path);
        ptr = __xmlGetNode(xid->start, nlen, path, &rlen);
        if (ptr)
        {
             xid->len = rlen;
             xid->start = ptr;
             ret = path;
        }
        else ret = 0;
    }
    else ret = 0;

    return ret;
}

int
xmlCompareString(const void *id, const char *s)
{
    struct _xml_id *xid = (struct _xml_id *)id;
    int ret = -1;

    if (xid && xid->len && s && (strlen(s) > 0))
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

int
xmlCompareNodeString(const void *id, const char *path, const char *s)
{
    struct _xml_id *xid = (struct _xml_id *)id;
    int ret = -1;

    if (xid && xid->len && path && s && (strlen(s) > 0))
    {
        char *str, *ps, *pe;
        size_t rlen;

        rlen = strlen(path);
        str = __xmlGetNode(xid->start, xid->len, path, &rlen);

        ps = str;
        pe = ps + rlen;
        pe--;

        while ((ps<pe) && isspace(*ps)) ps++;
        while ((pe>ps) && isspace(*pe)) pe--;
        pe++;

        if (str) ret = strncasecmp(pe, s, pe-ps);
    }

    return ret;
}

char *
xmlGetNodeString(void *id, const char *path)
{
    struct _xml_id *xid = (struct _xml_id *)id;
    char *str = 0;

    if (xid && xid->len && path)
    {
        str = __xmlCopyNode(xid->start, xid->len, path);
        if (str)
        {
            char *ps, *pe, *pend;
            int slen;

            slen = strlen(str);
            ps = str;
            pend = ps+slen;
            pe = pend-1;

            while ((ps<pe) && isspace(*ps)) ps++;
            while ((pe>ps) && isspace(*pe)) pe--;

            *++pe = 0;
            slen = (pe-ps);
            if (slen && (ps>str)) memmove(str, ps, slen);
            else if (!slen) *str = 0;
        }
    }

    return str;
}

char *
xmlGetString(void *id)
{
    struct _xml_id *xid = (struct _xml_id *)id;
    char *str = 0;

    if (xid && xid->len)
    {
        str = malloc(xid->len+1);
        if (str)
        {
            char *ps, *pe, *pend;
            int slen;

            slen = xid->len;
            memcpy(str, xid->start, slen);

            ps = str;
            pend = ps+slen;
            pe = pend-1;
            *pend = 0;

            while ((ps<pe) && isspace(*ps)) ps++;
            while ((pe>ps) && isspace(*pe)) pe--;

            if (pe<pend) *++pe = 0;
            slen = (pe-ps);
            if ((ps>str) && slen) memmove(str, ps, slen+1);
            else if (!slen) *str = 0;
        }
    }

    return str;
}

unsigned int
xmlCopyString(void *id, const char *path, char *buffer, unsigned int buflen)
{
    struct _xml_id *xid = (struct _xml_id *)id;
    unsigned int rlen = 0;

    if (xid && xid->len && path && buffer && buflen)
    {
        char *str;

        *buffer = 0;
        rlen = strlen(path);
        str = __xmlGetNode(xid->start, xid->len, path, &rlen);
        if (str)
        {
            char *ps, *pe;

            ps = str;
            pe = ps+rlen-1;

            while ((ps<pe) && isspace(*ps)) ps++;
            while ((pe>ps) && isspace(*pe)) pe--;

            rlen = (pe-ps)+1;
            if (rlen >= buflen) rlen = buflen-1;

            memcpy(buffer, ps, rlen);
            str = buffer + rlen;
            *str = 0;
        }
    }

    return rlen;
}

long int
xmlGetNodeInt(void *id, const char *path)
{
    struct _xml_id *xid = (struct _xml_id *)id;
    long int li = 0;

    if (path && xid && xid->len)
    {
        unsigned int rlen;
        char *str;

        rlen = strlen(path);
        str = __xmlGetNode(xid->start, xid->len, path, &rlen);
        if (str) li = strtol(str, (char **)NULL, 10);
    }

    return li;
}

long int
xmlGetInt(void *id)
{
    struct _xml_id *xid = (struct _xml_id *)id;
    long int li = 0;

    if (xid && xid->len)
        li = strtol(xid->start, (char **)NULL, 10);

    return li;
}

double
xmlGetNodeDouble(void *id, const char *path)
{
   struct _xml_id *xid = (struct _xml_id *)id;
   double d = 0.0;

    if (path && xid && xid->len)
    {
        unsigned int rlen;
        char *str;

        rlen = strlen(path);
        str = __xmlGetNode(xid->start, xid->len, path, &rlen);

        if (str) d = strtod(str, (char **)NULL);
    }

    return d;
}

double
xmlGetDouble(void *id)
{
   struct _xml_id *xid = (struct _xml_id *)id;
   double d = 0.0;

    if (xid && xid->len)
        d = strtod(xid->start, (char **)NULL);

    return d;
}


unsigned int
xmlGetNumElements(void *id, const char *path)
{
    struct _xml_id *xid = (struct _xml_id *)id;
    unsigned ret = 0;

    if (xid && xid->len && path)
    {
        unsigned int clen;
        char *p, *pathname;
        char *nname;

        pathname = (char *)path;
        if (*path == '/') pathname++;

        nname = strrchr(pathname, '/');
        if (nname)
        {
           clen = nname-pathname;
           p = __xmlGetNode(xid->start, xid->len, pathname, &clen);
        }
        else
        {
            nname = (char *)pathname;
            p = (char *)xid->start;
            clen = xid->len;
        }
           
        do
        {
           unsigned int slen = strlen(nname);
           p = __xmlGetNode(p, clen, nname, &slen);
           if (p) ret++;
        }
        while (p);
    }

    return ret;
}

void *
xmlMarkId(void *id)
{
   struct _xml_id *xmid = 0;

   if (id)
   {
      xmid = malloc(sizeof(struct _xml_id));
      if (xmid)
      {
          memcpy(xmid, id, sizeof(struct _xml_id));
          xmid->fd = 0;
      }
   }

   return (void *)xmid;
}

/* -------------------------------------------------------------------------- */

char *
__xmlCopyNode(char *start, size_t len, const char *path)
{
   char *p, *ret = 0;
   size_t rlen;

   rlen = strlen(path);
   p = __xmlGetNode(start, len, path, &rlen);
   if (p && rlen)
   {
      ret = calloc(1, rlen+1);
      memcpy(ret, p, rlen);
   }

   return ret;
}

char *
__xmlGetNode(char *start, size_t len, const char *path, size_t *rlen)
{
    char *ret = 0;

    if (len && rlen && *rlen)
    {
        size_t elem_len, newpath_len;
        char *newpath, *element;
        char last_node = 0;

        newpath_len = *rlen;
        element = (char *)path;
        if (*element == '/')
        {
            element++;	/* skip the leading '/' character */
            newpath_len--;
        }

        newpath = strchr(element, '/');
        if (!newpath)
        {
           last_node = 1;
           elem_len = newpath_len;
        }
        else
        {
           elem_len = newpath++ - element;
           newpath_len -= (newpath - element);
        }
         
        if (elem_len)
        {
            char *p, *cur;
            size_t newlen;
            void *newelem;

            cur = p = start;
            do
            {
                len -= cur - p;
                p = memchr(cur, '<', len);

                if (p)
                {
                    p++;
                    if (p >= (cur+len)) return 0;

                    len -= p - cur;
                    cur = p;

                    /* skip comments */
                    if (memcmp(cur, "!--", 3) == 0)
                    {
                        if (len < 6) return 0;

                        cur += 3;
                        len -= 3;

                        do
                        {
                            p = memchr(cur, '-', len);
                            if (p)
                            {
                                len -= p - cur;
                                if ((len > 3) && (memcmp(cur, "-->", 3) == 0))
                                {
                                    p += 3;
                                    len -= 3;
                                    break;
                                }
                                cur = p+1;
                            }
                            else return 0;
                        }
                        while (p && (len > 2));

                        if (!p || (len < 2)) return 0;
                    }
                    else if (*cur == '?')
                    {
                        if (len < 3) return 0;

                        cur++;
                        len--;
                        p = memchr(cur, '?', len);
                        if (!p || *(p+1) != '>') return 0;

                        p += 2;
                        len -= (p - cur);
                    }
                    else
                    {
                        newlen = elem_len;
                        newelem = element;

                        cur = __xml_memncasecmp(p, len, &newelem, &newlen);
                        if (cur)
                        {
                            break;
                        }

                        cur = p + elem_len;
                    }
                }
            }
            while (p);

            if (cur && p)
            {
                len -= elem_len;
                p = cur;
                while ((*cur++ != '>') && (cur<(p+len)));
                len -= cur - p;

                if (last_node)
                {
                    char *rptr = cur;
                    do
                    {
                        if ((p = __xml_memmem(cur, len, "</", 2)) != 0) 
                        {
                            char *r;

                            len -= (p + 2) - cur;
                            cur = p + 2;
                            r = __xml_memncasecmp(cur, len, &newelem, &newlen);
                            if (r && *r == '>') break;
                        }
                    }
                    while (p);

                    if (p)
                    {
                        *rlen = p-rptr;
                        ret = rptr;
                    }
                }
                else
                {
                    *rlen = newpath_len;
                    ret = __xmlGetNode(cur, len, newpath, rlen);
                }
            }
        }
    }

    return ret;
}


#define NOCASECMP(a,b)	( ((a)^(b)) & 0xdf )

void *
__xml_memmem(const void *haystack, size_t haystacklen,
                         const void *needle, size_t needlelen)
{
    void *rptr = 0;

    if (haystack && needle && (needlelen > 0) && (haystacklen >= needlelen))
    {
        char *ns, *hs, *ptr;

        hs = (char *)haystack;
        ns = (char *)needle;

        do
        {
            ptr = memchr(hs, *ns, haystacklen);
            if (ptr)
            {
                haystacklen -= (ptr - hs);

                if (haystacklen < needlelen) break;
                if (memcmp(ptr, needle, needlelen) == 0)
                {
                   rptr = ptr;
                   break;
                }

                hs = ptr+1;
            }
            else break;
        }
        while (haystacklen > needlelen);
    }

    return rptr;
}

void *
__xml_memncasecmp(void *haystack, size_t haystacklen,
                  void **needle, size_t *needlelen)
{
    void *rptr = 0;

    if (haystack && needle && needlelen && (*needlelen > 0)
        && (haystacklen >= *needlelen))
    {
        char *ns, *hs;
        size_t i;

        ns = (char *)*needle;
        hs = (char *)haystack;

        /* search for everything */
        if ((*ns == '*') && (*needlelen == 1))
        {
           char *he = hs + haystacklen;

           while ((hs < he) && (*hs != ' ') && (*hs != '>')) hs++;
           *needle = (void *)haystack;
           *needlelen = hs - (char *)haystack;
           rptr = hs;
        }
        else
        {
            size_t nlen = *needlelen;

            for (i=0; i<nlen; i++)
            {
                if (NOCASECMP(*hs,*ns) && (*ns != '?'))
                    break;
                hs++;
                ns++;
            }

            if (i == nlen) rptr = hs;
        }
    }

    return rptr;
}

#ifdef WIN32
/* Source:
 * https://mollyrocket.com/forums/viewtopic.php?p=2529
 */

void *
simple_mmap(int fd, unsigned int length, SIMPLE_UNMMAP *un)
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

    if (n) *n = GetFileSize(f, NULL);

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
