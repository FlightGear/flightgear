
/********************************************************************/
/*   STRIPE: converting a polygonal model to triangle strips    
     Francine Evans, 1996.
     SUNY @ Stony Brook
     Advisors: Steven Skiena and Amitabh Varshney
*/
/********************************************************************/

/*---------------------------------------------------------------------*/
/*   STRIPE: triangulate.h
-----------------------------------------------------------------------*/

void Blind_Triangulate(int size, int *index, FILE *output,
		       BOOL begin, int where ,int color1,int color2,
                       int color3);
void Non_Blind_Triangulate(int size,int *index, FILE *output,
			   int next_face_id,int face_id,int where,
                           int color1,int color2,int color3);
int Adjacent(int id2,int id1, int *list, int size);
void Delete_From_List(int id,int *list, int *size);
void Triangulate_Polygon(int out_edge1, int out_edge2, int in_edge1,
			 int in_edge2, int size, int *index,
			 FILE *output, int reversed, int face_id,
                         int where, int color1, int color2, int color3);
void Rearrange_Index(int *index, int size);
void Find_Local_Strips();
