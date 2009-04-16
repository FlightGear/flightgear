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
static void *simple_mmap(int, size_t, SIMPLE_UNMMAP *);
static void simple_unmmap(SIMPLE_UNMMAP *);

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
#include <string.h>
#include <strings.h>	/* strncasecmp */
#ifndef NDEBUG
#include <stdio.h>
#endif


struct _xml_id
{
    size_t len;
    char *start;
    char *name;
    union {
       size_t name_len;
       int fd;
    } node;
};

static char *__xmlNodeCopy(const char *, size_t, const char *);
static char *__xmlNodeGetPath(const char *, size_t *, char **, size_t *);
static char *__xmlNodeGet(const char *, size_t *, char **, size_t *, size_t *);
static char *__xmlCommentSkip(const char *, size_t);
static char *__xmlInfoSkip(const char *, size_t);

static void *__xml_memncasecmp(const char *, size_t *, char **, size_t *);

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
xmlOpen(const char *filename)
{
    struct _xml_id *id = 0;

    if (filename)
    {
        int fd = open(filename, O_RDONLY);
        if (fd > 0)
        {
            id = malloc(sizeof(struct _xml_id));
            if (id)
            {
                struct stat statbuf;

                fstat(fd, &statbuf);

                id->node.fd = fd;
                id->len = statbuf.st_size;
                id->start = mmap(0, id->len, PROT_READ, MAP_PRIVATE, fd, 0L);
                id->name = 0;
            }
        }
    }

    return (void *)id;
}

void
xmlClose(void *id)
{
    if (id)
    {
        struct _xml_id *xid = (struct _xml_id *)id;
        assert(xid->name == 0);

        munmap(xid->start, xid->len);
        close(xid->node.fd);
        free(id);
        id = 0;
    }
}

void *
xmlNodeGet(const void *id, const char *path)
{
   struct _xml_id *xsid = 0;

   if (id && path)
   {
      struct _xml_id *xid = (struct _xml_id *)id;
      size_t len, slen;
      char *ptr, *node;

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
             xsid->node.name_len = slen;
             xsid->name = node;
         }
      }
   }

   return (void *)xsid;
}

void *
xmlNodeCopy(const void *id, const char *path)
{
    struct _xml_id *xsid = 0;

    if (id && path)
    {
        struct _xml_id *xid = (struct _xml_id *)id;
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
                xsid->node.name_len = slen;
                xsid->name = node;

                memcpy(xsid->start, ptr, len);
           }
       }
   }

   return (void *)xsid;
}

char *
xmlNodeGetName(const void *id)
{
    struct _xml_id *xid = (struct _xml_id *)id;
    size_t len;
    char *ret;

    len = xid->node.name_len;
    ret = malloc(len+1);
    if (ret)
    {
        memcpy(ret, xid->name, len);
        *(ret + len) = 0;
    }

    return ret;
}

size_t
xmlNodeCopyName(const void *id, char *buf, size_t len)
{
    struct _xml_id *xid = (struct _xml_id *)id;
    size_t slen = 0;
 
    if (buf)
    {
        slen = len-1;
        if (slen > xid->node.name_len) slen = xid->node.name_len;
        memcpy(buf, xid->name, slen);
        *(buf + slen) = 0;
    }

    return slen;
}

unsigned int
xmlNodeGetNum(const void *id, const char *path)
{
    struct _xml_id *xid = (struct _xml_id *)id;
    size_t num = 0;

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
           char *node;

           len = xid->len;
           pathname++;
           slen -= pathname-nodename;
           node = pathname;
           p = __xmlNodeGetPath(xid->start, &len, &node, &slen);
        }
        else
        {
            p = xid->start;
            len = xid->len;
        }

        if (p)
        {
            char *node = nodename;
            __xmlNodeGet(p, &len, &node, &slen, &num);
        }
    }

    return num;
}

