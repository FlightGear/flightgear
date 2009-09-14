/********************************************************************/
/*   STRIPE: converting a polygonal model to triangle strips    
     Francine Evans, 1996.
     SUNY @ Stony Brook
     Advisors: Steven Skiena and Amitabh Varshney
*/
/********************************************************************/

/*---------------------------------------------------------------------*/
/*   STRIPE: struct.c
     Contains routines that update structures, and micellaneous routines.
*/
/*---------------------------------------------------------------------*/

#include <stdlib.h>
#include <string.h>
#include "polverts.h"
#include "ties.h"
#include "output.h"
#include "triangulate.h"
#include "sturcts.h"
#include "options.h"
#include "common.h"
#include "util.h"

int out1 = -1;
int out2 = -1;

int Get_Edge(int *edge1,int *edge2,int *index,int face_id,
	        int size, int id1, int id2)
{
	/*	Put the edge that is adjacent to face_id into edge1
		and edge2. For each edge see if it is adjacent to
		face_id. Id1 and id2 is the input edge, so see if 
		the orientation is reversed, and save it in reversed.
	*/
	register int x;
	int reversed = -1;
	BOOL set = FALSE;
    
     for (x=0; x< size; x++)
	{
		if (x == (size-1))
		{
			if ((*(index) == id1) && (*(index+size-1)==id2))
			{
				if (set)
                         return 1;
				reversed = 1;
			}
			else if ((*(index) == id2) && (*(index+size-1)==id1))
			{
				if (set)
                         return 0;
				reversed = 0;
			}
				
			if (Look_Up(*(index),*(index+size-1),face_id))
			{
				if ( (out1 != -1) && ( (out1 == *(index)) || (out1 == *(index+size-1)) ) &&
                       ( (out2 == *(index)) || (out2 == *(index+size-1)) ))
                    {
                         set = TRUE;
                         *edge1 = *(index);
                         *edge2 = *(index+size-1);
                    }
				else if (out1 == -1)
                    {
                         set = TRUE;
                         *edge1 = *(index);
                         *edge2 = *(index+size-1);
                    }
				if ((reversed != -1) && (set))  
					return reversed;
			}
		}		
		else
		{
			if ((*(index+x) == id1) && (*(index+x+1)==id2))
			{
				if (set)
                         return 0;
				reversed = 0;
			}
			else if ((*(index+x) == id2) && (*(index+x+1)==id1))
			{
				if (set)
					return 1;
				reversed = 1;
			}

			if (Look_Up(*(index+x),*(index+x+1),face_id))
			{
				if ( (out1 != -1) && ( (out1 == *(index+x)) || (out1 == *(index+x+1)) ) &&
                         ((out2 == *(index+x)) || (out2 == *(index+x+1))))
                    {
                         set = TRUE;
                         *edge1 = *(index+x);
                         *edge2 = *(index+x+1);
                    }
				else if (out1 == -1)
                    {
                         set = TRUE;
                         *edge1 = *(index+x);
				     *edge2 = *(index+x + 1);
                    }
				if ((reversed != -1) && (set))
                         return reversed;
			}
		}
	}			
	if ((x == size) && (reversed != -1))
	{
		/*	Could not find the output edge */
		printf("Error in the Lookup %d %d %d %d %d %d %d %d\n",face_id,id1,id2,reversed,*edge1,*edge2,out1,out2);
		exit(0);
	}
	return reversed;
}


