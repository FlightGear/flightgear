#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <malloc.h>
#include <assert.h>
#include "xml.h"

#define ROOTNODE	"/PropertyList/generic"
#define BUFLEN		4096

#define PRINT_ERROR_AND_EXIT(id) \
    if (xmlErrorGetNo(id, 0) != XML_NO_ERROR) { \
         const char *errstr = xmlErrorGetString(id, 0); \
         size_t column = xmlErrorGetColumnNo(id, 0); \
         size_t lineno = xmlErrorGetLineNo(id, 1); \
         printf("Error at line %i, column %i: %s\n", lineno, column, errstr); \
         exit(-1); \
      }

void print_binary_protocol(void *, char *, char *);

int main(int argc, char **argv)
{
   void *root_id, *protocol_id = 0;
   char *filename;

   if (argc != 2)
   {
      printf("Usage: %s <filename>\n", argv[0]);
      exit(-1);
   }

   filename = argv[1];
   root_id = xmlOpen(filename);
   if (root_id) protocol_id = xmlNodeGet(root_id, ROOTNODE);

   if (protocol_id)
   {
      void *dir_id;

      dir_id = xmlNodeGet(protocol_id, "input");
      if (dir_id)
      {
         if (!xmlNodeCompareString(dir_id, "binary_mode", "true")) {
            print_binary_protocol(dir_id, filename, "input");
         } else {
            printf("Only binary mode support is implemented at the moment.\n");
         }
         free(dir_id);
      }

      dir_id = xmlNodeGet(protocol_id, "output");
      if (dir_id)
      {
         if (!xmlNodeCompareString(dir_id, "binary_mode", "true")) {
            print_binary_protocol(dir_id, filename, "output");
         } else {
            printf("Only binary mode support is implemented at the moment.\n");
         }
         free(dir_id);
      }

      free(protocol_id);
   }

   if (root_id)
   {
      xmlClose(root_id);
   }

   return 0;
}

/* -------------------------------------------------------------------------- */

void print_binary_protocol(void *id, char *filename, char *dir)
{
   unsigned int i, num, pos = 0;
   void *xid;

   printf("\n%s\n", filename);
   printf("Generic binary %s protocol packet description:\n\n", dir);
   printf(" pos | size |  type  | factor     | description\n");
   printf("-----|------|--------|------------|------------------------\n");
   xid = xmlMarkId(id);
   num = xmlNodeGetNum(xid, "chunk");
   for (i=0; i<num; i++)
   {
      if (xmlNodeGetPos(id, xid, "chunk", i) != 0)
      {
         char factor[11];
         char *name, *type;
         int size;

         type = xmlNodeGetString(xid, "type");
         if (!strcasecmp(type, "bool")) {
            size = 1;
         } else if (!strcasecmp(type, "float")) {
            size = 4;
         } else if (!strcasecmp(type, "double")) {
            size = 8;
         } else if (!strcasecmp(type, "int")) {
            size = 4;
         } else {
           printf("Unsupported type sepcified: '%s'\n\n", type);
           free(type);
           free(xid);
           return;
         }

         xmlNodeCopyString(xid, "factor", (char *)&factor, 10);
         name = xmlNodeGetString(xid, "name");

         printf("%4i | %4i | %6s | %10s | %s\n", pos, size, type, factor, name);
         pos += size;

         free(type);
         free(name);
      }
   }

   printf("\ntotal package size: %i bytes\n\n", pos);
   free(xid);
}
