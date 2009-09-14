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
void ParseAndFreeList();
void FreePolygonNode();
void Free_Strips();
void FreeFaceTable();
void FreeEdgeTable();
void End_Face_Struct();
void End_Edge_Struct();


