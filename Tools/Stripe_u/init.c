/********************************************************************/
/*   STRIPE: converting a polygonal model to triangle strips    
     Francine Evans, 1996.
     SUNY @ Stony Brook
     Advisors: Steven Skiena and Amitabh Varshney
*/
/********************************************************************/

/*---------------------------------------------------------------------*/
/*   STRIPE: init.c
     This file contains the initialization of data structures.
*/
/*---------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include "global.h"
#include "polverts.h"

void init_vert_norms(int num_vert)
{
     /*   Initialize vertex/normal array to have all zeros to
          start with.
     */
     register int x;

     for (x = 0; x < num_vert; x++)
          *(vert_norms + x) = 0;
}

void init_vert_texture(int num_vert)
{
     /*   Initialize vertex/normal array to have all zeros to
          start with.
     */
     register int x;

     for (x = 0; x < num_vert; x++)
          *(vert_texture + x) = 0;
}

BOOL InitVertTable( int nSize )
{
     register int nIndex;

	/*   Initialize the vertex table */
     PolVerts = (ListHead**) malloc(sizeof(ListHead*) * nSize ); 
	if ( PolVerts )
	{
		for ( nIndex=0; nIndex < nSize; nIndex++ )
		{
			PolVerts[nIndex] = NULL;
		}
		return( TRUE );	
	}
	return( FALSE );
}  

BOOL InitFaceTable( int nSize )
{
        register int nIndex;

        /*     Initialize the face table */
        PolFaces = (ListHead**) malloc(sizeof(ListHead*) * nSize ); 
        if ( PolFaces )
        {
                for ( nIndex=0; nIndex < nSize; nIndex++ )
                {
                        PolFaces[nIndex] = NULL;
                }
                return( TRUE );
        }
        return( FALSE );
} 

BOOL InitEdgeTable( int nSize )
{
        register int nIndex;

        /*     Initialize the edge table */
        PolEdges = (ListHead**) malloc(sizeof(ListHead*) * nSize );
        if ( PolEdges )
        {
                for ( nIndex=0; nIndex < nSize; nIndex++ )
                {
                        PolEdges[nIndex] = NULL;
                }
                return( TRUE );
        }
        return( FALSE );
}


void InitStripTable(  )
{

     PLISTHEAD pListHead;

     /*   Initialize the strip table */
     pListHead = ( PLISTHEAD ) malloc(sizeof(ListHead));
	if ( pListHead )
     {
		InitList( pListHead );
		strips[0] = pListHead;
	}
     else
	{
	     printf("Out of memory !\n");
		exit(0);
	}

}

void Init_Table_SGI()
{
	PLISTHEAD pListHead;
	int max_adj = 60;
	register int x;
	
	/*   This routine will initialize the table that will
		have the faces sorted by the number of adjacent polygons
		to it.
	*/

	for (x=0; x< max_adj; x++)
	{
		/*   We are allowing the max number of sides of a polygon
			to be max_adj.
		*/                      
		pListHead = ( PLISTHEAD ) malloc(sizeof(ListHead));
		if ( pListHead )
	     {
			InitList( pListHead );
			array[x] = pListHead;
		}
	     else
		{
		     printf("Out of memory !\n");
			exit(0);
		}
	}
}

void BuildVertTable( int nSize )
{
     register int nIndex;
     PLISTHEAD pListHead;
	
	for ( nIndex=0; nIndex < nSize; nIndex++ )
	{
		pListHead = ( PLISTHEAD ) malloc(sizeof(ListHead));
		if ( pListHead )
        	{
			InitList( pListHead );
			PolVerts[nIndex] = pListHead;
		}
        	else
        		return;	
		
	}
}
   

void BuildFaceTable( int nSize )
{
        register int nIndex;
        PLISTHEAD pListHead;
        
        for ( nIndex=0; nIndex < nSize; nIndex++ )
        {
                pListHead = ( PLISTHEAD ) malloc(sizeof(ListHead));
                if ( pListHead )
                {
                        InitList( pListHead );
                        PolFaces[nIndex] = pListHead;
                }
                else
                        return; 
                
        }
}
   
void BuildEdgeTable( int nSize )
{
        register int nIndex;
        PLISTHEAD pListHead;

        for ( nIndex=0; nIndex < nSize; nIndex++ )
        {
                pListHead = ( PLISTHEAD ) malloc(sizeof(ListHead));
                if ( pListHead )
                {
                        InitList( pListHead );
                        PolEdges[nIndex] = pListHead;
                }
                else
                        return;
        }
}

void Start_Face_Struct(int numfaces)
{
	if (InitFaceTable(numfaces))
	{
		BuildFaceTable(numfaces);
	}
}

void Start_Edge_Struct(int numverts)
{
        if (InitEdgeTable(numverts))
        {
                BuildEdgeTable(numverts);
        }
}


