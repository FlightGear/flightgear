
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <simgear/props/props.hxx>
#include <simgear/props/props_io.hxx>
#include <simgear/structure/exception.hxx>


unsigned int _fcount = 0;
char **_filenames = 0;
char *_element = 0;
char *_value = 0;
char *_root = 0;
char *_print = 0;

int print_filenames = 0;

#define DEBUG 0

void free_and_exit(int i);


#define SHOW_NOVAL(opt) \
{ \
   printf("option '%s' requires a value\n\n", (opt)); \
   free_and_exit(-1); \
}

void show_help ()
{
  printf("usage: xmlgrep [options] [file ...]\n\n");
  printf("Options:\n");
  printf("\t-h\t\tshow this help message\n");
  printf("\t-e <id>\t\tshow sections that contain this element\n");
  printf("\t-p <id>\t\tprint this element as the output\n");
  printf("\t-r <path>\tspecify the XML search root\n");
  printf("\t-v <string>\tshow sections where on of the elements has this value \n");
  printf("\n");
  printf(" To print the contents of the 'type' element of the XML section ");
  printf("that begins\n at '/printer/output' one would use the following ");
  printf("syntax:\n\n\txmlgrep -r /printer/output -p type sample.xml\n\n");
  printf(" To filter out sections that contain the 'driver' element with ");
  printf("'generic' as\n it's value one would issue the following command:\n");
  printf("\n\txmlgrep -r /printer/output -e driver -v generic -p type ");
  printf("sample.xml\n\n");
  free_and_exit(0);
}

void free_and_exit(int i)
{
   if (_root) free(_root);
   if (_value) free(_value);
   if (_element) free(_element);
   if (_filenames)
   {
      for (i=0; i < _fcount; i++) {
         if (_filenames[i]) {
            if (print_filenames) printf("%s\n", _filenames[i]);
            free(_filenames[i]);
         }
      }
      free(_filenames);
   }

   exit(i);
}

int parse_option(char **args, int n, int max) {
  char *opt, *arg = 0;
  int sz;

  opt = args[n];
  if (opt[0] == '-' && opt[1] == '-')
    opt++;

  if ((arg = strchr(opt, '=')) != NULL)
    *arg++ = 0;

  else if (++n < max)
  {
    arg = args[n];
    if (arg && arg[0] == '-')
      arg = 0;
  }

#if DEBUG
    fprintf(stderr, "processing '%s'='%s'\n", opt, arg ? arg : "NULL");
#endif

  sz = strlen(opt);
  if (strncmp(opt, "-help", sz) == 0) {
    show_help();
  }

  else if (strncmp(opt, "-root", sz) == 0) {
    if (arg == 0) SHOW_NOVAL(opt);
    _root = strdup(arg);
#if DEBUG
    fprintf(stderr, "\troot=%s\n", _root);
#endif
    return 2;
  }

  else if (strncmp(opt, "-element", sz) == 0) {
    if (arg == 0) SHOW_NOVAL(opt);
    _element = strdup(arg);
#if DEBUG
    fprintf(stderr, "\telement=%s\n", _element);
#endif
    return 2;
  }

  else if (strncmp(opt, "-value", sz) == 0) {
    if (arg == 0) SHOW_NOVAL(opt);
    _value = strdup(arg);
#if DEBUG
    fprintf(stderr, "\tvalue=%s\n", _value);
#endif
    return 2;
  }

  else if (strncmp(opt, "-print", sz) == 0) {
    if (arg == 0) SHOW_NOVAL(opt);
    _print = strdup(arg);
#if DEBUG
    fprintf(stderr, "\tprint=%s\n", _print);
#endif
    return 2;
  }


  /* undocumented test argument */
  else if (strncmp(opt, "-list-filenames", sz) == 0) {
    print_filenames = 1;
    return 1;
  }

  else if (opt[0] == '-') {
    printf("Unknown option %s\n", opt);
    free_and_exit(-1);
  }

  else {
    int pos = _fcount++;
    if (_filenames == 0)
      _filenames = (char **)malloc(sizeof(char*));
    else {
      char **ptr = (char **)realloc(_filenames, _fcount*sizeof(char*));
      if (ptr == 0) {
         printf("Out of memory.\n\n");
         free_and_exit(-1);
      }
      _filenames = ptr;
    }

    _filenames[pos] = strdup(opt);
#if DEBUG
    fprintf(stderr, "\tadding filenames[%i]='%s'\n", pos, _filenames[pos]);
#endif
  }

  return 1;
}

void grep_file(unsigned num)
{
   SGPropertyNode root, *path;

#if DEBUG
   fprintf(stderr, "Reading filenames[%i]: %s ... ", num, _filenames[num]);
#endif
   try {
      readProperties(_filenames[num], &root);
   } catch (const sg_exception &e) {
      fprintf(stderr, "Error reading file '%s'\n", _filenames[num]);
      // free_and_exit(-1);
      return;
   }
#if DEBUG
   fprintf(stderr, "done.\n");
#endif

   if ((path = root.getNode(_root, false)) != NULL)
   {
      SGPropertyNode *elem;

      if (_element && _value)
      {
         if ((elem = path->getNode(_element, false)) != NULL)
         {
            if (strcmp(elem->getStringValue(), _value) == NULL)
            {
               SGPropertyNode *print = path->getNode(_print, false);
               if (print)
               {
                  printf("%s: <%s>%s</%s>\n", _filenames[num],
                                   _print, print->getStringValue(), _print);
               }
            }
         }
      }
      else if (_element)
      {
      }
      else if (_value)
      {
      }
   }
#if DEBUG
   else
      fprintf(stderr," No root node specified.\n");
#endif
}

inline void grep_files()
{
#if DEBUG
   fprintf(stderr, "Reading files ...\n");
#endif
   for (int i=0; i<_fcount; i++)
      grep_file(i);
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
#if DEBUG
      fprintf(stderr, "%i arguments processed.\n", ret);
#endif
   }


   grep_files();

   free_and_exit(0);

   return 0;
}
