/********************************************************************/
/*   STRIPE: converting a polygonal model to triangle strips    
     Francine Evans, 1996.
     SUNY @ Stony Brook
     Advisors: Steven Skiena and Amitabh Varshney
*/
/********************************************************************/

/*---------------------------------------------------------------------*/
/*   STRIPE: common.c
     This file contains common code used in both the local and global algorithm
*/
/*---------------------------------------------------------------------*/


#include <stdlib.h>
#include "polverts.h"
#include "extend.h"
#include "output.h"
#include "triangulate.h"
#include "util.h"
#include "add.h"

int  Old_Adj(int face_id)
{
	/*	Find the bucket that the face_id is currently in,
		because maybe we will be deleting it. 
	*/
	PF_FACES temp = NULL;
	ListHead *pListHead;
	int size,y;
	
	pListHead = PolFaces[face_id];
	temp = ( PF_FACES ) PeekList( pListHead, LISTHEAD, 0 );
	if ( temp == NULL )
	{
		printf("The face was already deleted, there is an error\n");
		exit(0);
	}
	
	size = temp->nPolSize;
	if (Done(face_id,size,&y) == NULL)
	{
		printf("There is an error in finding the face\n");
		exit(0);
	}
	return y;
}

int Number_Adj(int id1, int id2, int curr_id)
{
	/*	Given edge whose endpoints are specified by id1 and id2,
		determine how many polygons share this edge and return that
		number minus one (since we do not want to include the polygon
		that the caller has already).
	*/

	int size,y,count=0;
	PF_EDGES temp = NULL;
	PF_FACES temp2 = NULL;
	ListHead *pListHead;
	BOOL there= FALSE;

	/*	Always want smaller id first */
	switch_lower(&id1,&id2);
	
	pListHead = PolEdges[id1];
	temp = (PF_EDGES) PeekList(pListHead,LISTHEAD,count);
     if (temp == NULL)
     /*	new edge that was created might not be here */
		return 0;
	while (temp->edge[0] != id2)
     {
		count++;
		temp = (PF_EDGES) PeekList(pListHead,LISTHEAD,count);
          if (temp == NULL)
			/*	This edge was not there in the original, which
				mean that we created it in the partial triangulation.
				So it is adjacent to nothing.
			*/
			return 0;
	}
	/*	Was not adjacent to anything else except itself */
	if (temp->edge[2] == -1)
		return 0;
	else
	{
		/*	It was adjacent to another polygon, but maybe we did this
			polygon already, and it was done partially so that this edge
			could have been done
		*/
		if (curr_id != temp->edge[1])
		{
			/*	Did we use this polygon already?and it was deleted
				completely from the structure
			*/
			pListHead = PolFaces[temp->edge[1]];
			temp2 = ( PF_FACES ) PeekList( pListHead, LISTHEAD, 0 );
			if (Done(temp->edge[1],temp2->nPolSize,&size) == NULL)
				return 0;
		}
		else
		{
			pListHead = PolFaces[temp->edge[2]];
			temp2 = ( PF_FACES ) PeekList( pListHead, LISTHEAD, 0 );
			if (Done(temp->edge[2],temp2->nPolSize,&size)== NULL)
				return 0;
		}

		/*	Now we have to check whether it was partially done, before
			we can say definitely if it is adjacent.
			Check each edge of the face and tally the number of adjacent
			polygons to this face. 
		*/	      		
		if ( temp2 != NULL )
		{
			/*	Size of the polygon */
			size = temp2->nPolSize;
			for (y = 0; y< size; y++)
			{
				/*	If we are doing partial triangulation, we must check
					to see whether the edge is still there in the polygon,
					since we might have done a portion of the polygon
					and saved the rest for later.
				*/
				if (y != (size-1))
				{
					if( ((id1 == *(temp2->pPolygon+y)) && (id2 ==*(temp2->pPolygon+y+1)))
						|| ((id2 == *(temp2->pPolygon+y)) && (id1 ==*(temp2->pPolygon+y+1))))
						/*	edge is still there we are ok */
						there = TRUE;
				}
				else
				{
					if( ((id1 == *(temp2->pPolygon)) && (id2 == *(temp2->pPolygon+size-1)))
					|| ((id2 == *(temp2->pPolygon)) && (id1 ==*(temp2->pPolygon+size-1))))
					/*	edge is still there we are ok */
						there = TRUE;
				}
			}
		}
		
		if (there )
			return 1;
		return 0;
	}
}

