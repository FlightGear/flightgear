
#include <stdio.h>
#include <malloc.h>

#include "xml.h"

void print_xml(void *, char *, unsigned int);

int main(int argc, char **argv)
{
  if (argc < 1)
  {
    printf("usage: printtree <filename>\n\n");
  }
  else
  {
    void *rid;

    rid = xmlOpen(argv[1]);
    if (xmlErrorGetNo(rid, 0) != XML_NO_ERROR)
    {
       printf("%s\n", xmlErrorGetString(rid, 1));
    }
    else if (rid)
    {
      unsigned int i, num;
      void *xid;
 
      xid = xmlMarkId(rid);
      num = xmlNodeGetNum(xid, "*");
      for (i=0; i<num; i++)
      {
        if (xmlNodeGetPos(rid, xid, "*", i) != 0)
        {
          char name[4096] = "";
          print_xml(xid, (char *)&name, 0);
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

void print_xml(void *id, char *name, unsigned int len)
{
  void *xid = xmlMarkId(id);
  unsigned int i, num;
  
  num = xmlNodeGetNum(xid, "*");
  if (num == 0)
  {
    char *s;
    s = xmlGetString(xid);
    if (s)
    {
      printf("%s = %s\n", name, s);
      free(s);
    }
  }
  else
  {
    unsigned int i, q;

    name[len++] = '/';
    for (i=0; i<num; i++)
    {
      if (xmlNodeGetPos(id, xid, "*", i) != 0)
      {
        unsigned int res, i = 4096 - len;
        res = xmlNodeCopyName(xid, (char *)&name[len], i);
        if (res)
        {
          unsigned int index = xmlAttributeGetInt(xid, "n");
          if (index)
          {
            unsigned int pos = len+res;

            name[pos++] = '[';
            i = snprintf((char *)&name[pos], 4096-pos, "%i", index);
            name[pos+i] = ']';
            res += i+2;
          }
        }
        print_xml(xid, name, len+res);
      }
      else printf("error\n");
    }
  }
}
