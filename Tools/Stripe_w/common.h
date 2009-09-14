/********************************************************************/
/*   STRIPE: converting a polygonal model to triangle strips    
     Francine Evans, 1996.
     SUNY @ Stony Brook
     Advisors: Steven Skiena and Amitabh Varshney
*/
/********************************************************************/

/*---------------------------------------------------------------------*/
/*   STRIPE: common.h
-----------------------------------------------------------------------*/

void Add_AdjEdge(int v1,int v2,int fnum,int index1 );
void Find_Adjacencies(int num_faces);
void Add_Sgi_Adj(int bucket,int face_id);
int Num_Adj(int id1, int id2);
void Add_Id_Strips(int id, int where);
BOOL Look_Up(int id1,int id2,int face_id);
int Number_Adj(int id1, int id2, int curr_id);
int  Old_Adj(int face_id);
int Min_Adj(int id);
int Find_Face(int current_face, int id1, int id2, int *bucket);
void Edge_Least(int *index,int *new1,int *new2,int face_id,int size);
void Get_Input_Edge(int *index,int id1,int id2,int id3,int *new1,int *new2,
		    int size, int face_id);
int Get_Output_Edge(int face_id, int size, int *index,int id2,int id3);
void Check_In_Polygon(int face_id, int *min, int size);
void  Check_In_Quad(int face_id,int *min);
void New_Size_Face (int face_id);
void New_Face (int face_id, int v1, int v2, int v3);