int Min_Adj(int id)
{
	/*	Used for the lookahead to break ties. It will
		return the minimum adjacency found at this face.
	*/
	int y,numverts,t,x=60;
	PF_FACES temp=NULL;
	ListHead *pListHead;

	/*	If polygon was used then we can't use this face */
	if (Done(id,59,&y) == NULL)
		return 60;
		
	/*	It was not used already */
	pListHead = PolFaces[id];
	temp = ( PF_FACES ) PeekList( pListHead, LISTHEAD, 0 );
     if ( temp != NULL )
	{
		numverts = temp->nPolSize;
		for (y = 0; y< numverts; y++)
		{
			if (y != (numverts-1))
				t = Number_Adj(*(temp->pPolygon+y),*(temp->pPolygon+y+1),id);
			else
				t = Number_Adj(*(temp->pPolygon),*(temp->pPolygon+(numverts-1)),id);
			if (t < x)
				x = t;
		}
	}
	if (x == -1)
	{
		printf("Error in the look\n");
		exit(0);
	}
	return x;
}



void Edge_Least(int *index,int *new1,int *new2,int face_id,int size)
{
    /*   We had a polygon without an input edge and now we re going to pick one
         of the edges with the least number of adjacencies to be the input
         edge
    */
    register int x,value,smallest=60;

    for (x = 0; x<size; x++)
    {
        if (x != (size -1) )
            value = Number_Adj(*(index+x),*(index+x+1),face_id);
        else 
            value = Number_Adj(*(index),*(index+size-1),face_id);
        if (value < smallest)
        {
            smallest = value;
            if (x != (size -1))
            {
                *new1 = *(index+x);
                *new2 = *(index+x+1);
            }
            else
            {
                *new1 = *(index);
                *new2 = *(index+size-1);
            }
        }
    }
    if ((smallest == 60) || (smallest < 0))
    {
        printf("There is an error in getting the least edge\n");
        exit(0);
    }
}


void Check_In_Polygon(int face_id, int *min, int size)
{
    /*  Check to see the adjacencies by going into a polygon that has
        greater than 4 sides.
    */
    
    ListHead *pListHead;
    PF_FACES temp;
    int y,id1,id2,id3,x=0,z=0;
    int saved[2];
    int big_saved[60];

    pListHead = PolFaces[face_id];
    temp = ( PF_FACES ) PeekList( pListHead, LISTHEAD, 0 );

    /*   Get the input edge that we came in on */
    Last_Edge(&id1,&id2,&id3,0);

    /*  Find the number of adjacencies to the edges that are adjacent
        to the input edge.
    */
    for (y=0; y< size; y++)
    {
        if (y != (size-1))
        {
            if (((*(temp->pPolygon+y) == id2) && (*(temp->pPolygon+y+1) != id3))
                || ((*(temp->pPolygon+y) == id3) && (*(temp->pPolygon+y+1) != id2)))
            {
                saved[x++] = Number_Adj(*(temp->pPolygon+y),*(temp->pPolygon+y+1),face_id);
                big_saved[z++] = saved[x-1];
            }
            else
                big_saved[z++] = Number_Adj(*(temp->pPolygon+y),*(temp->pPolygon+y+1),face_id);
        }
        else
        {
            if (((*(temp->pPolygon) == id2) && (*(temp->pPolygon+size-1) != id3))
                || ((*(temp->pPolygon) == id3) && (*(temp->pPolygon+size-1) != id2)))
            {
                saved[x++] = Number_Adj(*(temp->pPolygon),*(temp->pPolygon+size-1),face_id);
                big_saved[z++] = saved[x-1];
            }
            else
                big_saved[z++] = Number_Adj(*(temp->pPolygon),*(temp->pPolygon+size-1),face_id);
        }
    }
    /*  There was an input edge */
    if (x == 2)
    {
        if (saved[0] < saved[1])
            /*  Count the polygon that we will be cutting as another adjacency*/
            *min = saved[0] + 1;
        else
            *min = saved[1] + 1;
    }
    /*  There was not an input edge */
    else
    {
        if (z != size)
        {
            printf("There is an error with the z %d %d\n",size,z);
            exit(0);
        }
        *min = 60;
        for (x = 0; x < size; x++)
        {
            if (*min > big_saved[x])
                *min = big_saved[x];
        }
    }
}


