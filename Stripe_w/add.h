/********************************************************************/
/*   STRIPE: converting a polygonal model to triangle strips    
     Francine Evans, 1996.
     SUNY @ Stony Brook
     Advisors: Steven Skiena and Amitabh Varshney
*/
/********************************************************************/

/*---------------------------------------------------------------------*/
/*   STRIPE: add.h
-----------------------------------------------------------------------*/

#include "global.h"

BOOL new_vertex(double difference, int id1,int id2,
                struct vert_struct *n);
BOOL Check_VN(int vertex,int normal, struct vert_added *added);
BOOL norm_array(int id, int vertex, double normal_difference,
                struct vert_struct *n, int num_vert);
void add_texture(int id,BOOL vertex);
int	add_vert_id(int id, int	index_count);
void	add_norm_id(int id, int index_count);
void AddNewFace(int ids[STRIP_MAX], int vert_count, int face_id, 
		int norms[STRIP_MAX]);
void CopyFace(int ids[STRIP_MAX], int vert_count, int face_id, 
	      int norms[STRIP_MAX]);
void Add_Edge(int v1,int v2);
void Add_AdjEdge(int v1,int v2,int fnum,int index1 );



