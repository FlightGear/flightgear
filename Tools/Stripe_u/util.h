/********************************************************************/
/*   STRIPE: converting a polygonal model to triangle strips    
     Francine Evans, 1996.
     SUNY @ Stony Brook
     Advisors: Steven Skiena and Amitabh Varshney
*/
/********************************************************************/

/*---------------------------------------------------------------------*/
/*   STRIPE: util.h
-----------------------------------------------------------------------*/

void switch_lower ();
int Compare ();
BOOL Exist();
int Get_Next_Id();
int Different();
int Return_Other();
int Get_Other_Vertex();
PLISTINFO Done();
void Output_Edge();
void Last_Edge();
void First_Edge();
BOOL member();
