/********************************************************************/
/*   STRIPE: converting a polygonal model to triangle strips    
     Francine Evans, 1996.
     SUNY @ Stony Brook
     Advisors: Steven Skiena and Amitabh Varshney
*/
/********************************************************************/

/*---------------------------------------------------------------------*/
/*   STRIPE: output.h
-----------------------------------------------------------------------*/


#include "polverts.h"

#define TRIANGLE 3
#define MAGNITUDE 1000000

void Output_Tri(int id1, int id2, int id3,FILE *bands, int color1, 
		int color2, int color3,BOOL end);
void Sgi_Test();
int Polygon_Output(P_ADJACENCIES temp,int face_id,int bucket,
		   ListHead *pListHead, BOOL first, int *swaps,
		   FILE *bands,int color1,int color2,int color3,
		   BOOL global, BOOL end);
void Last_Edge();
void Extend_Backwards();
int Finished(int *swap, FILE *output, BOOL global);
int Extend_Face(int face_id,int e1,int e2,int *swaps,FILE *bands,
                int color1,int color2,int color3,int *vert_norm, int normals,
                int *vert_texture, int texture);
void Fast_Reset();


