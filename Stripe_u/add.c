/********************************************************************/
/*   STRIPE: converting a polygonal model to triangle strips    
     Francine Evans, 1996.
     SUNY @ Stony Brook
     Advisors: Steven Skiena and Amitabh Varshney
*/
/********************************************************************/

/*---------------------------------------------------------------------*/
/*   STRIPE: add.c
     This file contains the procedure code that will add information
     to our data structures.
*/
/*---------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include "global.h"
#include "queue.h"
#include "polverts.h"
#include "triangulate.h"
#include "ties.h"
#include "outputex.h"
#include "options.h"
#include "local.h"

BOOL new_vertex(double difference, int id1,int id2,
                struct vert_struct *n)
{
     /*   Is the difference between id1 and id2 (2 normal vertices that
          mapped to the same vertex) greater than the
          threshold that was specified?
     */
     struct vert_struct *pn1,*pn2;
     double dot_product;
     double distance1, distance2,distance;
     double rad;
     char arg1[100];
     char arg2[100];

     pn1 = n + id1;
     pn2 = n + id2;
 
     dot_product = ((pn1->x) * (pn2->x)) +
                   ((pn1->y) * (pn2->y)) +
                   ((pn1->z) * (pn2->z));
     /*   Get the absolute value */
     if (dot_product < 0)
          dot_product = dot_product * -1;

     distance1 = sqrt( (pn1->x * pn1->x) +
                       (pn1->y * pn1->y) +
                       (pn1->z * pn1->z) );
     distance2 = sqrt( (pn2->x * pn2->x) +
                       (pn2->y * pn2->y) +
                       (pn2->z * pn2->z) );
     distance = distance1 * distance2;

     rad = acos((double)dot_product/(double)distance);
     /*   convert to degrees */
     rad = (180 * rad)/PI;
    
     if ( rad <= difference)
         return FALSE;
     
     /*   double checking because of imprecision with floating
          point acos function
     */
     sprintf( arg1,"%.5f", rad );
     sprintf( arg2,"%.5f", difference );
     if ( strcmp( arg1, arg2 ) <=0 )
          return( FALSE );
     if ( rad <= difference)
         return FALSE;
     else 
          return TRUE;
}

BOOL Check_VN(int vertex,int normal, struct vert_added *added)
{
     /*   Check to see if we already added this vertex and normal */
     register int x,n;

     n = (added+vertex)->num;
     for (x = 0; x < n; x++)
     {
          if (*((added+vertex)->normal+x) == normal)
               return TRUE;
     }
     return FALSE;
}

BOOL norm_array(int id, int vertex, double normal_difference,
                struct vert_struct *n, int num_vert)
{
     static int last;
     static struct vert_added *added;
     register int x;
     static BOOL first = TRUE;

     if (first)
     {
          /*   This is the first time that we are in here, so we will allocate
               a structure that will save the vertices that we added, so that we
               do not add the same thing twice
          */
          first = FALSE;
          added = (struct vert_added *) malloc (sizeof (struct vert_added ) * num_vert);
          /*   The number of vertices added for each vertex must be initialized to
               zero
          */
          for (x = 0; x < num_vert; x++)
               (added+x)->num = 0;
     }
     
