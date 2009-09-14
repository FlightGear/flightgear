/********************************************************************/
/*   STRIPE: converting a polygonal model to triangle strips    
     Francine Evans, 1996.
     SUNY @ Stony Brook
     Advisors: Steven Skiena and Amitabh Varshney
*/
/********************************************************************/

/*---------------------------------------------------------------------*/
/*   STRIPE: newpolve.c
     This routine contains the bulk of the code that will find the
     patches of quads in the data model
*/
/*---------------------------------------------------------------------*/

#include <stdlib.h>
#include "polverts.h"
#include "extend.h"
#include "output.h"
#include "triangulate.h"
#include "common.h"
#include "util.h"
#include "global.h"        
#include "init.h"
#include "add.h"

ListHead **PolVerts;
ListHead **PolFaces;
ListHead **PolEdges;
int length;
BOOL resetting = FALSE;
int     ids[MAX1];
int 	added_quad = 0;
BOOL reversed = FALSE;
int patch = 0;
int *vn;
int *vt;

int Calculate_Walks(int lastvert,int y, PF_FACES temp2)
{
	/* Find the length of the walk */
	
	int previous_edge1, previous_edge2;
	register int nextvert,numverts,counter,walk=0;
	BOOL flag;
	F_EDGES *node;
     ListHead *pListHead;
     static int seen = 0;
     
	/* Find the edge that we are currently on */
	if (y != 3)
	{
		previous_edge1 = *(temp2->pPolygon +y);
		previous_edge2 = *(temp2->pPolygon + y + 1);
	}
	else
	{
		previous_edge1 = *(temp2->pPolygon +y);
		previous_edge2 = *(temp2->pPolygon);
	}

	temp2->seen = seen;
     counter = y;

     /*Find the adjacent face to this edge */
	node = *(temp2->VertandId+y);			
	if (node->edge[2] != lastvert)
        nextvert = node->edge[2];
     else
        nextvert = node->edge[1];
					
	/* Keep walking in this direction until we cannot do so */
	while ((nextvert != lastvert) && (nextvert != -1))
	{
		walk++;
		pListHead = PolFaces[nextvert];
		temp2 = (PF_FACES) PeekList(pListHead,LISTHEAD,0);
		numverts = temp2->nPolSize;
		if ((numverts != 4) || (temp2->seen == seen))
		{
			walk--;
			nextvert = -1;
		}
		else
		{
			temp2->seen = seen;
               /* Find edge that is not adjacent to the previous one */
			counter = 0;
			flag = TRUE;
			while ((counter < 3) && (flag))
			{
				if ( ((*(temp2->pPolygon+counter) == previous_edge1) ||
					(*(temp2->pPolygon+counter+1) == previous_edge2)) ||
					((*(temp2->pPolygon+counter) == previous_edge2) || 
					(*(temp2->pPolygon+counter+1) == previous_edge1)) )
					counter++;		
				else
					flag = FALSE;	
			}
	     	/* Get the IDs of the next edge */
		     if (counter < 3)
		     {
			     previous_edge1 = *(temp2->pPolygon + counter);
			     previous_edge2 = *(temp2->pPolygon + counter + 1);
		     }
		     else
		     {
                    previous_edge1 = *(temp2->pPolygon + counter);
                    previous_edge2 = *(temp2->pPolygon);
		     }
	
		     node = *(temp2->VertandId + counter);
		     if (node->edge[1] == nextvert)
			     nextvert = node->edge[2];
		     else
			     nextvert = node->edge[1];
		}
	}
     seen++;
	return walk;
}


BOOL Check_Right(int last_seen,PF_FACES temp2,int y,int face_id)
{
	/* Check when we last saw the face to the right of the current
	   one. We want to have seen it just before we started this strip
	*/

	F_EDGES *node;
	ListHead *pListHead;
	register int nextvert,oldy;
	PF_FACES t;
	
     oldy = y;
	if (y != 3)
		y = y+1;
	else
		y = 0;
	node = *(temp2->VertandId + y);
	if (face_id == node->edge[1])
		nextvert = node->edge[2];
	else
		nextvert = node->edge[1];
	
     if (nextvert == -1)
          return FALSE;
     
     pListHead = PolFaces[nextvert];
	t = (PF_FACES) PeekList(pListHead,LISTHEAD,0);
	if (t->seen != (last_seen - 1))
	{
		 /* maybe because of the numbering, we are not
		    on the right orientation, so we have to check the
		    opposite one to be sure 
           */
		if (oldy != 0)
			y = oldy-1;
		else
			y = 3;
		node = *(temp2->VertandId + y);
		if (face_id == node->edge[1])
			nextvert = node->edge[2];
		else
			nextvert = node->edge[1];
		if (nextvert == -1)
               return FALSE;
          pListHead = PolFaces[nextvert];
		t = (PF_FACES) PeekList(pListHead,LISTHEAD,0);
		if (t->seen != (last_seen - 1))
		 	return FALSE;
	}
	return TRUE;
}


