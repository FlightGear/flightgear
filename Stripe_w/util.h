/********************************************************************/
/*   STRIPE: converting a polygonal model to triangle strips    
     Francine Evans, 1996.
     SUNY @ Stony Brook
     Advisors: Steven Skiena and Amitabh Varshney
*/
/********************************************************************/

/*---------------------------------------------------------------------*/
/*   STRIPE: util.h
-----------------------------------------------------------------------*/

void switch_lower (int *x, int *y);
int Compare (P_ADJACENCIES node1, P_ADJACENCIES node2);
BOOL Exist(int face_id, int id1, int id2);
int Get_Next_Id(int *index,int e3, int size);
int Different(int id1,int id2,int id3,int id4,int id5, int id6, int *x, int *y);
int Return_Other(int *index,int e1,int e2);
int Get_Other_Vertex(int id1,int id2,int id3,int *index);
PLISTINFO Done(int face_id, int size, int *bucket);
void Output_Edge(int *index,int e2,int e3,int *output1,int *output2);
void Last_Edge(int *id1, int *id2, int *id3, BOOL save);
void First_Edge(int *id1,int *id2, int *id3);
BOOL member(int x , int id1, int id2, int id3);
