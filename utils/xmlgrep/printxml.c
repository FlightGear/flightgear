
#include <stdio.h>
#include <malloc.h>
#include "xml.h"

void print_xml(void *);

int main(int argc, char **argv)
{
  if (argc < 1)
  {
    printf("usage: printxml <filename>\n\n");
  }
  else
  {
    void *rid;

    rid = xmlOpen(argv[1]);
    if (rid)
    {
      unsigned int i, num;
      void *xid;
 
      xid = xmlMarkId(rid);
      num = xmlNodeGetNum(xid, "*");
      for (i=0; i<num; i++)
      {
        if (xmlNodeGetPos(rid, xid, "*", i) != 0)
        {
          char name[256];
          xmlNodeCopyName(xid, (char *)&name, 256);
          printf("<%s>\n", name);
          print_xml(xid);
          printf("\n</%s>\n", name);
        }
      }
      free(xid);

      xmlClose(rid);
    }
    else
    {
      printf("Error while opening file for reading: '%s'\n", argv[1]);
    }
  }
}

void print_xml(void *id)
{
  static int level = 1;
  void *xid = xmlMarkId(id);
  unsigned int i, num;
  
  num = xmlNodeGetNum(xid, "*");
  if (num == 0)
  {
    char value[256];
    int q;

    q = xmlCopyString(xid, (char *)&value, 256);
    if (q) printf("%s", value);
  }
  else
  {
    unsigned int i, q;
    for (i=0; i<num; i++)
    {
      if (xmlNodeGetPos(id, xid, "*", i) != 0)
      {
        char name[256];

        xmlNodeCopyName(xid, (char *)&name, 256);

        printf("\n");
        for(q=0; q<level; q++) printf(" ");
        printf("<%s>", name);

        level++;
        print_xml(xid);
        level--;

        printf("</%s>", name);
      }
      else printf("error\n");
    }
    printf("\n");
    for(q=1; q<level; q++) printf(" ");
  }
}
