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
#include <assert.h>
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
static char *__xmlGetNode(const char *, size_t *, const char *, size_t *, int *);
static char *__xmlGetNodePath(const char *, size_t *, const char *, size_t *);
static char *__xmlSkipComment(const char *, size_t);
static char *__xmlSkipInfo(const char *, size_t);

static void *__xml_memmem(const void *, size_t, const void *, size_t);
static void *__xml_memncasecmp(void *, size_t *, void **, size_t *);
static void *__xml_memchr(const void *, int, size_t);

#define PRINT(a, b, c) { \
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
        size_t slen, len;
        char *ptr, *p;

        len = xid->len;
        slen = strlen(path);
        ptr = __xmlGetNodePath(xid->start, &len, path, &slen);
        if (ptr)
        {
            xsid = malloc(sizeof(struct _xml_id) + len);
            if (xsid)
            {
                p = (char *)xsid + sizeof(struct _xml_id);

                xsid->len = len;
                xsid->start = p;
                xsid->fd = 0;

                memcpy(xsid->start, ptr, len);
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
      size_t len, slen;
      char *ptr;

      len = xid->len;
      slen = strlen(path);
      ptr = __xmlGetNodePath(xid->start, &len, path, &slen);
      if (ptr)
      {
         xsid = malloc(sizeof(struct _xml_id));
         xsid->len = len;
         xsid->start = ptr;
         xsid->fd = 0;
      }
   }

   return (void *)xsid;
}

const char *
xmlGetNodeNum(const void *rid, void *id, char *element, int num)
{
    struct _xml_id *xrid = (struct _xml_id *)rid;
    const char *ret = 0;

    if (id && element)
    {
        struct _xml_id *xid = (struct _xml_id *)id;
        size_t len, slen;
        char *ptr;

        len = xrid->len;
        slen = strlen(element);
        ptr = __xmlGetNode(xrid->start, &len, element, &slen, &num);
        if (ptr)
        {
             xid->len = len;
             xid->start = ptr;
             ret = element;
        }
    }

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
        size_t len, slen;

        len = xid->len;
        slen = strlen(path);
        str = __xmlGetNodePath(xid->start, &len, path, &slen);
        if (str)
        {
            ps = str;
            pe = ps + len;
            pe--;

            while ((ps<pe) && isspace(*ps)) ps++;
            while ((pe>ps) && isspace(*pe)) pe--;
            pe++;

            ret = strncasecmp(pe, s, pe-ps);
        }
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
    unsigned int len = 0;

    if (xid && xid->len && path && buffer && buflen)
    {
        size_t slen;
        char *str;

        *buffer = 0;
        len = xid->len;
        slen = strlen(path);
        str = __xmlGetNodePath(xid->start, &len, path, &slen);
        if (str)
        {
            char *ps, *pe;

            ps = str;
            pe = ps+len-1;

            while ((ps<pe) && isspace(*ps)) ps++;
            while ((pe>ps) && isspace(*pe)) pe--;

            len = (pe-ps)+1;
            if (len >= buflen) len = buflen-1;

            memcpy(buffer, ps, len);
            str = buffer + len;
            *str = 0;
        }
    }

    return len;
}

long int
xmlGetNodeInt(void *id, const char *path)
{
    struct _xml_id *xid = (struct _xml_id *)id;
    long int li = 0;

    if (path && xid && xid->len)
    {
        size_t len, slen;
        char *str;

        len = xid->len;
        slen = strlen(path);
        str = __xmlGetNodePath(xid->start, &len, path, &slen);
        if (str)
        {
            char *end = str+len;
            li = strtol(str, &end, 10);
        }
    }

    return li;
}

long int
xmlGetInt(void *id)
{
    struct _xml_id *xid = (struct _xml_id *)id;
    long int li = 0;

    if (xid && xid->len)
    {
        char *end = xid->start + xid->len;
        li = strtol(xid->start, &end, 10);
    }

    return li;
}