     if (vertex)
          /*   Set the pointer to the vertex, we will be calling again with the
               normal to fill it with
          */
          last = id;
     else
     {    
          /*   Fill the pointer with the id of the normal */
          if (*(vert_norms + last) == 0)
               *(vert_norms + last) = id;
          else if ((*(vert_norms + last) != id) && ((int)normal_difference != 360))
          {
               /*   difference is big enough, we need to create a new vertex */
               if (new_vertex(normal_difference,id,*(vert_norms + last),n))
               {
                    /*   First check to see if we added this vertex and normal already */
                    if (Check_VN(last,id,added))
                         return FALSE;
                    /*   OK, create the new vertex, and have its id = the number of vertices
                         and its normal what we have here
                    */
                    vert_norms = realloc(vert_norms, sizeof(int) * (num_vert + 1));
                    if (!vert_norms)
                    {
                         printf("Allocation error - aborting\n");
                         exit(1);
                    }
                    *(vert_norms + num_vert) = id;
                    /*   We created a new vertex, now put it in our added structure so
                         we do not add the same thing twice
                    */
                    (added+last)->num = (added+last)->num + 1;
                    if ((added+last)->num == 1)
                    {
                         /*   First time */
                         (added+last)->normal =  (int *) malloc (sizeof (int ) * 1);
                         *((added+last)->normal) =  id;
                    }
                    else
                    {
                         /*   Not the first time, reallocate space */
                         (added+last)->normal = realloc((added+last)->normal,sizeof(int) * (added+last)->num);
                         *((added+last)->normal+((added+last)->num-1)) = id;
                    }
                    return TRUE;
               }
          }
     }
     return FALSE;
}

void add_texture(int id,BOOL vertex)
{
     /*   Save the texture with its vertex for future use when outputting */
     static int last;

     if (vertex)
          last = id;
     else
          *(vert_texture+last) = id;
}

int	add_vert_id(int id, int	index_count)
{
	register int x;
     
     /*   Test if degenerate, if so do not add degenerate vertex */
     for (x = 1; x < index_count ; x++)
     {
          if (ids[x] == id)
               return 0;
     }
     ids[index_count] = id;
     return 1;
}

void	add_norm_id(int id, int index_count)
{
	norms[index_count] = id;
}

void AddNewFace(int ids[MAX1], int vert_count, int face_id, int norms[MAX1])
{
PF_FACES pfNode;
int	*pTempInt;
int *pnorms;
F_EDGES **pTempVertptr;
int	*pTempmarked, *pTempwalked;
register int	y,count = 0,sum = 0;
	
	/*   Add a new face into our face data structure */

     pfNode = (PF_FACES) malloc(sizeof(F_FACES) );
     if ( pfNode )
     {
          pfNode->pPolygon = (int*) malloc(sizeof(int) * (vert_count) );
     	pfNode->pNorms = (int*) malloc(sizeof(int) * (vert_count) );
          pfNode->VertandId = (F_EDGES**)malloc(sizeof(F_EDGES*) * (vert_count)); 
		pfNode->marked  = (int*)malloc(sizeof(int) * (vert_count));
		pfNode->walked = (int*)malloc(sizeof(int) * (vert_count));
	}
	pTempInt =pfNode->pPolygon;
	pnorms = pfNode->pNorms;
     pTempmarked = pfNode->marked;
	pTempwalked = pfNode->walked;
	pTempVertptr = pfNode->VertandId;
	pfNode->nPolSize = vert_count;
	pfNode->seen = -1;
     pfNode->seen2 = -1;
	for (y=1;y<=vert_count;y++)
	{
		*(pTempInt + count) = ids[y];
		*(pnorms + count) = norms[y];
          *(pTempmarked + count) = FALSE;
		*(pTempwalked + count) =  -1;
		*(pTempVertptr+count) = NULL;
		count++;
	}
        AddHead(PolFaces[face_id-1],(PLISTINFO) pfNode);
}	

	
void CopyFace(int ids[MAX1], int vert_count, int face_id, int norms[MAX1])
{
PF_FACES pfNode;
int	*pTempInt;
int *pnorms;
F_EDGES **pTempVertptr;
int	*pTempmarked, *pTempwalked;
register int	y,count = 0,sum = 0;
	
	/*   Copy a face node into a new node, used after the global algorithm
          is run, so that we can save whatever is left into a new structure
     */
     
     pfNode = (PF_FACES) malloc(sizeof(F_FACES) );
     if ( pfNode )
     {
          pfNode->pPolygon = (int*) malloc(sizeof(int) * (vert_count) );
     	pfNode->pNorms = (int*) malloc(sizeof(int) * (vert_count) );
          pfNode->VertandId = (F_EDGES**)malloc(sizeof(F_EDGES*) * (vert_count)); 
		pfNode->marked  = (int*)malloc(sizeof(int) * (vert_count));
		pfNode->walked = (int*)malloc(sizeof(int) * (vert_count));
	}
	pTempInt =pfNode->pPolygon;
	pnorms = pfNode->pNorms;
     pTempmarked = pfNode->marked;
	pTempwalked = pfNode->walked;
	pTempVertptr = pfNode->VertandId;
	pfNode->nPolSize = vert_count;
	pfNode->seen = -1;
     pfNode->seen2 = -1;
	for (y=0;y<vert_count;y++)
	{
		*(pTempInt + count) = ids[y];
		*(pnorms + count) = norms[y];
          *(pTempmarked + count) = FALSE;
		*(pTempwalked + count) =  -1;
		*(pTempVertptr+count) = NULL;
		count++;
	}
	AddHead(PolFaces[face_id-1],(PLISTINFO) pfNode);
}	
	
