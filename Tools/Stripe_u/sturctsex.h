/********************************************************************/
/*   STRIPE: converting a polygonal model to triangle strips    
     Francine Evans, 1996.
     SUNY @ Stony Brook
     Advisors: Steven Skiena and Amitabh Varshney
*/
/********************************************************************/

/*---------------------------------------------------------------------*/
/*   STRIPE:sturctsex.h
-----------------------------------------------------------------------*/

#define EVEN(x) (((x) & 1) == 0)

BOOL Get_EdgeEx();
void add_vert_idEx();
void Update_FaceEx();
int Min_Face_AdjEx();
int Change_FaceEx();
void Delete_AdjEx();
int Number_AdjEx();
int Update_AdjacenciesEx();






