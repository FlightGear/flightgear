/********************************************************************/
/*   STRIPE: converting a polygonal model to triangle strips    
     Francine Evans, 1996.
     SUNY @ Stony Brook
     Advisors: Steven Skiena and Amitabh Varshney
*/
/********************************************************************/

/*---------------------------------------------------------------------*/
/*   STRIPE: local.c
     This file contains the code that initializes the data structures for
     the local algorithm, and starts the local algorithm going.
*/
/*---------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include "polverts.h"
#include "local.h"
#include "triangulatex.h"
#include "sturctsex.h"
#include "common.h"
#include "outputex.h"
#include "util.h"
#include "init.h"

void Find_StripsEx(FILE *output,FILE *strip,int *ties,
				 int  tie, int triangulate,
                 int  swaps,int *next_id)
{
	/*	This routine will peel off the strips from the model */

	ListHead *pListHead;
	P_ADJACENCIES temp = NULL;
	register int max,bucket=0;
	BOOL whole_flag = TRUE;
     int dummy = 0;
	
	/*  Set the last known input edge to be null */
     Last_Edge(&dummy,&dummy,&dummy,1);
    
     /*	Search for lowest adjacency polygon and output strips */
	while (whole_flag)
	{
		bucket = -1;
		/*	Search for polygons in increasing number of adjacencies */
		while (bucket < 59)
		{
			bucket++;
			pListHead = array[bucket];
			max = NumOnList(pListHead);
			if (max > 0)
			{
				temp = (P_ADJACENCIES) PeekList(pListHead,LISTHEAD,0);
				if (temp == NULL)
				{
					printf("Error in the buckets%d %d %d\n",bucket,max,0);
					exit(0);
				}
				Polygon_OutputEx(temp,temp->face_id,bucket,pListHead,
	       				       output,strip,ties,tie,triangulate,swaps,next_id,1);
				/*  Try to extend backwards, if the starting polygon in the
                        strip had 2 or more adjacencies to begin with
                    */
                    if (bucket >= 2)
                         Extend_BackwardsEx(temp->face_id,output,strip,ties,tie,triangulate,
                                            swaps,next_id);
                    break;  
			}
		}
		/*	Went through the whole structure, it is empty and we are done.
		*/
		if ((bucket == 59) && (max == 0))
			whole_flag = FALSE;
        
          /*  We just finished a strip, send dummy data to signal the end
              of the strip so that we can output it.
          */
         else
         {
             Output_TriEx(-1,-2,-3,output,-1,-10,1);
             Last_Edge(&dummy,&dummy,&dummy,1);
         }
	}
}



void SGI_Strip(int num_verts,int num_faces,FILE *output,
			   int ties,int triangulate)
               
{
	FILE *strip;
     int next_id = -1,t=0;

     strip = fopen("output.d","w");
     /*	We are going to output and find triangle strips
		according the the method that SGI uses, ie always
		choosing as the next triangle in our strip the triangle
		that has the least number of adjacencies. We do not have
		all triangles and will be triangulating on the fly those
		polygons that have more than 3 sides.
	*/

	/*	Build a table that has all the polygons sorted by the number
		of polygons adjacent to it.
	*/
	/*	Initialize it */
	Init_Table_SGI();
	/*	Build it */
	Build_SGI_Table(num_verts,num_faces);

	/*    We will have a structure to hold all the strips, until
          outputted.
     */
     InitStripTable();
     /*	Now we have the structure built to find the polygons according
		to the number of adjacencies. Now use the SGI Method to find
		strips according to the adjacencies
	*/
     Find_StripsEx(output,strip,&t,ties,triangulate,ON,&next_id);

}
