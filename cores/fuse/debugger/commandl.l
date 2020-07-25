/* commandl.l: Debugger command lexical scanner
   Copyright (c) 2002-2017 Philip Kendall, Sergio Baldoví

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License along
   with this program; if not, write to the Free Software Foundation, Inc.,
   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

   Author contact information:

   E-mail: philip-fuse@shadowmagic.org.uk

*/

%{

#include <config.h>

#include <ctype.h>

#include "debugger/debugger.h"
#include "debugger/debugger_internals.h"
#include "mempool.h"

#include "debugger/commandy.h"

#define YY_INPUT(buf,result,max_size) \
{ \
  int retval; \
  if( !debugger_command_input( buf, &retval, max_size ) ) retval = YY_NULL; \
  result = (yy_size_t)retval; \
}

#define YY_NO_INPUT

%}

ID	[a-z]([a-z0-9']|\\[ ])*

%option caseless

%s COMMANDSTATE1
%x COMMANDSTATE2

%%

ba|bas|base { return BASE; }
br|bre|brea|break|breakp|breakpo|breakpoi|breakpoin|breakpoint { return BREAK;}
co|con|cont|contin|continu|continue { return CONTINUE; }
com|comm|comma|comman|command|commands { BEGIN(COMMANDSTATE1); return COMMANDS; }
cond|condi|condit|conditi|conditio|condition { return CONDITION; }
cl|cle|clea|clear { return CLEAR; }
del|dele|delet|delete { return DEBUGGER_DELETE; }
di|dis|disa|disas|disass|disasse|disassm|disassmb|diasassmbl|disassemble {
	                                                  return DISASSEMBLE; }
ev|eve|even|event { return EVENT; }
ex|exi|exit { return EXIT; }
fi|fin|fini|finis|finish { return FINISH; }
if { return IF; }
ig|ign|igno|ignor|ignore { return DEBUGGER_IGNORE; }
n|ne|nex|next { return NEXT; }
o|ou|out { return DEBUGGER_OUT; }	/* Different name to avoid clashing
					   with OUT from z80/z80_macros.h */
p|po|por|port { return PORT; }
pr|pri|prin|print { return DEBUGGER_PRINT; }
re|rea|read { return READ; }
se|set { return SET; }
s|st|ste|step { return STEP; }
t|tb|tbr|tbre|tbrea|tbreak|tbreakp|tbreakpo|tbreakpoi|tbreakpoin|tbreakpoint {
							       return TBREAK; }
ti|tim|time { return TIME; }
w|wr|wri|writ|write { return WRITE; }

"("		{ return '('; }
")"		{ return ')'; }

"["             { return '['; }
"]"             { return ']'; }

"!"		{ yylval.token = '!'; return NEGATE; }
"~"		{ yylval.token = '~'; return NEGATE; }

 /* The hex constants used here are the appropriate Unicode characters */

"=="		{ yylval.token = DEBUGGER_TOKEN_EQUAL_TO; return EQUALITY; }
"!="		{ yylval.token = DEBUGGER_TOKEN_NOT_EQUAL_TO;
		  return EQUALITY; }

"<"		{ yylval.token = '<'; return COMPARISON; }
">"		{ yylval.token = '>'; return COMPARISON; }
"<="		{ yylval.token = DEBUGGER_TOKEN_LESS_THAN_OR_EQUAL_TO;
		  return COMPARISON; }
">="		{ yylval.token = DEBUGGER_TOKEN_GREATER_THAN_OR_EQUAL_TO;
		  return COMPARISON; }

"+"		{ return '+'; }
"-"		{ return '-'; }
"*"		{ return '*'; }
"/"		{ return '/'; }

"&"		{ return '&'; }
"^"		{ return '^'; }
"|"		{ return '|'; }

"&&"		{ return LOGICAL_AND; }
"||"		{ return LOGICAL_OR; }

":"		{ return ':'; }

$[[:xdigit:]]+	{ yylval.integer = strtol( yytext+1, NULL, 16 );
		  if( YY_START == COMMANDSTATE1 ) BEGIN( COMMANDSTATE2 );
		  return NUMBER; }
0x[[:xdigit:]]+	{ yylval.integer = strtol( yytext+2, NULL, 16 );
		  if( YY_START == COMMANDSTATE1 ) BEGIN( COMMANDSTATE2 );
	          return NUMBER; }
[[:digit:]]+	{ yylval.integer = atoi( yytext );
		  if( YY_START == COMMANDSTATE1 ) BEGIN( COMMANDSTATE2 );
		  return NUMBER; }

{ID}		{ char *dest, *source;
  yylval.string = mempool_malloc( debugger_memory_pool, strlen( yytext ) + 1 );
  /* remove escape characters */
  dest = yylval.string;
  source = yytext;
  while( *source ) {
    if( *source != '\\' ) {
      *dest++ = *source;
    }
    source++;
  }
  *dest = '\0';
  return STRING; }

${ID}		{ yylval.string = mempool_strdup( debugger_memory_pool, yytext + 1 ); return VARIABLE; }

\n		{ return '\n'; }

[ ]+		/* Swallow whitespace */

.		{ if( 0 ) unput( '\0' ); /* Prevent warning about yyunput
					    being defined and not used */
		  return DEBUGGER_ERROR; }

 /* Debugger command parsing */

<COMMANDSTATE2>{

end             { BEGIN(INITIAL); return DEBUGGER_END; }

[^\n]*          { yylval.string = mempool_strdup( debugger_memory_pool, yytext ); return STRING; }
\n              { return '\n'; }

}
