/********************************************************************/
/*   STRIPE: converting a polygonal model to triangle strips    
     Francine Evans, 1996.
     SUNY @ Stony Brook
     Advisors: Steven Skiena and Amitabh Varshney
*/
/********************************************************************/

/*---------------------------------------------------------------------*/
/*   STRIPE: output.h
-----------------------------------------------------------------------*/


#define TRIANGLE 3
#define MAGNITUDE 1000000

void Output_Tri();
void Sgi_Test();
int Polygon_Output();
void Last_Edge();
void Extend_Backwards();
int Finished();
int Extend_Face();
void Fast_Reset();