void Update_Face(int *next_bucket, int *min_face, int face_id, int *e1,
				 int *e2,int temp1,int temp2,int *ties)
{
	/*	We have a face id that needs to be decremented.
		We have to determine where it is in the structure,
		so that we can decrement it.
	*/
	/*	The number of adjacencies may have changed, so to locate
		it may be a little tricky. However we know that the number
		of adjacencies is less than or equal to the original number
		of adjacencies,
	*/
	int y,size;
	ListHead *pListHead;
	PF_FACES temp = NULL;
	PLISTINFO lpListInfo;
	static int each_poly = 0;
	BOOL there = FALSE;

	pListHead = PolFaces[face_id];
	temp = ( PF_FACES ) PeekList( pListHead, LISTHEAD, 0 );
	/*	Check each edge of the face and tally the number of adjacent
		polygons to this face. 
	*/	      		
	if ( temp != NULL )
	{
		/*	Size of the polygon */
        size = temp->nPolSize;
        /*  We did it already */
        if (size == 1)
            return;
        for (y = 0; y< size; y++)
	   {
			/*	If we are doing partial triangulation, we must check
				to see whether the edge is still there in the polygon,
				since we might have done a portion of the polygon
				and saved the rest for later.
			*/
            if (y != (size-1))
	  	  {
				if( ((temp1 == *(temp->pPolygon+y)) && (temp2 ==*(temp->pPolygon+y+1)))
					|| ((temp2 == *(temp->pPolygon+y)) && (temp1 ==*(temp->pPolygon+y+1))))
					/*	edge is still there we are ok */
					there = TRUE;
		  }
		  else
		  {
				if( ((temp1 == *(temp->pPolygon)) && (temp2 == *(temp->pPolygon+size-1)))
					|| ((temp2 == *(temp->pPolygon)) && (temp1 ==*(temp->pPolygon+size-1))))
					/*	edge is still there we are ok */
					there = TRUE;
		  }
	 }
		
      if (!there)
		/*	Original edge was already used, we cannot use this polygon */
			return;
		
	/*	We have a starting point to start our search to locate
		this polygon. 
	*/

	/*	Check to see if this polygon was done */
	lpListInfo = Done(face_id,59,&y);

	if (lpListInfo == NULL)
		return;

     /*  Was not done, but there is an error in the adjacency calculations */
     if (y == 0)
     {
            printf("There is an error in finding the adjacencies\n");
            exit(0);
     }

     /*	Now put the face in the proper bucket depending on tally. */
	/*	First add it to the new bucket, then remove it from the old */
	Add_Sgi_Adj(y-1,face_id);
	RemoveList(array[y],lpListInfo);
        
     /*	Save it if it was the smallest seen so far since then
		it will be the next face 
		Here we will have different options depending on
		what we want for resolving ties:
			1) First one we see we will use
			2) Random resolving
			3) Look ahead
			4) Alternating direction
	*/
	/*	At a new strip */
	if (*next_bucket == 60)
		*ties = *ties + each_poly;
	/*	Have a tie */
	if (*next_bucket == (y-1))
	{
		Add_Ties(face_id);
		each_poly++;
	}
	/*	At a new minimum */
	if (*next_bucket > (y-1))
	{
		*next_bucket = y-1;
		*min_face = face_id;
		*e1 = temp1;
		*e2 = temp2;
		each_poly = 0;
		Clear_Ties();
		Add_Ties(face_id);
	}
     }
}


void Delete_Adj(int id1, int id2,int *next_bucket,int *min_face, 
			 int current_face,int *e1,int *e2,int *ties)
{
	/*	Find the face that is adjacent to the edge and is not the
		current face. Delete one adjacency from it. Save the min
		adjacency seen so far.
	*/
	register int count=0;
	PF_EDGES temp = NULL;
	ListHead *pListHead;
	int next_face;

	/*	Always want smaller id first */
	switch_lower(&id1,&id2);
	
	pListHead = PolEdges[id1];
	temp = (PF_EDGES) PeekList(pListHead,LISTHEAD,count);
     if (temp == NULL)
	/*	It could be a new edge that we created. So we can
		exit, since there is not a face adjacent to it.
	*/
		return;
	while (temp->edge[0] != id2)
     {
		count++;
		temp = (PF_EDGES) PeekList(pListHead,LISTHEAD,count);
          if (temp == NULL)
			/*	Was a new edge that was created and therefore
				does not have anything adjacent to it
			*/
			return;
     }
	/*	Was not adjacent to anything else except itself */
	if (temp->edge[2] == -1)
		return;

	/*	Was adjacent to something */
	else
	{
		if (temp->edge[2] == current_face)
			next_face =  temp->edge[1];
		else 
			next_face = temp->edge[2];
	}
	/*	We have the other face adjacent to this edge, it is 
		next_face. Now we need to decrement this faces' adjacencies.
	*/
	Update_Face(next_bucket, min_face, next_face,e1,e2,id1,id2,ties);
}


