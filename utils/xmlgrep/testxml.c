#include <stdio.h>
#include <malloc.h>
#include <assert.h>
#include "xml.h"

#define ROOTNODE	"/Configuration/output/menu"
#define	LEAFNODE	"name"
#define PATH		ROOTNODE"/"LEAFNODE
#define BUFLEN		4096

#define PRINT_ERROR_AND_EXIT(id) \
    if (xmlErrorGetNo(id, 0) != XML_NO_ERROR) { \
         const char *errstr = xmlErrorGetString(id, 0); \
         size_t column = xmlErrorGetColumnNo(id, 0); \
         size_t lineno = xmlErrorGetLineNo(id, 1); \
         printf("Error at line %i, column %i: %s\n", lineno, column, errstr); \
         exit(-1); \
      }

int main()
{
   void *root_id;

   root_id = xmlOpen("sample.xml");
   if (root_id)
   {
      void *path_id, *node_id;
      char *s;

      printf("\nTesting xmlNodeGetString for /*/*/test:\t\t\t\t\t");
      s = xmlNodeGetString(root_id , "/*/*/test");
      if (s)
      {
         printf("failed.\n\t'%s' should be empty\n", s);
         free(s);
      }
      else
         printf("succes.\n");

      printf("Testing xmlGetString for /Configuration/output/test:\t\t\t");
      path_id = xmlNodeGet(root_id, "/Configuration/output/test");
      if (path_id)
      {
         s = xmlGetString(path_id);
         if (s)
         {
            printf("failed.\n\t'%s' should be empty\n", s);
            free(s);
         }
         else
            printf("succes.\n");
      }
      else
         PRINT_ERROR_AND_EXIT(root_id);

      path_id = xmlNodeGet(root_id, PATH);
      node_id = xmlNodeGet(root_id, ROOTNODE);

      if (path_id && node_id)
      {
         char buf[BUFLEN];
         size_t len;
        
         xmlCopyString(path_id, buf, BUFLEN);
         printf("Testing xmlNodeCopyString against xmlGetString:\t\t\t\t");
         if ((s = xmlGetString(path_id)) != 0)
         {
            if (strcmp(s, buf)) /* not the same */
               printf("failed.\n\t'%s' differs from '%s'\n", s, buf);
            else
               printf("succes.\n");

            printf("Testing xmlCopyString against xmlGetString:\t\t\t\t");
            xmlCopyString(path_id, buf, BUFLEN);
            if (strcmp(s, buf)) /* not the same */
               printf("failed.\n\t'%s' differs from\n\t'%s'\n", s, buf);
            else
               printf("succes.\n");
         }
         else
            PRINT_ERROR_AND_EXIT(path_id);

         printf("Testing xmlCopyString against xmlCompareString:\t\t\t\t");
         if (xmlCompareString(path_id, buf)) /* not the same */
            printf ("failed.\n\t'%s' differs from\n\t'%s'\n", s, buf);
         else
            printf("succes.\n");

         printf("Testing xmlCopyString against xmlNodeCompareString:\t\t\t");
         if (xmlNodeCompareString(node_id, LEAFNODE, buf)) /* not the same */
            printf("failed.\n\t'%s' differs from\n\t'%s'\n", s, buf);
         else
            printf("succes.\n");

         if (s) free(s);
 
         printf("Testing xmlCopyString against xmlNodeGetString:\t\t\t\t");
         if ((s = xmlNodeGetString(node_id, LEAFNODE)) != 0)
         {
            if (strcmp(s, buf)) /* not the same */
               printf("failed.\n\t'%s' differs from\n\t'%s'\n", s, buf);
            else
               printf("succes.\n");
            free(s);
         }
         else
            PRINT_ERROR_AND_EXIT(node_id);

         free(path_id);
         path_id = xmlNodeGet(root_id, "/Configuration/backend/name");
         if (path_id)
         {
            printf("Testing xmlAttributeCopyString against xmlAttributeCompareString:\t");
            xmlAttributeCopyString(path_id, "type", buf, BUFLEN);
            if (xmlAttributeCompareString(path_id, "type", buf)) /* no match */
               printf("failed.\n\t'%s' differs\n", buf);
            else
               printf("succes.\n");

            printf("Testing xmlAttributeCopyString against xmlAttributeGetString:\t\t");
            if ((s = xmlAttributeGetString(path_id, "type")) != 0)
            {
                if (strcmp(s, buf)) /* not the same */
                   printf("failed.\n\t'%s' differs from '%s'\n", s, buf);
                else
                   printf("succes.\n");
                free(s);
            }
            else
                PRINT_ERROR_AND_EXIT(path_id);

         }
         else
            PRINT_ERROR_AND_EXIT(root_id);

         free(node_id);
         free(path_id);

         path_id = xmlNodeGet(root_id, "Configuration/output/sample/test");
         if (path_id)
         {
            xmlNodeCopyString(root_id ,"Configuration/output/menu/name", buf, BUFLEN);
            printf("Testing xmlCompareString against a fixed string: \t\t\t");
            if (xmlCompareString(path_id, buf)) 	/* no match */
               printf("failed.\n\t'%s' differs\n", buf);
            else
               printf("succes.\n");

            s = xmlGetString(path_id);
            if (s)
            {
               printf("Testing xmlGetString  against a fixed string: \t\t\t\t");
               if (strcmp(s, buf))			/* mismatch */
                  printf("failed.\n\t'%s' differs from\n\t'%s'\n", s, buf);
               else
                  printf("succes.\n");

               printf("Testing xmlCopyString gainst a fixed string: \t\t\t\t");
               xmlCopyString(path_id, buf, BUFLEN);
               if (strcmp(s, buf))      		/* mismatch */
                  printf("failed.\n\t'%s' differs from\n\t'%s'\n", s, buf);
               else
                  printf("succes.\n"); 

               free(s);
            }
            else
               PRINT_ERROR_AND_EXIT(path_id);

            free(path_id);
         }
      }

      if (xmlErrorGetNo(root_id, 0) != XML_NO_ERROR)
      {
         const char *errstr = xmlErrorGetString(root_id, 0);
         size_t column = xmlErrorGetColumnNo(root_id, 0);
         size_t lineno = xmlErrorGetLineNo(root_id, 1);

         printf("Error at line %i, column %i: %s\n", lineno, column, errstr);
      }

      xmlClose(root_id);
   }
   printf("\n");

   return 0;
}
