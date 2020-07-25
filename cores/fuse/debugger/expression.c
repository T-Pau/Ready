/* expression.c: A numeric expression
   Copyright (c) 2003-2016 Philip Kendall

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

#include <config.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "debugger_internals.h"
#include "fuse.h"
#include "mempool.h"
#include "ui/ui.h"
#include "utils.h"

typedef enum expression_type {

  DEBUGGER_EXPRESSION_TYPE_INTEGER,
  DEBUGGER_EXPRESSION_TYPE_UNARYOP,
  DEBUGGER_EXPRESSION_TYPE_BINARYOP,
  DEBUGGER_EXPRESSION_TYPE_SYSVAR,
  DEBUGGER_EXPRESSION_TYPE_VARIABLE,

} expression_type;

enum precedence_t {

  /* Lowest precedence */
  PRECEDENCE_LOGICAL_OR,
  PRECEDENCE_LOGICAL_AND,
  PRECEDENCE_BITWISE_OR,
  PRECEDENCE_BITWISE_XOR,
  PRECEDENCE_BITWISE_AND,
  PRECEDENCE_EQUALITY,
  PRECEDENCE_COMPARISON,
  PRECEDENCE_ADDITION,
  PRECEDENCE_MULTIPLICATION,
  PRECEDENCE_NEGATE,
  PRECEDENCE_DEREFERENCE,

  PRECEDENCE_ATOMIC,
  /* Highest precedence */

};

struct unaryop_type {

  int operation;
  debugger_expression *op;

};

struct binaryop_type {

  int operation;
  debugger_expression *op1, *op2;

};

struct debugger_expression {

  expression_type type;
  enum precedence_t precedence;

  union {
    int integer;
    struct unaryop_type unaryop;
    struct binaryop_type binaryop;
    char *variable;
    int system_variable;
  } types;

};

static libspectrum_dword evaluate_unaryop( struct unaryop_type *unaryop );
static libspectrum_dword evaluate_binaryop( struct binaryop_type *binary );

static int deparse_unaryop( char *buffer, size_t length,
			    const struct unaryop_type *unaryop );
static int deparse_binaryop( char *buffer, size_t length,
			     const struct binaryop_type *binaryop );
static int
brackets_necessary( int top_operation, debugger_expression *operand );
static int is_non_associative( int operation );

static enum precedence_t
unaryop_precedence( int operation )
{
  switch( operation ) {

  case '!': case '~': case '-': return PRECEDENCE_NEGATE;

  case DEBUGGER_TOKEN_DEREFERENCE:
    return PRECEDENCE_DEREFERENCE;

  default:
    ui_error( UI_ERROR_ERROR, "unknown unary operator %d", operation );
    fuse_abort();
  }
}

static enum precedence_t
binaryop_precedence( int operation )
{
  switch( operation ) {

  case DEBUGGER_TOKEN_LOGICAL_OR: return PRECEDENCE_LOGICAL_OR;
  case DEBUGGER_TOKEN_LOGICAL_AND: return PRECEDENCE_LOGICAL_AND;
  case    '|':		    return PRECEDENCE_BITWISE_OR;
  case    '^':		    return PRECEDENCE_BITWISE_XOR;
  case    '&':		    return PRECEDENCE_BITWISE_AND;
  case    '+': case    '-': return PRECEDENCE_ADDITION;
  case    '*': case    '/': return PRECEDENCE_MULTIPLICATION;

  case DEBUGGER_TOKEN_EQUAL_TO:
  case DEBUGGER_TOKEN_NOT_EQUAL_TO:
    return PRECEDENCE_EQUALITY;

  case    '<': case    '>':
  case DEBUGGER_TOKEN_LESS_THAN_OR_EQUAL_TO:
  case DEBUGGER_TOKEN_GREATER_THAN_OR_EQUAL_TO:
    return PRECEDENCE_COMPARISON;

  default:
    ui_error( UI_ERROR_ERROR, "unknown binary operator %d", operation );
    fuse_abort();
  }
}

debugger_expression*
debugger_expression_new_number( libspectrum_dword number, int pool )
{
  debugger_expression *exp;

  exp = mempool_new( pool, debugger_expression, 1 );

  exp->type = DEBUGGER_EXPRESSION_TYPE_INTEGER;
  exp->precedence = PRECEDENCE_ATOMIC;
  exp->types.integer = number;

  return exp;
}

