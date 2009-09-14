/********************************************************************/
/*   STRIPE: converting a polygonal model to triangle strips    
     Francine Evans, 1996.
     SUNY @ Stony Brook
     Advisors: Steven Skiena and Amitabh Varshney
*/
/********************************************************************/

/*---------------------------------------------------------------------*/
/*   STRIPE: outputex.h
-----------------------------------------------------------------------*/


#include "polverts.h"


#define TRIANGLE 3
#define MAGNITUDE 1000000

void Output_TriEx(int id1, int id2, int id3, FILE *output, int next_face, 
		  int flag, int where);
void Sgi_Test();
void Polygon_OutputEx(P_ADJACENCIES temp,int face_id,int bucket,
		      ListHead *pListHead, FILE *output, FILE *strips,
		      int *ties, int tie, int triangulate, int swaps,
		      int *next_id, int where);
void Extend_BackwardsEx(int face_id, FILE *output, FILE *strip, int *ties, 
			int tie, int triangulate, int swaps,int *next_id);
void FinishedEx();


