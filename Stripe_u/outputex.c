/********************************************************************/
/*   STRIPE: converting a polygonal model to triangle strips    
     Francine Evans, 1996.
     SUNY @ Stony Brook
     Advisors: Steven Skiena and Amitabh Varshney
*/
/********************************************************************/

/*---------------------------------------------------------------------*/
/*   STRIPE: outputex.c
     This file contains routines that are used for various functions in
     the local algorithm.
*/
/*---------------------------------------------------------------------*/


#include <stdio.h>
#include <stdlib.h>
#include "global.h"
#include "outputex.h"
#include "triangulatex.h"
#include "polverts.h"
#include "ties.h"
#include "partial.h"
#include "sturctsex.h"
#include "options.h"
#include "output.h"
#include "common.h"
#include "util.h"


void Output_TriEx(int id1, int id2, int id3, FILE *output, int next_face, int flag,
		int where)
{
     /*   We will save everything into a list, rather than output at once,
	  as was done in the old routine. This way for future modifications
	  we can change the strips later on if we want to.
     */

    int swap,temp1,temp2,temp3;
    static int total=0;
    static int tri=0;
    static int strips = 0;
    static int cost = 0;
    
    if (flag == -20)
    {
         cost = cost + where+total+tri+strips+strips;
         printf("We will need to send %d vertices to the renderer\n",cost);
         total = 0;
         tri = 0;
         strips = 0;
         return ;
    }


    if (flag == -10)
	 /*    We are finished, now is time to output the triangle list
	 */
    {
          fprintf(output,"\nt ");
          tri = tri + Finished(&swap,output,FALSE);
          total = total + swap;
          strips++;
          /*printf("There are %d swaps %d tri %d strips\n",total,tri,strips);*/
    }
       
    else
    {
	 Last_Edge(&temp1,&temp2,&temp3,0);
	 Add_Id_Strips(id1,where);
	 Add_Id_Strips(id2,where);
	 Add_Id_Strips(id3,where);
	 Last_Edge(&id1,&id2,&id3,1);
    }
}

		


void Extend_BackwardsEx(int face_id, FILE *output, FILE *strip, int *ties, 
		      int tie, int triangulate, 
		      int swaps,int *next_id)
{
    /*  We just made a strip, now we are going to see if we can extend
	   backwards from the starting face, which had 2 or more adjacencies
	   to start with.
    */
    int bucket,next_face,num,x,y,z,c,d=1,max,f;
    ListHead *pListFace;
    PF_FACES face;
    P_ADJACENCIES temp;

    /*  Get the first triangle that we have saved the the strip data 
	   structure, so we can see if there are any polygons adjacent
	   to this edge or a neighboring one
    */
    First_Edge(&x,&y,&z); 
    
    pListFace  = PolFaces[face_id];
    face = (PF_FACES) PeekList(pListFace,LISTHEAD,0);

    num = face->nPolSize;

    /*  Go through the edges to see if there is an adjacency
	   with a vertex in common to the first triangle that was
	   outputted in the strip. (maybe edge was deleted....)
    */
    for (c=0; c<num ; c++)
    {
	   
	if ( (c != (num-1)) && 
	     (( (*(face->pPolygon+c) == x) && (*(face->pPolygon+c+1) == y)) ||
		(*(face->pPolygon+c) == y) && (*(face->pPolygon+c+1) == x)))
	{
	    /*  Input edge is still there see if there is an adjacency */
	    next_face = Find_Face(face_id, x, y, &bucket);
	    if (next_face == -1)
		/*  Could not find a face adjacent to the edge */
	     	break;
	    pListFace = array[bucket];
	    max = NumOnList(pListFace);
	    for (f=0;;f++)
	    {
			temp = (P_ADJACENCIES) PeekList(pListFace,LISTHEAD,f);	
		     if (temp->face_id == next_face)
		     {
		          Last_Edge(&z,&y,&x,1);
		          Polygon_OutputEx(temp,temp->face_id,bucket,pListFace,
        			  		       output,strip,ties,tie,triangulate,swaps,next_id,0);
		          return;
		     }

			if (temp == NULL)
			{
					printf("Error in the new buckets%d %d %d\n",bucket,max,0);
					exit(0);
			}
	    }

	}
	else if ( (c == (num -1)) &&
	   ( ((*(face->pPolygon) == x) && (*(face->pPolygon+num-1) == y)) ||
	      (*(face->pPolygon) == y) && (*(face->pPolygon+num-1) == x)))
	{
	     next_face = Find_Face(face_id,x,y,&bucket);
	     if (next_face == -1)
		/*  Could not find a face adjacent to the edge */
		     break;
		pListFace = array[bucket];
		max = NumOnList(pListFace);
		for (f=0;;f++)
		{
			temp = (P_ADJACENCIES) PeekList(pListFace,LISTHEAD,f);
		     if (temp->face_id == next_face)
		     {
		          Last_Edge(&z,&y,&x,1);
		          Polygon_OutputEx(temp,temp->face_id,bucket,pListFace,
				   	            output,strip,ties,tie,triangulate,swaps,next_id,0);
		          return;
		     }

			if (temp == NULL)
			{
					printf("Error in the new buckets%d %d %d\n",bucket,max,0);
					exit(0);
			}
	    }
	}
    
    }

}

