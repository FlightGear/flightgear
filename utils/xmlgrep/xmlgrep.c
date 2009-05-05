#include <stdio.h>

#define _GNU_SOURCE
#include <stdlib.h>
#include <string.h>
#ifndef _MSC_VER
# include <strings.h>
#else
# define strncasecmp strnicmp
# include <stdlib.h>
#endif
#include <assert.h>
#include <unistd.h>	/* read */
#include <sys/stat.h>	/* fstat */
#include <fcntl.h>	/* open */

#include "xml.h"

static const char *_static_root = "/";
static const char *_static_element = "*";
static unsigned int _fcount = 0;
static char **_filenames = 0;
static char *_element = 0;
static char *_value = 0;
static char *_root = 0;
static char *_print = 0;
static char *_attribute = 0;
static int print_filenames = 0;

static void free_and_exit(int i);

#define USE_BUFFER		0
#define NODE_NAME_LEN		256
#define STRING_LEN		2048

#define SHOW_NOVAL(opt) \
{ \
    printf("option '%s' requires a value\n\n", (opt)); \
    free_and_exit(-1); \
}

void
show_help ()
{
    printf("usage: xmlgrep [options] [file ...]\n\n");
    printf("Options:\n");
    printf("\t-h\t\tshow this help message\n");
    printf("\t-a <string>\tprint this attribute as the output\n");
    printf("\t-e <id>\t\tshow sections that contain this element\n");
    printf("\t-p <id>\t\tprint this element as the output\n");
    printf("\t-r <path>\tspecify the XML search root\n");
    printf("\t-v <string>\tfilter sections that contain this vale\n\n");
    printf(" To print the contents of the 'type' element of the XML section ");
    printf("that begins\n at '/printer/output' use the following command:\n\n");
    printf("\txmlgrep -r /printer/output -p type sample.xml\n\n");
    printf(" To filter 'output' elements under '/printer' that have attribute");
    printf(" 'n' set to '1'\n use the following command:\n\n");
    printf("\txmlgrep -r /printer -p output -a n -v 1 sample.xml\n\n");
    printf(" To filter out sections that contain the 'driver' element with ");
    printf("'generic' as\n it's value use the following command:");
    printf("\n\n\txmlgrep -r /printer/output -e driver -v generic sample.xml");
    printf("\n\n");
    free_and_exit(0);
}

void
free_and_exit(int ret)
{
    if (_root && _root != _static_root) free(_root);
    if (_element && _element != _static_element) free(_element);
    if (_value) free(_value);
    if (_print) free(_print);
    if (_attribute) free(_attribute);
    if (_filenames)
    {
        unsigned int i;
        for (i=0; i < _fcount; i++)
        {
            if (_filenames[i])
            {
                if (print_filenames) printf("%s\n", _filenames[i]);
                free(_filenames[i]);
            }
        }
        free(_filenames);
    }
 
    exit(ret);
}