void New_Face (int face_id, int v1, int v2, int v3)
{
	/*	We want to change the face that was face_id, we will
		change it to a triangle, since the rest of the polygon
		was already outputtted
	*/
	ListHead *pListHead;
	PF_FACES temp = NULL;

	pListHead = PolFaces[face_id];
     temp = ( PF_FACES ) PeekList( pListHead, LISTHEAD, 0);
	/*	Check each edge of the face and tally the number of adjacent
		polygons to this face. 
	*/	      		
	if ( temp != NULL )
	{
		/*	Size of the polygon */
		if (temp->nPolSize != 4)
		{
			printf("There is a miscalculation in the partial\n");
			exit (0);
		}
		temp->nPolSize = 3;
		*(temp->pPolygon) = v1;
		*(temp->pPolygon+1) = v2;
		*(temp->pPolygon+2) = v3;
	}
}

void New_Size_Face (int face_id)
{
	/*	We want to change the face that was face_id, we will
		change it to a triangle, since the rest of the polygon
		was already outputtted
	*/
	ListHead *pListHead;
	PF_FACES temp = NULL;

	pListHead = PolFaces[face_id];
	temp = ( PF_FACES ) PeekList( pListHead, LISTHEAD, 0 );
	/*	Check each edge of the face and tally the number of adjacent
		polygons to this face. 
	*/	      		
	if ( temp != NULL )
		(temp->nPolSize)--;
	else
		printf("There is an error in updating the size\n");
}



void  Check_In_Quad(int face_id,int *min)
{
     /*   Check to see what the adjacencies are for the polygons that
          are inside the quad, ie the 2 triangles that we can form.
     */
    ListHead *pListHead;
    int y,id1,id2,id3,x=0;
    int saved[4];
    PF_FACES temp;
    register int size = 4;

    pListHead = PolFaces[face_id];
    temp = ( PF_FACES ) PeekList( pListHead, LISTHEAD, 0 );
	
     /*   Get the input edge that we came in on */
    Last_Edge(&id1,&id2,&id3,0);
	
    /*    Now find the adjacencies for the inside triangles */
    for (y = 0; y< size; y++)
	{
         /*     Will not do this if the edge is the input edge */
         if (y != (size-1))
         {
              if ((((*(temp->pPolygon+y) == id2) && (*(temp->pPolygon+y+1) == id3))) ||
             (((*(temp->pPolygon+y) == id3) && (*(temp->pPolygon+y+1) == id2))))
                    saved[x++] = -1;
              else
              {
                   if (x == 4)
                   {
                        printf("There is an error in the check in quad \n");
                        exit(0);
                   }
                   /*    Save the number of Adjacent Polygons to this edge */
                   saved[x++] = Number_Adj(*(temp->pPolygon+y),*(temp->pPolygon+y+1),face_id);
              }
         }
		else if ((((*(temp->pPolygon) == id2) && (*(temp->pPolygon+size-1) == id3))) ||
             (((*(temp->pPolygon) == id3) && (*(temp->pPolygon+size-1) == id2))) )
             saved[x++] = -1;
        else
        {
               if (x == 4)
               {
                    printf("There is an error in the check in quad \n");
                    exit(0);
               }
               /*    Save the number of Adjacent Polygons to this edge */
               saved[x++] = Number_Adj(*(temp->pPolygon),*(temp->pPolygon+size-1),face_id);

        }
    }
    if (x != 4)
    {
         printf("Did not enter all the values %d \n",x);
         exit(0);
    }
    
    *min = 10;
    for (x=0; x<4; x++)
    {
         if (x!= 3)
         {
              if ((saved[x] != -1) && (saved[x+1] != -1) && 
                   ((saved[x] + saved[x+1]) < *min))
                   *min = saved[x] + saved[x+1];
         }
         else
         {
              if ((saved[0] != -1) && (saved[x] != -1) &&
                   ((saved[x] + saved[0]) < *min))
                   *min = saved[0] + saved[x];
         }
    }
}



