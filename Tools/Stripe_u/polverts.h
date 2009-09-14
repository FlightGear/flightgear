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

#include "queue.h"
#include <malloc.h>


/*      external functions */
void Find_Adjacencies();
void Test_Adj_Struct();
void Test_SGI_Struct();
void Write_Edges();
void Build_SGI_Table();
void Save_Walks();
void Find_Bands();
void Save_Rest();
void Assign_Walk();
void Save_Walks();
	
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


/*      Globals */
ListHead **PolVerts;
ListHead **PolFaces;
ListHead **PolEdges;
ListHead *array[60];
int     id_array[60];
ListHead *strips[1];
ListHead *all_strips[100000]; /*  Assume max 100000 strips */