double
xmlGetNodeDouble(void *id, const char *path)
{
   struct _xml_id *xid = (struct _xml_id *)id;
   double d = 0.0;

    if (path && xid && xid->len)
    {
        size_t len, slen;
        char *str;

        len = xid->len;
        slen = strlen(path);
        str = __xmlGetNodePath(xid->start, &len, path, &slen);
        if (str)
        {
            char *end = str+len;
            d = strtod(str, &end);
        }
    }

    return d;
}

double
xmlGetDouble(void *id)
{
   struct _xml_id *xid = (struct _xml_id *)id;
   double d = 0.0;

    if (xid && xid->len)
    {
        char *end = xid->start + xid->len;
        d = strtod(xid->start, &end);
    }

    return d;
}


unsigned int
xmlGetNumNodes(void *id, const char *path)
{
    struct _xml_id *xid = (struct _xml_id *)id;
    unsigned num = 0;

    if (xid && xid->len && path)
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
           len = xid->len;
           pathname++;
           slen -= pathname-nodename;
           p = __xmlGetNodePath(xid->start, &len, pathname, &slen);
        }
        else
        {
            p = (char *)xid->start;
            len = xid->len;
        }
           
        if (p) __xmlGetNode(p, &len, nodename, &slen, &num);
    }

    return num;
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

static char *
__xmlCopyNode(char *start, size_t len, const char *path)
{
   char *p, *ret = 0;
   size_t rlen, slen;

   rlen = len;
   slen = strlen(path);
   p = __xmlGetNodePath(start, &rlen, path, &slen);
   if (p && rlen)
   {
      ret = calloc(1, rlen+1);
      memcpy(ret, p, rlen);
   }

   return ret;
}

char *
__xmlGetNodePath(const char *start, size_t *len, const char *name, size_t *plen)
{
    char *node;
    char *ret = 0;

    assert (start != 0);
    assert (len != 0);
    assert (name != 0);
    assert (plen != 0);

    if ((*len == 0) || (*plen == 0) || (*plen > *len))
        return 0;

    node = (char *)name;
    if (*name == '/') node++;
    if (node)
    {
        size_t plen, slen;
        char *path;
        int num;

        slen = strlen(node);
        path = strchr(node, '/');

        if (!path) plen = slen;
        else plen = path++ - node;

        num = 0;
        ret = __xmlGetNode(start, len, node, &plen, &num);
        if (ret && path)
        {
           plen = slen - (path-node);
           ret = __xmlGetNodePath(ret, len, path, &plen);
        }
    }

    return ret;
}

char *
__xmlGetNode(const char *start, size_t *len, const char *name, size_t *rlen, int *nodenum)
{
    char *new, *cur, *ne, *ret = 0;
    size_t restlen, elementlen;
    int found, num;
    void *element;

    assert (start != 0);
    assert (len != 0);
    assert (name != 0);
    assert (rlen != 0);
    assert (nodenum != 0);

    if ((*len == 0) || (*rlen == 0) || (*rlen > *len))
        return 0;

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
            new = __xmlSkipComment(cur, restlen);
            if (!new) return 0;
            restlen -= new-cur;
            cur = new;
            continue;
        }
        else if (*cur == '?')
        {
            new = __xmlSkipInfo(cur, restlen);
            if (!new) return 0;

            restlen -= new-cur;
            cur = new;
            continue;
        }

        element = (char *)name;
        elementlen = *rlen;
        new = __xml_memncasecmp(cur, &restlen, &element, &elementlen);
        if (new)
        {
            if (found == num ) ret = new+1;
        }
        else
        {
            new = cur+elementlen;
            if (new >= ne) return 0;
            element = (char *)name;
            elementlen = *rlen;
        }

        /* restlen -= new-cur; not necessary because of __xml_memncasecmp */
        cur = new;
        new = memchr(cur, '<', restlen);
        if (!new) return 0;

        restlen -= new-cur;
        cur = new;
        if (*(cur+1) != '/')                            /* new node found */
        {
            size_t slen = restlen;
            size_t nlen = 1;
            int pos = -1;

            new = __xmlGetNode(cur, &slen, "*", &nlen, &pos);
            if (!new)
            {
                if (slen == restlen) return 0;
                else new = cur + slen;
            }

            restlen -= slen;
            cur = new;

            new = memchr(cur, '<', restlen);
            if (!new) return 0;

            restlen -= new-cur;
            cur = new;
        }

        if (*(cur+1) == '/')				/* closing tag found */
        {
            if (!strncasecmp(new+2, element, elementlen))
            {
                if (found == num) *len = new-ret;
                found++;
            }

            new = memchr(cur, '>', restlen);
            if (!new) return 0;

            restlen -= new-cur;
            cur = new;
        }
        else return 0;
    }

    *rlen = elementlen;
    *nodenum = found;

    return ret;
}

