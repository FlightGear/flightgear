/********************************************************************/
/*   STRIPE: converting a polygonal model to triangle strips    
     Francine Evans, 1996.
     SUNY @ Stony Brook
     Advisors: Steven Skiena and Amitabh Varshney
*/
/********************************************************************/

/*---------------------------------------------------------------------*/
/*   STRIPE: partial.c
     This file contains routines that are used partial triangulation of polygons
*/
/*---------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "global.h"
#include "outputex.h"
#include "polyvertsex.h"
#include "triangulatex.h"
#include "sturctsex.h"
#include "polverts.h"
#include "common.h"
#include "util.h"

void P_Triangulate_Quad(int out_edge1,int out_edge2,int in_edge1,
					  int in_edge2,int size,int *index,
					  FILE *output,FILE *fp,int reversed,int face_id,
					  int *next_id,ListHead *pListHead, 
					  P_ADJACENCIES temp,
                      int where)
{
    int vertex4,vertex5,dummy=60;
	
    /*	This routine will nonblindly triangulate a quad, meaning
		that there is a definite input and a definite output
		edge that we must adhere to. Reversed will tell the orientation
		of the input edge. (Reversed is -1 is we do not have an input
		edge, in other words we are at the beginning of a strip.)
		Out_edge* is the output edge, and in_edge* is the input edge. 
		Index are the edges of the polygon
		and size is the size of the polygon. Begin is whether we are
		at the start of a new strip.
		Note that we will not necessarily triangulate the whole quad;
		maybe we will do half and leave the other half (a triangle)
		for later.
	*/
	

    /*	If we do not have an input edge, then we can make our input
		edge whatever we like, therefore it will be easier to come
		out on the output edge.	In this case the whole quad is done.
	*/
	if (reversed == -1)
	{
		vertex4 = AdjacentEx(out_edge1,out_edge2,index,size);
		vertex5 = Get_Other_Vertex(vertex4,out_edge1,out_edge2,index);
		Output_TriEx(vertex5,vertex4,out_edge1,output,-1,-1,where);
		Output_TriEx(vertex4,out_edge1,out_edge2,output,-1,-1,where);
		dummy = Update_AdjacenciesEx(face_id, &dummy, &dummy,&dummy,&dummy);
		RemoveList(pListHead,(PLISTINFO) temp);
		return;
	}
	
	/*	These are the 5 cases that we can have for the output edge */
	
	/*  Are they consecutive so that we form a triangle to
		peel off, but cannot use the whole quad?
	*/

	if (in_edge2 == out_edge1) 
	{
		/*	Output the triangle that comes out the correct
			edge. Save the other half for later.
		*/
		vertex4 = Get_Other_Vertex(in_edge1,in_edge2,out_edge2,index);
          Output_TriEx(in_edge1,in_edge2,out_edge2,output,-1,-1,where);
		/*	Now we have a triangle used, and a triangle that is
			left for later.
		*/
		
		/*	Now delete the adjacencies by one for all the faces
			that are adjacent to the triangle that we just outputted.
		*/
		Delete_AdjEx(in_edge1,in_edge2,&dummy,&dummy,
				face_id,&dummy,&dummy,&dummy);
		Delete_AdjEx(out_edge2,in_edge2,&dummy,&dummy, 
				face_id,&dummy,&dummy,&dummy);
		/*	Put the new face in the proper bucket of adjacencies 
			There are 2 edges that need to be checked for the triangle
			that was just outputted. For the output edge we definitely
			will be decreasing the adjacency, but we must check for the
			input edge.
		*/

		dummy = Change_FaceEx(face_id,in_edge1,in_edge2,pListHead,temp,FALSE);
		dummy = Change_FaceEx(face_id,in_edge2,out_edge2,pListHead,temp,TRUE);
		
		/*	Update the face data structure, by deleting the old
			face and putting in the triangle as the new face 
		*/
		New_Face(face_id,in_edge1,out_edge2,vertex4);
		return;									  
	}
	else if (in_edge1 == out_edge1)
	{
		/*	We want to output the first triangle (whose output
			edge is not the one that we want.
			We have to find the vertex that we need, which is
			the other vertex which we do not have.
		*/						
		vertex4 = Get_Other_Vertex(in_edge1,in_edge2,out_edge2,index);
		Output_TriEx(in_edge2,in_edge1,out_edge2,output,-1,-1,where);
		/*	Now we have a triangle used, and a triangle that is
			left for later.
		*/
		
		/*	Now delete the adjacencies by one for all the faces
			that are adjacent to the triangle that we just outputted.
		*/
		Delete_AdjEx(in_edge1,in_edge2,&dummy,&dummy,face_id,
				&dummy,&dummy,&dummy);
		Delete_AdjEx(out_edge2,out_edge1,&dummy,&dummy, 
				face_id,&dummy,&dummy,&dummy);
		
		/*	Put the new face in the proper bucket of adjacencies */
		dummy = Change_FaceEx(face_id,in_edge1,in_edge2,pListHead,temp,FALSE);
		dummy = Change_FaceEx(face_id,in_edge1,out_edge2,pListHead,temp,TRUE);
		
		/*	Update the face data structure, by deleting the old
			face and putting in the triangle as the new face 
		*/
		New_Face(face_id,in_edge2,out_edge2,vertex4);
		return;
	}
	
	/*	Consecutive cases again, but with the output edge reversed */
	else if (in_edge1 == out_edge2)
	{
		vertex4 = Get_Other_Vertex(in_edge1,in_edge2,out_edge1,index);
		Output_TriEx(in_edge2,in_edge1,out_edge1,output,-1,-1,where);
		/*	Now we have a triangle used, and a triangle that is
			left for later.
		*/
		
		/*	Now delete the adjacencies by one for all the faces
			that are adjacent to the triangle that we just outputted.
		*/
		Delete_AdjEx(in_edge1,in_edge2,&dummy,&dummy,face_id,
				&dummy,&dummy,&dummy);
		Delete_AdjEx(out_edge2,out_edge1,&dummy,&dummy, 
				face_id,&dummy,&dummy,&dummy);
		
		/*	Put the new face in the proper bucket of adjacencies */
		dummy = Change_FaceEx(face_id,in_edge1,in_edge2,pListHead,temp,FALSE);
		dummy = Change_FaceEx(face_id,out_edge1,out_edge2,pListHead,temp,TRUE);
		
		/*	Update the face data structure, by deleting the old
			face and putting in the triangle as the new face 
		*/
		New_Face(face_id,in_edge2,out_edge1,vertex4);
          return;
	}
	else if (in_edge2 == out_edge2)
	{
		vertex4 = Get_Other_Vertex(in_edge1,in_edge2,out_edge1,index);
		Output_TriEx(in_edge1,in_edge2,out_edge1,output,-1,-1,where);
		/*	Now we have a triangle used, and a triangle that is
			left for later.
		*/
		/*	Now delete the adjacencies by one for all the faces
			that are adjacent to the triangle that we just outputted.
		*/
		Delete_AdjEx(in_edge1,in_edge2,&dummy,&dummy,face_id,
				&dummy,&dummy,&dummy);
		Delete_AdjEx(out_edge2,out_edge1,&dummy,&dummy, 
				face_id,&dummy,&dummy,&dummy);
		
		/*	Put the new face in the proper bucket of adjacencies */
		dummy = Change_FaceEx(face_id,in_edge1,in_edge2,pListHead,temp,FALSE);
		dummy = Change_FaceEx(face_id,out_edge1,out_edge2,pListHead,temp,TRUE);
		
		/*	Update the face data structure, by deleting the old
			face and putting in the triangle as the new face 
		*/
		New_Face(face_id,in_edge1,out_edge1,vertex4);
		return;
	}

	/*	The final case is where we want to come out the opposite
		edge.
	*/
	else
	{
        if( ((!reversed) && (out_edge1 == (AdjacentEx(in_edge1,in_edge2,index,size)))) ||
                    ((reversed) && (out_edge2 == (AdjacentEx(in_edge2,in_edge1,index,size)))))
		{
			/*	We need to know the orientation of the input
				edge, so we know which way to put the diagonal.
                And also the output edge, so that we triangulate
                correctly. Does not need partial.
             */
			Output_TriEx(in_edge1,in_edge2,out_edge2,output,-1,-1,where);
			Output_TriEx(in_edge2,out_edge2,out_edge1,output,-1,-1,where);
			dummy = Update_AdjacenciesEx(face_id, &dummy, &dummy,&dummy,&dummy);
			RemoveList(pListHead,(PLISTINFO) temp);
		}
		else
		{
			/*      Input and output orientation was reversed, so diagonal will
					be reversed from above.
			*/
			Output_TriEx(in_edge1,in_edge2,out_edge1,output,-1,-1,where);
			Output_TriEx(in_edge2,out_edge1,out_edge2,output,-1,-1,where);
			dummy = Update_AdjacenciesEx(face_id, &dummy, &dummy,&dummy,&dummy);
			RemoveList(pListHead,(PLISTINFO) temp);
		}
		return;
	}
}