int Update_and_Test(PF_FACES temp2,int y,BOOL first,int distance,int lastvert, int val)
{
	     
        static int last_seen = 17;
        int previous_edge1, previous_edge2;
        register int original_distance,nextvert,numverts,counter;
        BOOL flag;
        F_EDGES *node;
        ListHead *pListHead;
                                                        
        original_distance = distance;
        /* Find the edge that we are currently on */
        if (y != 3)
        {
                previous_edge1 = *(temp2->pPolygon +y);
                previous_edge2 = *(temp2->pPolygon + y + 1);
        }
        else
        {
                previous_edge1 = *(temp2->pPolygon +y);
                previous_edge2 = *(temp2->pPolygon);
        }

        temp2->seen = val;
        temp2->seen2 = val;
		
        node = *(temp2->VertandId+y);                   
        if (lastvert != node->edge[2])
			nextvert = node->edge[2];
	   else
			nextvert = node->edge[1];
                                        
        /* Keep walking in this direction until we cannot do so  or
		we go to distance */
        while ((distance > 0)  && (nextvert != lastvert) && (nextvert != -1))
        {
                distance--;
			           
                pListHead = PolFaces[nextvert];
                temp2 = (PF_FACES) PeekList(pListHead,LISTHEAD,0);
                temp2->seen = val;
				
                if (temp2->seen2 == val)
                {
                     last_seen++;
                     return (original_distance - distance);
                }
                
                temp2->seen2 = val;
                
                numverts = temp2->nPolSize;
                                
	     	 if (numverts != 4)
          	     nextvert = -1;
		
                else if ((!first) && (!(Check_Right(last_seen,temp2,y,nextvert))))
                {
                    last_seen++;
                    return (original_distance - distance);
                }
		      else
                {
                        /* Find edge that is not adjacent to the previous one */
                        counter = 0;
                        flag = TRUE;
                        while ((counter < 3) && (flag))
                        {
                                if ( ((*(temp2->pPolygon+counter) == previous_edge1) ||
                                        (*(temp2->pPolygon+counter+1) == previous_edge2)) ||
                                        ((*(temp2->pPolygon+counter) == previous_edge2) ||
                                        (*(temp2->pPolygon+counter+1) == previous_edge1)) )
                                        counter++;
                                else
                                        flag = FALSE;
                        }
                        /* Get the IDs of the next edge */
                        if (counter < 3)
                        {
                              previous_edge1 = *(temp2->pPolygon + counter);
                              previous_edge2 = *(temp2->pPolygon + counter + 1);
                        }
                        else
                        {
                              previous_edge1 = *(temp2->pPolygon + counter);
                              previous_edge2 = *(temp2->pPolygon);
                        }
                        if      ( ((*(temp2->walked+counter) == -1) && 
                                (*(temp2->walked+counter+2) == -1)))
                        {
                                printf("There is an error in the walks!\n");
                                printf("1Code %d %d \n",*(temp2->walked+counter),*(temp2->walked+counter+2));
                                exit(0);
                        }
                        else
                        {
                                if      ((*(temp2->walked+counter) == -1) && 
                                        (*(temp2->walked+counter-2) ==  -1))
                                {
                                        printf("There is an error in the walks!\n");
                                        printf("2Code %d %d \n",*(temp2->walked+counter),*(temp2->walked+counter-2));
                                        exit(0);
                                }
                        }
                        node = *(temp2->VertandId + counter);
                        y = counter;
		              if (node->edge[1] == nextvert)
                              nextvert = node->edge[2];
                        else
                              nextvert = node->edge[1];
             }
    }
	
    last_seen++;

    if  (distance != 0)  
    {
		if (((nextvert == -1) || (nextvert == lastvert)) && (distance != 1))
			return (original_distance - distance);
    }
    return original_distance;
}


int Test_Adj(PF_FACES temp2,int x,int north,int distance,int lastvert, int value)
{
	/* if first time, then just update the last seen field */
	if (x==1)
		return(Update_and_Test(temp2,north,TRUE,distance,lastvert,value));
	/* else we have to check if we are adjacent to the last strip */
	else
		return(Update_and_Test(temp2,north,FALSE,distance,lastvert,value));
}
  
void Get_Band_Walk(PF_FACES temp2,int face_id,int *dir1,int *dir2, 
					int orientation,int cutoff_length)
{
	int previous_edge1, previous_edge2;
	F_EDGES *node;
	ListHead *pListHead;
	register int walk = 0, nextvert,numverts,counter;
	BOOL flag;
	
	/*	Get the largest band that will include this face, starting
		from orientation. Save the values of the largest band
		(either north and south together, or east and west together)
		in the direction variables.
	*/
	/* Find the edge that we are currently on */
     if (orientation != 3)
     {
                previous_edge1 = *(temp2->pPolygon + orientation);
                previous_edge2 = *(temp2->pPolygon + orientation + 1);
     }
     else
     {
                previous_edge1 = *(temp2->pPolygon + orientation );
                previous_edge2 = *(temp2->pPolygon);
     }
		
     if (orientation == 0)
     {
			 if (*dir1 > *(temp2->walked + 1))
				*dir1 = *(temp2->walked + 1);
			 if (*dir2 > *(temp2->walked + 3))
				*dir2 = *(temp2->walked + 3);
	}
	else if (orientation == 3)
	{
			if (*dir1 > *(temp2->walked + orientation - 3))
			     *dir1 = *(temp2->walked + orientation - 3) ;
			if (*dir2 > *(temp2->walked + orientation -1 ))
				*dir2 = *(temp2->walked + orientation - 1);
	}
	else
	{
			if (*dir1 > *(temp2->walked + orientation - 1))
				*dir1 = *(temp2->walked + orientation -1) ;
			if (*dir2 > *(temp2->walked+ orientation + 1))
				*dir2 = *(temp2->walked + orientation + 1);
	}
        
     /*	if we know already that we can't extend the
        	band from this face, we do not need to do the walk
     */
     if ((*dir1 != 0) && (*dir2 != 0))
     {
		/* Find the adjacent face to this edge */
        	node = *(temp2->VertandId+orientation);                   
        	if (face_id == node->edge[1])
			nextvert = node->edge[2];
        	else 
			nextvert = node->edge[1];
	}
     else
        	nextvert = -1; /* leave w/o walking */                                
	
	/* Keep walking in this direction until we cannot do so */
     while ((nextvert != face_id) && (nextvert != -1))
     {
            walk++;
            pListHead = PolFaces[nextvert];
            temp2 = (PF_FACES) PeekList(pListHead,LISTHEAD,0);
            numverts = temp2->nPolSize;
            if ((numverts != 4)	|| (walk > cutoff_length))
                   nextvert = -1;
            else
            {
	  		  /* Find edge that is not adjacent to the previous one */
                 counter = 0;
                 flag = TRUE;
                 while ((counter < 3) && (flag))
                 {
                           if ( ((*(temp2->pPolygon+counter) == previous_edge1) ||
                                (*(temp2->pPolygon+counter+1) == previous_edge2)) ||
                                ((*(temp2->pPolygon+counter) == previous_edge2) ||
                                (*(temp2->pPolygon+counter+1) == previous_edge1)) )
                                     counter++;
                           else
                                flag = FALSE;
                 }
                 /* Get the IDs of the next edge */
                 if (counter < 3)
                 {
                        previous_edge1 = *(temp2->pPolygon + counter);
                        previous_edge2 = *(temp2->pPolygon + counter + 1);
                 }
                 else
                 {
                        previous_edge1 = *(temp2->pPolygon + counter);
                        previous_edge2 = *(temp2->pPolygon);
                 }
                
			  /* 	find out how far we can extend in the 2 directions
					along this new face in the walk
			  */
			  if (counter == 0)
			  {
					if (*dir1 > *(temp2->walked + 1))
						*dir1 = *(temp2->walked + 1);
					if (*dir2 > *(temp2->walked + 3))
						*dir2 = *(temp2->walked + 3);
			  }
			  else if (counter == 3)
			  {
					if (*dir1 > *(temp2->walked + counter - 3))
						*dir1 = *(temp2->walked + counter - 3) ;
					if (*dir2 > *(temp2->walked + counter -1 ))
						*dir2 = *(temp2->walked + counter -1);
			  }
			  else
			  {
					if (*dir1 > *(temp2->walked + counter - 1))
						*dir1 = *(temp2->walked + counter -1) ;
					if (*dir2 > *(temp2->walked + counter + 1))
						*dir2 = *(temp2->walked + counter + 1);
			  }
        
        	      /*	if we know already that we can't extend the
        	     	band from this face, we do not need to do the walk
        	      */
	        	 if ((*dir1 == 0) || (*dir2 == 0))
                	nextvert = -1;
                if (nextvert != -1)
                {
                	node = *(temp2->VertandId + counter);
                	if (node->edge[1] == nextvert)
                        nextvert = node->edge[2];
                	else
                        nextvert = node->edge[1];
                }

           }
        }
}




