/********************************************************************/
/*   STRIPE: converting a polygonal model to triangle strips    
     Francine Evans, 1996.
     SUNY @ Stony Brook
     Advisors: Steven Skiena and Amitabh Varshney
*/
/********************************************************************/

/*---------------------------------------------------------------------*/
/*   STRIPE: free.c
     This file contains the code used to free the data structures.
*/
/*---------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include "polverts.h"

void ParseAndFreeList( ListHead *pListHead )
{
    PLISTINFO value;
    register int c,num;
	  
    /*    Freeing a linked list */
    num = NumOnList(pListHead);
    for (c = 0; c< num; c++)
	     value =   RemHead(pListHead);
} 

void FreePolygonNode( PF_VERTS pfVerts)
{
	/*   Free a vertex node */
     if ( pfVerts->pPolygon )
		free( pfVerts->pPolygon );
	free( pfVerts );

}
 
void Free_Strips()
{
    P_STRIPS temp = NULL;

    /*    Free strips data structure */
    if (strips[0] == NULL)
         return;
    else
         ParseAndFreeList(strips[0]);
}

void FreeFaceNode( PF_FACES pfFaces)
{
	/*   Free face node */
     if ( pfFaces->pPolygon )
                free( pfFaces->pPolygon );
        free( pfFaces );
}


void FreeFaceTable(int nSize)
{
     register int nIndex;

     for ( nIndex=0; nIndex < nSize; nIndex++ )
     { 
    		if ( PolFaces[nIndex] != NULL ) 
             ParseAndFreeList( PolFaces[nIndex] );
     }
     free( PolFaces );
}

void FreeEdgeTable(int nSize)
{
        register int nIndex;

        for ( nIndex=0; nIndex < nSize; nIndex++ )
        {
                if ( PolEdges[nIndex] != NULL )
                        ParseAndFreeList( PolEdges[nIndex] );
        }
        free( PolEdges );
}


void Free_All_Strips()
{

	ListHead *pListHead;
	register int y;

	for (y =0; ; y++)
	{
		pListHead = all_strips[y];
		if (pListHead == NULL)
			return;
		else
			ParseAndFreeList(all_strips[y]);
	}
}

void End_Face_Struct(int numfaces)
{
     FreeFaceTable(numfaces);
}

void End_Edge_Struct(int numverts)
{
     FreeEdgeTable(numverts);
}	


