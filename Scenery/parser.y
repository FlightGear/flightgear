/**************************************************************************
 * parser.y -- scenery file parser
 *
 * Written by Curtis Olson, started May 1997.
 *
 * $Id$
 * (Log is kept at end of this file)
 **************************************************************************/


/* C pass through */
%{
#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "mesh.h"
#include "scenery.h"


    /*#DEFINE YYDEBUG 1 */

    /* interfacing with scanner.l (lex) */
    extern int line_num;
    extern char *yytext;
    int push_input_stream ( char *input );

    /* we must define this ourselves */
    int yyerror(char *s);

    /* handle for a mesh structure */
    struct mesh *mesh_ptr;
%}


/* top level reserved words */
%token IncludeSym MeshSym RowSym ChunkSym BoundsSym PlaceSym

/* basic tokens */
%token Identifier Number StringLiteral 

/* symbol tokens  */
%token HashSym EqualSym LBraceSym RBraceSym LParenSym RParenSym CommaSym

/* error tokens */
%token BadStringLiteral ErrorMisc


/* Start Symbol */
%start	start


/* Rules Section */
%%

start : 
    object_list
;

object_list : 
    object 
    | object_list object
;

object : 
    include 
    | mesh 
    | chunk
;

/* includes */
include : 
    HashSym IncludeSym StringLiteral 
    { 
	yytext = strip_quotes(yytext);
	printf("Need to include %s\n", yytext);
	push_input_stream(yytext);
    }
;

/* terrain mesh rules */
mesh : 
    MeshSym 
    {
	/* allocate a structure for this terrain mesh */
	mesh_ptr = new_mesh();
    }
    Identifier LBraceSym mesh_body RBraceSym
    {
	/* The following two lines *SHOULD* be here, they are just temporarily
	   commented out until the scenery mngmt stuff gets hashed out. */

	/* free up the mem used by this structure */
	/* free(mesh_ptr->mesh_data); */
	/* free(mesh_ptr); */
    }
;

mesh_body : 
    mesh_header_list
    {
	/* We've read the headers, so allocate a pseudo 2d array */
	mesh_ptr->mesh_data = new_mesh_data(mesh_ptr->rows, mesh_ptr->cols);
	printf("Beginning to read mesh data\n");
    }
    mesh_row_list
;

mesh_header_list :
     mesh_header 
     | mesh_header_list mesh_header
;

mesh_header : 
    Identifier 
    {
	mesh_set_option_name(mesh_ptr, yytext);
    }
    EqualSym Number
    {
	mesh_set_option_value(mesh_ptr, yytext);
    }
;

mesh_row_list :
    mesh_row
    | mesh_row_list mesh_row
;

mesh_row : 
    RowSym 
    {
	/* printf("Ready to read a row\n"); */
	mesh_ptr->cur_col = 0;
    }
    EqualSym LParenSym mesh_row_items RParenSym
    {
	mesh_ptr->cur_row++;
    }
;

mesh_row_items : 
    mesh_item 
    | mesh_row_items mesh_item
;

mesh_item :
    Number
    {
	/* mesh data is a pseudo 2d array */
	mesh_ptr->mesh_data[mesh_ptr->cur_row * mesh_ptr->rows + 
			    mesh_ptr->cur_col] = atof(yytext);
	mesh_ptr->cur_col++;
    }
;

/* chunk rules */
chunk 		: ChunkSym Identifier LBraceSym chunk_body RBraceSym
                ;

chunk_body      : chunk_header_list chunk_bounds place_list
                ;

chunk_header_list : chunk_header
                | chunk_header_list chunk_header
                ;

chunk_header    : Identifier EqualSym StringLiteral
                ;

chunk_bounds    : BoundsSym EqualSym LBraceSym 
                Number CommaSym
                Number CommaSym
                Number CommaSym
                Number CommaSym
                Number CommaSym
                Number CommaSym
                Number CommaSym
                Number
                RBraceSym
                ;

/* place rules */
place_list      : place | place_list place
                ;

place           : PlaceSym Identifier LBraceSym place_options_list RBraceSym
                ;

place_options_list : place_option | place_options_list place_option
                ; 

place_option    : Identifier EqualSym place_value
                ;

place_value     : geodetic_coord | Number
                ;

geodetic_coord  : LParenSym Number CommaSym Number CommaSym 
                  Number RParenSym
                ;


/* C Function Section */
%%


int yyerror(char *s) {
    printf("Error: %s at line %d.\n", s, line_num);
    return 0;
}


/* this is a simple main for testing the parser */

/*
int main(int argc, char **argv) {
#ifdef YYDEBUG
    yydebug = 1;
#endif

    printf("input file = %s\n", argv[1]);
    push_input_stream(argv[1]);
    yyparse();

    return 0;
}
*/


/* parse a scenery file */
int parse_scenery(char *file) {
    int result;

    printf("input file = %s\n", file);
    push_input_stream(file);
    result = yyparse();

    /* return(result) */
    return(mesh_ptr);
}