int Find_Max(PF_FACES temp2,int lastvert,int north,int left,
			int *lastminup,int *lastminleft)
{
	int temp,walk,counter,minup,x,band_value;
	int previous_edge1, previous_edge2;
	F_EDGES	*node;
	ListHead *pListHead;
	BOOL flag;	
	static int last_seen = 0;
	register int t,smallest_so_far,nextvert,max=-1;
	        
     t= lastvert;
     *lastminup = MAX_BAND;
	*lastminleft = 1;

     if (left == 3)
	{
          previous_edge1 = *(temp2->pPolygon + left);
          previous_edge2 = *(temp2->pPolygon);
	}
                
	else
	{
          previous_edge1 = *(temp2->pPolygon + left + 1);
          previous_edge2 = *(temp2->pPolygon + left);
	}

	temp2->seen = last_seen;
     walk = *(temp2->walked + left);

     for (x=1;x<=(walk+1); x++)
	{
		/*   test to see if we have a true band
		     that is, are they adjacent to each other
		*/
        
         minup = *(temp2->walked + north) + 1;

	    /*	if we are at the very first face, then we do not
	    	     have to check the adjacent faces going up
	    	     and our north distance is the distance of this face's
			north direction. 
	    */
         if (x == 1) 
	    {
			*lastminup = minup;
			minup = Test_Adj(temp2,x,north,*lastminup,lastvert,last_seen);
			*lastminup = minup;
               smallest_so_far = minup;	
         }
		
	
	    /* find the largest band that we can have */
	    if (minup < (*lastminup))
	    {
			/*	see if we really can go up all the way 
				temp should by less than our equal to minup
				if it is less, then one of the faces was not
				adjacent to those next to it and the band height
				will be smaller
			*/
			temp = Test_Adj(temp2,x,north,minup,lastvert,last_seen);
			if (temp > minup)
			{
				printf("There is an error in the test adj\n");
				exit(0);
			}
			minup = temp;
			band_value = x * minup;
			if (minup < smallest_so_far)
			{
				if (band_value > max)
               	{
					smallest_so_far = minup;
					*lastminup = minup;
					*lastminleft = x;
                         max = band_value;
                    }
				else
					smallest_so_far = minup;
			}
			else
			{
				band_value = x * smallest_so_far;
     	          if (band_value > max)
                    {
                	     *lastminup = smallest_so_far;
                         *lastminleft = x;
                         max = band_value;
                    }
			}
		}
		else
		{
			if (x != 1)
               {
                    temp = Test_Adj(temp2,x,north,smallest_so_far,lastvert,last_seen);
			     if (temp > smallest_so_far)
			     {
				    printf("There is an error in the test adj\n");
				    exit(0);
			     }
			    smallest_so_far = temp;
               }
               band_value = x * smallest_so_far; 
			if (band_value > max)
			{
				*lastminup = smallest_so_far;
				*lastminleft = x;
				max = band_value;
			}
		}
		if ( x != (walk + 1))
		{
			node = *(temp2->VertandId+left);
			if (lastvert == node->edge[1])
				nextvert = node->edge[2];
			else
				nextvert = node->edge[1];

               lastvert = nextvert;
               
               if (nextvert == -1)
                    return max;
               
               pListHead = PolFaces[nextvert];
			temp2 = (PF_FACES) PeekList(pListHead, LISTHEAD, 0);
			
               /* if we have visited this face before, then there is an error */
               if (((*(temp2->walked) == -1) && (*(temp2->walked+1) == -1) &&
				(*(temp2->walked+2) == -1) && (*(temp2->walked+3) == -1))
				|| (temp2->nPolSize !=4) || (temp2->seen == last_seen))
			{

                    if (lastvert == node->edge[1])
                         nextvert = node->edge[2];
                    else
                         nextvert = node->edge[1];
                    if (nextvert == -1)
                         return max;
                    lastvert = nextvert;
                    /*   Last attempt to get the face ... */
                    pListHead = PolFaces[nextvert];
     			temp2 = (PF_FACES) PeekList(pListHead, LISTHEAD, 0);
                    if (((*(temp2->walked) == -1) && (*(temp2->walked+1) == -1) &&
				     (*(temp2->walked+2) == -1) && (*(temp2->walked+3) == -1))
				     || (temp2->nPolSize !=4) || (temp2->seen == last_seen))
                         return max;    /*   The polygon was not saved with the edge, not
                                             enough room. We will get the walk when we come
                                             to that polygon later.
                                        */
			}
			else
			{
				counter = 0;
				flag = TRUE;
				temp2->seen = last_seen;

                    while ((counter < 3) && (flag))
			     {

                         if ( ((*(temp2->pPolygon+counter) == previous_edge1) ||
                                        (*(temp2->pPolygon+counter+1) == previous_edge2)) ||
                                        ((*(temp2->pPolygon+counter) == previous_edge2) ||
                                        (*(temp2->pPolygon+counter+1) == previous_edge1)) )
                                        counter++;
				     else
                                        flag = FALSE;
                    }
               }
		
               /* Get the IDs of the next edge */
               left = counter;
		     north = left+1;
               if (left ==3)
			     north = 0;	
		     if (counter < 3)
               {
                        previous_edge1 = *(temp2->pPolygon + counter + 1);
                        previous_edge2 = *(temp2->pPolygon + counter);
               }
               else
               {
                        previous_edge1 = *(temp2->pPolygon + counter);
                        previous_edge2 = *(temp2->pPolygon);
               }

          } 

}
last_seen++;
return max;
}

