
/********************************************************************/
/*   STRIPE: converting a polygonal model to triangle strips    
     Francine Evans, 1996.
     SUNY @ Stony Brook
     Advisors: Steven Skiena and Amitabh Varshney
*/
/********************************************************************/

/*---------------------------------------------------------------------*/
/*   STRIPE: triangulate.h
-----------------------------------------------------------------------*/

void Blind_Triangulate();
void Non_Blind_Triangulate();
int Adjacent();
void Delete_From_List();
void Triangulate_Polygon();
void Rearrange_Index();
void Find_Local_Strips();



