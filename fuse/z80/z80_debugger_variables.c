/* z80_debugger_variables.c: routines to expose Z80 registers to the debugger
   Copyright (c) 2016 Philip Kendall

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

#include <libspectrum.h>

#include "debugger/debugger.h"
#include "z80.h"
#include "z80_internals.h"
#include "z80_macros.h"

static const char * const debugger_type_string = "z80";

static const char * const a_detail_string = "a";
static const char * const b_detail_string = "b";
static const char * const c_detail_string = "c";
static const char * const d_detail_string = "d";
static const char * const e_detail_string = "e";
static const char * const f_detail_string = "f";
static const char * const h_detail_string = "h";
static const char * const l_detail_string = "l";

static const char * const a__detail_string = "a'";
static const char * const b__detail_string = "b'";
static const char * const c__detail_string = "c'";
static const char * const d__detail_string = "d'";
static const char * const e__detail_string = "e'";
static const char * const f__detail_string = "f'";
static const char * const h__detail_string = "h'";
static const char * const l__detail_string = "l'";

static const char * const af_detail_string = "af";
static const char * const bc_detail_string = "bc";
static const char * const de_detail_string = "de";
static const char * const hl_detail_string = "hl";

static const char * const af__detail_string = "af'";
static const char * const bc__detail_string = "bc'";
static const char * const de__detail_string = "de'";
static const char * const hl__detail_string = "hl'";

static const char * const sp_detail_string = "sp";
static const char * const pc_detail_string = "pc";
static const char * const ix_detail_string = "ix";
static const char * const iy_detail_string = "iy";

static const char * const i_detail_string = "i";
static const char * const r_detail_string = "r";

static const char * const memptr_detail_string = "memptr";
static const char * const wz_detail_string = "wz"; /* Synonym for MEMPTR */

static const char * const im_detail_string = "im";
static const char * const iff1_detail_string = "iff1";
static const char * const iff2_detail_string = "iff2";

static const char * const q_detail_string = "q";

#define DEBUGGER_CALLBACKS(reg) static libspectrum_dword \
get_##reg( void ) \
{ \
  return reg; \
} \
\
static void \
set_##reg( libspectrum_dword value ) \
{ \
  reg = value; \
}

DEBUGGER_CALLBACKS(A)
DEBUGGER_CALLBACKS(B)
DEBUGGER_CALLBACKS(C)
DEBUGGER_CALLBACKS(D)
DEBUGGER_CALLBACKS(E)
DEBUGGER_CALLBACKS(F)
DEBUGGER_CALLBACKS(H)
DEBUGGER_CALLBACKS(L)

DEBUGGER_CALLBACKS(A_)
DEBUGGER_CALLBACKS(B_)
DEBUGGER_CALLBACKS(C_)
DEBUGGER_CALLBACKS(D_)
DEBUGGER_CALLBACKS(E_)
DEBUGGER_CALLBACKS(F_)
DEBUGGER_CALLBACKS(H_)
DEBUGGER_CALLBACKS(L_)

DEBUGGER_CALLBACKS(AF)
DEBUGGER_CALLBACKS(BC)
DEBUGGER_CALLBACKS(DE)
DEBUGGER_CALLBACKS(HL)

DEBUGGER_CALLBACKS(AF_)
DEBUGGER_CALLBACKS(BC_)
DEBUGGER_CALLBACKS(DE_)
DEBUGGER_CALLBACKS(HL_)

DEBUGGER_CALLBACKS(SP)
DEBUGGER_CALLBACKS(PC)
DEBUGGER_CALLBACKS(IX)
DEBUGGER_CALLBACKS(IY)

DEBUGGER_CALLBACKS(I)

DEBUGGER_CALLBACKS(Q)

static libspectrum_dword
get_R( void )
{
  return ( R7 & 0x80 ) | ( R & 0x7f );
}

static void
set_R( libspectrum_dword value )
{
  R = R7 = value;
}

static libspectrum_dword
get_memptr( void )
{
  return z80.memptr.w;
}

static void
set_memptr( libspectrum_dword value )
{
  z80.memptr.w = value;
}

static libspectrum_dword
get_IM( void )
{
  return IM;
}

static void
set_IM( libspectrum_dword value )
{
  if( value <= 2 ) IM = value;
}

static libspectrum_dword
get_IFF1( void )
{
  return IFF1;
}

