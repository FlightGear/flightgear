/********************************************************************/
/*   STRIPE: converting a polygonal model to triangle strips    
     Francine Evans, 1996.
     SUNY @ Stony Brook
     Advisors: Steven Skiena and Amitabh Varshney
*/
/********************************************************************/

/*---------------------------------------------------------------------*/
/*   STRIPE: global.h
-----------------------------------------------------------------------*/

#ifndef _GLOBAL_H
#define _GLOBAL_H


#define   VRDATA		double
#define   STRIP_MAX            60
	
#define	TRUE	      	1
#define	FALSE		0

#ifndef PI
#define   PI	     	3.1415926573
#endif

#define   ATOI(C)        (C -'0')
#define   X              0
#define   Y              1
#define   Z              2
#define   EVEN(x)       (((x) & 1) == 0)
#define   MAX_BAND      10000

struct vert_struct {
	VRDATA	x, y, z;	/* point coordinates */
};

extern int     ids[STRIP_MAX];
extern int     norms[STRIP_MAX];
extern int     *vert_norms;
extern int     *vert_texture;


#endif _GLOBAL_H