void Mark_Face(PF_FACES temp2, int color1, int color2,
				int color3, FILE *output_file, BOOL end, int *edge1, int *edge2, 
                    int *face_id, int norms, int texture)
{
     static int last_quad[4];
     register int x,y,z=0;
     int saved[2];
     static int output1, output2,last_id;
     BOOL cptexture;

     /*   Are we done with the patch? If so return the last edge that
          we will come out on, and that will be the edge that we will
          start to extend upon.
     */

     cptexture = texture;
     if (end)
     {
          *edge1 = output1;
          *edge2 = output2;
          *face_id = last_id;
          return;
     }

     last_id = *face_id;
	*(temp2->walked) = -1;
	*(temp2->walked+1) = -1;
	*(temp2->walked+2) = -1;
	*(temp2->walked+3) = -1;
	added_quad++;
	temp2->nPolSize = 1;

     if (patch == 0)
     {
          /*   At the first quad in the strip -- save it */
          last_quad[0] = *(temp2->pPolygon);
          last_quad[1] = *(temp2->pPolygon+1);
          last_quad[2] = *(temp2->pPolygon+2);
          last_quad[3] = *(temp2->pPolygon+3);
          patch++;
     }
     else
     {
          /*   Now we have a triangle to output, find the edge in common */
          for (x=0; x < 4 ;x++)
          {
              for (y=0; y< 4; y++)
              {
                   if (last_quad[x] == *(temp2->pPolygon+y))
                   {
                        saved[z++] = last_quad[x];               
                        if (z > 2)
                        {
                             /*    This means that there was a non convex or
                                   an overlapping polygon
                             */
                             z--;
                             break;
                        }
                   }                             
              }
          }
          
          if (z != 2)
          {
               printf("Z is not 2 %d \n",patch);
               printf("4 %d %d %d %d %d %d %d\n",*(temp2->pPolygon),
				*(temp2->pPolygon+1),*(temp2->pPolygon+2),*(temp2->pPolygon+3),
				color1,color2,color3);
               printf("%d %d %d %d\n",last_quad[0],last_quad[1],last_quad[2],last_quad[3]);
               exit(1);
          }
          
          if (patch == 1)
          {
               /*   First one to output, there was no output edge */
               patch++;
               x = Adjacent(saved[0],saved[1],last_quad,4);
               y = Adjacent(saved[1],saved[0],last_quad,4);
               
               /*   Data might be mixed and we do not have textures for some of the vertices */
               if ((texture) && ( ((vt[x]) == 0) || ((vt[y])==0) || ((vt[saved[1]])==0)))
                    cptexture = FALSE;

               if ((!norms) && (!cptexture))
               {
                    fprintf(output_file,"\nt %d %d %d ",x+1,y+1,saved[1]+1);
                    fprintf(output_file,"%d ",saved[0]+1);
               }
               else if ((norms) && (!cptexture))
               {
                    fprintf(output_file,"\nt %d//%d %d//%d %d//%d ",x+1,vn[x] +1,
                                                                    y+1,vn[y] +1,
                                                                    saved[1]+1,vn[saved[1]]+1);
                    fprintf(output_file,"%d//%d ",saved[0]+1,vn[saved[0]]+1);
               }
               else if ((cptexture) && (!norms))
               {
                    fprintf(output_file,"\nt %d/%d %d/%d %d/%d ",x+1,vt[x] +1,
                                                                    y+1,vt[y] +1,
                                                                    saved[1]+1,vt[saved[1]]+1);
                    fprintf(output_file,"%d//%d ",saved[0]+1,vt[saved[0]]+1);
               }
               else
               {
                    fprintf(output_file,"\nt %d/%d/%d %d/%d/%d %d/%d/%d ",x+1,vt[x]+1,vn[x] +1,
                                                                    y+1,vt[y]+1,vn[y] +1,
                                                                    saved[1]+1,vt[saved[1]]+1,vn[saved[1]]+1);
                    fprintf(output_file,"%d/%d/%d ",saved[0]+1,vt[saved[0]]+1,vn[saved[0]]+1);
               }

               x = Adjacent(saved[0],saved[1],temp2->pPolygon,4);
               y = Adjacent(saved[1],saved[0],temp2->pPolygon,4);

               /*   Data might be mixed and we do not have textures for some of the vertices */
               if ((texture) && ( (vt[x] == 0) || (vt[y]==0)))
               {
                    if (cptexture)
                         fprintf(output_file,"\nq ");
                    cptexture = FALSE;
               }
               if ((!norms) && (!cptexture))
               {
                    fprintf(output_file,"%d ",x+1);
                    fprintf(output_file,"%d ",y+1);
               }
               else if ((norms) && (!cptexture))
               {
                    fprintf(output_file,"%d//%d ",x+1,vn[x]+1);
                    fprintf(output_file,"%d//%d ",y+1,vn[y]+1);
               }
               else if ((cptexture) && (!norms))
               {
                    fprintf(output_file,"%d/%d ",x+1,vt[x]+1);
                    fprintf(output_file,"%d/%d ",y+1,vt[y]+1);
               }
               else
               {
                    fprintf(output_file,"%d/%d/%d ",x+1,vt[x]+1,vn[x]+1);
                    fprintf(output_file,"%d/%d/%d ",y+1,vt[y]+1,vn[y]+1);
               }

               output1 = x;
               output2 = y;
          }
          
          else 
          {
               x = Adjacent(output2,output1,temp2->pPolygon,4);
               y = Adjacent(output1,output2,temp2->pPolygon,4);
               /*   Data might be mixed and we do not have textures for some of the vertices */
               if ((texture) && ( ((vt[x]) == 0) || ((vt[y])==0) ))
                    texture = FALSE;

               if ((!norms) && (!texture))
               {
                    fprintf(output_file,"\nq %d ",x+1);
                    fprintf(output_file,"%d ",y+1);
               }
               else if ((norms) && (!texture))
               {
                    fprintf(output_file,"\nq %d//%d ",x+1,vn[x]+1);
                    fprintf(output_file,"%d//%d ",y+1,vn[y]+1);
               }
               else if ((texture) && (!norms))
               {
                    fprintf(output_file,"\nq %d/%d ",x+1,vt[x]+1);
                    fprintf(output_file,"%d/%d ",y+1,vt[y]+1);
               }
               else
               {
                    fprintf(output_file,"\nq %d/%d/%d ",x+1,vt[x]+1,vn[x]+1);
                    fprintf(output_file,"%d/%d/%d ",y+1,vt[y]+1,vn[y]+1);
               }
               
               output1 = x;
               output2 = y;
          }
          
          last_quad[0] = *(temp2->pPolygon);
          last_quad[1] = *(temp2->pPolygon+1);
          last_quad[2] = *(temp2->pPolygon+2);
          last_quad[3] = *(temp2->pPolygon+3);
     }
}