debugger_expression*
debugger_expression_new_binaryop( int operation, debugger_expression *operand1,
				  debugger_expression *operand2, int pool )
{
  debugger_expression *exp;

  exp = mempool_new( pool, debugger_expression, 1 );

  exp->type = DEBUGGER_EXPRESSION_TYPE_BINARYOP;
  exp->precedence = binaryop_precedence( operation );

  exp->types.binaryop.operation = operation;
  exp->types.binaryop.op1 = operand1;
  exp->types.binaryop.op2 = operand2;

  return exp;
}

debugger_expression*
debugger_expression_new_unaryop( int operation, debugger_expression *operand,
				 int pool )
{
  debugger_expression *exp;

  exp = mempool_new( pool, debugger_expression, 1 );

  exp->type = DEBUGGER_EXPRESSION_TYPE_UNARYOP;
  exp->precedence = unaryop_precedence( operation );

  exp->types.unaryop.operation = operation;
  exp->types.unaryop.op = operand;

  return exp;
}

debugger_expression*
debugger_expression_new_system_variable( const char *type, const char *detail,
                                         int pool )
{
  debugger_expression *exp;
  int system_variable;

  system_variable = debugger_system_variable_find( type, detail );
  if( system_variable == -1 ) {
    ui_error( UI_ERROR_WARNING, "System variable %s:%s not known", type,
              detail );
    return NULL;
  }

  exp = mempool_new( pool, debugger_expression, 1 );

  exp->type = DEBUGGER_EXPRESSION_TYPE_SYSVAR;
  exp->precedence = PRECEDENCE_ATOMIC;
  exp->types.system_variable = system_variable;

  return exp;
}

debugger_expression*
debugger_expression_new_variable( const char *name, int pool )
{
  debugger_expression *exp;

  exp = mempool_new( pool, debugger_expression, 1 );

  exp->type = DEBUGGER_EXPRESSION_TYPE_VARIABLE;
  exp->precedence = PRECEDENCE_ATOMIC;
  exp->types.variable = mempool_strdup( pool, name );

  return exp;
}

void
debugger_expression_delete( debugger_expression *exp )
{
  switch( exp->type ) {
    
  case DEBUGGER_EXPRESSION_TYPE_INTEGER:
  case DEBUGGER_EXPRESSION_TYPE_SYSVAR:
    break;

  case DEBUGGER_EXPRESSION_TYPE_UNARYOP:
    debugger_expression_delete( exp->types.unaryop.op );
    break;

  case DEBUGGER_EXPRESSION_TYPE_BINARYOP:
    debugger_expression_delete( exp->types.binaryop.op1 );
    debugger_expression_delete( exp->types.binaryop.op2 );
    break;

  case DEBUGGER_EXPRESSION_TYPE_VARIABLE:
    libspectrum_free( exp->types.variable );
    break;
  }
    
  libspectrum_free( exp );
}

debugger_expression*
debugger_expression_copy( debugger_expression *src )
{
  debugger_expression *dest;

  dest = libspectrum_new( debugger_expression, 1 );
  if( !dest ) return NULL;

  dest->type = src->type;
  dest->precedence = src->precedence;

  switch( dest->type ) {

  case DEBUGGER_EXPRESSION_TYPE_INTEGER:
    dest->types.integer = src->types.integer;
    break;

  case DEBUGGER_EXPRESSION_TYPE_UNARYOP:
    dest->types.unaryop.operation = src->types.unaryop.operation;
    dest->types.unaryop.op = debugger_expression_copy( src->types.unaryop.op );
    if( !dest->types.unaryop.op ) {
      libspectrum_free( dest );
      return NULL;
    }
    break;

  case DEBUGGER_EXPRESSION_TYPE_BINARYOP:
    dest->types.binaryop.operation = src->types.binaryop.operation;
    dest->types.binaryop.op1 =
      debugger_expression_copy( src->types.binaryop.op1 );
    if( !dest->types.binaryop.op1 ) {
      libspectrum_free( dest );
      return NULL;
    }
    dest->types.binaryop.op2 =
      debugger_expression_copy( src->types.binaryop.op2 );
    if( !dest->types.binaryop.op2 ) {
      debugger_expression_delete( dest->types.binaryop.op1 );
      libspectrum_free( dest );
      return NULL;
    }
    break;

  case DEBUGGER_EXPRESSION_TYPE_SYSVAR:
    dest->types.system_variable = src->types.system_variable;
    break;

  case DEBUGGER_EXPRESSION_TYPE_VARIABLE:
    dest->types.variable = utils_safe_strdup( src->types.variable );
    break;
  }

  return dest;
}

