/**************************************************************************
 * parser.y -- scenery file parser
 *
 * Written by Curtis Olson, started June 1997.
 *
 * Copyright (C) 1997  Curtis L. Olson  - curt@infoplane.com
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * $Id$
 **************************************************************************/


/* C pass through */
%{
#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "parsevrml.h"
#include "geometry.h"
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
%token IncludeSym ShapeSym GeometrySym ElevationGridSym

/* basic tokens */
%token Identifier Number StringLiteral 

/* symbol tokens */
%token EqualSym LBraceSym RBraceSym LParenSym RParenSym 
%token LSqBracketSym RSqBracketSym CommaSym

/* error tokens */
%token BadStringLiteral ErrorMisc


/* Start Symbol */
%start vrml


/* Rules Section */
%%

vrml : 
    node_list
;

node_list : 
    node 
    | node_list node
;

node : 
    include 
    | shape 
;

/* includes */
include : 
    IncludeSym StringLiteral 
    { 
	yytext = strip_quotes(yytext);
	printf("Need to include %s\n", yytext);
	push_input_stream(yytext);
    }
;

/* Shape rules */

shape :
    ShapeSym LBraceSym shape_body RBraceSym
;

shape_body :
    /* appearance */ geometry
;

/* appearance : */
/* Empty OK */
/* ; */

geometry :
    GeometrySym 
    Identifier 
    {
        vrmlInitGeometry(yytext);
    }
    LBraceSym geom_item_list RBraceSym
    {
	vrmlHandleGeometry();
	vrmlFreeGeometry();
    }
;

geom_item_list :
    geom_item
    | geom_item_list geom_item
;

geom_item :
    Identifier { vrmlGeomOptionName(yytext); }
    geom_rvalue
;

geom_rvalue :
    value
    | LSqBracketSym value_list RSqBracketSym
;

value_list :
    value
    | value_list value
;

value :
    Number { vrmlGeomOptionsValue(yytext); }
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


/* parse a VRML scenery file */
int fgParseVRML(char *file) {
    int result;

    printf("input file = %s\n", file);
    push_input_stream(file);
    result = yyparse();

    /* return(result) */
    return(result);
}
