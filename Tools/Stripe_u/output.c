/********************************************************************/
/*   STRIPE: converting a polygonal model to triangle strips    
     Francine Evans, 1996.
     SUNY @ Stony Brook
     Advisors: Steven Skiena and Amitabh Varshney
*/
/********************************************************************/

/*---------------------------------------------------------------------*/
/*   STRIPE: output.c
     This file contains routines that are finding and outputting the
     strips from the local algorithm
*/
/*---------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include "global.h"
#include "polverts.h"
#include "triangulate.h"
#include "partial.h"
#include "sturcts.h"
#include "ties.h"
#include "options.h"
#include "common.h"
#include "util.h"
#include "free.h"

int *vn;
int *vt;
int norm;
int text;

int Finished(int *swap, FILE *output, BOOL global)
{
     /*   We have finished all the triangles, now is time to output to
	     the data file. In the strips data structure, every three ids
	     is  a triangle. Now we see whether we can swap, or make a new strip
	     or continue the strip, and output the data accordingly to the
	     data file. 
     */
     register int start_swap = 0;
     int num,x,vertex1,vertex2;
     ListHead *pListHead;
     int id[2],other1,other2,index = 0,a,b,c;
     P_STRIPS temp1,temp2,temp3,temp4,temp5,temp6;
     BOOL cptexture;
     *swap =0;
    
     cptexture = text;
     pListHead = strips[0];
     if (pListHead == NULL)
         return 0;

     num = NumOnList(pListHead);
     /*printf ("There are %d triangles in the extend\n",num/3);*/

     /* Go through the list triangle by triangle */
	temp1 = ( P_STRIPS ) PeekList( pListHead, LISTHEAD, 0);
	temp2 = ( P_STRIPS ) PeekList( pListHead, LISTHEAD, 1);
	temp3 = ( P_STRIPS ) PeekList( pListHead, LISTHEAD, 2);
	
      /*   Next triangle for lookahead */
     temp4 = ( P_STRIPS ) PeekList( pListHead, LISTHEAD, 3);
	  

     /*    There is only one polygon in the strip */
     if (temp4 == NULL)
     {
           /*   Data might be mixed and we do not have textures for some of the vertices */
           if ((text) &&  (vt[temp3->face_id] == 0))
                    cptexture = FALSE;
           if ((norm) && (!cptexture))
                fprintf(output,"%d//%d %d//%d %d//%d",temp3->face_id+1,vn[temp3->face_id]+1,
                                temp2->face_id+1,vn[temp2->face_id]+1,
                                temp1->face_id+1,vn[temp1->face_id]+1);
           else if ((cptexture) && (!norm))
                fprintf(output,"%d/%d %d/%d %d/%d",temp3->face_id+1,vt[temp3->face_id]+1,
                        temp2->face_id+1,vt[temp2->face_id]+1,
                        temp1->face_id+1,vt[temp1->face_id]+1);
           else if ((cptexture)&& (norm))
                fprintf(output,"%d/%d/%d %d/%d/%d %d/%d/%d",temp3->face_id+1,vt[temp3->face_id]+1,vn[temp3->face_id]+1,
                        temp2->face_id+1,vt[temp2->face_id]+1,vn[temp2->face_id]+1,
                        temp1->face_id+1,vt[temp1->face_id]+1,vn[temp1->face_id]+1);
           else 
                fprintf(output,"%d %d %d",temp3->face_id+1,temp2->face_id+1,temp1->face_id+1);
           Free_Strips();
	      return 1;
	  }
	  
	  /*    We have a real strip */
	  temp5 = ( P_STRIPS ) PeekList( pListHead, LISTHEAD, 4);
	  temp6 = ( P_STRIPS ) PeekList( pListHead, LISTHEAD, 5);
	  
	  if ((temp1 == NULL) || (temp2 == NULL) || (temp3 == NULL) || (temp5 == NULL) || (temp6 == NULL))
	  {
	       printf("There is an error in the output of the triangles\n");
	       exit(0);
	  }

	  /*   Find the vertex in the first triangle that is not in the second */
	  vertex1 = Different(temp1->face_id,temp2->face_id,temp3->face_id,temp4->face_id,temp5->face_id,temp6->face_id,&other1,&other2);
	  /*    Find the vertex in the second triangle that is not in the first */
	  vertex2 = Different(temp4->face_id,temp5->face_id,temp6->face_id,temp1->face_id,temp2->face_id,temp3->face_id,&other1,&other2);

	  /* Lookahead for the correct order of the 2nd and 3rd vertex of the first triangle */
       temp1 = ( P_STRIPS ) PeekList( pListHead, LISTHEAD, 6);
	  temp2 = ( P_STRIPS ) PeekList( pListHead, LISTHEAD, 7);
	  temp3 = ( P_STRIPS ) PeekList( pListHead, LISTHEAD, 8);
       
       if (temp1 != NULL)
          other1 = Different(temp3->face_id,temp4->face_id,temp5->face_id,temp1->face_id,temp2->face_id,temp3->face_id,&other1,&a);
	   
	  id[index] = vertex1; index = !index;
	  id[index] = other1; index = !index;
	  id[index] = other2; index = !index;

	  a = temp4->face_id; 
	  b = temp5->face_id; 
	  c = temp6->face_id;

      /*    If we need to rearrange the first sequence because otherwise
            there would have been a swap.
      */

      if ((temp3 != NULL) && (text) && ( vt[temp3->face_id]==0))
           cptexture = FALSE;
      if ((norm) && (!cptexture))
           fprintf(output,"%d//%d %d//%d %d//%d ",vertex1+1,vn[vertex1]+1,other1+1,vn[other1]+1,
                                                  other2+1,vn[other2]+1);
      else if ((cptexture) && (!norm))
           fprintf(output,"%d/%d %d/%d %d/%d ",vertex1+1,vt[vertex1]+1,other1+1,vt[other1]+1,
                                               other2+1,vt[other2]+1);
      else if ((cptexture) && (norm))
           fprintf(output,"%d/%d/%d %d/%d/%d %d/%d/%d ",vertex1+1,vt[vertex1]+1,vn[vertex1]+1,
                                                        other1+1,vt[other1]+1,vn[other1]+1,
                                                        other2+1,vt[other2]+1,vn[other2]+1);
      else
           fprintf(output,"%d %d %d ",vertex1+1,other1+1,other2+1);


	 for (x = 6; x < num ; x = x+3)
	  {

	      /*    Get the next triangle */
	      temp1 = ( P_STRIPS ) PeekList( pListHead, LISTHEAD, x);
	      temp2 = ( P_STRIPS ) PeekList( pListHead, LISTHEAD, x+1);
	      temp3 = ( P_STRIPS ) PeekList( pListHead, LISTHEAD, x+2);

           /*    Error checking */
	      if (!(member(id[0],a,b,c)) || !(member(id[1],a,b,c)) || !(member(vertex2,a,b,c)))
	      {
		     /*   If we used partial we might have a break in the middle of a strip */
		     fprintf(output,"\nt ");
	          /*   Find the vertex in the first triangle that is not in the second */
	          vertex1 = Different(a,b,c,temp1->face_id,temp2->face_id,temp3->face_id,&other1,&other2);
	          /*    Find the vertex in the second triangle that is not in the first */
	          vertex2 = Different(temp1->face_id,temp2->face_id,temp3->face_id,a,b,c,&other1,&other2);
	   
	          id[index] = vertex1; index = !index;
	          id[index] = other1; index = !index;
	          id[index] = other2; index = !index;
	      }

	      if ((temp1 == NULL ) || (temp2 == NULL) || (temp3 == NULL))
	      {
		  printf("There is an error in the triangle list \n");
		  exit(0);
	      }
         
           if ((id[0] == id[1]) || (id[0] == vertex2))
                continue;

           if ((member(id[index],temp1->face_id,temp2->face_id,temp3->face_id)))
           {
                if ((text) && ( vt[id[index]]==0))
                     cptexture = FALSE;
                if ((!norm) && (!cptexture))
                     fprintf(output,"%d ",id[index]+1);
                else if ((norm) && (!cptexture))
                     fprintf(output,"%d//%d ",id[index]+1,vn[id[index]]+1);
                else if ((!norm) && (cptexture))
                     fprintf(output,"%d/%d ",id[index]+1,vt[id[index]]+1);
                else
                     fprintf(output,"%d/%d/%d ",id[index]+1,vt[id[index]]+1,vn[id[index]]+1);
                index = !index;
                *swap = *swap + 1;
           }
           
           if ((text) && ( vt[vertex2]==0))
                cptexture = FALSE;
	      if ((!norm) && (!cptexture))
                fprintf(output,"\nq %d ",vertex2+1);
           else if ((norm) && (!cptexture))
                fprintf(output,"\nq %d//%d ",vertex2+1,vn[vertex2]+1);
           else if ((!norm) && (cptexture))
                fprintf(output,"\nq %d/%d ",vertex2+1,vt[vertex2]+1);
           else
                fprintf(output,"\nq %d/%d/%d ",vertex2+1,vt[vertex2]+1,vn[vertex2]+1);

	      id[index] = vertex2; index = !index;

	      /*    Get the next vertex not in common */
	      vertex2 = Different(temp1->face_id,temp2->face_id,temp3->face_id,a,b,c,&other1,&other2);
	      a = temp1->face_id;
	      b = temp2->face_id;
	      c = temp3->face_id;
	  }
	  /*    Do the last vertex */
       if ((text) && (vt[vertex2]==0))
       {
            if (cptexture)
                 fprintf(output,"\nq ");
            cptexture = FALSE;
       }
       if ((!norm) && (!cptexture))
            fprintf(output,"%d ",vertex2+1);
       else if ((norm) && (!cptexture))
            fprintf(output,"%d//%d ",vertex2+1,vn[vertex2]+1);
       else if ((!norm) && (cptexture))
            fprintf(output,"%d/%d ",vertex2+1,vt[vertex2]+1);
       else
            fprintf(output,"%d/%d/%d ",vertex2+1,vt[vertex2]+1,vn[vertex2]+1);

	  
	 Free_Strips();
      return (num/3);	      
}

