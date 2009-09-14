/********************************************************************/
/*   STRIPE: converting a polygonal model to triangle strips    
     Francine Evans, 1996.
     SUNY @ Stony Brook
     Advisors: Steven Skiena and Amitabh Varshney
*/
/********************************************************************/

/*---------------------------------------------------------------------*/
/*   STRIPE: util.c
     This file contains routines that are used for various functions
*/
/*---------------------------------------------------------------------*/


#include <stdlib.h>
#include "polverts.h"

void switch_lower (int *x, int *y)
{
	register int temp;
	
	/*	Put lower value in x */
	if (*y < *x)
	{
		temp = *x;
		*x = *y;
		*y = temp;
	}
}

BOOL member(int x , int id1, int id2, int id3)
{
    /*  Is x in the triangle specified by id1,id2,id3 */
    if ((x != id1) && (x != id2) && (x != id3))
	return FALSE;
    return TRUE;
}


int Compare (P_ADJACENCIES node1, P_ADJACENCIES node2)
{
	/*	This will only return whether 2 adjacency nodes
		are equivalent.								  
	*/
	if (node1->face_id == node2->face_id)
		return TRUE;
	else
		return FALSE;
}


BOOL Exist(int face_id, int id1, int id2)
{
	/*	Does the edge specified by id1 and id2 exist in this
		face currently? Maybe we deleted in partial triangulation
	*/
	ListHead *pListHead;
	PF_FACES temp;
	register int x,size;
	BOOL a=FALSE,b =FALSE; 

	pListHead = PolFaces[face_id];
	temp = ( PF_FACES ) PeekList( pListHead, LISTHEAD, 0 );
	size = temp->nPolSize;
	for (x=0; x<size; x++)
	{
		if (*(temp->pPolygon+x) == id1)
			a = TRUE;
		if (*(temp->pPolygon+x) == id2)
			b = TRUE;
		if (a && b)
			return TRUE;
	}
	return FALSE;
}

int Get_Next_Id(int *index,int e3, int size)
{
    /*  Return the id following e3 in the list of vertices */

    register int x;

    for (x = 0; x< size; x++)
    {
        if ((*(index+x) == e3) && (x != (size-1)))
            return *(index+x+1);
        else if (*(index+x) == e3)
            return *(index);
    }
    printf("There is an error in the next id\n");
    exit(0);
}

int Different (int id1,int id2,int id3,int id4,int id5, int id6, int *x, int *y)
{
    /*    Find the vertex in the first 3 numbers that does not exist in 
	     the last three numbers
    */
    if ((id1 != id4) && (id1 != id5) && (id1 != id6))
    {
	*x = id2;
	*y = id3;
	return id1;
    }
    if ((id2 != id4) && (id2 != id5) && (id2 != id6))
    {
	*x = id1;
	*y = id3;
	return id2;
    }
    if ((id3 != id4) && (id3 != id5) && (id3 != id6))
    {
	*x = id1;
	*y = id2;
	return id3;
    }
    
    /*  Because there are degeneracies in the data, this might occur */
    *x = id5;
    *y = id6;
    return id4;
}

int Return_Other(int *index,int e1,int e2)
{
	/*   We have a triangle and want to know the third vertex of it */
	register int x;

	for (x=0;x<3;x++)
	{
		if ((*(index+x) != e1) && (*(index+x) != e2))
			return *(index+x);
	}
     /*   If there is a degenerate triangle return arbitrary */
     return e1;
}

int Get_Other_Vertex(int id1,int id2,int id3,int *index)
{
	/*	We have a list index of 4 numbers and we wish to
          return the number that is not id1,id2 or id3
	*/
	register int x;

	for (x=0; x<4; x++)
	{
		if ((*(index+x) != id1) && (*(index+x) != id2) &&
			(*(index+x) != id3))
			return *(index+x);
	}
	/*   If there is some sort of degeneracy this might occur,
          return arbitrary 
     */
     if (x==4)
          return id1;
}


PLISTINFO Done(int face_id, int size, int *bucket)
{
	/*	Check to see whether the polygon with face_id was used
		already, return NULL if it was, otherwise return a pointer to the face.
	*/
	P_ADJACENCIES pfNode;
	register int y;
	PLISTINFO lpListInfo;
	
	pfNode = (P_ADJACENCIES) malloc(sizeof(ADJACENCIES) );
	if ( pfNode )
		pfNode->face_id = face_id;
		
	for (y=size; ; y--)
	{
		lpListInfo = SearchList(array[y], pfNode,
			(int (*)(void *,void *)) (Compare));
		if (lpListInfo != NULL)
		{
			*bucket = y;
			return lpListInfo;
		}
		if (y == 0)
		/*	This adjacent face was done already */
			return lpListInfo;
	}
	free (pfNode);
}

void Output_Edge(int *index,int e2,int e3,int *output1,int *output2)
{
    /*  Given a quad and an input edge return the other 2 vertices of the
        quad.
    */
    
    *output1 = -1;
    *output2 = -1;

    if ((*(index) != e2) && (*(index) != e3))
        *output1 = *(index);

    if ((*(index+1) != e2) && (*(index+1) != e3))
    {
        if (*output1 == -1)
            *output1 = *(index+1);
        else
        {
            *output2 = *(index+1);
            return;
        }
    }

    if ((*(index+2) != e2) && (*(index+2) != e3))
    {
        if (*output1 == -1)
            *output1 = *(index+2);
        else
        {
            *output2 = *(index+2);
            return;
        }
    }

    *output2 = *(index+3);
}


void First_Edge(int *id1,int *id2, int *id3)
{
    /*  Get the first triangle in the strip we just found, we will use this to
	   try to extend backwards in the strip
    */

    ListHead *pListHead;
    register int num;
    P_STRIPS temp1,temp2,temp3;
	 
    pListHead = strips[0];
    num = NumOnList(pListHead);
     
    /*    Did not have a strip */
    if (num < 3)
         return;
	  
    temp1 = ( P_STRIPS ) PeekList( pListHead, LISTHEAD, 0);
    temp2 = ( P_STRIPS ) PeekList( pListHead, LISTHEAD, 1);
    temp3 = ( P_STRIPS ) PeekList( pListHead, LISTHEAD, 2);
    *id1 = temp1->face_id;
    *id2 = temp2->face_id;
    *id3 = temp3->face_id;
 
}

void Last_Edge(int *id1, int *id2, int *id3, BOOL save)
{
	/*   We need the last edge that we had  */
	static int v1, v2, v3;

	if (save)
	{
		v1 = *id1;
		v2 = *id2;
		v3 = *id3;
	}
	else
	{
		*id1 = v1;
		*id2 = v2;
		*id3 = v3;
	}
}


