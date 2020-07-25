/* commandy.y: Parse a debugger command
   Copyright (c) 2002-2016 Philip Kendall
   Copyright (c) 2015 Sergio Baldoví

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License along
   with this program; if not, write to the Free Software Foundation, Inc.,
   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

   Author contact information:

   E-mail: philip-fuse@shadowmagic.org.uk

*/

%{

#include <config.h>

#include <stdio.h>		/* Needed by NetBSD yacc */
#include <stdlib.h>
#include <string.h>

#include "debugger/debugger.h"
#include "debugger/debugger_internals.h"
#include "mempool.h"
#include "ui/ui.h"
#include "z80/z80.h"
#include "z80/z80_macros.h"

#define YYDEBUG 1
#define YYERROR_VERBOSE

%}

%union {

  int token;

  libspectrum_dword integer;
  char *string;

  debugger_breakpoint_type bptype;
  debugger_breakpoint_life bplife;
  struct { libspectrum_word mask, value; } port;
  struct { int source; int page; int offset; } location;

  debugger_expression* exp;

}

%debug

/* Tokens as returned from the Flex scanner (commandl.l) */

/* Some tokens are named DEBUGGER_* to avoid clashes with <windows.h> */

%token		 LOGICAL_OR	/* || */
%token		 LOGICAL_AND	/* && */
%token <token>	 COMPARISON	/* < > <= >= */
%token <token>   EQUALITY	/* == != */
%token <token>   NEGATE		/* ! ~ */

%token		 BASE
%token		 BREAK
%token		 TBREAK
%token		 CLEAR
%token           COMMANDS
%token		 CONDITION
%token		 CONTINUE
%token		 DEBUGGER_DELETE
%token		 DISASSEMBLE
%token           DEBUGGER_END
%token		 EVENT
%token		 EXIT
%token		 FINISH
%token		 IF
%token		 DEBUGGER_IGNORE
%token		 NEXT
%token		 DEBUGGER_OUT
%token		 PORT
%token		 DEBUGGER_PRINT
%token		 READ
%token		 SET
%token		 STEP
%token		 TIME
%token		 WRITE

%token <integer> NUMBER

%token <string>	 STRING
%token <string>	 VARIABLE

%token		 DEBUGGER_ERROR

/* Derived types */

%type  <bplife>  breakpointlife
%type  <bptype>  breakpointtype
%type  <port>    breakpointport
%type  <location> breakpointlocation
%type  <bptype>  portbreakpointtype
%type  <integer> numberorpc
%type  <integer> number

%type  <exp>     optionalcondition

%type  <exp>     expressionornull
%type  <exp>     expression;

%type  <string>  debuggercommand
%type  <string>  debuggercommands

/* Operator precedences */

/* Low precedence */

%left LOGICAL_OR
%left LOGICAL_AND
%left '|'
%left '^'
%left '&'
%left EQUALITY
%left COMPARISON
%left '+' '-'
%left '*' '/'
%right NEGATE		/* Unary minus, unary plus, !, ~ */

/* High precedence */

%%

input:	 /* empty */
       | command
       | error
       | input '\n' command
;

command:   BASE number { debugger_output_base = $2; }
	 | breakpointlife breakpointtype breakpointlocation optionalcondition {
             debugger_breakpoint_add_address( $2, $3.source, $3.page, $3.offset,
                                              0, $1, $4 );
	   }
	 | breakpointlife PORT portbreakpointtype breakpointport optionalcondition {
	     int mask = $4.mask;
	     if( mask == 0 ) mask = ( $4.value < 0x100 ? 0x00ff : 0xffff );
	     debugger_breakpoint_add_port( $3, $4.value, mask, 0, $1, $5 );
           }
	 | breakpointlife TIME number optionalcondition {
	     debugger_breakpoint_add_time( DEBUGGER_BREAKPOINT_TYPE_TIME,
					   $3, 0, $1, $4 );
	   }
	 | breakpointlife EVENT STRING ':' STRING optionalcondition {
	     debugger_breakpoint_add_event( DEBUGGER_BREAKPOINT_TYPE_EVENT,
					    $3, $5, 0, $1, $6 );
	   }
	 | breakpointlife EVENT STRING ':' '*' optionalcondition {
	     debugger_breakpoint_add_event( DEBUGGER_BREAKPOINT_TYPE_EVENT,
					    $3, "*", 0, $1, $6 );
	   }
	 | CLEAR numberorpc { debugger_breakpoint_clear( $2 ); }
	 | COMMANDS number '\n' debuggercommands DEBUGGER_END { debugger_breakpoint_set_commands( $2, $4 ); }
	 | CONDITION NUMBER expressionornull {
	     debugger_breakpoint_set_condition( $2, $3 );
           }
	 | CONTINUE { debugger_run(); }
	 | DEBUGGER_DELETE { debugger_breakpoint_remove_all(); }
	 | DEBUGGER_DELETE number { debugger_breakpoint_remove( $2 ); }
	 | DISASSEMBLE number { ui_debugger_disassemble( $2 ); }
	 | EXIT expressionornull { debugger_exit_emulator( $2 ); }
	 | FINISH   { debugger_breakpoint_exit(); }
	 | DEBUGGER_IGNORE NUMBER number {
	     debugger_breakpoint_ignore( $2, $3 );
	   }
	 | NEXT	    { debugger_next(); }
	 | DEBUGGER_OUT number NUMBER { debugger_port_write( $2, $3 ); }
	 | DEBUGGER_PRINT number { printf( "0x%x\n", $2 ); }
	 | SET NUMBER number { debugger_poke( $2, $3 ); }
	 | SET VARIABLE number { debugger_variable_set( $2, $3 ); }
         | SET STRING ':' STRING number { debugger_system_variable_set( $2, $4, $5 ); }
	 | STEP	    { debugger_step(); }
