/**************************************************************************
 * scenery.l -- Lexical Analyzer for scenery files
 *
 * Written by Curtis Olson, started May 1997.
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
 * NOTE: Compiles with flex and gcc.
 *
 * $Id$
 **************************************************************************/


/* C Pass Through */
%{
    #include <stdio.h>
    #include "parser.h"

    int line_num = 1;
    char c;

    /* custom print routine */
    static int scanner_debug = 0;

    /* Routines to manage a stack of nested input buffers (for
       processing the #include directive */
    #define MAX_INCLUDE_DEPTH 10
    static YY_BUFFER_STATE include_stack[MAX_INCLUDE_DEPTH];
    static int include_stack_ptr = 0;

    int push_input_stream ( char *input ) { 
	FILE *yyin_save;
        if ( include_stack_ptr >= MAX_INCLUDE_DEPTH ) {
	    fprintf( stderr, "Scanner says:  Includes nested too deeply\n" );
	    return(0);
	}

	/* save yyin in case the following fails */
	yyin_save = yyin;

	yyin = fopen( input, "r" );
	if ( ! yyin ) {
	    fprintf( stderr, "Scanner says:  cannot open '%s'\n", input );

	    /* The failed attempt destroyed yyin, so restore it */
	    yyin = yyin_save;

	    return(0);
	}
 
	include_stack[include_stack_ptr++] = YY_CURRENT_BUFFER;
     	yy_switch_to_buffer( yy_create_buffer(yyin, YY_BUF_SIZE) );

	return(1);
    }

    int pop_input_stream () { 
	if ( --include_stack_ptr < 1 ) {
	    /* end of last input stream */
	    return(0);
	} else {
	    /* end of current input stream, restore previous and continue */
	    yy_delete_buffer( YY_CURRENT_BUFFER );
	    yy_switch_to_buffer( include_stack[include_stack_ptr] );
	    return(1);
	}
    }
%}


/* Definitions */
letter		[A-Za-z]
digit		[0-9]
connecter       [_-]
ident		{letter}({letter}|{digit}|{connecter})*

integer		[+-]?{digit}+

plain_real	{integer}"."{integer}
short_exp_real  {integer}[Ee][+-]?{integer}
exp_real	{plain_real}[Ee][+-]?{integer}
real		[+-]?{plain_real}|{short_exp_real}|{exp_real}

number          {real}|{integer}

string          \"[^"\n]+\"

bad_string      \"([^"\n]|\n)+\"

ws		[ \t]+
other		.


/* Rules */ 
%%

include         { if ( scanner_debug ) {
                      printf("return IncludeSym\n");
                  }
                  return IncludeSym;
                }

Shape           { if ( scanner_debug ) {
                      printf("return ShapeSym\n");
                  }
                  return ShapeSym;
                }

Geometry        { if ( scanner_debug ) {
                      printf("return GeometrySym\n");
                  }
                  return GeometrySym;
                }

{ident}		{ if ( scanner_debug ) {
                      printf("return Identifier = %s\n", yytext);
                  }
                  return Identifier;
                }

{number}	{ if ( scanner_debug ) {
                      printf("return Number\n");
                  }
                  return Number;
                }

{string}        { if ( scanner_debug ) {
                      printf("return StringLiteral = %s\n", yytext);
                  }
                  return StringLiteral;
                }

{bad_string}    { if ( scanner_debug ) {
                      printf("return BadStringLiteral = %s\n", yytext);
                  }
                  return BadStringLiteral; 
                }

"\n"		{ line_num++; 
                  if ( scanner_debug ) {
                      printf("Line number = %d\n", line_num);
                  }
               }

"="		{ if ( scanner_debug ) {
                      printf("return EqualSym\n");
                  }
                  return EqualSym; 
                }

","		{ if ( scanner_debug ) {
                      printf("return CommaSym\n");
                  }
                  return CommaSym; 
                }

"{"		{ if ( scanner_debug ) {
                      printf("return LBraceSym\n");
                  }
                  return LBraceSym; 
                }

"}"		{ if ( scanner_debug ) {
                      printf("return RBraceSym\n");
                  }
                  return RBraceSym;
                }

"("		{ if ( scanner_debug ) {
                      printf("return LParenSym\n");
                  }
                  return LParenSym; 
                }

")"		{ if ( scanner_debug ) {
                      printf("return RParenSym\n");
                  }
                  return RParenSym;
                }

"["		{ if ( scanner_debug ) {
                      printf("return LSqBracketSym\n");
                  }
                  return LSqBracketSym; 
                }

"]"		{ if ( scanner_debug ) {
                      printf("return RSqBracketSym\n");
                  }
                  return RSqBracketSym;
                }

"//"    	{ /* burn through "//" comment to end of line */ 
                  if ( scanner_debug ) {
                      printf("Skipping comment: ");
                  }
		  c = input();
		  while ( (c != '\n') ) {
		      c = input();
                      if ( scanner_debug ) {
                          printf("%c", c);
                      }
		  }
		  unput(c);
		}

"#"             { /* burn through "#" comment to end of line */ 
                  if ( scanner_debug ) {
                      printf("Skipping comment: ");
                  }
		  c = input();
		  while ( (c != '\n') ) {
		      c = input();
                      if ( scanner_debug ) {
                          printf("%c", c);
                      }
		  }
		  unput(c);
		}

{ws}		{ ; }

{other}		{ if ( scanner_debug ) {
                      printf("Scanned some unexpected text = `%s'\n", yytext);
                  }
		  return ErrorMisc; 
		}

<<EOF>>         { if ( pop_input_stream() == 0 ) {
                      /* End of last input stream */
                      yyterminate();
                  }
                }
