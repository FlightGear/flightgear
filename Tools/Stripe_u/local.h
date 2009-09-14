/********************************************************************/
/*   STRIPE: converting a polygonal model to triangle strips    
     Francine Evans, 1996.
     SUNY @ Stony Brook
     Advisors: Steven Skiena and Amitabh Varshney
*/
/********************************************************************/

/*---------------------------------------------------------------------*/
/*   STRIPE:local.h
-----------------------------------------------------------------------*/

void Local_Polygon_Output();
void Local_Output_Tri();
int Different();
void Local_Non_Blind_Triangulate();
void Local_Blind_Triangulate();
void Local_Triangulate_Polygon();
void SGI_Strip();
