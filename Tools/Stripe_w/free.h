/********************************************************************/
/*   STRIPE: converting a polygonal model to triangle strips    
     Francine Evans, 1996.
     SUNY @ Stony Brook
     Advisors: Steven Skiena and Amitabh Varshney
*/
/********************************************************************/

/*---------------------------------------------------------------------*/
/*   STRIPE: free.h
-----------------------------------------------------------------------*/

void Free_All_Strips();
void ParseAndFreeList( ListHead *pListHead );
void FreePolygonNode( PF_VERTS pfVerts);
void Free_Strips();
void FreeFaceTable(int nSize);
void FreeEdgeTable(int nSize);
void End_Face_Struct(int numfaces);
void End_Edge_Struct(int numverts);