int Get_Output_Edge(int face_id, int size, int *index,int id2,int id3)
{
    /*  Return the vertex adjacent to either input1 or input2 that
        is adjacent to the least number of polygons on the edge that
        is shared with either input1 or input2.
    */
    register int x=0,y;
    int saved[2];
    int edges[2][1];

    for (y = 0; y < size; y++)
    {
        if (y != (size-1))
        {
            if (((*(index+y) == id2) && (*(index+y+1) != id3))
                || ((*(index+y) == id3) && (*(index+y+1) != id2)))
            {
                saved[x++] = Number_Adj(*(index+y),*(index+y+1),face_id);
                edges[x-1][0] = *(index+y+1);
            }
            else if (y != 0)
            {
                if (( (*(index+y) == id2) && (*(index+y-1) != id3) ) ||
                    ( (*(index+y) == id3) && (*(index+y-1) != id2)) )
                {
                    saved[x++] = Number_Adj(*(index+y),*(index+y-1),face_id);
                    edges[x-1][0] = *(index+y-1);
                }
            }
            else if (y == 0)
            {
                if (( (*(index) == id2) && (*(index+size-1) != id3) ) ||
                    ( (*(index) == id3) && (*(index+size-1) != id2)) )
                {
                    saved[x++] = Number_Adj(*(index),*(index+size-1),face_id);
                    edges[x-1][0] = *(index+size-1);
                }
            }

        }
        else
        {
            if (((*(index+size-1) == id2) && (*(index) != id3))
                || ((*(index+size-1) == id3) && (*(index) != id2)))
            {
                saved[x++] = Number_Adj(*(index),*(index+size-1),face_id);
                edges[x-1][0] = *(index);
            }

            if (( (*(index+size-1) == id2) && (*(index+y-1) != id3) ) ||
                    ( (*(index+size-1) == id3) && (*(index+y-1) != id2)) )
                {
                    saved[x++] = Number_Adj(*(index+size-1),*(index+y-1),face_id);
                    edges[x-1][0] = *(index+y-1);
                }
        }
    }
    if ((x != 2))
    {
        printf("There is an error in getting the input edge %d \n",x);
        exit(0);
    }
    if (saved[0] < saved[1])
        return edges[0][0];
    else
        return edges[1][0];

}