void Fast_Reset(int x)
{
	register int y,numverts;
	register int front_walk, back_walk;
	ListHead *pListHead;
	PF_FACES temp = NULL;

     pListHead = PolFaces[x];
	temp = (PF_FACES) PeekList(pListHead,LISTHEAD,0);
	numverts = temp->nPolSize;
     
     front_walk = 0; 
     back_walk = 0;          
     resetting = TRUE;
          
     /* we are doing this only for quads */
	if (numverts == 4)
	{
			/* 	for each face not seen yet, do North and South together
				and East and West together
			*/
			for (y=0;y<2;y++)
			{
				/* Check if the opposite sides were seen already */
	     		/* Find walk for the first edge */
				front_walk = Calculate_Walks(x,y,temp);
				/* Find walk in the opposite direction */
				back_walk = Calculate_Walks(x,y+2,temp);
				/* 	Now put into the data structure the numbers that
		          	we have found
				*/
                    Assign_Walk(x,temp,front_walk,y,back_walk);
				Assign_Walk(x,temp,back_walk,y+2,front_walk);
			}
	}
     resetting = FALSE;
}


void Reset_Max(PF_FACES temp2,int face_id,int north,int last_north, int orientation,
		int last_left,FILE *output_file,int color1,int color2,int color3,
		BOOL start)
{
	register int walk = 0,count = 0;
	int previous_edge1,previous_edge2;
	int static last_seen = 1000;
	F_EDGES *node;
	ListHead *pListHead;
	int f,t,nextvert,counter;
     BOOL flag;

 
     /*   Reset walks on faces, since we just found a patch */
     if (orientation !=3)
     {
                previous_edge1 = *(temp2->pPolygon + orientation+1);
                previous_edge2 = *(temp2->pPolygon + orientation );
     }
     else
     {
                previous_edge1 = *(temp2->pPolygon + orientation );
                previous_edge2 = *(temp2->pPolygon);
     }

	/* only if we are going left, otherwise there will be -1 there */
	/*Find the adjacent face to this edge */
        
     for (t = 0; t <=3 ; t++)
     {
             node = *(temp2->VertandId+t);
              
             if (face_id == node->edge[1])
                f = node->edge[2];
             else
               f = node->edge[1];
       
             if (f != -1)
                  Fast_Reset(f);
        }

        node = *(temp2->VertandId+orientation);
        if (face_id == node->edge[1])
             nextvert = node->edge[2];
        else
             nextvert = node->edge[1];

	while ((last_left--) > 1)
	{
               
          if (start)
               Reset_Max(temp2,face_id,orientation,last_left,north,last_north,output_file,color1,color2,color3,FALSE);		
        
          face_id = nextvert;
          pListHead = PolFaces[nextvert];                
          temp2 = (PF_FACES) PeekList(pListHead,LISTHEAD,0);
          if ((temp2->nPolSize != 4) && (temp2->nPolSize != 1))
          {
             /*   There is more than 2 polygons on the edge, and we could have
                  gotten the wrong one
             */
             if (nextvert != node->edge[1])
                  nextvert = node->edge[1];
             else
                  nextvert = node->edge[2];
             pListHead = PolFaces[nextvert];          
             temp2 = (PF_FACES) PeekList(pListHead,LISTHEAD,0);
             node = *(temp2->VertandId+orientation);
          }

                
         if (!start)
         {
             for (t = 0; t <=3 ; t++)
             {
                    node = *(temp2->VertandId+t);
              
                    if (face_id == node->edge[1])
                         f = node->edge[2];
                    else
                         f = node->edge[1];
          
                    if (f != -1)
                         Fast_Reset(f);
             }
        }


        counter = 0;
	   flag = TRUE;
	   while ((counter < 3) && (flag))
        {
             if ( ((*(temp2->pPolygon+counter) == previous_edge1) ||
                   (*(temp2->pPolygon+counter+1) == previous_edge2)) ||
                  ((*(temp2->pPolygon+counter) == previous_edge2) ||
                   (*(temp2->pPolygon+counter+1) == previous_edge1)) )
                   counter++;
             else
                  flag = FALSE;
        }

        /* Get the IDs of the next edge */
        if (counter < 3)
        {
             previous_edge1 = *(temp2->pPolygon + counter+1);
             previous_edge2 = *(temp2->pPolygon + counter);
        }
        else
        {
             previous_edge1 = *(temp2->pPolygon + counter);
             previous_edge2 = *(temp2->pPolygon);
        }
        orientation = counter;

        node = *(temp2->VertandId + counter);
	   if (node->edge[1] == nextvert)
			nextvert = node->edge[2];
	   else
			nextvert = node->edge[1];

        if (!reversed)
        {
               if (counter != 3)
			     north = counter +1;
		     else
			     north = 0;
        }
        else
        {
               if (counter != 0)
                    north = counter -1;
               else
                    north = 3;
    
        }
     }
if (start)
	Reset_Max(temp2,face_id,orientation,last_left,north,last_north,output_file,color1,color2,color3,FALSE);
else if (nextvert != -1)       
     Fast_Reset(nextvert);

}