int Change_Face(int face_id,int in1,int in2,
				 ListHead *pListHead, P_ADJACENCIES temp, BOOL no_check)
{
	/*	We are doing a partial triangulation and we need to
		put the new face of triangle into the correct bucket
	*/
	int input_adj,y;
	
	/*	Find the old number of adjacencies to this face,
		so we know where to delete it from
	*/
	y = Old_Adj(face_id);
	
	/*	Do we need to change the adjacency? Maybe the edge on the triangle
		that was outputted was not adjacent to anything. We know if we
		have to check by "check". We came out on the output edge
		that we needed, then we know that the adjacencies will decrease
		by exactly one.
	*/
	if (!no_check)
	{
		input_adj = Number_Adj(in1,in2,face_id);
		/*	If there weren't any then don't do anything */
		if (input_adj == 0)
			return y;
	}

	RemoveList(pListHead,(PLISTINFO)temp);
	/*	Before we had a quad with y adjacencies. The in edge
		did not have an adjacency, since it was just deleted,
		since we came in on it. The outedge must have an adjacency
		otherwise we would have a bucket 0, and would not be in this
		routine. Therefore the new adjacency must be y-1
	*/
    
    Add_Sgi_Adj(y-1,face_id);
    return (y-1);
}

int Update_Adjacencies(int face_id, int *next_bucket, int *e1, int *e2,
					   int *ties)
{
	/*	Give the face with id face_id, we want to decrement
		all the faces that are adjacent to it, since we will
		be deleting face_id from the data structure.
		We will return the face that has the least number
		of adjacencies.
	*/
	PF_FACES temp = NULL;
	ListHead *pListHead;
	int size,y,min_face = -1;
	
     *next_bucket = 60;
	pListHead = PolFaces[face_id];
	temp = ( PF_FACES ) PeekList( pListHead, LISTHEAD, 0 );
	
	if ( temp == NULL )
	{
		printf("The face was already deleted, there is an error\n");
		exit(0);
	}
	
	/*	Size of the polygon */
	size = temp->nPolSize;
	for (y = 0; y< size; y++)
	{
		if (y != (size-1))
			Delete_Adj(*(temp->pPolygon+y),*(temp->pPolygon+y+1),
				next_bucket,&min_face,face_id,e1,e2,ties);
		else
			Delete_Adj(*(temp->pPolygon),*(temp->pPolygon+(size-1)),
				next_bucket,&min_face,face_id,e1,e2,ties);
	}
	return (min_face);
}