void Get_Input_Edge(int *index,int id1,int id2,int id3,int *new1,int *new2,int size,
                    int face_id)
{
    /*  We had a polygon without an input edge and now we are going to pick one
        as the input edge. The last triangle was id1,id2,id3, we will try to
        get an edge to have something in common with one of those vertices, otherwise
        we will pick the edge with the least number of adjacencies.
    */

    register int x;
    int saved[3];

    saved[0] = -1;
    saved[1] = -1;
    saved[2] = -1;
    
    /*  Go through the edges to see if there is one in common with one
        of the vertices of the last triangle that we had, preferably id2 or
        id3 since those are the last 2 things in the stack of size 2.
    */
    for (x=0; x< size; x++)
    {
        if (*(index+x) == id1)
        {
            if (x != (size-1))
                saved[0] = *(index+x+1);
            else
                saved[0] = *(index);
        }

        if (*(index+x) == id2)
        {
            if (x != (size-1))
                saved[1] = *(index+x+1);
            else
                saved[1] = *(index);
        }
        
        if (*(index+x) == id3)
        {
            if (x != (size -1))
                saved[2] = *(index+x+1);
            else
                saved[2] = *(index);
        }
    }
    /*  Now see what we saved */
    if (saved[2] != -1)
    {
        *new1 = id3;
        *new2 = saved[2];
        return;
    }
    else if (saved[1] != -1)
    {
        *new1 = id2;
        *new2 = saved[1];
        return;
    }
    else if (saved[0] != -1)
    {
        *new1 = id1;
        *new2 = saved[0];
        return;
    }
    /*  We did not find anything so get the edge with the least number of adjacencies */
    Edge_Least(index,new1,new2,face_id,size);

}

int Find_Face(int current_face, int id1, int id2, int *bucket)
{
	/*	Find the face that is adjacent to the edge and is not the
		current face.
	*/
	register int size,each_poly=0,y,tally=0,count=0;
	PF_EDGES temp = NULL;
	PF_FACES temp2 = NULL;
	ListHead *pListHead;
	int next_face;
	BOOL there = FALSE;

    
    /*	Always want smaller id first */
	switch_lower(&id1,&id2);
	
	pListHead = PolEdges[id1];
	temp = (PF_EDGES) PeekList(pListHead,LISTHEAD,count);
     /*  The input edge was a new edge */
     if (temp == NULL)
        return -1;
        
     while (temp->edge[0] != id2)
     {
		count++;
		temp = (PF_EDGES) PeekList(pListHead,LISTHEAD,count);
          /*  The input edge was a new edge */
          if (temp == NULL)
            return -1;
     }
	/*	Was not adjacent to anything else except itself */
	if (temp->edge[2] == -1)
		return -1;
	else
	{
		if (temp->edge[2] == current_face)
			next_face =  temp->edge[1];
		else 
			next_face = temp->edge[2];
	}
	/*	We have the other face adjacent to this edge, it is 
		next_face. 
	*/
	pListHead = PolFaces[next_face];
	temp2 = ( PF_FACES ) PeekList( pListHead, LISTHEAD, 0 );
		
     /*	See if the face was already deleted, and where
              it is if it was not
     */
		
     if (Done(next_face,59,bucket) == NULL)
        return -1;

     /*  Make sure the edge is still in this polygon, and that it is not
         done
     */
		/*	Size of the polygon */
		size = temp2->nPolSize;
		for (y = 0; y< size; y++)
		{
			/*	Make sure that the edge is still in the
				polygon and was not deleted, because if the edge was
				deleted, then we used it already.
			*/
			if (y != (size-1))
			{
				if( ((id1 == *(temp2->pPolygon+y)) && (id2 ==*(temp2->pPolygon+y+1)))
					|| ((id2 == *(temp2->pPolygon+y)) && (id1 ==*(temp2->pPolygon+y+1))))
					/*	edge is still there we are ok */
					there = TRUE;
			}
			else
			{		
				if( ((id1 == *(temp2->pPolygon)) && (id2 ==*(temp2->pPolygon+size-1)))
					|| ((id2 == *(temp2->pPolygon)) && (id1 ==*(temp2->pPolygon+size-1))))
					/*	edge is still there we are ok */
					there = TRUE;
			}
		}
		
		if (!there)
			/*	Edge already used and deleted from the polygon*/
			return -1;
         else
            return next_face;
}