int Peel_Max(PF_FACES temp2,int face_id,int north,int last_north, int orientation,
		int last_left,FILE *output_file,int color1,int color2,int color3,
		BOOL start, int *swaps_added, int norms, int texture)
{
	int end1,end2,last_id,s=0,walk = 0,count = 0;
	int previous_edge1,previous_edge2;
	int static last_seen = 1000;
	F_EDGES *node;
	ListHead *pListHead;
	int nextvert,numverts,counter,dummy,tris=0;
     BOOL flag;

     /*   Peel the patch from the model.
          We will try and extend off the end of each strip in the patch. We will return the
          number of triangles completed by this extension only, and the number of swaps
          in the extension only.
     */	
     patch = 0;
     
     if (orientation !=3)
     {
                previous_edge1 = *(temp2->pPolygon + orientation+1);
                previous_edge2 = *(temp2->pPolygon + orientation );
     }
     else
     {
                previous_edge1 = *(temp2->pPolygon + orientation );
                previous_edge2 = *(temp2->pPolygon);
     }


     walk = *(temp2->walked + orientation);
	
     /* only if we are going left, otherwise there will be -1 there */
	if ((start) && ((walk+1) < last_left))
	{
		printf("There is an error in the left %d %d\n",walk,last_left);
		exit(0);
	}
	
     /* Find the adjacent face to this edge */
     node = *(temp2->VertandId+orientation);
     if (face_id == node->edge[1])
          nextvert = node->edge[2];
     else
          nextvert = node->edge[1];
	temp2->seen = last_seen;


	while ((last_left--) > 1)
	{
 		if (start)
             tris += Peel_Max(temp2,face_id,orientation,last_left,north,last_north,output_file,
                              color1,color2,color3,FALSE,swaps_added,norms,texture);		    
          else
             Mark_Face(temp2,color1,color2,color3,output_file,FALSE,&dummy,&dummy,&face_id,norms,texture);
		

          pListHead = PolFaces[nextvert];      
          temp2 = (PF_FACES) PeekList(pListHead,LISTHEAD,0);
          numverts = temp2->nPolSize;
	
          if ((numverts != 4) || (temp2->seen == last_seen) 
			||  (nextvert == -1))
          {
  	
             /*   There is more than 2 polygons on the edge, and we could have
                  gotten the wrong one
             */
             if (nextvert != node->edge[1])
                  nextvert = node->edge[1];
             else
                  nextvert = node->edge[2];
             pListHead = PolFaces[nextvert];
             temp2 = (PF_FACES) PeekList(pListHead,LISTHEAD,0);
             numverts = temp2->nPolSize;
             if ((numverts != 4) || (temp2->seen == last_seen) )
             {
                  printf("Peel 2 %d\n",numverts);
                  exit(1);
             }
        }

        face_id = nextvert;
        temp2->seen = last_seen;
                
        counter = 0;
        flag = TRUE;
	   while ((counter < 3) && (flag))
        {
             if ( ((*(temp2->pPolygon+counter) == previous_edge1) ||
                   (*(temp2->pPolygon+counter+1) == previous_edge2)) ||
                  ((*(temp2->pPolygon+counter) == previous_edge2) ||
                   (*(temp2->pPolygon+counter+1) == previous_edge1)) )
                   counter++;
             else
                  flag = FALSE;
        }
        /* Get the IDs of the next edge */
        if (counter < 3)
        {
             previous_edge1 = *(temp2->pPolygon + counter+1);
             previous_edge2 = *(temp2->pPolygon + counter);
        }
        else
        {
             previous_edge1 = *(temp2->pPolygon + counter);
             previous_edge2 = *(temp2->pPolygon);
        }
        orientation = counter;
		              
	   node = *(temp2->VertandId + counter);
	   if (node->edge[1] == nextvert)
			nextvert = node->edge[2];
	   else
			nextvert = node->edge[1];

	   if (!reversed)
        {
               if (counter != 3)
			     north = counter +1;
		     else
			     north = 0;
        }
        else
        {
               if (counter != 0)
                    north = counter -1;
               else
                    north = 3;
        }
}

if (start)
	tris += Peel_Max(temp2,face_id,orientation,last_left,north,last_north,output_file,
                        color1,color2,color3,FALSE,swaps_added,norms,texture);	
else
     Mark_Face(temp2,color1,color2,color3,output_file,FALSE,&dummy,&dummy,&face_id,norms,texture);/* do the last face */

last_seen++;

/*    Get the edge that we came out on the last strip of the patch */
Mark_Face(NULL,0,0,0,output_file,TRUE,&end1,&end2,&last_id,norms,texture);
tris += Extend_Face(last_id,end1,end2,&s,output_file,color1,color2,color3,vn,norms,vt,texture);
*swaps_added = *swaps_added + s;
return tris;
}



