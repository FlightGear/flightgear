/********************************************************************/
/*   STRIPE: converting a polygonal model to triangle strips    
     Francine Evans, 1996.
     SUNY @ Stony Brook
     Advisors: Steven Skiena and Amitabh Varshney
*/
/********************************************************************/

/*---------------------------------------------------------------------*/
/*   STRIPE: polvertsex.h
-----------------------------------------------------------------------*/

#include "queue.h"
#include <malloc.h>


/*      external functions */
void Start_Vert_Struct();
void Start_Face_StructEx();
void Start_Edge_StructEx();
void AddNewNode();
void AddNewFaceEx();      
void Find_AdjacenciesEx();
void Test_Adj_Struct();
void Test_SGI_Struct();
void Write_Edges();
void End_Verts_Struct();
void End_Face_StructEx();
void End_Edge_StructEx();
void Build_SGI_TableEx();
void Add_AdjEdgeEx();