BOOL Look_Up(int id1,int id2,int face_id)
{
	/*	See if the endpoints of the edge specified by id1 and id2
		are adjacent to the face with face_id 
	*/
	register int count = 0;
	PF_EDGES temp  = NULL;
	ListHead *pListHead;
	PF_FACES temp2 = NULL;
	
	/*	Always want smaller id first */
	switch_lower(&id1,&id2);

	pListHead = PolEdges[id1];
	temp = (PF_EDGES) PeekList(pListHead,LISTHEAD,count);
     if (temp == NULL)
     /*	Was a new edge that we created */
		return 0;
	
	while (temp->edge[0] != id2)
     {
		count++;
		temp = (PF_EDGES) PeekList(pListHead,LISTHEAD,count);
          if (temp == NULL)
			/*	Was a new edge that we created */
			return 0;
     }
	/*	Was not adjacent to anything else except itself */
	if ((temp->edge[2] == face_id) || (temp->edge[1] == face_id))
	{
		/*	Edge was adjacent to face, make sure that edge is 
			still there
		*/
		if (Exist(face_id,id1,id2))
			return 1;
		else
			return 0;
	}
	else
		return 0;
}


void Add_Id_Strips(int id, int where)
{
     /*    Just save the triangle for later  */
     P_STRIPS pfNode;

	pfNode = (P_STRIPS) malloc(sizeof(Strips) );
	if ( pfNode )
	{
	     pfNode->face_id = id;
	     if (where == 1)
     		 AddTail(strips[0],(PLISTINFO) pfNode);
	     /* We are backtracking in the strip */
	     else
		 AddHead(strips[0],(PLISTINFO) pfNode);
	}
	else
	{
	     printf("There is not enough memory to allocate for the strips\n");
	     exit(0);
	}
}


int Num_Adj(int id1, int id2)
{
	/*   Given edge whose endpoints are specified by id1 and id2,
		determine how many polygons share this edge and return that
		number minus one (since we do not want to include the polygon
		that the caller has already).
	*/

	PF_EDGES temp = NULL;
	ListHead *pListHead;
	register count=-1;

	/*	Always want smaller id first */
	switch_lower(&id1,&id2);
	
	pListHead = PolEdges[id1];
	temp = (PF_EDGES) PeekList(pListHead,LISTHEAD,count);
     if (temp == NULL)
	{
		printf("There is an error in the creation of the table \n");
		exit(0);
	}
	while (temp->edge[0] != id2)
     {
		count++;
		temp = (PF_EDGES) PeekList(pListHead,LISTHEAD,count);
	     if (temp == NULL)
		{
			printf("There is an error in the creation of the table\n");
			exit(0);
		}
	}
	/*      Was not adjacent to anything else except itself */
	if (temp->edge[2] == -1)
		return 0;
	return 1;
}


void Add_Sgi_Adj(int bucket,int face_id)
{
	/*   This routine will add the face to the proper bucket,
		depending on how many faces are adjacent to it (what the
		value bucket should be).
	*/
	P_ADJACENCIES pfNode;

	pfNode = (P_ADJACENCIES) malloc(sizeof(ADJACENCIES) );
     if ( pfNode )
     {
		pfNode->face_id = face_id;
	     AddHead(array[bucket],(PLISTINFO) pfNode);
	}
	else
	{
		printf("Out of memory for the SGI adj list!\n");
		exit(0);
	}
}

void Find_Adjacencies(int num_faces)
{
     register int	x,y;
     register int	numverts;
     PF_FACES temp=NULL;
     ListHead *pListHead;

	/*   Fill in the adjacencies data structure for all the faces */
     for (x=0;x<num_faces;x++)
	{
        	pListHead = PolFaces[x];
		temp = ( PF_FACES ) PeekList( pListHead, LISTHEAD, 0 );
     	if ( temp != NULL )
		{
			numverts = temp->nPolSize;
			if (numverts != 1)
               {
                    for (y = 0; y< numverts; y++)
			     {
				     if (y != (numverts-1))
					     Add_AdjEdge(*(temp->pPolygon+y),*(temp->pPolygon+y+1),x,y);
			
				     else 
					     Add_AdjEdge(*(temp->pPolygon),*(temp->pPolygon+(numverts-1)),x,numverts-1);
			
                    }
               }
			temp = NULL;
		}
	}
}


