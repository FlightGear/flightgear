/********************************************************************/
/*   STRIPE: converting a polygonal model to triangle strips    
     Francine Evans, 1996.
     SUNY @ Stony Brook
     Advisors: Steven Skiena and Amitabh Varshney
*/
/********************************************************************/

/*---------------------------------------------------------------------*/
/*   STRIPE: triangulatex.h
-----------------------------------------------------------------------*/

enum swap_type
{ ON, OFF};

void SGI_StripEx();
void Blind_TriangulateEx();
void Non_Blind_TriangulateEx();
int AdjacentEx();
void Delete_From_ListEx();
void Triangulate_PolygonEx();
void Rearrange_IndexEx();
void Find_StripsEx();