void Find_Adj_Tally(int id1, int id2,int *next_bucket,int *min_face, 
				int current_face,int *ties)
{
	/*	Find the face that is adjacent to the edge and is not the
		current face. Save the min adjacency seen so far.
	*/
	int size,each_poly=0,y,count=0;
	PF_EDGES temp = NULL;
	PF_FACES temp2 = NULL;
	ListHead *pListHead;
	int next_face;
	BOOL there = FALSE;

    
    /*	Always want smaller id first */
	switch_lower(&id1,&id2);
	
	pListHead = PolEdges[id1];
	temp = (PF_EDGES) PeekList(pListHead,LISTHEAD,count);
     if (temp == NULL)
     /*	This was a new edge that was created, so it is
		adjacent to nothing.
	*/
		return;

	while (temp->edge[0] != id2)
     {
		count++;
		temp = (PF_EDGES) PeekList(pListHead,LISTHEAD,count);
          if (temp == NULL)
			/*	This was a new edge that we created */
			return;
     }
	/*	Was not adjacent to anything else except itself */
	if (temp->edge[2] == -1)
		return;
	else
	{
		if (temp->edge[2] == current_face)
			next_face =  temp->edge[1];
		else 
			next_face = temp->edge[2];
	}
	/*	We have the other face adjacent to this edge, it is 
		next_face. Find how many faces it is adjacent to.
	*/
	pListHead = PolFaces[next_face];
	temp2 = ( PF_FACES ) PeekList( pListHead, LISTHEAD, 0 );
	/*	Check each edge of the face and tally the number of adjacent 
		polygons to this face. This will be the original number of
		polygons adjacent to this polygon, we must then see if this
		number has been decremented
	*/	      		
	if ( temp2 != NULL )
	{
		/*	Size of the polygon */
		size = temp2->nPolSize;
		/*  We did it already */
          if (size == 1)
            return;
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
			return;
		
		/*	See if the face was already deleted, and where
			it is if it was not
		*/
		if (Done(next_face,size,&y) == NULL)
			return;
		
		/*	Save it if it was the smallest seen so far since then
			it will be the next face 
			Here we will have different options depending on
			what we want for resolving ties:
			1) First one we see we will use
			2) Random resolving
			3) Look ahead
			4) Alternating direction
		*/
		
		/*	At a new strip */
		if (*next_bucket == 60)
			*ties = *ties + each_poly;
		/*	Have a tie */
		if (*next_bucket == (y-1))
		{
			Add_Ties(next_face);
			each_poly++;
		}
		/*	At a new minimum */
		if (*next_bucket > (y-1))
		{
			*next_bucket = y-1;
			*min_face = next_face;
			each_poly = 0;
			Clear_Ties();
			Add_Ties(next_face);
		}
	}
}


int Min_Face_Adj(int face_id, int *next_bucket, int *ties)
{
	/*	Used for the Partial triangulation to find the next
		face. It will return the minimum adjacency face id
		found at this face.
	*/
	PF_FACES temp = NULL;
	ListHead *pListHead;
	int size,y,min_face,test_face;
	
	*next_bucket = 60;
	pListHead = PolFaces[face_id];
	temp = ( PF_FACES ) PeekList( pListHead, LISTHEAD, 0 );
	
	if ( temp == NULL )
	{
		printf("The face was already deleted, there is an error\n");
		exit(0);
	}
	
	/*	Size of the polygon */
	size = temp->nPolSize;
	for (y = 0; y< size; y++)
	{
		if (y != (size-1))
			Find_Adj_Tally(*(temp->pPolygon+y),*(temp->pPolygon+y+1),
				next_bucket,&min_face,face_id,ties);
		else
			Find_Adj_Tally(*(temp->pPolygon),*(temp->pPolygon+(size-1)),
				next_bucket,&min_face,face_id,ties);
	}
	/*    Maybe we can do better by triangulating the face, because
          by triangulating the face we will go to a polygon of lesser
          adjacencies
    */
    if (size == 4)
    {
         /*    Checking for a quad whether to do the whole polygon will
               result in better performance because the triangles in the polygon
               have less adjacencies
         */
         Check_In_Quad(face_id,&test_face);
         if (*next_bucket > test_face)
              /*    We can do better by going through the polygon */
              min_face = face_id;
    }

    /*  We have a polygon with greater than 4 sides, check to see if going
        inside is better than going outside the polygon for the output edge.
    */
    else
    {
        Check_In_Polygon(face_id,&test_face,size);
        if (*next_bucket > test_face)
            /*  We can do better by going through the polygon */
            min_face = face_id;
    }
    
    return (min_face);
}