char *
__xmlSkipComment(const char *start, size_t len)
{
    char *cur, *new;

    cur = (char *)start;
    new = 0;

    if (memcmp(cur, "!--", 3) == 0)
    {
        if (len < 6) return 0;

        cur += 3;
        len -= 3;

        do
        {
            new = memchr(cur, '-', len);
            if (new)
            {
                len -= new - cur;
                if ((len > 3) && (memcmp(cur, "-->", 3) == 0))
                {
                    new += 3;
                    len -= 3;
                    break;
                }
                cur = new+1;
            }
            else return 0;
        }
        while (new && (len > 2));

        if (!new || (len < 2)) return 0;
    }

    return new;
}

char *
__xmlSkipInfo(const char *start, size_t len)
{
    char *cur, *new;

    cur = (char *)start;
    new = 0;

    if (*cur == '?')
    {
        if (len < 3) return 0;

        cur++;
        len--;
        new = memchr(cur, '?', len);
        if (!new || *(new+1) != '>') return 0;

        new += 2;
    }

    return new;
}


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


#define NOCASECMP(a,b)  ( ((a)^(b)) & 0xdf )
void *
__xml_memncasecmp(void *haystack, size_t *haystacklen,
                  void **needle, size_t *needlelen)
{
    void *rptr = 0;
    char *hs = (char *)haystack;

    if (haystack && needle && needlelen && (*needlelen > 0)
        && (*haystacklen >= *needlelen))
    {
        char *ns;
        size_t i;

        ns = (char *)*needle;

        /* search for everything */
        if ((*ns == '*') && (*needlelen == 1))
        {
           char *he = hs + *haystacklen;

           while ((hs < he) && (*hs != ' ') && (*hs != '>')) hs++;
           *needle = (void *)haystack;
           *needlelen = hs - (char *)haystack;

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
                if ((*hs == ' ') || (*hs == '>')) break;
                hs++;
                ns++;
            }

            if (i != nlen)
            {
                while((hs < he) && (*hs != ' ') && (*hs != '>')) hs++;
                *needle = (void *)haystack;
                *needlelen = hs - (char *)haystack;
            }
            else
            {
                *needle = (void *)haystack;
                *needlelen = hs - (char *)haystack;
                while ((hs < he) && (*hs != '>')) hs++;
                rptr = hs;
            }
        }

        *haystacklen -= hs - (char *)haystack;
    }

    return rptr;
}

#if 0
void *
__xml_memchr(const void *d, int c, size_t len)
{
   char *s, *ret = 0;
   size_t i;

   s = (char *)d;
   for (i=0; i<len; i++)
   {
#if 1
      printf("i: %u, len: %u\n", i, len);
#endif
      if (s[i] == c)
      {
         ret = (char *)s + i;
         break;
      }
   }

   return (void *)ret;
}
#endif

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