void *
xmlNodeGetPos(const void *pid, void *id, const char *element, size_t num)
{
    void *ret = 0;

    if (pid && element)
    {
        struct _xml_id *xpid = (struct _xml_id *)pid;
        struct _xml_id *xid = (struct _xml_id *)id;
        size_t len, slen;
        char *ptr, *node;

        len = xpid->len;
        slen = strlen(element);
        node = (char *)element;
        ptr = __xmlNodeGet(xpid->start, &len, &node, &slen, &num);
        if (ptr)
        {
             xid->len = len;
             xid->start = ptr;
             xid->name = node;
             xid->node.name_len = slen;
             ret = xid;
        }
    }

    return ret;
}

char *
xmlGetString(const void *id)
{
    struct _xml_id *xid = (struct _xml_id *)id;
    char *str = 0;

    if (xid && xid->len)
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
        }
    }

    return str;
}

size_t
xmlCopyString(const void *id, char *buffer, size_t buflen)
{
    struct _xml_id *xid = (struct _xml_id *)id;
    size_t ret = 0;

    if (xid && xid->len && buffer && buflen)
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
            if (nlen >= buflen) nlen = buflen-1;
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

char *
xmlNodeGetString(const void *id, const char *path)
{
    struct _xml_id *xid = (struct _xml_id *)id;
    char *str = 0;

    if (xid && xid->len && path)
    {
        str = __xmlNodeCopy(xid->start, xid->len, path);
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
    }

    return str;
}

size_t
xmlNodeCopyString(const void *id, const char *path, char *buffer, size_t buflen)
{
    struct _xml_id *xid = (struct _xml_id *)id;
    size_t ret = 0;

    if (xid && xid->len && path && buffer && buflen)
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
            if (nlen >= buflen) nlen = buflen-1;

            memcpy(buffer, ps, nlen);
            *(buffer + nlen) = '\0';
            ret = nlen;
        }
    }

    return ret;
}

int
xmlNodeCompareString(const void *id, const char *path, const char *s)
{
    struct _xml_id *xid = (struct _xml_id *)id;
    int ret = -1;

    if (xid && xid->len && path && s && (strlen(s) > 0))
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
    }

    return ret;
}

long int
xmlGetInt(const void *id)
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

long int
xmlNodeGetInt(const void *id, const char *path)
{
    struct _xml_id *xid = (struct _xml_id *)id;
    long int li = 0;

    if (path && xid && xid->len)
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
    }

    return li;
}

double
xmlGetDouble(const void *id)
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

double
xmlNodeGetDouble(const void *id, const char *path)
{
    struct _xml_id *xid = (struct _xml_id *)id;
    double d = 0.0;

    if (path && xid && xid->len)
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
    }

    return d;
}

void *
xmlMarkId(const void *id)
{
   struct _xml_id *xmid = 0;

   if (id)
   {
      xmid = malloc(sizeof(struct _xml_id));
      if (xmid)
      {
          memcpy(xmid, id, sizeof(struct _xml_id));
      }
   }

   return (void *)xmid;
}

double
xmlAttributeGetDouble(const void *id, const char *name)
{
   struct _xml_id *xid = (struct _xml_id *)id;
   double ret = 0.0;

   if (xid && xid->start && xid->len && xid->node.name_len)
   {
       char *p, *end, *new = 0;

       assert(xid->start > xid->name);

       p = xid->name + xid->node.name_len;
       end = xid->start-1;

       while ((p<end) && isspace(*p)) p++;
       while (p<end)
       {
          size_t elementlen = strlen(name);
          size_t restlen = end-p;
          char *element = (char *)name;

          new = __xml_memncasecmp(p, &restlen, &element, &elementlen);
          if (new)
          {
             restlen = end-p;
             new = memchr(p, '=', restlen);
             if (new)
             {
                new++;
                if (*new == '"') new++;
                restlen -= (new-p);
                end = new+restlen;
                ret = strtod(new, &end);
             }
             break;
          }

          while ((p<end) && !isspace(*p)) p++;
          if (p<end)
             while ((p<end) && isspace(*p)) p++;
       }
   }

   return ret;
}

