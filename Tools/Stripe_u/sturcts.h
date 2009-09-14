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

BOOL Get_Edge();
void add_vert_id();
void Update_Face();
int Min_Adj();
int Min_Face_Adj();
int Change_Face();
void Delete_Adj();
int Update_Adjacencies();
int Get_Output_Edge();
int Find_Face();