void P_Triangulate_Polygon(int out_edge1,int out_edge2,int in_edge1,
						   int in_edge2,int size,
						  int *index,FILE *output,FILE *fp,
						  int reversed,int face_id,int *next_id,
						  ListHead *pListHead, P_ADJACENCIES temp2,
                          int where)
{
	/*	We have a polygon greater than 4 sides, which we wish
		to partially triangulate
	*/
	int next_bucket,vertex4,dummy = 60;
	int *temp;
	P_ADJACENCIES pfNode;

		
    /*	Since we are calling this recursively, we have to check whether		
		we are down to the case of the quad.
	*/
	if (size == 4)
	{
		P_Triangulate_Quad(out_edge1,out_edge2,in_edge1,in_edge2,size,
							index,output,fp,reversed,face_id,next_id,
							pListHead,temp2,where);
		return;
	}
	
	/*	We do not have a specified input edge, and therefore we
		can make it anything we like, as long as we still come out 
		the output edge that we want.
	*/
	if (reversed  == -1)
	{
		/*	Get the vertex for the last triangle, which is
			the one coming out the output edge, before we do
			any deletions to the list. We will be doing this
			bottom up.
		*/
		vertex4 = AdjacentEx(out_edge1,out_edge2,index,size);
		temp = (int *) malloc(sizeof(int) * size);
		memcpy(temp,index,sizeof(int)*size);
		Delete_From_ListEx(out_edge2,index,size);
		/*	We do not have to partially triangulate, since
			we will do the whole thing, so use the whole routine
		*/
		/* Triangulate_PolygonEx(vertex4,out_edge1,in_edge2,
				      vertex4,size-1,index,output,fp,reversed,
				      face_id,next_id,pListHead,temp2,where); */
		Triangulate_PolygonEx(vertex4,out_edge1,in_edge2,
				      vertex4,size-1,index,output,fp,reversed,
				      face_id,where);
		memcpy(index,temp,sizeof(int)*size);
		/*	Lastly do the triangle that comes out the output
			edge.
		*/
		Output_TriEx(vertex4,out_edge1,out_edge2,output,-1,-1,where);
		/*	We were able to do the whole polygon, now we
			can delete the whole thing from our data structure.
		*/
		dummy = Update_AdjacenciesEx(face_id, &dummy, &dummy,&dummy,&dummy);
		RemoveList(pListHead,(PLISTINFO) temp2);
		return;
	}	  

	/*	These are the 5 cases that we can have for the output edge */
	
	/*  Are they consecutive so that we form a triangle to
		peel off that comes out the correct output edge, 
		but we cannot use the whole polygon?
	*/
	if (in_edge2 == out_edge1) 
	{
		Output_TriEx(in_edge1,out_edge1,out_edge2,output,-1,-1,where);
		
		/*	Now delete the adjacencies by one for all the faces
			that are adjacent to the triangle that we just outputted.
		*/
		Delete_AdjEx(in_edge1,in_edge2,&dummy,&dummy,face_id,
				&dummy,&dummy,&dummy);
		Delete_AdjEx(out_edge2,out_edge1,&dummy,&dummy, 
				face_id,&dummy,&dummy,&dummy);
		
		/*	Put the new face in the proper bucket of adjacencies */
		next_bucket = Change_FaceEx(face_id,in_edge1,in_edge2,pListHead,temp2,FALSE);
		next_bucket = Change_FaceEx(face_id,out_edge1,out_edge2,pListHead,temp2,TRUE);
		
		/*	Create a new edgelist without the triangle that
			was just outputted.
		*/
          Delete_From_ListEx(in_edge2,index,size);
		/*	Update the face data structure, by deleting the old
			face and putting in the polygon minus the triangle 
			as the new face, here we will be decrementing the size
			by one.
		*/
		New_Size_Face(face_id);
		return;
	}

	/*	Next case is where it is again consecutive, but the triangle
		formed by the consecutive edges do not come out of the
		correct output edge. (the input edge will be reversed in
		the next triangle)
	*/
	else if (in_edge1 == out_edge1)
	{
		/*	Get vertex adjacent to in_edge2, but is not in_edge1 */
		Output_TriEx(in_edge2,in_edge1,out_edge2,output,-1,-1,where);
		
		/*	Now delete the adjacencies by one for all the faces
			that are adjacent to the triangle that we just outputted.
		*/
		Delete_AdjEx(in_edge1,in_edge2,&dummy,&dummy,face_id,
				&dummy,&dummy,&dummy);
		Delete_AdjEx(out_edge2,out_edge1,&dummy,&dummy, 
				face_id,&dummy,&dummy,&dummy);
		
		/*	Put the new face in the proper bucket of adjacencies */
		next_bucket = Change_FaceEx(face_id,in_edge1,in_edge2,pListHead,temp2,FALSE);
		next_bucket = Change_FaceEx(face_id,out_edge1,out_edge2,pListHead,temp2,TRUE);
		
		/*	Create a new edgelist without the triangle that
			was just outputted.
		*/
          Delete_From_ListEx(in_edge1,index,size);
		/*	Update the face data structure, by deleting the old
			face and putting in the polygon minus the triangle 
			as the new face, here we will be decrementing the size
			by one.
		*/
		New_Size_Face(face_id);
		return;
	}
	
	/*	Consecutive cases again, but with the output edge reversed */
	else if (in_edge1 == out_edge2)
	{
		Output_TriEx(in_edge2,in_edge1,out_edge1,output,-1,-1,where);
		
		/*	Now delete the adjacencies by one for all the faces
			that are adjacent to the triangle that we just outputted.
		*/
		Delete_AdjEx(in_edge1,in_edge2,&dummy,&dummy,face_id,
				&dummy,&dummy,&dummy);
		Delete_AdjEx(out_edge1,out_edge2,&dummy,&dummy, 
				face_id,&dummy,&dummy,&dummy);
		
		/*	Put the new face in the proper bucket of adjacencies */
		next_bucket = Change_FaceEx(face_id,in_edge1,in_edge2,pListHead,temp2,FALSE);
		next_bucket = Change_FaceEx(face_id,out_edge1,out_edge2,pListHead,temp2,TRUE);
		
		/*	Create a new edgelist without the triangle that
			was just outputted.
		*/
        Delete_From_ListEx(in_edge1,index,size);
		/*	Update the face data structure, by deleting the old
			face and putting in the polygon minus the triangle 
			as the new face, here we will be decrementing the size
			by one.
		*/
		New_Size_Face(face_id);
		return;
	}
	else if (in_edge2 == out_edge2)
	{
		Output_TriEx(in_edge1,in_edge2,out_edge1,output,-1,-1,where);
		
		/*	Now delete the adjacencies by one for all the faces
			that are adjacent to the triangle that we just outputted.
		*/
		Delete_AdjEx(in_edge1,in_edge2,&dummy,&dummy,face_id,
				&dummy,&dummy,&dummy);
		Delete_AdjEx(out_edge2,out_edge1,&dummy,&dummy, 
				face_id,&dummy,&dummy,&dummy);
		
		/*	Put the new face in the proper bucket of adjacencies */
		next_bucket = Change_FaceEx(face_id,in_edge1,in_edge2,pListHead,temp2,FALSE);
		next_bucket = Change_FaceEx(face_id,out_edge1,out_edge2,pListHead,temp2,TRUE);
		
		/*	Create a new edgelist without the triangle that
			was just outputted.
		*/
          Delete_From_ListEx(in_edge2,index,size);
		/*	Update the face data structure, by deleting the old
			face and putting in the polygon minus the triangle 
			as the new face, here we will be decrementing the size
			by one.
		*/
		New_Size_Face(face_id);
		return;
	}

	/*	Else the edge is not consecutive, and it is sufficiently
		far away, for us not to make a conclusion at this time.
		So we can take off a triangle and recursively call this
		function.
	*/
	else
	{
          if (!reversed)
		{
			vertex4 = AdjacentEx(in_edge2,in_edge1,index,size);
			Output_TriEx(in_edge1,in_edge2,vertex4,output,-1,-1,where);
			
			/*	Now delete the adjacencies by one for all the faces
				that are adjacent to the triangle that we just outputted.
			*/
			Delete_AdjEx(in_edge1,in_edge2,&dummy,&dummy,face_id,
				&dummy,&dummy,&dummy);
			Delete_AdjEx(in_edge1,vertex4,&dummy,&dummy, 
				face_id,&dummy,&dummy,&dummy);
			
			/*	Put the new face in the proper bucket of adjacencies */
			next_bucket = Change_FaceEx(face_id,in_edge1,in_edge2,pListHead,temp2,FALSE);
			next_bucket = Change_FaceEx(face_id,in_edge1,vertex4,pListHead,temp2,FALSE);
			
			/*	Create a new edgelist without the triangle that
				was just outputted.
			*/
			Delete_From_ListEx(in_edge1,index,size);
			/*	Update the face data structure, by deleting the old
				face and putting in the polygon minus the triangle 
				as the new face, here we will be decrementing the size
				by one.
			*/
			New_Size_Face(face_id);

			/*	Save the info for the new bucket, we will need it on
				the next pass for the variables, pListHead and temp 
			*/
			pListHead = array[next_bucket];
			pfNode = (P_ADJACENCIES) malloc(sizeof(ADJACENCIES) );
			if ( pfNode )
				pfNode->face_id = face_id;
			temp2 = (P_ADJACENCIES) (SearchList(array[next_bucket], pfNode,
				(int (*)(void *,void *)) (Compare)));
			if (temp2 == NULL)
			{
				printf("There is an error finding the next polygon10 %d %d\n",next_bucket,face_id);
				exit(0);
			}

			P_Triangulate_Polygon(out_edge1,out_edge2,in_edge2,
						 vertex4,size-1,index,output,fp,!reversed,
						 face_id,next_id,pListHead,temp2,where);
		}
		else
		{
			vertex4 = AdjacentEx(in_edge1,in_edge2,index,size);
			Output_TriEx(in_edge2,in_edge1,vertex4,output,-1,-1,where);

			/*	Now delete the adjacencies by one for all the faces
				that are adjacent to the triangle that we just outputted.
			*/
			Delete_AdjEx(in_edge1,in_edge2,&dummy,&dummy,face_id,
				&dummy,&dummy,&dummy);
			Delete_AdjEx(in_edge2,vertex4,&dummy,&dummy, 
				face_id,&dummy,&dummy,&dummy);
			
			/*	Put the new face in the proper bucket of adjacencies */
			next_bucket = Change_FaceEx(face_id,in_edge1,in_edge2,pListHead,temp2,FALSE);
			next_bucket = Change_FaceEx(face_id,in_edge2,vertex4,pListHead,temp2,FALSE);
			
			/*	Create a new edgelist without the triangle that
				was just outputted.
			*/
			Delete_From_ListEx(in_edge2,index,size);
			
			/*	Update the face data structure, by deleting the old
				face and putting in the polygon minus the triangle 
				as the new face, here we will be decrementing the size
				by one.
			*/
			New_Size_Face(face_id);
			
			/*	Save the info for the new bucket, we will need it on
				the next pass for the variables, pListHead and temp 
			*/
			pListHead = array[next_bucket];
			pfNode = (P_ADJACENCIES) malloc(sizeof(ADJACENCIES) );
			if ( pfNode )
				pfNode->face_id = face_id;
			temp2 = (P_ADJACENCIES) (SearchList(array[next_bucket], pfNode,
				(int (*)(void *,void *)) (Compare)));
			if (temp2 == NULL)
			{
				printf("There is an error finding the next polygon11 %d %d\n",face_id,next_bucket);
				exit(0);
			}

			P_Triangulate_Polygon(out_edge1,out_edge2,vertex4,
						       in_edge1,size-1,index,output,fp,!reversed,
						       face_id,next_id,pListHead,temp2,where);
		}
		return;
	}
}