long int
xmlAttributeGetInt(const void *id, const char *name)
{
   struct _xml_id *xid = (struct _xml_id *)id;
   long int ret = 0;

   if (xid && xid->start && xid->len && xid->node.name_len)
   {
       char *p, *end, *new = 0;

       assert(xid->start > xid->name);

       p = xid->name + xid->node.name_len;
       end = xid->start-1;

       while ((p<end) && isspace(*p)) p++;
       while (p<end)
       {
          size_t elementlen = strlen(name);
          size_t restlen = end-p;
          char *element = (char *)name;

          new = __xml_memncasecmp(p, &restlen, &element, &elementlen);
          if (new)
          {
             restlen = end-p;
             new = memchr(p, '=', restlen);
             if (new)
             {
                new++;
                if (*new == '"') new++;
                restlen -= (new-p);
                end = new+restlen;
                ret = strtol(new, &end, 10);
             }
             break;
          }

          while ((p<end) && !isspace(*p)) p++;
          if (p<end)
             while ((p<end) && isspace(*p)) p++;
       }
   }

   return ret;
}

char *
xmlAttributeGetString(const void *id, const char *name)
{
   struct _xml_id *xid = (struct _xml_id *)id;
   char *ret = 0;

   if (xid && xid->start && xid->len && xid->node.name_len)
   {
       char *p, *end, *new = 0;

       assert(xid->start > xid->name);

       p = xid->name + xid->node.name_len;
       end = xid->start-1;

       while ((p<end) && isspace(*p)) p++;
       while (p<end)
       {
          size_t elementlen = strlen(name);
          size_t restlen = end-p;
          char *element = (char *)name;

          new = __xml_memncasecmp(p, &restlen, &element, &elementlen);
          if (new)
          {
             restlen = end-p;
             new = memchr(p, '=', restlen);
             if (new)
             {
                new++;
                if (*new == '"') new++;
                p = new;
                while ((p<end) && (*p != '"') && (*p != ' ')) p++;
                if (p<end)
                {
                   ret = calloc(1, p-new);
                   memcpy(ret, new, (p-new));
                }
             }
             break;
          }

          while ((p<end) && !isspace(*p)) p++;
          if (p<end)
             while ((p<end) && isspace(*p)) p++;
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

   if (xid && xid->start && xid->len && xid->node.name_len
       && buffer && buflen)
   {
       char *ps, *pe;

       *buffer = '\0';
       ps = xid->name + xid->node.name_len + 1;
       pe = xid->start - 1;

       assert((int)(pe-ps) > 0);

       while ((ps<pe) && isspace(*ps)) ps++;
       while (ps<pe)
       {
          size_t slen = strlen(name);
          size_t restlen = pe-ps;

          if ((restlen >= slen) && (strncasecmp(ps, name, slen) == 0))
          {
             char *new;

             restlen = pe-ps;
             new = memchr(ps, '=', restlen);
             if (new)
             {
                new++;
                if (*new == '"') new++;
                ps = new;
                while ((ps<pe) && (*ps != '"') && (*ps != ' ')) ps++;
                if (ps<pe)
                {
                   restlen = ps-new;
                   if (restlen >= buflen) restlen = buflen-1;
                   memcpy(buffer, new, restlen);
                   *(buffer+restlen) = 0;
                   ret = restlen;
                }
             }
             break;
          }

          while ((ps<pe) && !isspace(*ps)) ps++;
          if (ps<pe)
             while ((ps<pe) && isspace(*ps)) ps++;
       }
   }

   return ret;
}

int
xmlAttributeCompareString(const void *id, const char *name, const char *s)
{
   struct _xml_id *xid = (struct _xml_id *)id;
   int ret = -1;

   if (xid && xid->start && xid->len && xid->node.name_len
       && s && strlen(s))
   {
      char *ps, *pe;

       ps = xid->name + xid->node.name_len + 1;
       pe = xid->start - 1;

       assert((int)(pe-ps) > 0);

       while ((ps<pe) && isspace(*ps)) ps++;
       while (ps<pe)
       {
          size_t slen = strlen(name);
          size_t restlen = pe-ps;

          if ((restlen >= slen) && (strncasecmp(ps, name, slen) == 0))
          {
             char *new;

             restlen = pe-ps;
             new = memchr(ps, '=', restlen);
             if (new)
             {
                new++;
                if (*new == '"') new++;
                ps = new;
                while ((ps<pe) && (*ps != '"') && (*ps != ' ')) ps++;
                if (ps<pe)
                {
                   int alen = ps-new;
                   ret = strncasecmp(new, s, alen);
                }
             }
             break;
          }

          while ((ps<pe) && !isspace(*ps)) ps++;
          if (ps<pe)
             while ((ps<pe) && isspace(*ps)) ps++;
       }
   }

   return ret;
}


/* -------------------------------------------------------------------------- */

static char *
__xmlNodeCopy(const char *start, size_t len, const char *path)
{
   char *node, *p, *ret = 0;
   size_t rlen, slen;

   rlen = len;
   slen = strlen(path);
   node = (char *)path;
   p = __xmlNodeGetPath(start, &rlen, &node, &slen);
   if (p && rlen)
   {
      ret = calloc(1, rlen+1);
      memcpy(ret, p, rlen);
   }

   return ret;
}

char *
__xmlNodeGetPath(const char *start, size_t *len, char **name, size_t *plen)
{
    char *node;
    char *ret = 0;

    assert (start != 0);
    assert (len != 0);
    assert (name != 0);
    assert (*name != 0);
    assert (plen != 0);

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

        num = 0;
        ret = __xmlNodeGet(start, len, &node, &plen, &num);
        if (ret && path)
        {
           plen = slen - (path - *name);
           ret = __xmlNodeGetPath(ret, len, &path, &plen);
        }

        *name = path;
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

    assert (start != 0);
    assert (len != 0);
    assert (name != 0);
    assert (*name != 0);
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
            new = __xmlCommentSkip(cur, restlen);
            if (!new) return 0;
            restlen -= new-cur;
            cur = new;
            continue;
        }
        else if (*cur == '?')
        {
            new = __xmlInfoSkip(cur, restlen);
            if (!new) return 0;

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
            if (found == num )
            {
              ret = new+1;
              start_tag = element;
            }
            else start_tag = 0;
        }
        else
        {
            new = cur+elementlen;
            if (new >= ne) return 0;
            element = *name;
        }

        /* restlen -= new-cur; not necessary because of __xml_memncasecmp */
        cur = new;
        new = memchr(cur, '<', restlen);
        if (!new) return 0;

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
                if (found == num) 
                {
                   assert(start_tag != 0);
                   *len = new-ret;
                   *name = start_tag;
                }
                found++;
            }

            new = memchr(cur, '>', restlen);
            if (!new) return 0;

            restlen -= new-cur;
            cur = new;
        }
        else return 0;
    }

    *rlen = retlen;
    *nodenum = found;

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
        if (len < 6) return 0;

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
__xmlInfoSkip(const char *start, size_t len)
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

           while ((hs < he) && (*hs != ' ') && (*hs != '>')) hs++;
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
                if ((*hs == ' ') || (*hs == '>')) break;
                hs++;
                ns++;
            }

            if (i != nlen)
            {
                while((hs < he) && (*hs != ' ') && (*hs != '>')) hs++;
                *needle = (char *)haystack;
                *needlelen = hs - haystack;
            }
            else
            {
                *needle = (char *)haystack;
                *needlelen = hs - haystack;
                while ((hs < he) && (*hs != '>')) hs++;
                rptr = hs;
            }
        }

        *haystacklen -= hs - haystack;
    }

    return rptr;
}

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