int
parse_option(char **args, int n, int max)
{
    char *opt, *arg = 0;
    unsigned int alen = 0;
    unsigned int olen;

    opt = args[n];
    if (strncmp(opt, "--", 2) == 0)
        opt++;

    if ((arg = strchr(opt, '=')) != NULL)
    {
        *arg++ = 0;
    }
    else if (++n < max)
    {
        arg = args[n];
#if 0
        if (arg && arg[0] == '-')
            arg = 0;
#endif
    }

    olen = strlen(opt);
    if (strncmp(opt, "-help", olen) == 0)
    {
        show_help();
    }
    else if (strncmp(opt, "-root", olen) == 0)
    {
        if (arg == 0) SHOW_NOVAL(opt);
        alen = strlen(arg)+1;
        if (_root) free(_root);
        _root = malloc(alen);
        memcpy(_root, arg, alen);
        return 2;
    }
    else if (strncmp(opt, "-element", olen) == 0)
    {
        if (arg == 0) SHOW_NOVAL(opt);
        alen = strlen(arg)+1;
        if (_element) free(_element);
        _element = malloc(alen);
        memcpy(_element, arg, alen);
        return 2;
    }
    else if (strncmp(opt, "-value", olen) == 0)
    {
        if (arg == 0) SHOW_NOVAL(opt);
        alen = strlen(arg)+1;
        if (_value) free(_value);
        _value = malloc(alen);
        memcpy(_value, arg, alen);
        return 2;
    }
    else if (strncmp(opt, "-print", olen) == 0)
    {
        if (arg == 0) SHOW_NOVAL(opt);
        alen = strlen(arg)+1;
        if (_print) free(_print);
        _print = malloc(alen);
        memcpy(_print, arg, alen);
        return 2;
    }
    else if (strncmp(opt, "-attribute", olen) == 0)
    {
        if (arg == 0) SHOW_NOVAL(opt);
        alen = strlen(arg)+1;
        if (_attribute) free(_attribute);
        _attribute = malloc(alen);
        memcpy(_attribute, arg, alen);
        return 2;
    }
    else if (strncmp(opt, "-list-filenames", olen) == 0)
    { /* undocumented test argument */
        print_filenames = 1;
        return 1;
    }
    else if (opt[0] == '-')
    {
        printf("Unknown option %s\n", opt);
        free_and_exit(-1);
    }
    else
    {
        int pos = _fcount++;
        if (_filenames == 0)
        {
            _filenames = (char **)malloc(sizeof(char*));
        }
        else
        {
            char **ptr = (char **)realloc(_filenames, _fcount*sizeof(char*));
            if (ptr == 0)
            {
                printf("Out of memory.\n\n");
                free_and_exit(-1);
            }
           _filenames = ptr;
        }

        alen = strlen(opt)+1;
        _filenames[pos] = malloc(alen);
        memcpy(_filenames[pos], opt, alen);
    }

    return 1;
}

void walk_the_tree(size_t num, void *xid, char *tree)
{
    unsigned int i, no_elements;

    if (!tree)					/* last node from the tree */
    {
        void *xmid = xmlMarkId(xid);
        if (xmid && _print)
        {
            no_elements = xmlNodeGetNum(xid, _print);
            for (i=0; i<no_elements; i++)
            {
                if (xmlNodeGetPos(xid, xmid, _print, i) != 0)
                {
                    char *value;

                    value = xmlGetString(xmid);
                    if (_value && _attribute && value)
                    {
                       if (!xmlAttributeCompareString(xmid, _attribute, _value))
                       {
                          printf("%s: <%s %s=\"%s\">%s</%s>\n",
                                 _filenames[num], _print, _attribute, _value,
                                                  value, _print);
                       }
                       if (value) free(value);
                    }
                    else
                    {
                       printf("%s: <%s>%s</%s>\n",
                              _filenames[num], _print, value, _print);
                    }
                }
            }
            free(xmid);
        }
        else if (xmid && _value)
        {
            no_elements = xmlNodeGetNum(xmid, _element);
            for (i=0; i<no_elements; i++)
            {
                if (xmlNodeGetPos(xid, xmid, _element, i) != 0)
                {
                    char nodename[NODE_NAME_LEN];

                    xmlNodeCopyName(xmid, (char *)&nodename, NODE_NAME_LEN);
                    if (xmlCompareString(xmid, _value) == 0)
                    {
                        printf("%s: <%s>%s</%s>\n",
                               _filenames[num], nodename, _value, nodename);
                    }
                }
            }
            free(xmid);
        }
        else if (xmid && _element)
        {
            char parentname[NODE_NAME_LEN];

            xmlNodeCopyName(xid, (char *)&parentname, NODE_NAME_LEN);

            no_elements = xmlNodeGetNum(xmid, _element);
            for (i=0; i<no_elements; i++)
            {
                if (xmlNodeGetPos(xid, xmid, _element, i) != 0)
                {
                    char nodename[NODE_NAME_LEN];

                    xmlNodeCopyName(xmid, (char *)&nodename, NODE_NAME_LEN);
                    if (!strncasecmp((char*)&nodename, _element, NODE_NAME_LEN))
                    {
                        char value[NODE_NAME_LEN];
                        xmlCopyString(xmid, (char *)&value, NODE_NAME_LEN);
                        printf("%s: <%s> <%s>%s</%s> </%s>\n",
                                _filenames[num], parentname, nodename, value,
                                                 nodename, parentname);
                    }
                }
            }
        }
        else printf("Error executing xmlMarkId\n");
    }
    else if (xid)			 /* walk the rest of the tree */
    {
        char *elem, *next;
        void *xmid;

        elem = tree;
        if (*elem == '/') elem++;

        next = strchr(elem, '/');
        
        xmid = xmlMarkId(xid);
        if (xmid)
        {
            if (next)
            {
               *next++ = 0;
            }

            no_elements = xmlNodeGetNum(xid, elem);
            for (i=0; i<no_elements; i++)
            {
                if (xmlNodeGetPos(xid, xmid, elem, i) != 0)
                    walk_the_tree(num, xmid, next);
            }

            if (next)
            {
               *--next = '/';
            }

            free(xmid);
        }
        else printf("Error executing xmlMarkId\n");
    }
}