libspectrum_dword
debugger_expression_evaluate( debugger_expression *exp )
{
  switch( exp->type ) {

  case DEBUGGER_EXPRESSION_TYPE_INTEGER:
    return exp->types.integer;

  case DEBUGGER_EXPRESSION_TYPE_UNARYOP:
    return evaluate_unaryop( &( exp->types.unaryop ) );

  case DEBUGGER_EXPRESSION_TYPE_BINARYOP:
    return evaluate_binaryop( &( exp->types.binaryop ) );

  case DEBUGGER_EXPRESSION_TYPE_SYSVAR:
    return debugger_system_variable_get( exp->types.system_variable );

  case DEBUGGER_EXPRESSION_TYPE_VARIABLE:
    return debugger_variable_get( exp->types.variable );

  }

  ui_error( UI_ERROR_ERROR, "unknown expression type %d", exp->type );
  fuse_abort();
}

static libspectrum_dword
evaluate_unaryop( struct unaryop_type *unary )
{
  switch( unary->operation ) {

  case '!': return !debugger_expression_evaluate( unary->op );
  case '~': return ~debugger_expression_evaluate( unary->op );
  case '-': return -debugger_expression_evaluate( unary->op );

  case DEBUGGER_TOKEN_DEREFERENCE:
    return readbyte_internal( debugger_expression_evaluate( unary->op ) );

  }

  ui_error( UI_ERROR_ERROR, "unknown unary operator %d", unary->operation );
  fuse_abort();
}

static libspectrum_dword
evaluate_binaryop( struct binaryop_type *binary )
{
  switch( binary->operation ) {

  case '+': return debugger_expression_evaluate( binary->op1 ) +
		   debugger_expression_evaluate( binary->op2 );

  case '-': return debugger_expression_evaluate( binary->op1 ) -
		   debugger_expression_evaluate( binary->op2 );

  case '*': return debugger_expression_evaluate( binary->op1 ) *
		   debugger_expression_evaluate( binary->op2 );

  case '/': {
      libspectrum_dword op2 = debugger_expression_evaluate( binary->op2 );
      if( op2 == 0 ) {
        ui_error( UI_ERROR_ERROR, "divide by 0" );
        return 0;
      }
      return debugger_expression_evaluate( binary->op1 ) / op2;
    }

  case DEBUGGER_TOKEN_EQUAL_TO:
            return debugger_expression_evaluate( binary->op1 ) ==
                   debugger_expression_evaluate( binary->op2 );

  case DEBUGGER_TOKEN_NOT_EQUAL_TO:
	    return debugger_expression_evaluate( binary->op1 ) !=
		   debugger_expression_evaluate( binary->op2 );

  case '>': return debugger_expression_evaluate( binary->op1 ) >
		   debugger_expression_evaluate( binary->op2 );

  case '<': return debugger_expression_evaluate( binary->op1 ) <
	           debugger_expression_evaluate( binary->op2 );

  case DEBUGGER_TOKEN_LESS_THAN_OR_EQUAL_TO:
	    return debugger_expression_evaluate( binary->op1 ) <=
		   debugger_expression_evaluate( binary->op2 );

  case DEBUGGER_TOKEN_GREATER_THAN_OR_EQUAL_TO:
	    return debugger_expression_evaluate( binary->op1 ) >=
		   debugger_expression_evaluate( binary->op2 );

  case '&': return debugger_expression_evaluate( binary->op1 ) &
	           debugger_expression_evaluate( binary->op2 );

  case '^': return debugger_expression_evaluate( binary->op1 ) ^
		   debugger_expression_evaluate( binary->op2 );

  case '|': return debugger_expression_evaluate( binary->op1 ) |
		   debugger_expression_evaluate( binary->op2 );

  case DEBUGGER_TOKEN_LOGICAL_AND:
	    return debugger_expression_evaluate( binary->op1 ) &&
		   debugger_expression_evaluate( binary->op2 );

  case DEBUGGER_TOKEN_LOGICAL_OR:
	    return debugger_expression_evaluate( binary->op1 ) ||
		   debugger_expression_evaluate( binary->op2 );

  }

  ui_error( UI_ERROR_ERROR, "unknown binary operator %d", binary->operation );
  fuse_abort();
}