void Output_Tri(int id1, int id2, int id3,FILE *bands, int color1, int color2, int color3,BOOL end)
{
     /*   We will save everything into a list, rather than output at once,
	     as was done in the old routine. This way for future modifications
	     we can change the strips later on if we want to.
     */

    int temp1,temp2,temp3;
    
    /*  Make sure we do not have an error */
    /*    There are degeneracies in some of the files */
	if ( (id1 == id2) || (id1 == id3) || (id2 == id3))
	{
		printf("Degenerate triangle %d %d %d\n",id1,id2,id3);
		exit(0);
	}
     else
     {
          Last_Edge(&temp1,&temp2,&temp3,0);
	     Add_Id_Strips(id1,end);
	     Add_Id_Strips(id2,end);
	     Add_Id_Strips(id3,end);
	     Last_Edge(&id1,&id2,&id3,1);
     }
}


int Polygon_Output(P_ADJACENCIES temp,int face_id,int bucket,
					ListHead *pListHead, BOOL first, int *swaps,
                         FILE *bands,int color1,int color2,int color3,BOOL global, BOOL end)
{
	ListHead *pListFace;
	PF_FACES face;
	P_ADJACENCIES pfNode;
	static BOOL begin = TRUE;
	int old_face,next_face_id,next_bucket,e1,e2,e3,other1,other2,other3;
	P_ADJACENCIES lpListInfo; 
     int ties=0;
     int  tie = SEQUENTIAL;
     
     /* We have a polygon to output, the id is face id, and the number
	   of adjacent polygons to it is bucket. This routine extends the patches from
        either end to make longer triangle strips.
	*/
                
                   
     /*  Now get the edge */
     Last_Edge(&e1,&e2,&e3,0);
    
     /*  Get the polygon with id face_id */
	pListFace  = PolFaces[face_id];
	face = (PF_FACES) PeekList(pListFace,LISTHEAD,0);

     /*  We can't go any more */
     if ((face->nPolSize == 1) || ((face->nPolSize == 4) && (global))) /* if global, then we are still doing patches */
     {
        /*     Remove it from the list so we do not have to waste
               time visiting it in the future, or winding up in an infinite loop
               if it is the first on that we are looking at for a possible strip
        */
        if (face->nPolSize == 1)
             RemoveList(pListHead,(PLISTINFO) temp);
        if (first)
             return 0;
        else
             return (Finished(swaps,bands,global));
    }

    if (face->nPolSize == 3)
    {
		/*      It is already a triangle */
		if (bucket == 0)
		{
			/*      It is not adjacent to anything so we do not have to
	 			   worry about the order of the sides or updating adjacencies
			*/
			    
	          next_face_id = Different(*(face->pPolygon),*(face->pPolygon+1),*(face->pPolygon+2),
		                              e1,e2,e3,&other1,&other2);  
			face->nPolSize = 1;
                       
               /* If this is the first triangle in the strip */
               if ((e2 == 0) && (e3 ==0))
               {
                    e2 = other1;
                    e3 = other2;
               }

               Output_Tri(e2,e3,next_face_id,bands,color1,color2,color3,end);
               RemoveList(pListHead,(PLISTINFO) temp);
               return (Finished(swaps,bands,global));
          }
		
        
          /*  It is a triangle with adjacencies. This means that we
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
            
               next_face_id = Update_Adjacencies(face_id, &next_bucket, &e1,&e2,&ties);
               /*  Maybe we deleted something in a patch and could not find an adj polygon */
               if (next_face_id == -1)
               {
                       Output_Tri(*(face->pPolygon),*(face->pPolygon+1),*(face->pPolygon+2),bands,color1,
                                  color2,color3,end);
                       face->nPolSize = 1;
                       RemoveList(pListHead,(PLISTINFO) temp);
                       return (Finished(swaps,bands,global));
               }
		  
               old_face = next_face_id;
		     /*      Find the other vertex to transmit in the triangle */
		     e3 = Return_Other(face->pPolygon,e1,e2);
	          Last_Edge(&other1,&other2,&other3,0);
	    
	          if ((other2 != 0) && (other3 != 0))
	          {
	              /*   See which vertex in the output edge is not in the input edge */
	              if ((e1 != other2) && (e1 != other3))
		            e3 = e1;
	              else if ((e2 != other2) && (e2 != other3))
		             e3 = e2;
	              else
	              {
		            printf("There is an error in the tri with adj\n");
		            exit(0);
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
		            printf("There is an error in getting the tri with adj\n");
		            exit(0);
	              }
		      
	          }
               else
               {
                  /*     We are the first triangle in the strip and the starting edge
                         has not been set yet
                  */
                  /*  Maybe we deleted something in a patch and could not find an adj polygon */
                  if (next_face_id == -1)
                  {
                       Output_Tri(*(face->pPolygon),*(face->pPolygon+1),*(face->pPolygon+2),bands,color1,
                                  color2,color3,end);
                       face->nPolSize = 1;
                       RemoveList(pListHead,(PLISTINFO) temp);
                       return (Finished(swaps,bands,global));
                  }

                  other1 = e3;
                  e3 = e2;
                  other2 = e1;
               }
	   
	          /*   At this point the adjacencies have been updated  and we
				have the next polygon id 
	          */

               Output_Tri(other1,other2,e3,bands,color1,color2,color3,end);
               face->nPolSize = 1;
		     RemoveList(pListHead,(PLISTINFO) temp);
		     begin = FALSE;
	          
               /*  Maybe we deleted something in a patch and could not find an adj polygon */
               if (next_face_id == -1)
                    return (Finished(swaps,bands,global));
        
               if (Done(next_face_id,59,&next_bucket) == NULL)
		     {
			     printf("We deleted the next face 4%d\n",next_face_id);
			     exit(0);
	          }

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
			return (Polygon_Output(lpListInfo,next_face_id,next_bucket,
					             pListHead, FALSE, swaps,bands,color1,color2,color3,global,end));

		}
	}

	else
	{
		/*   It is not a triangle, we have to triangulate it .
			Since it is not adjacent to anything we can triangulate it
			blindly
		*/
		if (bucket == 0)
		{
		          /*   It is the first polygon in the strip, therefore there is no
                         input edge to start with.
                    */
                    if ((e2 == 0) && (e3 ==0))
                       Blind_Triangulate(face->nPolSize,face->pPolygon,bands,
			                          TRUE,1,color1,color2,color3);

                    else
                       Blind_Triangulate(face->nPolSize,face->pPolygon,bands,
			                          FALSE,1,color1,color2,color3);

			     RemoveList(pListHead,(PLISTINFO) temp);
			               
                    /*      We will be at the beginning of the next strip. */
                    face->nPolSize = 1;
                    return (Finished(swaps,bands,global));
		}


		else
		{
			
             
               /*  WHOLE triangulation */
	          /*  It is not a triangle and has adjacencies. 
				This means that we have to:
				1. Triangulate this polygon, not blindly because
					we have an edge that we want to come out on, that
					is the edge that is adjacent to a polygon with the
					least number of adjacencies. Also we must come in
					on the last seen edge.
				2. Update the adjacencies in the list, because we are
					using this polygon .
				3. Get the next polygon.
			*/
			/*      Return the face_id of the next polygon we will be using,
				while updating the adjacency list by decrementing the
				adjacencies of everything adjacent to the current polygon.
			*/
				
               next_face_id = Update_Adjacencies(face_id, &next_bucket, &e1,&e2,&ties);
	    
               /*  Maybe we deleted something in a patch and could not find an adj polygon */
               if (next_face_id == -1)
               {
 
                    /*   If we are at the first polygon in the strip and there is no input
                         edge, then begin is TRUE
                    */
                    if ((e2 == 0) && (e3 == 0))
                         Blind_Triangulate(face->nPolSize,face->pPolygon,
			                            bands,TRUE,1,color1,color2,color3);

                    else
                         Blind_Triangulate(face->nPolSize,face->pPolygon,
			                            bands,FALSE,1,color1,color2,color3);

		          RemoveList(pListHead,(PLISTINFO) temp);
		              
    	               /*      We will be at the beginning of the next strip. */
                    face->nPolSize = 1;
                    return (Finished(swaps,bands,global));
               }

               if (Done(next_face_id,59,&next_bucket) == NULL)
	          {
			    printf("We deleted the next face 6 %d %d\n",next_face_id,face_id);
			    exit(0);
		     }
			
			Non_Blind_Triangulate(face->nPolSize,face->pPolygon, 
				                 bands,next_face_id,face_id,1,color1,color2,color3);
				     
               RemoveList(pListHead,(PLISTINFO) temp);
               begin = FALSE;
               face->nPolSize = 1;
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
			return (Polygon_Output(lpListInfo,next_face_id,next_bucket,
					             pListHead, FALSE, swaps,bands,color1,color2,color3,global,end));
		}

	}
    Last_Edge(&e1,&e2,&e3,0);

}       


