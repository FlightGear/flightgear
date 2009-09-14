/********************************************************************/
/*   STRIPE: converting a polygonal model to triangle strips    
     Francine Evans, 1996.
     SUNY @ Stony Brook
     Advisors: Steven Skiena and Amitabh Varshney
*/
/********************************************************************/

/*---------------------------------------------------------------------*/
/*   STRIPE: init.h
-----------------------------------------------------------------------*/

void init_vert_norms();
void init_vert_texture();
BOOL InitVertTable();
BOOL InitFaceTable();
BOOL InitEdgeTable();
void InitStripTable();
void Init_Table_SGI();
void BuildVertTable();
void BuildFaceTable();
void BuildEdgeTable();
void Start_Face_Struct();
void Start_Edge_Struct();