void P_Triangulate(int out_edge1,int out_edge2,int in_edge1,
				 int in_edge2,int size,int *index,
				 FILE *fp,FILE *output,int reversed,int face_id,
				 int *next_id,ListHead *pListHead, 
				 P_ADJACENCIES temp,int where)
{
		
	if (size == 4)
		P_Triangulate_Quad(out_edge1,out_edge2,in_edge1,in_edge2,size,
			              index,fp,output,reversed,face_id,next_id,pListHead, temp,where);
	else
		P_Triangulate_Polygon(out_edge1,out_edge2,in_edge1,in_edge2,size,
			                 index,fp,output,reversed,face_id,next_id,pListHead,temp,where);
}

 void Partial_Triangulate(int size,int *index, FILE *fp,
						 FILE *output,int next_face_id,int face_id,
						 int *next_id,ListHead *pListHead,
						 P_ADJACENCIES temp, int where)
{
	int id1,id2,id3;
	int nedge1,nedge2;
	int reversed;

	/*	We have a polygon that has to be triangulated and we cannot
		do it blindly, ie we will try to come out on the edge that
		has the least number of adjacencies, But also we do not
		want to triangulate the whole polygon now, so that means 
		we will output the least number of triangles that we can
		and then update the data structures, with the polygon
		that is left after we are done.
	*/
	Last_Edge(&id1,&id2,&id3,0);
	
	/*	Find the edge that is adjacent to the new face ,
		also return whether the orientation is reversed in the
		face of the input edge, which is id2 and id3.
	*/
	reversed = Get_EdgeEx(&nedge1,&nedge2,index,next_face_id,size,id2,id3);
	
	/*   Input edge and output edge can be the same if there are more than
          one polygon on an edge 
     */
     if ( ((nedge1 == id2) && (nedge2 == id3)) ||
          ((nedge1 == id3) && (nedge2 == id2)) )
          /*   Set output edge arbitrarily but when come out of here the
               next face will be on the old output edge (identical one)
          */
          nedge2 = Return_Other(index,id2,id3);

          /*	Do the triangulation */	
	P_Triangulate(nedge1,nedge2,id2,id3,size,index,fp,output,reversed,
		         face_id,next_id,pListHead,temp,where);
}

 void Input_Edge(int face_id, int *index, int size, int in_edge1, int in_edge2, 
                FILE *fp, FILE *output,ListHead *pListHead, P_ADJACENCIES temp2,
                int where)
 {
     /* The polygon had an input edge, specified by input1 and input2 */
    
     int output1;
     int vertex4, vertex5,dummy=60;

     output1 = Get_Output_Edge(face_id,size,index,in_edge1,in_edge2);
	vertex5 = AdjacentEx(in_edge2,in_edge1,index,size); 
     vertex4 = AdjacentEx(in_edge1,in_edge2,index,size);

     if (vertex4 == output1)
     {
		Output_TriEx(in_edge2,in_edge1,output1,output,-1,-1,where);
		/*	Now delete the adjacencies by one for all the faces
			that are adjacent to the triangle that we just outputted.
		*/
		Delete_AdjEx(in_edge1,in_edge2,&dummy,&dummy,face_id,
				&dummy,&dummy,&dummy);
		Delete_AdjEx(in_edge2,output1,&dummy,&dummy, 
				face_id,&dummy,&dummy,&dummy);
		/*	Put the new face in the proper bucket of adjacencies */
		Change_FaceEx(face_id,in_edge1,in_edge2,pListHead,temp2,FALSE);
		Change_FaceEx(face_id,in_edge2,output1,pListHead,temp2,FALSE);
		
		/*	Create a new edgelist without the triangle that
			was just outputted.
		*/
          Delete_From_ListEx(in_edge2,index,size);

    }	
    else if (vertex5 == output1)
    {
          Output_TriEx(in_edge1,in_edge2,vertex5,output,-1,-1,where);
		/*	Now delete the adjacencies by one for all the faces
			that are adjacent to the triangle that we just outputted.
		*/
		Delete_AdjEx(in_edge1,in_edge2,&dummy,&dummy,face_id,
				&dummy,&dummy,&dummy);
		Delete_AdjEx(in_edge1,vertex5,&dummy,&dummy, 
				face_id,&dummy,&dummy,&dummy);
		/*	Put the new face in the proper bucket of adjacencies */
		Change_FaceEx(face_id,in_edge1,in_edge2,pListHead,temp2,FALSE);
		Change_FaceEx(face_id,in_edge1,vertex5,pListHead,temp2,FALSE);
		
		/*	Create a new edgelist without the triangle that
			was just outputted.
		*/
          Delete_From_ListEx(in_edge1,index,size);
    }
		
    /*	Update the face data structure, by deleting the old
		face and putting in the polygon minus the triangle 
		as the new face, here we will be decrementing the size
		by one.
    */
    New_Size_Face(face_id);
    return;
 }
 
 void Inside_Polygon(int size,int *index,FILE *fp,FILE *output,
                   int next_face_id,int face_id,int *next_id,
                   ListHead *pListHead,P_ADJACENCIES temp, int where)
 {
     /* We know that we have a polygon that is greater than 4 sides, and
        that it is better for us to go inside the polygon for the next
        one, since inside will have less adjacencies than going outside.
        So, we are not doing partial for a part of the polygon.
      */
    int id1,id2,id3;
    int new1,new2;

    Last_Edge(&id1,&id2,&id3,0);

    /*  See if the input edge existed in the polygon, that will help us */
	if (Exist(face_id,id2,id3))
        Input_Edge(face_id,index,size,id2,id3,output,fp,pListHead,temp,where);
    else
    {
        /*  Make one of the input edges 
            We will choose it by trying to get an edge that has something
            in common with the last triangle, or by getting the edge that
            is adjacent to the least number of thigs, with preference given
            to the first option
        */
               
        Get_Input_Edge(index,id1,id2,id3,&new1,&new2,size,face_id);
        Input_Edge(face_id,index,size,new1,new2,output,fp,pListHead,temp,where);
    }
 }


