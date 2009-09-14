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

void init_vert_norms(int num_vert);
void init_vert_texture(int num_vert);
BOOL InitVertTable( int nSize );
BOOL InitFaceTable( int nSize );
BOOL InitEdgeTable( int nSize );
void InitStripTable(  );
void Init_Table_SGI();
void BuildVertTable( int nSize );
void BuildFaceTable( int nSize );
void BuildEdgeTable( int nSize );
void Start_Face_Struct(int numfaces);
void Start_Edge_Struct(int numverts);