;

breakpointlife:   BREAK  { $$ = DEBUGGER_BREAKPOINT_LIFE_PERMANENT; }
		| TBREAK { $$ = DEBUGGER_BREAKPOINT_LIFE_ONESHOT; }
;

breakpointtype:   /* empty */ { $$ = DEBUGGER_BREAKPOINT_TYPE_EXECUTE; }
	        | READ        { $$ = DEBUGGER_BREAKPOINT_TYPE_READ; }
                | WRITE       { $$ = DEBUGGER_BREAKPOINT_TYPE_WRITE; }
;

breakpointport:   number { $$.mask = 0; $$.value = $1; }
		| number ':' number { $$.mask = $1; $$.value = $3; }
;

breakpointlocation:   numberorpc { $$.source = memory_source_any; $$.offset = $1; }
                    | STRING ':' number ':' number {
                        $$.source = memory_source_find( $1 );
                        if( $$.source == -1 ) {
                          char buffer[80];
                          snprintf( buffer, 80, "unknown memory source \"%s\"", $1 );
                          yyerror( buffer );
                          YYERROR;
                        }
                        $$.page = $3;
                        $$.offset = $5;
                      }

portbreakpointtype:   READ  { $$ = DEBUGGER_BREAKPOINT_TYPE_PORT_READ; }
		    | WRITE { $$ = DEBUGGER_BREAKPOINT_TYPE_PORT_WRITE; }
;

optionalcondition:   /* empty */   { $$ = NULL; }
		   | IF expression { $$ = $2; }
;

numberorpc:   /* empty */ { $$ = PC; }
	    | number      { $$ = $1; }
;

expressionornull:   /* empty */ { $$ = NULL; }
	          | expression  { $$ = $1; }
;

number:   expression { $$ = debugger_expression_evaluate( $1 ); }
;

expression:   NUMBER { $$ = debugger_expression_new_number( $1, debugger_memory_pool );
		       if( !$$ ) YYABORT;
		     }
            | STRING ':' STRING { $$ = debugger_expression_new_system_variable( $1, $3, debugger_memory_pool );
                                  if( !$$ ) YYABORT;
                                }
	    | VARIABLE { $$ = debugger_expression_new_variable( $1, debugger_memory_pool );
			 if( !$$ ) YYABORT;
		       }
	    | '(' expression ')' { $$ = $2; }
            | '[' expression ']' {
                $$ = debugger_expression_new_unaryop( DEBUGGER_TOKEN_DEREFERENCE, $2, debugger_memory_pool );
                if( !$$ ) YYABORT;
              }
	    | '+' expression %prec NEGATE { $$ = $2; }
	    | '-' expression %prec NEGATE {
	        $$ = debugger_expression_new_unaryop( '-', $2, debugger_memory_pool );
		if( !$$ ) YYABORT;
	      }
	    | NEGATE expression {
	        $$ = debugger_expression_new_unaryop( $1, $2, debugger_memory_pool );
		if( !$$ ) YYABORT;
	      }
	    | expression '+' expression {
	        $$ = debugger_expression_new_binaryop( '+', $1, $3, debugger_memory_pool );
		if( !$$ ) YYABORT;
	      }
	    | expression '-' expression {
	        $$ = debugger_expression_new_binaryop( '-', $1, $3, debugger_memory_pool );
		if( !$$ ) YYABORT;
	      }
	    | expression '*' expression {
	        $$ = debugger_expression_new_binaryop( '*', $1, $3, debugger_memory_pool );
		if( !$$ ) YYABORT;
	      }
	    | expression '/' expression {
	        $$ = debugger_expression_new_binaryop( '/', $1, $3, debugger_memory_pool );
		if( !$$ ) YYABORT;
	      }
	    | expression EQUALITY expression {
	        $$ = debugger_expression_new_binaryop( $2, $1, $3, debugger_memory_pool );
		if( !$$ ) YYABORT;
	      }
	    | expression COMPARISON expression {
	        $$ = debugger_expression_new_binaryop( $2, $1, $3, debugger_memory_pool );
		if( !$$ ) YYABORT;
	      }
	    | expression '&' expression {
	        $$ = debugger_expression_new_binaryop( '&', $1, $3, debugger_memory_pool );
		if( !$$ ) YYABORT;
	      }
	    | expression '^' expression {
	        $$ = debugger_expression_new_binaryop( '^', $1, $3, debugger_memory_pool );
		if( !$$ ) YYABORT;
	      }
	    | expression '|' expression {
	        $$ = debugger_expression_new_binaryop( '|', $1, $3, debugger_memory_pool );
		if( !$$ ) YYABORT;
	      }
	    | expression LOGICAL_AND expression {
	        $$ = debugger_expression_new_binaryop(
		  DEBUGGER_TOKEN_LOGICAL_AND, $1, $3, debugger_memory_pool
                );
		if( !$$ ) YYABORT;
	      }
	    | expression LOGICAL_OR expression {
	        $$ = debugger_expression_new_binaryop(
		  DEBUGGER_TOKEN_LOGICAL_OR, $1, $3, debugger_memory_pool
		);
		if( !$$ ) YYABORT;
	      }
;

debuggercommands:   debuggercommand { $$ = $1; }
                  | debuggercommands debuggercommand {
                      $$ = mempool_new( debugger_memory_pool, char, strlen( $1 ) + strlen( $2 ) + 2 );
                      sprintf( $$, "%s\n%s", $1, $2 );
                    }

debuggercommand: STRING '\n'

%%
