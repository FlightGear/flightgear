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


#include <sys/types.h>
#include <stdlib.h>
#include <assert.h>

#include "xml.h"

#ifndef NDEBUG
# define PRINT(a, b, c) { \
   size_t l1 = (b), l2 = (c); \
   char *s = (a); \
   if (s) { \
      size_t q, len = l2; \
      if (l1 < l2) len = l1; \
      if (len < 50000) { \
         printf("(%i) '", len); \
         for (q=0; q<len; q++) printf("%c", s[q]); \
         printf("'\n"); \
      } else printf("Length (%u) seems too large at line %i\n",len, __LINE__); \
   } else printf("NULL pointer at line %i\n", __LINE__); \
}
#endif

#if !defined(XML_USE_NODECACHE)
void *
cacheNodeGet(void *id, const char *node)
{
    return 0;
}

#else

#define NODE_BLOCKSIZE          16

struct _xml_node
{
    void *parent;
    char *name;
    size_t name_len;
    char *data;
    size_t data_len;
    void **node;
    size_t no_nodes;
    size_t first_free;
};

char *
__xmlNodeGetFromCache(void **nc, const char *start, size_t *len,
                      char **element, size_t *elementlen , size_t *nodenum)
{
    struct _xml_node *cache;
    size_t num = *nodenum;
    char *name = *element;
    void *rv = 0;

    assert(nc != 0);
 
    cache = (struct _xml_node *)*nc;
    assert(cache != 0);

    assert((cache->first_free > num) || (cache->first_free == 0));

    if (cache->first_free == 0) /* leaf node */
    {
        rv = cache->data;
        *len = cache->data_len;
        *element = cache->name;
        *elementlen = cache->name_len;
        *nodenum = 0;
    }
    else if (*name == '*')
    {
        struct _xml_node *node = cache->node[num];
        *nc = node;
        rv = node->data;
        *len = node->data_len;
        *element = node->name;
        *elementlen = node->name_len;
        *nodenum = cache->first_free;
    }
    else
    {
        size_t namelen = *elementlen;
        size_t i, pos = 0;

        for (i=0; i<cache->first_free; i++)
        {
            struct _xml_node *node = cache->node[i];

            assert(node);

            if ((node->name_len == namelen) &&
                (!memcmp(node->name, name, namelen)))
            {
                 if (pos == num)
                 {
                     *nc = node;
                     rv = node->data;
                     *element = node->name;
                     *elementlen = node->name_len;
                     *len = node->data_len;
                     *nodenum = cache->first_free;
                     break;
                 }
                 pos++;
            }
        }
    }

    return rv;
}


void *
cacheInit()
{
   return calloc(1, sizeof(struct _xml_node));
}

void
cacheInitLevel(void *nc)
{
    struct _xml_node *cache = (struct _xml_node *)nc;

    assert(cache != 0);

    cache->node = calloc(NODE_BLOCKSIZE, sizeof(struct _xml_node *));
    cache->no_nodes = NODE_BLOCKSIZE;
}

void
cacheFree(void *nc)
{
    struct _xml_node *cache = (struct _xml_node *)nc;
    struct _xml_node **node;
    size_t i = 0;

    assert(nc != 0);

    node = (struct _xml_node **)cache->node;
    while(i < cache->first_free)
    {
        cacheFree(node[i++]);
    }

    free(node);
    /* free(cache); */
}

void *
cacheNodeGet(void *id)
{
    struct _xml_id *xid = (struct _xml_id *)id;
    struct _xml_node *cache = 0;

    assert(xid != 0);

    if (xid->name)
    {
        cache = xid->node;
    }
    else
    {
        struct _root_id *rid = (struct _root_id *)xid;
        cache = rid->node;
    }

    return cache;
}

void *
cacheNodeNew(void *nc)
{
    struct _xml_node *cache = (struct _xml_node *)nc;
    struct _xml_node *rv = 0;
    size_t i = 0;

    assert(nc != 0);

    i = cache->first_free;
    if (i == cache->no_nodes)
    {
        size_t size, no_nodes;
        void *p;

        no_nodes = cache->no_nodes + NODE_BLOCKSIZE;
        size = no_nodes * sizeof(struct _xml_node *);
        p = realloc(cache->node, size);
        if (!p) return 0;

        cache->node = p;
        cache->no_nodes = no_nodes;
    }

    rv = calloc(1, sizeof(struct _xml_node));
    if (rv) rv->parent = cache;
    cache->node[i] = rv;
    cache->first_free++;

    return rv;
}

void
cacheDataSet(void *n, char *name, size_t namelen, char *data, size_t datalen)
{
   struct _xml_node *node = (struct _xml_node *)n;
   size_t len = datalen;
   char *ptr = data;

   assert(node != 0);
   assert(name != 0);
   assert(namelen != 0);
   assert(data != 0);

   node->name = name;
   node->name_len = namelen;
   node->data = data;
   node->data_len = datalen;
}

#endif

