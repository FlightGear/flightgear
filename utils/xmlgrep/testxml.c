#include <stdio.h>
#include <malloc.h>
#include "xml.h"

#define ROOTNODE	"/Configuration/output/menu"
#define	LEAFNODE	"name"
#define PATH		ROOTNODE"/"LEAFNODE
#define BUFLEN		4096
int main()
{
   void *root_id;

   root_id = xmlOpen("sample.xml");
   if (root_id)
   {
      void *path_id, *node_id;
      char *s;

      printf("\nTesting xmlNodeGetString for /Configuration/output/test:\t\t");
      s = xmlNodeGetString(root_id , "/Configuration/output/test");
      if (s)
      {
         printf("failed.\n\t'%s' should be empty\n", s);
         free(s);
      }
      else
         printf("succes.\n");

      printf("Testing xmlGetString for Configuration/output/test:\t\t\t");
      path_id = xmlNodeGet(root_id, "*/*/test");
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
               printf("failed.\n\t'%s' differs from '%s'\n", s, buf);
            else
               printf("succes.\n");
            free(s);
         }
         else
            printf("Error while fetching node's value.\n");

         printf("Testing xmlCopyString against xmlCompareString:\t\t\t\t");
         if (xmlCompareString(path_id, buf)) /* not the same */
            printf ("failed.\n\t'%s' differs\n", buf);
         else
            printf("succes.\n");

         printf("Testing xmlCopyString against xmlNodeCompareString:\t\t\t");
         if (xmlNodeCompareString(node_id, LEAFNODE, buf)) /* not the same */
            printf ("failed.\n\t'%s' differs\n", buf);
         else
            printf("succes.\n");
 
         printf("Testing xmlCopyString against xmlNodeGetString:\t\t\t\t");
         if ((s = xmlNodeGetString(node_id, LEAFNODE)) != 0)
         {
            if (strcmp(s, buf)) /* not the same */
               printf("failed.\n\t'%s' differs from '%s'\n", s, buf);
            else
               printf("succes.\n");
            free(s);
         }
         else
            printf("Error while fetching value from node.\n");

         free(path_id);
         path_id = xmlNodeGet(root_id, "/Configuration/backend/name");
         if (path_id)
         {
            xmlAttributeCopyString(path_id, "type", buf, BUFLEN);
            
            printf("Testing xmlAttributeCopyString against xmlAttributeCompareString:\t");
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
            printf("Error while fetching value from attribute.\n");

         }
         else
            printf("Error while fetching node's attribute.\n");

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

            free(path_id);
         }
      }
      else
      {
         printf("Error: %s\n", xmlErrorGetString(root_id, 1));
      }

      xmlClose(root_id);
   }
   printf("\n");

   return 0;
}
