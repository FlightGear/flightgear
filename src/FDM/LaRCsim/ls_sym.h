/***************************************************************************

	TITLE:		ls_sym.h
	
----------------------------------------------------------------------------

	FUNCTION:	Header file for symbol table routines

----------------------------------------------------------------------------

	MODULE STATUS:	production

----------------------------------------------------------------------------

	GENEALOGY:	Created 930629 by E. B. Jackson

----------------------------------------------------------------------------

	DESIGNED BY:	Bruce Jackson
	
	CODED BY:	same
	
	MAINTAINED BY:	

----------------------------------------------------------------------------

	MODIFICATION HISTORY:
	
	DATE	PURPOSE						BY

	950227	Added header and declarations for ls_print_findsym_error(),
		ls_get_double(), and ls_get_double() routines.	EBJ

	950302	Added structure for symbol description.		EBJ
	
	950306	Added ls_get_sym_val() and ls_set_sym_val() routines.
		This is now the production version.		EBJ
	
	CURRENT RCS HEADER:

$Header$
$Log$
Revision 1.1  2002/09/10 01:14:02  curt
Initial revision

Revision 1.1.1.1  1999/06/17 18:07:33  curt
Start of 0.7.x branch

Revision 1.1.1.1  1999/04/05 21:32:45  curt
Start of 0.6.x branch.

Revision 1.3  1998/08/06 12:46:40  curt
Header change.

Revision 1.2  1998/01/22 02:59:34  curt
Changed #ifdef FILE_H to #ifdef _FILE_H

Revision 1.1  1997/05/29 00:10:00  curt
Initial Flight Gear revision.

 * Revision 1.9  1995/03/07  12:52:33  bjax
 * This production version now specified ls_get_sym_val() and ls_put_sym_val().
 *
 * Revision 1.6.1.2  1995/03/06  18:45:41  bjax
 * Added def'n of ls_get_sym_val() and ls_set_sym_val(); changed symbol_rec
 * Addr field from void * to char *.
 *  EBJ
 *
 * Revision 1.6.1.1  1995/03/03  01:17:44  bjax
 * Experimental version with just ls_get_double and ls_set_double() routines.
 *
 * Revision 1.6  1995/02/27  19:50:49  bjax
 * Added header and declarations for ls_print_findsym_error(),
 * ls_get_double(), and ls_set_double().  EBJ
 *

----------------------------------------------------------------------------

	REFERENCES:

----------------------------------------------------------------------------

	CALLED BY:

----------------------------------------------------------------------------

	CALLS TO:

----------------------------------------------------------------------------

	INPUTS:

----------------------------------------------------------------------------

	OUTPUTS:

--------------------------------------------------------------------------*/

#ifndef _LS_SYM_H
#define _LS_SYM_H


/* Return codes */

#define SYM_NOT_LOADED -2
#define SYM_UNEXPECTED_ERR -1
#define SYM_OK 0
#define SYM_OPEN_ERR 1
#define SYM_NO_SYMS 2
#define SYM_MOD_NOT_FOUND 3
#define SYM_VAR_NOT_FOUND 4
#define SYM_NOT_SCALAR 5
#define SYM_NOT_STATIC 6
#define SYM_MEMORY_ERR 7 
#define SYM_UNMATCHED_PAREN 8
#define SYM_BAD_SYNTAX 9
#define SYM_INDEX_BOUNDS_ERR 10

typedef enum {Unknown, Char, UChar, SHint, USHint, Sint, Uint, Slng, Ulng, flt, dbl} vartype;

typedef char		SYMBOL_NAME[64];
typedef	vartype		SYMBOL_TYPE;



typedef struct
{
    SYMBOL_NAME	Mod_Name;
    SYMBOL_NAME	Par_Name;
    SYMBOL_TYPE Par_Type;
    SYMBOL_NAME Alias;
    char	*Addr;
}	symbol_rec;


extern int 	ls_findsym( const char *modname, const char *varname, 
			    char **addr, vartype *vtype );
  
extern void 	ls_print_findsym_error(int result, 
				       char *mod_name, 
				       char *var_name);
  
extern double	ls_get_double(vartype sym_type, void *addr );
  
extern void 	ls_set_double(vartype sym_type, void *addr, double value );

extern double	ls_get_sym_val( symbol_rec *symrec, int *error );

	/* This routine attempts to return the present value of the symbol
	   described in symbol_rec. If Addr is non-zero, the value of that
	   location, interpreted as type double, will be returned. If Addr
	   is zero, and Mod_Name and Par_Name are both not null strings, 
	   the ls_findsym() routine is used to try to obtain the address
	   by looking at debugger symbol tables in the executable image, and
	   the value of the double contained at that address is returned, 
	   and the symbol record is updated to contain the address of that
	   symbol. If an error is discovered, 'error' will be non-zero and
	   and error message is printed on stderr.			*/

extern void 	ls_set_sym_val( symbol_rec *symrec, double value );

	/* This routine sets the value of a double at the location pointed
	   to by the symbol_rec's Addr field, if Addr is non-zero. If Addr
	   is zero, and Mod_Name and Par_Name are both not null strings, 
	   the ls_findsym() routine is used to try to obtain the address
	   by looking at debugger symbol tables in the executable image, and
	   the value of the double contained at that address is returned, 
	   and the symbol record is updated to contain the address of that
	   symbol. If an error is discovered, 'error' will be non-zero and
	   and error message is printed on stderr.			*/


#endif /* _LS_SYM_H */


