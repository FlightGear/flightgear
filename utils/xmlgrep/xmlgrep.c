#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "xml.h"

static const char *_static_root = "/";
static unsigned int _fcount = 0;
static char **_filenames = 0;
static char *_element = 0;
static char *_value = 0;
static char *_root = 0;
static char *_print = 0;
static int print_filenames = 0;

static void free_and_exit(int i);

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
    printf("\t-e <id>\t\tshow sections that contain this element\n");
    printf("\t-p <id>\t\tprint this element as the output\n");
    printf("\t-r <path>\tspecify the XML search root\n");
    printf("\t-v <string>\tshow sections where one of the elements has this ");
    printf("value\n\n");
    printf(" To print the contents of the 'type' element of the XML section ");
    printf("that begins\n at '/printer/output' one would use the following ");
    printf("syntax:\n\n\txmlgrep -r /printer/output -p type sample.xml\n\n");
    printf(" To filter out sections that contain the 'driver' element with ");
    printf("'generic' as\n it's value one would issue the following command:");
    printf("\n\n\txmlgrep -r /printer/output -e driver -v generic sample.xml");
    printf("\n\n");
    free_and_exit(0);
}

void
free_and_exit(int i)
{
    if (_root != _static_root) free(_root);
    if (_element) free(_element);
    if (_value) free(_value);
    if (_print) free(_print);
    if (_filenames)
    {
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
 
    exit(i);
}

int
parse_option(char **args, int n, int max)
{
    char *opt, *arg = 0;
    int sz;

    opt = args[n];
    if (opt[0] == '-' && opt[1] == '-')
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

    sz = strlen(opt);
    if (strncmp(opt, "-help", sz) == 0)
    {
        show_help();
    }
    else if (strncmp(opt, "-root", sz) == 0)
    {
        if (arg == 0) SHOW_NOVAL(opt);
        _root = strdup(arg);
        return 2;
    }
    else if (strncmp(opt, "-element", sz) == 0)
    {
        if (arg == 0) SHOW_NOVAL(opt);
        _element = strdup(arg);
        return 2;
    }
    else if (strncmp(opt, "-value", sz) == 0)
    {
        if (arg == 0) SHOW_NOVAL(opt);
        _value = strdup(arg);
        return 2;
    }
    else if (strncmp(opt, "-print", sz) == 0)
    {
        if (arg == 0) SHOW_NOVAL(opt);
        _print = strdup(arg);
        return 2;
    }
    else if (strncmp(opt, "-list-filenames", sz) == 0)
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

        _filenames[pos] = strdup(opt);
    }

    return 1;
}

void walk_the_tree(size_t num, void *xid, char *tree)
{
    unsigned int q, no_elements;
    char *elem, *next;

    elem = tree;
    if (*elem == '/') elem++;

    next = strchr(elem, '/');
    if (!next)					/* last node from the tree */
    {
        void *elem_id = xmlMarkId(xid);

        no_elements = xmlGetNumElements(xid, elem);
        for (q=0; q<no_elements; q++)
        {
            void *node_id = xmlGetNextElement(xid, elem_id, elem);
            if (node_id && _print)
            {
                unsigned int i, no_nodes;
                void *xmid;

                xmid = xmlMarkId(node_id);

                no_nodes = xmlGetNumElements(node_id, _print);
                for (i=0; i<no_nodes; i++)
                {
                    if (xmlGetNextElement(node_id, xmid, _print) != 0)
                    {
                        char *value = xmlGetString(xmid);
                        if (value)
                        {
                            printf("%s: <%s>%s</%s>\n",
                                   _filenames[num], _print, value, _print);
                            free(value);
                        }
                    }
                }
                free(xmid);
            }
            else if (node_id && _value)
            {
                if (_element)
                {
                    unsigned int i, no_nodes;
                    void *xmid;

                    xmid = xmlMarkId(node_id);

                    no_nodes = xmlGetNumElements(node_id, _element);
                    for (i=0; i<no_nodes; i++)
                    {
                        xmlGetNextElement(node_id, xmid, _element);
                        if (xmlCompareString(xmid, _value) == 0)
                        {
                            printf("%s: <%s>%s</%s>\n",
                                   _filenames[num], _element, _value, _element);
                        }
                    }
                    free(xmid);
                }
                else
                {
                }
            }
            else if (node_id && _element)
            {
            }
        }
        free(elem_id);
    }
    else				 /* walk the rest of the tree */
    {
        void *xmid;

        xmid = xmlMarkId(xid);
        if (xmid)
        {
            *next++ = 0;

            no_elements = xmlGetNumElements(xid, elem);
            for (q=0; q<no_elements; q++)
            {
                void *elem_id = xmlGetNextElement(xid, xmid, elem);
                walk_the_tree(num, elem_id, next);
            }

            *--next = '/';

            free(xmid);
        }
        else printf("Error executing xmlMarkId\n");
    }
}


void grep_file(unsigned num)
{
    void *xrid;

    xrid = xmlOpen(_filenames[num]);
    if (xrid)
    {
       void *xid = xmlMarkId(xrid);
       walk_the_tree(num, xrid, _root);
       free(xrid);
       xrid = xid;
    }
    else
    {
        fprintf(stderr, "Error reading file '%s'\n", _filenames[num]);
    }

    xmlClose(xrid);
}

int
main (int argc, char **argv)
{
    int i;

    if (argc == 1)
        show_help();

    for (i=1; i<argc;)
    {
        int ret = parse_option(argv, i, argc);
        i += ret;
    }

    if (_root == 0) (_root = (char *)_static_root);

    for (i=0; i<_fcount; i++)
        grep_file(i);

    free_and_exit(0);

    return 0;
}
