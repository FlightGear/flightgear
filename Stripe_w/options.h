/********************************************************************/
/*   STRIPE: converting a polygonal model to triangle strips    
     Francine Evans, 1996.
     SUNY @ Stony Brook
     Advisors: Steven Skiena and Amitabh Varshney
*/
/********************************************************************/

/*---------------------------------------------------------------------*/
/*   STRIPE: options.h
-----------------------------------------------------------------------*/

double get_options(int argc, char **argv, int *f, int *t, int *tr, int *group);
enum file_options {ASCII,BINARY};
enum tie_options {FIRST, RANDOM, ALTERNATE, LOOK, SEQUENTIAL};
enum triangulation_options {PARTIAL,WHOLE};
     