void Polygon_OutputEx(P_ADJACENCIES temp,int face_id,int bucket,
					ListHead *pListHead, FILE *output, FILE *strips,
					int *ties, int tie, 
					int triangulate, int swaps,
					int *next_id, int where)
{
	ListHead *pListFace;
	PF_FACES face;
	P_ADJACENCIES pfNode;
	static BOOL begin = TRUE;
	int old_face,next_face_id,next_bucket,e1,e2,e3,other1,other2,other3;
	P_ADJACENCIES lpListInfo; 

	/*      We have a polygon to output, the id is face id, and the number
		   of adjacent polygons to it is bucket.
	*/

     Last_Edge(&e1,&e2,&e3,0);
    
     /*  Get the polygon with id face_id */
	pListFace  = PolFaces[face_id];
	face = (PF_FACES) PeekList(pListFace,LISTHEAD,0);

     if (face->nPolSize == 3)
	{
		/*      It is already a triangle */
		if (bucket == 0)
		{
			/*      It is not adjacent to anything so we do not have to
				   worry about the order of the sides or updating adjacencies
			*/
			    
	          Last_Edge(&e1,&e2,&e3,0);
	          next_face_id = Different(*(face->pPolygon),*(face->pPolygon+1),*(face->pPolygon+2),
		                              e1,e2,e3,&other1,&other2);
	          /*  No input edge, at the start */
	          if ((e2 ==0) && (e3 == 0))
	          {
		          e2 = other1;
		          e3 = other2;
	          }
	    
			Output_TriEx(e2,e3,next_face_id,strips,-1,begin,where);
			RemoveList(pListHead,(PLISTINFO) temp);
			/*      We will be at the beginning of the next strip. */
			begin = TRUE;
		}
		/*      It is a triangle with adjacencies. This means that we
			   have to:
				1. Update the adjacencies in the list, because we are
					using this polygon and it will be deleted.
				2. Get the next polygon.
		*/
		else
		{
			/*   Return the face_id of the next polygon we will be using,
				while updating the adjacency list by decrementing the
				adjacencies of everything adjacent to the current triangle.
			*/
            
               next_face_id = Update_AdjacenciesEx(face_id, &next_bucket, &e1,&e2,ties);
			old_face = next_face_id;
				        
              /*  Break the tie,  if there was one */
		    if (tie != FIRST)
				old_face = Get_Next_Face(tie,face_id,triangulate);

              if (next_face_id == -1)
              {
                    Polygon_OutputEx(temp,face_id,0,pListHead,output,strips,ties,tie, 
	     		    triangulate,swaps,next_id,where);
                    return;
              }


	        /*  We are using a different face */
	        if ((tie != FIRST) && (old_face != next_face_id) && (swaps == ON))
             {
		        next_face_id = old_face;
		        /*  Get the new output edge, since e1 and e2 are for the
		            original next face that we got.
		        */
                  e3 = Get_EdgeEx(&e1,&e2,face->pPolygon,next_face_id,face->nPolSize,0,0);
	        }
			
		   /*      Find the other vertex to transmit in the triangle */
		   e3 = Return_Other(face->pPolygon,e1,e2);
	        Last_Edge(&other1,&other2,&other3,0);
	    
	        if ((other1 != 0) && (other2 != 0))
	        {
	               /*   See which vertex in the output edge is not in the input edge */
	               if ((e1 != other2) && (e1 != other3))
		               e3 = e1;
	               else if ((e2 != other2) && (e2 != other3))
		               e3 = e2;
	               /* can happen with > 2 polys on an edge  but won't form a good strip so stop
                       the strip here
                    */
                    else
	               {
                         Polygon_OutputEx(temp,face_id,0,pListHead,output,strips,ties,tie, 
         		                           triangulate,swaps,next_id,where);
                         return;
	       }

	       /*   See which vertex of the input edge is not in the output edge */
	       if ((other2 != e1) && (other2 != e2))
	       {
		          other1 = other2;
		          other2 = other3;
	       }
	       else if ((other3 != e1) && (other3 != e2))
		          other1 = other3;
	       else
	       {
                 /* Degenerate triangle just return*/
               Output_TriEx(other1,other2,e3,strips,next_face_id,begin,where);
			RemoveList(pListHead,(PLISTINFO) temp);
			begin = FALSE;
               return;
	       }
		      
	   }
	   
	   /*   There was not an input edge, we are the first triangle in a strip */
	   else 
	   {
	       /*   Find the correct order to transmit the triangle, what is
		       the output edge that we want ?
	       */
	       other1 = e3;
	       e3 = e2;
	       other2 = e1;
	   }
	   
	   /*   At this point the adjacencies have been updated  and we
		   have the next polygon id 
	   */
        Output_TriEx(other1,other2,e3,strips,next_face_id,begin,where);
  	   RemoveList(pListHead,(PLISTINFO) temp);
	   begin = FALSE;

	   if (Done(next_face_id,59,&next_bucket) == NULL)
		return;

        pListHead = array[next_bucket];
	   pfNode = (P_ADJACENCIES) malloc(sizeof(ADJACENCIES) );
	   if ( pfNode )
	  	pfNode->face_id = next_face_id;
	   lpListInfo = (P_ADJACENCIES) (SearchList(array[next_bucket], pfNode,
				 (int (*)(void *,void *)) (Compare)));
	   if (lpListInfo == NULL)
	   {
				printf("There is an error finding the next polygon3 %d\n",next_face_id);
				exit(0);
	   }
	   Polygon_OutputEx(lpListInfo,next_face_id,next_bucket,
					pListHead, output, strips,ties,tie,triangulate,swaps,next_id,where);

	}
}

	else
	{
		/*      It is not a triangle, we have to triangulate it .
			   Since it is not adjacent to anything we can triangulate it
			   blindly
		*/
		if (bucket == 0)
		{
			/*  Check to see if there is not an input edge */
	          Last_Edge(&other1,&other2,&other3,0);
	          if ((other1 == 0) && (other2 ==0))
		          Blind_TriangulateEx(face->nPolSize,face->pPolygon, strips,
						          output,TRUE,where);
	          else
		          Blind_TriangulateEx(face->nPolSize,face->pPolygon,strips,
			                         output,FALSE,where);

			RemoveList(pListHead,(PLISTINFO) temp);
			/*      We will be at the beginning of the next strip. */
			begin = TRUE;
		}

		 /*  If we have specified PARTIAL triangulation then
			we will go to special routines that will break the
			polygon and update the data structure. Else everything
			below will simply triangulate the whole polygon 
		*/
		else if (triangulate == PARTIAL)
		{
	    
	          /*  Return the face_id of the next polygon we will be using,
			*/
			next_face_id = Min_Face_AdjEx(face_id,&next_bucket,ties);

		
			/* Don't do it partially, because we can go inside and get
		        less adjacencies, for a quad we can do the whole thing.
	          */
	          if ((face_id == next_face_id) && (face->nPolSize == 4)  && (swaps == ON))
	          {
                    next_face_id = Update_AdjacenciesEx(face_id, &next_bucket, &e1,&e2,ties);
		          if (next_face_id == -1)
		          {
		               /*  There is no sequential face to go to, end the strip */
		               Polygon_OutputEx(temp,face_id,0,pListHead,output,strips,ties,tie, 
					                 triangulate,swaps,next_id,where);
		               return;
		          }
               
                    /* Break the tie,  if there was one */
		          if (tie != FIRST)
			          next_face_id = Get_Next_Face(tie,face_id,triangulate);
		          Non_Blind_TriangulateEx(face->nPolSize,face->pPolygon, strips,
						              output,next_face_id,face_id,where);
			     RemoveList(pListHead,(PLISTINFO) temp);
	          }
	   
	          /*   Was not a quad but we still do not want to do it partially for
		          now, since we want to only do one triangle at a time
	          */
	          else if ((face_id == next_face_id) && (swaps == ON))
	               Inside_Polygon(face->nPolSize,face->pPolygon,strips,output,
		                         next_face_id,face_id,next_id,pListHead,temp,where);

	          else
	          {
					 if ((tie != FIRST) && (swaps == ON))
						 next_face_id = Get_Next_Face(tie,face_id,triangulate);
					 Partial_Triangulate(face->nPolSize,face->pPolygon,strips,
						                output,next_face_id,face_id,next_id,pListHead,temp,where);
					/*    Check the next bucket again ,maybe it changed 
						 We calculated one less, but that might not be the case
					*/
               }

			if (Done(next_face_id,59,&next_bucket) == NULL)
			{
     			/*  Check to see if there is not an input edge */
	               Last_Edge(&other1,&other2,&other3,0);
	               if ((other1 == 0) && (other2 ==0))
		               Blind_TriangulateEx(face->nPolSize,face->pPolygon, strips,
						               output,TRUE,where);
	               else
		               Blind_TriangulateEx(face->nPolSize,face->pPolygon,strips,
			                              output,FALSE,where);
                    
                    if (Done(face_id,59,&bucket) != NULL)
                    {
                         pListHead = array[bucket];
			          pfNode = (P_ADJACENCIES) malloc(sizeof(ADJACENCIES) );
			          if ( pfNode )
				          pfNode->face_id = face_id;
			          lpListInfo = (P_ADJACENCIES) (SearchList(array[bucket], pfNode,
				                  (int (*)(void *,void *)) (Compare)));
			          RemoveList(pListHead,(PLISTINFO)lpListInfo);
                    }
                    begin = TRUE;
				return;
			}
			
			begin = FALSE;
			pListHead = array[next_bucket];
			pfNode = (P_ADJACENCIES) malloc(sizeof(ADJACENCIES) );
			if ( pfNode )
				pfNode->face_id = next_face_id;
			lpListInfo = (P_ADJACENCIES) (SearchList(array[next_bucket], pfNode,
				        (int (*)(void *,void *)) (Compare)));
			if (lpListInfo == NULL)
			{
				printf("There is an error finding the next polygon1 %d %d\n",next_face_id,next_bucket);
				exit(0);
			}
			Polygon_OutputEx(lpListInfo,next_face_id,next_bucket,
				            pListHead, output, strips,ties,tie,triangulate,swaps,next_id,where);
		}

          
		else
		{
			/*  WHOLE triangulation */
	          /*  It is not a triangle and has adjacencies. 
				This means that we have to:
				1. TriangulateEx this polygon, not blindly because
					we have an edge that we want to come out on, that
					is the edge that is adjacent to a polygon with the
					least number of adjacencies. Also we must come in
					on the last seen edge.
				2. Update the adjacencies in the list, because we are
					using this polygon .
				3. Get the next polygon.
			*/
			/*   Return the face_id of the next polygon we will be using,
				while updating the adjacency list by decrementing the
				adjacencies of everything adjacent to the current polygon.
			*/
				
               next_face_id = Update_AdjacenciesEx(face_id, &next_bucket, &e1,&e2,ties);

               if (Done(next_face_id,59,&next_bucket) == NULL)
               {
                    Polygon_OutputEx(temp,face_id,0,pListHead,output,strips,ties,tie, 
			                      triangulate,swaps,next_id,where);
                    /*    Because maybe there was more than 2 polygons on the edge */
                    return;
		     }

		     /*      Break the tie,  if there was one */
			else if (tie != FIRST)
				next_face_id = Get_Next_Face(tie,face_id,triangulate);
				
			Non_Blind_TriangulateEx(face->nPolSize,face->pPolygon, strips,
						         output,next_face_id,face_id,where);
			RemoveList(pListHead,(PLISTINFO) temp);
			begin = FALSE;
			pListHead = array[next_bucket];
			pfNode = (P_ADJACENCIES) malloc(sizeof(ADJACENCIES) );
			if ( pfNode )
				pfNode->face_id = next_face_id;
			lpListInfo = (P_ADJACENCIES) (SearchList(array[next_bucket], pfNode,
					   (int (*)(void *,void *)) (Compare)));
			if (lpListInfo == NULL)
		     {
					printf("There is an error finding the next polygon2 %d %d\n",next_face_id,next_bucket);
					exit(0);
               }
			Polygon_OutputEx(lpListInfo,next_face_id,next_bucket,
					       pListHead, output, strips,ties,tie,triangulate,swaps,next_id,where);
		}

	}
    Last_Edge(&e1,&e2,&e3,0);

}       



	