int
debugger_expression_deparse( char *buffer, size_t length,
			     const debugger_expression *exp )
{
  switch( exp->type ) {

  case DEBUGGER_EXPRESSION_TYPE_INTEGER:
    if( debugger_output_base == 10 ) {
      snprintf( buffer, length, "%d", exp->types.integer );
    } else {
      snprintf( buffer, length, "0x%x", exp->types.integer );
    }
    return 0;

  case DEBUGGER_EXPRESSION_TYPE_UNARYOP:
    return deparse_unaryop( buffer, length, &( exp->types.unaryop ) );

  case DEBUGGER_EXPRESSION_TYPE_BINARYOP:
    return deparse_binaryop( buffer, length, &( exp->types.binaryop ) );

  case DEBUGGER_EXPRESSION_TYPE_SYSVAR:
    debugger_system_variable_text( buffer, length, exp->types.system_variable );
    return 0;

  case DEBUGGER_EXPRESSION_TYPE_VARIABLE:
    snprintf( buffer, length, "$%s", exp->types.variable );
    return 0;

  }

  ui_error( UI_ERROR_ERROR, "unknown expression type %d", exp->type );
  fuse_abort();
}
  
static int
deparse_unaryop( char *buffer, size_t length,
		 const struct unaryop_type *unaryop )
{
  char *operand_buffer; const char *operation_string = NULL;
  const char *operation_suffix = "";
  int brackets_possible = 1;
  int brackets = 0;

  int error;

  operand_buffer = libspectrum_new( char, length );

  error = debugger_expression_deparse( operand_buffer, length, unaryop->op );
  if( error ) { libspectrum_free( operand_buffer ); return error; }

  switch( unaryop->operation ) {
  case '!': operation_string = "!"; break;
  case '~': operation_string = "~"; break;
  case '-': operation_string = "-"; break;
  case DEBUGGER_TOKEN_DEREFERENCE:
    operation_string = "[";
    operation_suffix = "]";
    brackets_possible = 0;
    break;

  default:
    ui_error( UI_ERROR_ERROR, "unknown unary operation %d",
	      unaryop->operation );
    fuse_abort();
  }

  if( brackets_possible )
    brackets = ( unaryop->op->precedence                  < 
                 unaryop_precedence( unaryop->operation )   );
    
  snprintf( buffer, length, "%s%s%s%s%s", operation_string,
	    brackets ? "( " : "", operand_buffer,
	    brackets ? " )" : "", operation_suffix );

  libspectrum_free( operand_buffer );

  return 0;
}

