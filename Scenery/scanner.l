/**************************************************************************
 * scenery.l -- Lexical Analyzer for scenery files
 *
 * Written by Curtis Olson, started May 1997.
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
    static int scanner_msg_len;
    static char scanner_msg[1024];

    static void scanner_print(char *s) {
        if ( scanner_debug ) {
            printf("%s", s);
        }
    }

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

include         { scanner_print("return IncludeSym\n");
                  return IncludeSym;
                }

mesh            { scanner_print("return MeshSym\n");
                  return MeshSym;
                }

row             { scanner_print("return RowSym\n");
                  return RowSym;
                }

chunk           { scanner_print("return ChunkSym\n");
                  return ChunkSym;
                }

bounds          { scanner_print("return BoundsSym\n");
                  return BoundsSym;
                }

place           { scanner_print("return PlaceSym\n");
                  return PlaceSym;
                }

{ident}		{ scanner_msg_len = snprintf(scanner_msg, 1024, 
                           "return Identifier = %s\n", yytext);
                  scanner_msg[scanner_msg_len] = '\0';
                  scanner_print(scanner_msg);
                  return Identifier;
                }

{number}	{ scanner_print("return Number\n");
                  return Number;
                }

{string}        { scanner_msg_len = snprintf(scanner_msg, 1024,
			   "return StringLiteral = %s\n", yytext);
                  scanner_msg[scanner_msg_len] = '\0';
                  scanner_print(scanner_msg);
                  return StringLiteral;
                }

{bad_string}    { scanner_msg_len = snprintf(scanner_msg, 1024,
                           "return BadStringLiteral = %s\n", yytext);
                  scanner_msg[scanner_msg_len] = '\0';
                  scanner_print(scanner_msg);
                  return BadStringLiteral; 
                }

"\n"		{ line_num++; 
                  scanner_msg_len = snprintf(scanner_msg, 1024,
                           "Line number = %d\n", line_num);
                  scanner_msg[scanner_msg_len] = '\0';
                  scanner_print(scanner_msg);
                }

"#"		{ scanner_print("return HashSym\n");
                  return HashSym; 
                }

"="		{ scanner_print("return EqualSym\n");
                  return EqualSym; 
                }

","		{ scanner_print("return CommaSym\n");
                  return CommaSym; 
                }

"{"		{ scanner_print("return LBraceSym\n");
                  return LBraceSym; 
                }

"}"		{ scanner_print("return RBraceSym\n");
                  return RBraceSym;
                }

"("		{ scanner_print("return LParenSym\n");
                  return LParenSym; 
                }

")"		{ scanner_print("return RParenSym\n");
                  return RParenSym;
                }

"//"    	{ /* burn through "#" comment */ 
		  c = input();
		  while ( (c != '\n') ) {
		      c = input();
		      /* scanner_print("%c", c); */
		  }
		  unput(c);
		  /* line_num++; */
		}

{ws}		{ ; }

{other}		{ scanner_msg_len = snprintf(scanner_msg, 1024,
			   "Scanned some unexpected text = `%s'\n", yytext);
                  scanner_msg[scanner_msg_len] = '\0';
                  scanner_print(scanner_msg);
		  return ErrorMisc; 
		}

<<EOF>>         { if ( pop_input_stream() == 0 ) {
                      /* End of last input stream */
                      yyterminate();
                  }
                }
