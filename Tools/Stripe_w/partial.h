/********************************************************************/
/*   STRIPE: converting a polygonal model to triangle strips    
     Francine Evans, 1996.
     SUNY @ Stony Brook
     Advisors: Steven Skiena and Amitabh Varshney
*/
/********************************************************************/

/*---------------------------------------------------------------------*/
/*   STRIPE: partial.h
-----------------------------------------------------------------------*/

void Partial_Triangulate(int size,int *index, FILE *fp,
			 FILE *output,int next_face_id,int face_id,
			 int *next_id,ListHead *pListHead,
			 P_ADJACENCIES temp, int where);
void Inside_Polygon(int size,int *index,FILE *fp,FILE *output,
		    int next_face_id,int face_id,int *next_id,
		    ListHead *pListHead,P_ADJACENCIES temp, int where);

