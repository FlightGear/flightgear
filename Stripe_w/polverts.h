/********************************************************************/
/*   STRIPE: converting a polygonal model to triangle strips    
     Francine Evans, 1996.
     SUNY @ Stony Brook
     Advisors: Steven Skiena and Amitabh Varshney
*/
/********************************************************************/

/*---------------------------------------------------------------------*/
/*   STRIPE: polverts.h
-----------------------------------------------------------------------*/


#ifndef _POLVERTS_H
#define _POLVERTS_H


#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include "queue.h"

#ifdef HAVE_STDLIB_H
#  include <stdlib.h>
#else
#  include <malloc.h>
#endif


typedef struct adjacencies
{
	Node ListNode;
	int face_id;
} ADJACENCIES,*P_ADJACENCIES;

typedef struct FVerts
{
	Node ListNode;
	int *pPolygon;
	int nPolSize;
	int nId;
} F_VERTS, *PF_VERTS;

/*Every time we need to use this, cast it ( ListInfo*)*/

typedef struct FEdges
{
	Node ListNode;
	int edge[3];
}F_EDGES,*PF_EDGES;

typedef struct FFaces
{
	Node ListNode;
	int *pPolygon;
	int *pNorms;
    int     seen;
    int seen2;
    int seen3;
	int nPolSize;
	F_EDGES **VertandId;
	int *marked;
		int *walked;
} F_FACES,*PF_FACES;
	

typedef struct Strips
{
	Node ListNode;
	int face_id;
} Strips,*P_STRIPS;


     struct vert_added
     {
          int num;
          int *normal;
     };


/*      external functions */
void Find_Adjacencies(int num_faces);
void Test_Adj_Struct();
void Test_SGI_Struct();
void Write_Edges();
void Build_SGI_Table(int num_verts,int num_faces);
void Save_Walks(int numfaces);
void Find_Bands(int numfaces, FILE *output_file, int *swaps, int *bands, 
                int *cost, int *tri, int norms, int *vert_norms, int texture,
		int *vert_texture);
void Save_Rest(int *numfaces);
void Assign_Walk(int lastvert, PF_FACES temp2, int front_walk,int y,
				int back_walk);
void Save_Walks(int numfaces);
	

/*      Globals */
extern ListHead **PolVerts;
extern ListHead **PolFaces;
extern ListHead **PolEdges;
extern ListHead *array[60];
extern int     id_array[60];
extern ListHead *strips[1];
extern ListHead *all_strips[100000]; /*  Assume max 100000 strips */


#endif _POLVERTS_H