static void
set_IFF1( libspectrum_dword value )
{
  IFF1 = !!value;
}

static libspectrum_dword
get_IFF2( void )
{
  return IFF2;
}

static void
set_IFF2( libspectrum_dword value )
{
  IFF2 = !!value;
}

void
z80_debugger_variables_init( void )
{
  debugger_system_variable_register( debugger_type_string, a_detail_string,
                                     get_A, set_A );
  debugger_system_variable_register( debugger_type_string, b_detail_string,
                                     get_B, set_B );
  debugger_system_variable_register( debugger_type_string, c_detail_string,
                                     get_C, set_C );
  debugger_system_variable_register( debugger_type_string, d_detail_string, 
                                     get_D, set_D );
  debugger_system_variable_register( debugger_type_string, e_detail_string, 
                                     get_E, set_E );
  debugger_system_variable_register( debugger_type_string, f_detail_string, 
                                     get_F, set_F );
  debugger_system_variable_register( debugger_type_string, h_detail_string, 
                                     get_H, set_H );
  debugger_system_variable_register( debugger_type_string, l_detail_string, 
                                     get_L, set_L );
  
  debugger_system_variable_register( debugger_type_string, a__detail_string, 
                                     get_A_, set_A_ );
  debugger_system_variable_register( debugger_type_string, b__detail_string, 
                                     get_B_, set_B_ );
  debugger_system_variable_register( debugger_type_string, c__detail_string, 
                                     get_C_, set_C_ );
  debugger_system_variable_register( debugger_type_string, d__detail_string, 
                                     get_D_, set_D_ );
  debugger_system_variable_register( debugger_type_string, e__detail_string, 
                                     get_E_, set_E_ );
  debugger_system_variable_register( debugger_type_string, f__detail_string, 
                                     get_F_, set_F_ );
  debugger_system_variable_register( debugger_type_string, h__detail_string, 
                                     get_H_, set_H_ );
  debugger_system_variable_register( debugger_type_string, l__detail_string, 
                                     get_L_, set_L_ );
  
  debugger_system_variable_register( debugger_type_string, af_detail_string, 
                                     get_AF, set_AF );
  debugger_system_variable_register( debugger_type_string, bc_detail_string, 
                                     get_BC, set_BC );
  debugger_system_variable_register( debugger_type_string, de_detail_string, 
                                     get_DE, set_DE );
  debugger_system_variable_register( debugger_type_string, hl_detail_string, 
                                     get_HL, set_HL );

  debugger_system_variable_register( debugger_type_string, af__detail_string, 
                                     get_AF_, set_AF_ );
  debugger_system_variable_register( debugger_type_string, bc__detail_string, 
                                     get_BC_, set_BC_ );
  debugger_system_variable_register( debugger_type_string, de__detail_string, 
                                     get_DE_, set_DE_ );
  debugger_system_variable_register( debugger_type_string, hl__detail_string, 
                                     get_HL_, set_HL_ );

  debugger_system_variable_register( debugger_type_string, sp_detail_string, 
                                     get_SP, set_SP );
  debugger_system_variable_register( debugger_type_string, pc_detail_string, 
                                     get_PC, set_PC );
  debugger_system_variable_register( debugger_type_string, ix_detail_string, 
                                     get_IX, set_IX );
  debugger_system_variable_register( debugger_type_string, iy_detail_string, 
                                     get_IY, set_IY );
  
  debugger_system_variable_register( debugger_type_string, i_detail_string, 
                                     get_I, set_I );
  debugger_system_variable_register( debugger_type_string, r_detail_string, 
                                     get_R, set_R );

  debugger_system_variable_register( debugger_type_string, memptr_detail_string,
		  		     get_memptr, set_memptr );
  /* WZ is a synonym for MEMPTR */
  debugger_system_variable_register( debugger_type_string, wz_detail_string,
		  		     get_memptr, set_memptr );

  debugger_system_variable_register( debugger_type_string, im_detail_string, 
                                     get_IM, set_IM );
  debugger_system_variable_register( debugger_type_string, iff1_detail_string, 
                                     get_IFF1, set_IFF1 );
  debugger_system_variable_register( debugger_type_string, iff2_detail_string, 
                                     get_IFF2, set_IFF2 );

  debugger_system_variable_register( debugger_type_string, q_detail_string, 
                                     get_Q, set_Q );
}
