/********************************************************************/
/*   STRIPE: converting a polygonal model to triangle strips    
     Francine Evans, 1996.
     SUNY @ Stony Brook
     Advisors: Steven Skiena and Amitabh Varshney
*/
/********************************************************************/

/*---------------------------------------------------------------------*/
/*   STRIPE: sturcts.h
-----------------------------------------------------------------------*/

#define EVEN(x) (((x) & 1) == 0)

int Get_Edge(int *edge1,int *edge2,int *index,int face_id,
	     int size, int id1, int id2);
void add_vert_id();
void Update_Face(int *next_bucket, int *min_face, int face_id, int *e1,
		 int *e2,int temp1,int temp2,int *ties);
int Min_Adj();
int Min_Face_Adj(int face_id, int *next_bucket, int *ties);
int Change_Face(int face_id,int in1,int in2, ListHead *pListHead, 
		P_ADJACENCIES temp, BOOL no_check);
void Delete_Adj(int id1, int id2,int *next_bucket,int *min_face, 
		int current_face,int *e1,int *e2,int *ties);
int Update_Adjacencies(int face_id, int *next_bucket, int *e1, int *e2,
		       int *ties);
int Get_Output_Edge();
int Find_Face();