void grep_file(unsigned num)
{
    void *xid;

    xid = xmlOpen(_filenames[num]);
    if (xid)
    {
       void *xrid = xmlMarkId(xid);
       int r = 0;

       walk_the_tree(num, xrid, _root);

       r = xmlErrorGetNo(xrid, 0);
       if (r)
       {
            size_t n = xmlErrorGetLineNo(xrid, 0);
            size_t c = xmlErrorGetColumnNo(xrid, 0);
            const char *s = xmlErrorGetString(xrid, 1); /* clear the error */
            printf("%s: at line %u, column %u: '%s'\n",_filenames[num], n,c, s);
       }

       free(xrid);
    }
    else
    {
        fprintf(stderr, "Error reading file '%s'\n", _filenames[num]);
    }

    xmlClose(xid);
}


void grep_file_buffer(unsigned num)
{
    struct stat st;
    void *xid, *buf;
    int fd, res;

    fd = open(_filenames[num], O_RDONLY);
    if (fd == -1)
    {
        printf("read error opening file '%s'\n", _filenames[num]);
        return;
    }
    
    fstat(fd, &st);
    buf = malloc(st.st_size);
    if (!buf)
    {
        printf("unable to allocate enough memory for reading.\n");
        return;
    }

    res = read(fd, buf, st.st_size);
    if (res == -1)
    {
        printf("unable to read from file '%s'.\n", _filenames[num]);
        return;
    }
    close(fd);

    xid = xmlInitBuffer(buf, st.st_size);
    if (xid)
    {
       void *xrid = xmlMarkId(xid);
       int r = 0;

       walk_the_tree(num, xrid, _root);

       r = xmlErrorGetNo(xrid, 0);
       if (r)
       {
            size_t n = xmlErrorGetLineNo(xrid, 0);
            size_t c = xmlErrorGetColumnNo(xrid, 0);
            const char *s = xmlErrorGetString(xrid, 1); /* clear the error */
            printf("%s: at line %u, column %u: '%s'\n",_filenames[num], n,c, s);
       }

       free(xrid);
    }
    else
    {
        fprintf(stderr, "Error reading file '%s'\n", _filenames[num]);
    }

    xmlClose(xid);
    free(buf);
}

int
main (int argc, char **argv)
{
    unsigned int u;
    int i;

    if (argc == 1)
        show_help();

    for (i=1; i<argc;)
    {
        int ret = parse_option(argv, i, argc);
        i += ret;
    }

    if (_root == 0) _root = (char *)_static_root;
    if (_element == 0) _element = (char *)_static_element;

    for (u=0; u<_fcount; u++)
#if USE_BUFFER
        grep_file_buffer(u);
#else
        grep_file(u);
#endif

    free_and_exit(0);

    return 0;
}