void Add_Edge(int v1,int v2)
{
PF_EDGES temp  = NULL;
ListHead *pListHead;
BOOL flag = TRUE;
register int t,count = 0;
	
	/*   Add a new edge into the edge data structure */
     if (v1 > v2)
	{
		t  = v1;
		v1 = v2;
		v2 = t;
	}
	
     pListHead = PolEdges[v1];
	temp = (PF_EDGES) PeekList(pListHead,LISTHEAD,count);
	if (temp == NULL)
     {
          printf("Have the wrong edge \n:");
          exit(1);
     }
	
	while (flag)
	{
		if (v2 == temp->edge[0])
               return;
          else
             	temp = (PF_EDGES) PeekList(pListHead,LISTHEAD,++count);

	}                	
}

void Add_AdjEdge(int v1,int v2,int fnum,int index1 )
{
     PF_EDGES temp  = NULL;
     PF_FACES temp2 = NULL;
     PF_EDGES pfNode;
     ListHead *pListHead;
     ListHead *pListFace;
     BOOL 	flag = TRUE;
     register int	count = 0;
     register int	t,v3 = -1;
	
	if (v1 > v2)
	{
		t  = v1;
		v1 = v2;
		v2 = t;
	}
	pListFace  = PolFaces[fnum];
	temp2 = (PF_FACES) PeekList(pListFace,LISTHEAD,0);
	pListHead = PolEdges[v1];
	temp = (PF_EDGES) PeekList(pListHead,LISTHEAD,count);
	if (temp == NULL)
		flag = FALSE;
	count++;
	while (flag)
	{
		if (v2 == temp->edge[0])
		{
               /*   If greater than 2 polygons adjacent to an edge, then we will
                    only save the first 2 that we found. We will have a small performance
                    hit, but this does not happen often.
               */
               if (temp->edge[2] == -1)
                    temp->edge[2] = fnum;
               else
                    v3 = temp->edge[2];
			flag = FALSE;
		}
		else
		{
			temp = (PF_EDGES) PeekList(pListHead,LISTHEAD,count);
			count++;
			if (temp == NULL)
				flag = FALSE;
		}
	}
                
	/*   Did not find it */
     if (temp == NULL)
	{
		pfNode = (PF_EDGES) malloc(sizeof(F_EDGES) );
          if ( pfNode )
          {
               pfNode->edge[0] = v2;
			pfNode->edge[1] = fnum;
	          pfNode->edge[2] =  v3;
			AddTail( PolEdges[v1], (PLISTINFO) pfNode );
          }
		else
          {
               printf("Out of memory!\n");
               exit(1);
          }
		
          *(temp2->VertandId+index1) = pfNode;
	}
	else
		*(temp2->VertandId+index1) =  temp;
		
}