void Find_Bands(int numfaces, FILE *output_file, int *swaps, int *bands, 
                int *cost, int *tri, int norms, int *vert_norms, int texture, int *vert_texture)
{

	register int x,y,max1,max2,numverts,face_id,flag,maximum = 25;
	ListHead *pListHead;
	PF_FACES temp = NULL;
	int color1 = 0, color2 = 100, color3 = 255;
	int color = 0,larger,smaller;
	int north_length1,last_north,left_length1,last_left,north_length2,left_length2;
     int total_tri = 0, total_swaps = 0,last_id;
     int end1, end2,s=0;
     register int cutoff = 20;
    
     /*   Code that will find the patches. "Cutoff" will be
          the cutoff of the area of the patches that we will be allowing. After
          we reach this cutoff length, then we will run the local algorithm on the
          remaining faces.
     */

	/* 	For each faces that is left find the largest possible band that we can
		have with the remaining faces. Note that we will only be finding patches
          consisting of quads.
	*/

vn = vert_norms;
vt = vert_texture;
y=1;
*bands = 0;

while ((maximum >= cutoff))
{
	y++;
     maximum = -1;
	for (x=0; x<numfaces; x++)
	{ 
			
          /*   Used to produce the triangle strips */
               
          /* for each face, get the face */
		pListHead = PolFaces[x];
		temp = (PF_FACES) PeekList(pListHead,LISTHEAD,0);
		numverts = temp->nPolSize;
		
          /* we are doing this only for quads */
		if (numverts == 4)
		{
			/*   We want a face that is has not been used yet,
                    since we know that that face must be part of
				a band. Then we will find the largest band that
				the face may be contained in
			*/
			
               /*  Doing the north and the left */
			if ((*(temp->walked) != -1) && (*(temp->walked+3) != -1))
				max1 = Find_Max(temp,x,0,3,&north_length1,&left_length1);
			if ((*(temp->walked+1) != -1) && (*(temp->walked+2) != -1))
				max2 = Find_Max(temp,x,2,1,&north_length2,&left_length2);
			if ((max1 != (north_length1 * left_length1)) ||
			    (max2 != (north_length2 * left_length2)))
			{
				printf("Max1 %d, %d %d	Max2 %d, %d %d\n",max1,north_length1,left_length1,max2,north_length2,left_length2);
				exit(0);
			}
                
                    
               if ((max1 > max2) && (max1 > maximum))
			{
                    maximum = max1;
                    face_id = x;
                    flag = 1; 
                    last_north = north_length1;
                    last_left = left_length1;
                    /* so we know we saved max1 */
			}
			else if ((max2 > maximum) )
			{
                    maximum = max2;
                    face_id = x;
                    flag = 2; 
                    last_north = north_length2;
                    last_left = left_length2;
                    /* so we know we saved max2 */
               }
          }
     }
     if ((maximum < cutoff) && (*bands == 0))
          return;
     pListHead = PolFaces[face_id];
     temp = (PF_FACES) PeekList(pListHead,LISTHEAD,0);	
     /*   There are no patches that we found in this pass */
     if (maximum == -1)
          break;
     /*printf("The maximum is  face %d area %d: lengths %d %d\n",face_id,maximum,last_north,last_left);*/

     if (last_north > last_left)
     {
          larger = last_north;
          smaller = last_left;
     }
     else
     {
          larger = last_left;
          smaller = last_north;
     }

     length = larger;

if (flag == 1)
{
	if (last_north > last_left) /*     go north sequentially */
     {
          total_tri += Peel_Max(temp,face_id,0,last_north,3,last_left,output_file,color1,color2,color3,TRUE,&s,norms,texture);
          Reset_Max(temp,face_id,0,last_north,3,last_left,output_file,color1,color2,color3,TRUE);
          total_swaps += s;
     }
    else
    {
         reversed = TRUE;
         total_tri += Peel_Max(temp,face_id,3,last_left,0,last_north,output_file,color1,color2,color3,TRUE,&s,norms,texture);
         Reset_Max(temp,face_id,3,last_left,0,last_north,output_file,color1,color2,color3,TRUE);
         reversed = FALSE;
         total_swaps += s;
    }

     
    /*    Get the edge that we came out on the last strip of the patch */
    Mark_Face(NULL,0,0,0,NULL,TRUE,&end1,&end2,&last_id,norms,texture);
    total_tri += Extend_Face(last_id,end1,end2,&s,output_file,color1,color2,color3,vn,norms,vt,texture);
    total_swaps += s;

}
else
{
     if (last_north > last_left)
     {
          total_tri += Peel_Max(temp,face_id,2,last_north,1,last_left,output_file,color1,color2,color3,TRUE,&s,norms,texture); 
          Reset_Max(temp,face_id,2,last_north,1,last_left,output_file,color1,color2,color3,TRUE); 
          total_swaps += s;
     }
     else
     {
          reversed = TRUE;
          total_tri += Peel_Max(temp,face_id,1,last_left,2,last_north,output_file,color1,color2,color3,TRUE,&s,norms,texture);
          Reset_Max(temp,face_id,1,last_left,2,last_north,output_file,color1,color2,color3,TRUE);
          reversed = FALSE;
          total_swaps += s;
     }

     /*    Get the edge that we came out on on the patch */
    Mark_Face(NULL,0,0,0,NULL,TRUE,&end1,&end2,&last_id,norms,texture);
    total_tri += Extend_Face(last_id,end1,end2,&s,output_file,color1,color2,color3,vn,norms,vt,texture);
    total_swaps += s;
}

    /*  Now compute the cost of transmitting this band, is equal to 
        going across the larger portion sequentially,
        and swapping 3 times per other dimension
    */

total_tri += (maximum * 2);
*bands = *bands + smaller;

}

/*printf("We transmitted %d triangles,using %d swaps and %d strips\n",total_tri,
        total_swaps, *bands);
printf("COST %d\n",total_tri + total_swaps + *bands + *bands);*/
*cost = total_tri + total_swaps + *bands + *bands;
*tri = total_tri;
added_quad = added_quad * 4;
*swaps = total_swaps;
}

 
void Save_Rest(int *numfaces)
{
    /*  Put the polygons that are left into a data structure so that we can run the
        stripping code on it.
    */
    register int x,y=0,numverts;
    ListHead *pListHead;
    PF_FACES temp=NULL;

    for (x=0; x<*numfaces; x++)
    { 
			/* for each face, get the face */
			pListHead = PolFaces[x];
			temp = (PF_FACES) PeekList(pListHead,LISTHEAD,0);
			numverts = temp->nPolSize;
               /*  If we did not do the face before add it to data structure with new 
                   face id number
               */
               if (numverts != 1)
               {
                   CopyFace(temp->pPolygon,numverts,y+1,temp->pNorms);
                   y++;
               }
               /*   Used it, so remove it */
               else
                    RemoveList(pListHead,(PLISTINFO) temp);

    }
    *numfaces = y;
}