static int
deparse_binaryop( char *buffer, size_t length,
		  const struct binaryop_type *binaryop )
{
  char *operand1_buffer, *operand2_buffer; const char *operation_string = NULL;
  int brackets_necessary1, brackets_necessary2;

  int error;

  operand1_buffer = libspectrum_new( char, 2 * length );
  operand2_buffer = &operand1_buffer[ length ];

  error = debugger_expression_deparse( operand1_buffer, length,
				       binaryop->op1 );
  if( error ) { libspectrum_free( operand1_buffer ); return error; }

  error = debugger_expression_deparse( operand2_buffer, length,
				       binaryop->op2 );
  if( error ) { libspectrum_free( operand1_buffer ); return error; }

  switch( binaryop->operation ) {
  case    '+': operation_string = "+";  break;
  case    '-': operation_string = "-";  break;
  case    '*': operation_string = "*";  break;
  case    '/': operation_string = "/";  break;
  case DEBUGGER_TOKEN_EQUAL_TO: operation_string = "=="; break;
  case DEBUGGER_TOKEN_NOT_EQUAL_TO: operation_string = "!="; break;
  case    '<': operation_string = "<";  break;
  case    '>': operation_string = ">";  break;
  case DEBUGGER_TOKEN_LESS_THAN_OR_EQUAL_TO: operation_string = "<="; break;
  case DEBUGGER_TOKEN_GREATER_THAN_OR_EQUAL_TO: operation_string = ">="; break;
  case    '&': operation_string = "&";  break;
  case    '^': operation_string = "^";  break;
  case    '|': operation_string = "|";  break;
  case DEBUGGER_TOKEN_LOGICAL_AND: operation_string = "&&"; break;
  case DEBUGGER_TOKEN_LOGICAL_OR: operation_string = "||"; break;

  default:
    ui_error( UI_ERROR_ERROR, "unknown binary operation %d",
	      binaryop->operation );
    fuse_abort();
  }

  brackets_necessary1 = brackets_necessary( binaryop->operation,
					    binaryop->op1 );
  brackets_necessary2 = brackets_necessary( binaryop->operation,
					    binaryop->op2 );

  snprintf( buffer, length, "%s%s%s %s %s%s%s",
	    brackets_necessary1 ? "( " : "", operand1_buffer,
	    brackets_necessary1 ? " )" : "",
	    operation_string,
	    brackets_necessary2 ? "( " : "", operand2_buffer,
	    brackets_necessary2 ? " )" : "" );

  libspectrum_free( operand1_buffer );

  return 0;
}

/* When deparsing, do we need to put brackets around `operand' when
   being used as an operand of the binary operation `top_operation'? */
static int
brackets_necessary( int top_operation, debugger_expression *operand )
{
  enum precedence_t top_precedence, bottom_precedence;
  
  top_precedence = binaryop_precedence( top_operation );
  bottom_precedence = operand->precedence;

  /* If the top level operation has a higher precedence than the
     bottom level operation, we always need brackets */
  if( top_precedence > bottom_precedence ) return 1;

  /* If the two operations are of equal precedence, we need brackets
     i) if the top level operation is non-associative, or
     ii) if the operand is a non-associative operation

     Note the assumption here that all things with a precedence equal to
     a binary operator are also binary operators

     Strictly, we don't need brackets in either of these cases, but
     otherwise the user is going to have to remember operator
     left-right associativity; I think things are clearer with
     brackets in.
  */
  if( top_precedence == bottom_precedence ) {

    if( is_non_associative( top_operation ) ) return 1;

    /* Sanity check */
    if( operand->type != DEBUGGER_EXPRESSION_TYPE_BINARYOP ) {
      ui_error( UI_ERROR_ERROR,
		"binary operator has same precedence as non-binary operator" );
      fuse_abort();
    }

    return is_non_associative( operand->types.binaryop.operation );
  }

  /* Otherwise (ie if the top level operation is of lower precedence
     than the bottom, or both operators have equal precedence and
     everything is associative) we don't need brackets */
  return 0;
}

/* Is a binary operator non-associative? */
static int
is_non_associative( int operation )
{
  switch( operation ) {

  /* Simple cases */
  case '+': case '*': return 0;
  case '-': case '/': return 1;

  /* None of the comparison operators are associative due to them
     returning truth values */
  case DEBUGGER_TOKEN_EQUAL_TO:
  case DEBUGGER_TOKEN_NOT_EQUAL_TO:
  case '<': case '>':
  case DEBUGGER_TOKEN_LESS_THAN_OR_EQUAL_TO:
  case DEBUGGER_TOKEN_GREATER_THAN_OR_EQUAL_TO:
    return 1;

  /* The logical operators are associative */
  case DEBUGGER_TOKEN_LOGICAL_AND: return 0;
  case DEBUGGER_TOKEN_LOGICAL_OR: return 0;

  /* The bitwise operators are also associative (consider them as
     vectorised logical operators) */
  case    '&': return 0;
  case    '^': return 0;
  case    '|': return 0;

  }

  /* Should never get here */
  ui_error( UI_ERROR_ERROR, "unknown binary operation %d", operation );
  fuse_abort();
}