int Extend_Face(int face_id,int e1,int e2,int *swaps,FILE *bands,
                int color1,int color2,int color3,int *vert_norm, int normals,
                int *vert_texture, int texture)
{
    int dummy=0,next_bucket;
    P_ADJACENCIES pfNode,lpListInfo;
    ListHead *pListHead;

    /*    Try to extend backwards off of the local strip that we just found */
    
    vn = vert_norm;
    vt = vert_texture;
    norm = normals;
    text = texture;

    *swaps = 0;
    /*	Find the face that is adjacent to the edge and is not the
		current face.
    */
    face_id = Find_Face(face_id, e1, e2,&next_bucket);
    if (face_id == -1)
          return 0;
			
    pListHead = array[next_bucket];
    pfNode = (P_ADJACENCIES) malloc(sizeof(ADJACENCIES) );
    if ( pfNode )
		pfNode->face_id = face_id;
    lpListInfo = (P_ADJACENCIES) (SearchList(array[next_bucket], pfNode,
		       (int (*)(void *,void *)) (Compare)));
    if (lpListInfo == NULL)
    {
		printf("There is an error finding the next polygon3 %d\n",face_id);
		exit(0);
    }
    Last_Edge(&dummy,&e1,&e2,1);
    
    /*  Find a strip extending from the patch and return the cost */
    return (Polygon_Output(lpListInfo,face_id,next_bucket,pListHead,TRUE,swaps,bands,color1,color2,color3,TRUE,TRUE));
}