void Assign_Walk(int lastvert,PF_FACES temp2, int front_walk,int y,
				int back_walk)
{
/*      Go back and do the walk again, but this time save the lengths inside
        the data structure.
        y was the starting edge number for the front_walk length
        back_walk is the length of the walk along the opposite edge
 */
        int previous_edge1, previous_edge2;
        register int walk = 0,nextvert,numverts,counter;
        BOOL flag;
        F_EDGES *node;
        ListHead *pListHead;
        register int total_walk, start_back_walk;
        static int seen = 0;
        static BOOL first = TRUE;         
        int test;
        BOOL f = TRUE, wrap = FALSE, set = FALSE;
             test = lastvert;

        /*     In the "Fast_Reset" resetting will be true */
        if ((resetting) && (first))
        {
             seen = 0;
             first = FALSE;
        }

        seen++;
        total_walk = front_walk + back_walk;
        start_back_walk = back_walk;
        /*     Had a band who could be a cycle  */
        if (front_walk == back_walk)
             wrap = TRUE;
             
        /* Find the edge that we are currently on */
        if (y != 3)
        {
                previous_edge1 = *(temp2->pPolygon +y);
                previous_edge2 = *(temp2->pPolygon + y + 1);
        }
        else
        {
                previous_edge1 = *(temp2->pPolygon +y);
                previous_edge2 = *(temp2->pPolygon);
        }

        /* Assign the lengths */
	   if (y < 2) 
	   {
                *(temp2->walked+y) = front_walk--;
         		 *(temp2->walked+y+2) = back_walk++;
    	   }
	   else
	   {				
               *(temp2->walked+y) = front_walk--;
              	*(temp2->walked+y-2) = back_walk++;
  	   }

	   /*Find the adjacent face to this edge */
        node = *(temp2->VertandId+y);                   

        if (node->edge[2] != lastvert)
          nextvert = node->edge[2];
        else
          nextvert = node->edge[1];
                                       
        temp2->seen3 = seen;
        
        /* Keep walking in this direction until we cannot do so */
        while ((nextvert != lastvert) && (nextvert != -1) && (front_walk >= 0))
        {
		     walk++;
               pListHead = PolFaces[nextvert];
               
               temp2 = (PF_FACES) PeekList(pListHead,LISTHEAD,0);
               numverts = temp2->nPolSize;
               if ((numverts != 4))
               {
                    nextvert = -1;
                    /* Don't include this face in the walk */
                    walk--;
               }
               else
               {
                    /* Find edge that is not adjacent to the previous one */
                    counter = 0;
                    flag = TRUE;
                    while ((counter < 3) && (flag))
                    {
                         if ( ((*(temp2->pPolygon+counter) == previous_edge1) ||
                               (*(temp2->pPolygon+counter+1) == previous_edge2)) ||
                              ((*(temp2->pPolygon+counter) == previous_edge2) ||
                               (*(temp2->pPolygon+counter+1) == previous_edge1)) )
                              counter++;
                         else
                              flag = FALSE;
                    }
                    /* Get the IDs of the next edge */
                    if (counter < 3)
                    {
                         previous_edge1 = *(temp2->pPolygon + counter);
                         previous_edge2 = *(temp2->pPolygon + counter + 1);
                    }
                    else
                    {
                         previous_edge1 = *(temp2->pPolygon + counter);
                         previous_edge2 = *(temp2->pPolygon);
                    }
               

		          /*      Put in the walk lengths */
		          if (counter < 2)
		          {
                        if (((*(temp2->walked + counter) >= 0)
			          || (*(temp2->walked +counter + 2) >= 0)))
			          {
				          if ((resetting == FALSE) && ((temp2->seen3) != (seen-1)))
				          {
					          /*   If there are more than 2 polygons adjacent
                                        to an edge then we can be trying to assign more than
                                        once. We will save the smaller one
                                   */
                                   temp2->seen3 = seen;
                                   if ( (*(temp2->walked+counter) <= front_walk) &&
                                        (*(temp2->walked+counter+2) <= back_walk) )
                                        return;
                                   if (*(temp2->walked+counter) > front_walk)
                                       *(temp2->walked+counter) = front_walk--;
                                   else
                                        front_walk--;
                                   if (*(temp2->walked+counter+2) > back_walk)
                                       *(temp2->walked+counter+2) = back_walk++;
                                   else
                                        back_walk++;
				          }
				          else if (resetting == FALSE)
				          {
					          /* if there was a cycle then all lengths are the same */
					          walk--;
					          back_walk--;
					          front_walk++;
                                   temp2->seen3 = seen;
                                   *(temp2->walked+counter) = front_walk--;
                                   *(temp2->walked+counter+2) = back_walk++;
                              }
                              else if (((temp2->seen3 == (seen-1))
                                   && (wrap) && (walk == 1)) || (set))
                              {
					          /* if there was a cycle then all lengths are the same */
					          set = TRUE;
                                   walk--;
					          back_walk--;
					          front_walk++;
                                   temp2->seen3 = seen;
                                   *(temp2->walked+counter) = front_walk--;
                                   *(temp2->walked+counter+2) = back_walk++;
                              }
                              else
                              {
                                   temp2->seen3 = seen;
                                   *(temp2->walked+counter) = front_walk--;
                                   *(temp2->walked+counter+2) = back_walk++;
                              }
                        } /* if was > 0 */	
                        else
                        {
                             temp2->seen3 = seen;
                             *(temp2->walked+counter) = front_walk--;
                             *(temp2->walked+counter+2) = back_walk++;
                        }
                    }
		
               else
               {
                    if (((*(temp2->walked + counter) >= 0 )
                        || (*(temp2->walked +counter - 2) >= 0)) )
                    {
                         if ((temp2->seen3 != (seen-1))  && (resetting == FALSE))
                         {
                              /*   If there are more than 2 polygons adjacent
                                   to an edge then we can be trying to assign more than
                                   once. We will save the smaller one
                              */
                              temp2->seen3 = seen;
                              if ( (*(temp2->walked+counter) <= front_walk) &&
                                   (*(temp2->walked+counter-2) <= back_walk) )
                                   return;
                              if (*(temp2->walked+counter) > front_walk)
                                   *(temp2->walked+counter) = front_walk--;
                              else
                                   front_walk--;
                              if (*(temp2->walked+counter-2) > back_walk)
                                   *(temp2->walked+counter-2) = back_walk++;
                              else
                                   back_walk++;
                        	}
				     else if (resetting == FALSE)
	     			{
     					walk--;
		     			back_walk--;
			     		front_walk++;
	                         temp2->seen3 = seen;
                              *(temp2->walked+counter) = front_walk--;
                              *(temp2->walked+counter-2) = back_walk++;
                         }
                         else if (((temp2->seen3 == (seen-1)) && (walk == 1) && (wrap))
                              || (set))
                         {
					     /* if there was a cycle then all lengths are the same */
					     set = TRUE;
                              walk--;
					     back_walk--;
					     front_walk++;
                              temp2->seen3 = seen;
                              *(temp2->walked+counter) = front_walk--;
                              *(temp2->walked+counter-2) = back_walk++;
                         }
                         else
                         {
                              temp2->seen3 = seen;
                              *(temp2->walked+counter) = front_walk--;
                              *(temp2->walked+counter-2) = back_walk++;
                         }
     			}
                    else
                    {
                         temp2->seen3 = seen;
                         *(temp2->walked+counter) = front_walk--;
                         *(temp2->walked+counter-2) = back_walk++;
                    }
		                
  		     } 
		     if (nextvert != -1)
		     {
			     node = *(temp2->VertandId + counter);
                	if (node->edge[1] == nextvert)
                        	nextvert = node->edge[2];
                	else
                        	nextvert = node->edge[1];
               }
		
     }
}
if ((EVEN(seen)) )
     seen+=2;
}

void Save_Walks(int numfaces)
{
	int x,y,numverts;
	int front_walk, back_walk;
	ListHead *pListHead;
	PF_FACES temp = NULL;

	for (x=0; x<numfaces; x++)
	{ 
		/* for each face, get the face */
		pListHead = PolFaces[x];
		temp = (PF_FACES) PeekList(pListHead,LISTHEAD,0);
		numverts = temp->nPolSize;
		front_walk = 0; 
          back_walk = 0;

          /* we are finding patches only for quads */
		if (numverts == 4)
		{
			/* 	for each face not seen yet, do North and South together
				and East and West together
			*/
			for (y=0;y<2;y++)
			{
				/*   Check if the opposite sides were seen already from another
                         starting face, if they were then there is no need to do the walk again
                    */

				if 	( ((*(temp->walked+y) == -1) &&
					(*(temp->walked+y+2) == -1) ))
				{
					/* Find walk for the first edge */
					front_walk = Calculate_Walks(x,y,temp);
					/* Find walk in the opposite direction */
					back_walk = Calculate_Walks(x,y+2,temp);
					/* 	Now put into the data structure the numbers that
						we have found
					*/
                         Assign_Walk(x,temp,front_walk,y,back_walk);
					Assign_Walk(x,temp,back_walk,y+2,front_walk);
	     		}
			}
		}
	}
}


