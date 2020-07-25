/* sna.c: Routines for handling .sna snapshots
   Copyright (c) 2001-2002 Philip Kendall
   Copyright (c) 2016 Fredrick Meunier

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

#include <string.h>

#include "internals.h"

#define LIBSPECTRUM_SNA_HEADER_LENGTH 27
#define LIBSPECTRUM_SNA_128_HEADER_LENGTH 4

static int identify_machine( size_t buffer_length, libspectrum_snap *snap );
static int libspectrum_sna_read_header( const libspectrum_byte *buffer,
					size_t buffer_length,
					libspectrum_snap *snap );
static int libspectrum_sna_read_data( const libspectrum_byte *buffer,
				      size_t buffer_length,
				      libspectrum_snap *snap );
static int libspectrum_sna_read_128_header( const libspectrum_byte *buffer,
					    size_t buffer_length,
					    libspectrum_snap *snap );
static int libspectrum_sna_read_128_data( const libspectrum_byte *buffer,
					  size_t buffer_length,
					  libspectrum_snap *snap );

static void
write_header( libspectrum_buffer *buffer, libspectrum_snap *snap,
              libspectrum_word sp );
static libspectrum_error
write_48k_sna( libspectrum_buffer *buffer, libspectrum_snap *snap,
               libspectrum_word *new_sp );
static void
write_128k_sna( libspectrum_buffer *buffer, libspectrum_snap *snap );
static void
write_page( libspectrum_buffer *buffer, libspectrum_snap *snap, int page );

libspectrum_error
internal_sna_read( libspectrum_snap *snap,
		   const libspectrum_byte *buffer, size_t buffer_length )
{
  int error;

  error = identify_machine( buffer_length, snap );
  if( error != LIBSPECTRUM_ERROR_NONE ) return error;

  error = libspectrum_sna_read_header( buffer, buffer_length, snap );
  if( error != LIBSPECTRUM_ERROR_NONE ) return error;

  error = libspectrum_sna_read_data(
    &buffer[LIBSPECTRUM_SNA_HEADER_LENGTH],
    buffer_length - LIBSPECTRUM_SNA_HEADER_LENGTH, snap );
  if( error != LIBSPECTRUM_ERROR_NONE ) return error;

  return LIBSPECTRUM_ERROR_NONE;
}

static int
identify_machine( size_t buffer_length, libspectrum_snap *snap )
{
  switch( buffer_length ) {
  case 49179:
    libspectrum_snap_set_machine( snap, LIBSPECTRUM_MACHINE_48 );
    break;
  case 131103:
  case 147487:
    libspectrum_snap_set_machine( snap, LIBSPECTRUM_MACHINE_PENT );
    break;
  default:
    libspectrum_print_error( LIBSPECTRUM_ERROR_CORRUPT,
			     "libspectrum_sna_identify: unknown length" );
    return LIBSPECTRUM_ERROR_CORRUPT;
  }

  return LIBSPECTRUM_ERROR_NONE;
}

static int
libspectrum_sna_read_header( const libspectrum_byte *buffer,
			     size_t buffer_length, libspectrum_snap *snap )
{
  int iff;

  if( buffer_length < LIBSPECTRUM_SNA_HEADER_LENGTH ) {
    libspectrum_print_error(
      LIBSPECTRUM_ERROR_CORRUPT,
      "libspectrum_sna_read_header: not enough data in buffer"
    );
    return LIBSPECTRUM_ERROR_CORRUPT;
  }

  libspectrum_snap_set_a  ( snap, buffer[22] );
  libspectrum_snap_set_f  ( snap, buffer[21] );
  libspectrum_snap_set_bc ( snap, buffer[13] + buffer[14]*0x100 );
  libspectrum_snap_set_de ( snap, buffer[11] + buffer[12]*0x100 );
  libspectrum_snap_set_hl ( snap, buffer[ 9] + buffer[10]*0x100 );
  libspectrum_snap_set_a_ ( snap, buffer[ 8] );
  libspectrum_snap_set_f_ ( snap, buffer[ 7] );
  libspectrum_snap_set_bc_( snap, buffer[ 5] + buffer[ 6]*0x100 );
  libspectrum_snap_set_de_( snap, buffer[ 3] + buffer[ 4]*0x100 );
  libspectrum_snap_set_hl_( snap, buffer[ 1] + buffer[ 2]*0x100 );
  libspectrum_snap_set_ix ( snap, buffer[17] + buffer[18]*0x100 );
  libspectrum_snap_set_iy ( snap, buffer[15] + buffer[16]*0x100 );
  libspectrum_snap_set_i  ( snap, buffer[ 0] );
  libspectrum_snap_set_r  ( snap, buffer[20] );
  libspectrum_snap_set_pc ( snap, buffer[ 6] + buffer[ 7]*0x100 );
  libspectrum_snap_set_sp ( snap, buffer[23] + buffer[24]*0x100 );

  iff = ( buffer[19] & 0x04 ) ? 1 : 0;
  libspectrum_snap_set_iff1( snap, iff );
  libspectrum_snap_set_iff2( snap, iff );
  libspectrum_snap_set_im( snap, buffer[25] & 0x03 );

  libspectrum_snap_set_out_ula( snap, buffer[26] & 0x07 );

  return LIBSPECTRUM_ERROR_NONE;
}

static int
libspectrum_sna_read_data( const libspectrum_byte *buffer,
			   size_t buffer_length, libspectrum_snap *snap )
{
  int error, page, i;
  libspectrum_word sp, offset;

  if( buffer_length < 0xc000 ) {
    libspectrum_print_error(
      LIBSPECTRUM_ERROR_CORRUPT,
      "libspectrum_sna_read_data: not enough data in buffer"
    );
    return LIBSPECTRUM_ERROR_CORRUPT;
  }

  switch( libspectrum_snap_machine( snap ) ) {

  case LIBSPECTRUM_MACHINE_48:

    sp = libspectrum_snap_sp( snap );
    if( sp < 0x4000 || sp == 0xffff ) {
      libspectrum_print_error(
        LIBSPECTRUM_ERROR_CORRUPT,
        "libspectrum_sna_read_data: SP invalid (0x%04x)", sp
      );
      return LIBSPECTRUM_ERROR_CORRUPT;
    }

    /* Rescue PC from the stack */
    offset = sp - 0x4000;
    libspectrum_snap_set_pc( snap, buffer[offset] + 0x100 * buffer[offset+1] );

    /* Increase SP as PC has been unstacked */
    libspectrum_snap_set_sp( snap, libspectrum_snap_sp( snap ) + 2 );

    /* And split the pages up */
    error = libspectrum_split_to_48k_pages( snap, buffer );
    if( error != LIBSPECTRUM_ERROR_NONE ) return error;

    break;

  case LIBSPECTRUM_MACHINE_PENT:
    
    for( i=0; i<8; i++ ) {
      libspectrum_byte *ram = libspectrum_new( libspectrum_byte, 0x4000 );
      libspectrum_snap_set_pages( snap, i, ram );
    }

    memcpy( libspectrum_snap_pages( snap, 5 ), &buffer[0x0000], 0x4000 );
    memcpy( libspectrum_snap_pages( snap, 2 ), &buffer[0x4000], 0x4000 );

    error = libspectrum_sna_read_128_header( buffer + 0xc000,
					     buffer_length - 0xc000, snap );
    if( error != LIBSPECTRUM_ERROR_NONE ) return error;

    page = libspectrum_snap_out_128_memoryport( snap ) & 0x07;
    if( page == 5 || page == 2 ) {
      if( memcmp( libspectrum_snap_pages( snap, page ),
		  &buffer[0x8000], 0x4000 ) ) {
	libspectrum_print_error(
          LIBSPECTRUM_ERROR_CORRUPT,
	  "libspectrum_sna_read_data: duplicated page not identical"
	);
	return LIBSPECTRUM_ERROR_CORRUPT;
      }
    } else {
      memcpy( libspectrum_snap_pages( snap, page ), &buffer[0x8000], 0x4000 );
    }

    buffer += 0xc000 + LIBSPECTRUM_SNA_128_HEADER_LENGTH;
    buffer_length -= 0xc000 + LIBSPECTRUM_SNA_128_HEADER_LENGTH;
    error = libspectrum_sna_read_128_data( buffer, buffer_length, snap );
    if( error != LIBSPECTRUM_ERROR_NONE ) return error;

    break;

  default:
    libspectrum_print_error( LIBSPECTRUM_ERROR_LOGIC,
			     "libspectrum_sna_read_data: unknown machine" );
    return LIBSPECTRUM_ERROR_LOGIC;
  }
  
  return LIBSPECTRUM_ERROR_NONE;
}

static int
libspectrum_sna_read_128_header( const libspectrum_byte *buffer,
				 size_t buffer_length, libspectrum_snap *snap )
{
  if( buffer_length < 4 ) {
    libspectrum_print_error(
      LIBSPECTRUM_ERROR_CORRUPT,
      "libspectrum_sna_read_128_header: not enough data in buffer"
    );
    return LIBSPECTRUM_ERROR_CORRUPT;
  }

  libspectrum_snap_set_pc( snap, buffer[0] + 0x100 * buffer[1] );
  libspectrum_snap_set_out_128_memoryport( snap, buffer[2] );

  return LIBSPECTRUM_ERROR_NONE;
}

static int
libspectrum_sna_read_128_data( const libspectrum_byte *buffer,
			       size_t buffer_length, libspectrum_snap *snap )
{
  int i, page;

  page = libspectrum_snap_out_128_memoryport( snap ) & 0x07;

  for( i=0; i<=7; i++ ) {

    if( i==2 || i==5 || i==page ) continue; /* Already got this page */

    /* Check we've still got some data to read */
    if( buffer_length < 0x4000 ) {
      libspectrum_print_error(
        LIBSPECTRUM_ERROR_CORRUPT,
        "libspectrum_sna_read_128_data: not enough data in buffer"
      );
      return LIBSPECTRUM_ERROR_CORRUPT;
    }
    
    /* Copy the data across */
    memcpy( libspectrum_snap_pages( snap, i ), buffer, 0x4000 );
    
    /* And update what we're looking at here */
    buffer += 0x4000; buffer_length -= 0x4000;

  }

  return LIBSPECTRUM_ERROR_NONE;
}

libspectrum_error
libspectrum_sna_write( libspectrum_buffer *buffer, int *out_flags,
                       libspectrum_snap *snap, int in_flags GCC_UNUSED )
{
  libspectrum_word snap_sp;
  libspectrum_buffer *buffer_mem;
  libspectrum_error error = LIBSPECTRUM_ERROR_NONE;

  /* Minor info loss already due to things like tstate count, halted state,
     etc which are not stored in .sna format */
  *out_flags = LIBSPECTRUM_FLAG_SNAPSHOT_MINOR_INFO_LOSS;

  /* We don't store +D info at all */
  if( libspectrum_snap_plusd_active( snap ) )
    *out_flags |= LIBSPECTRUM_FLAG_SNAPSHOT_MAJOR_INFO_LOSS;

  /* We don't store Beta info at all */
  if( libspectrum_snap_beta_active( snap ) )
    *out_flags |= LIBSPECTRUM_FLAG_SNAPSHOT_MAJOR_INFO_LOSS;

  /* We don't store Opus info at all */
  if( libspectrum_snap_opus_active( snap ) )
    *out_flags |= LIBSPECTRUM_FLAG_SNAPSHOT_MAJOR_INFO_LOSS;

  /* We don't save IDE interface info at all */
  if( libspectrum_snap_zxatasp_active( snap ) )
    *out_flags |= LIBSPECTRUM_FLAG_SNAPSHOT_MAJOR_INFO_LOSS;
  if( libspectrum_snap_zxcf_active( snap ) )
    *out_flags |= LIBSPECTRUM_FLAG_SNAPSHOT_MAJOR_INFO_LOSS;
  if( libspectrum_snap_simpleide_active( snap ) )
    *out_flags |= LIBSPECTRUM_FLAG_SNAPSHOT_MAJOR_INFO_LOSS;
  if( libspectrum_snap_divide_active( snap ) )
    *out_flags |= LIBSPECTRUM_FLAG_SNAPSHOT_MAJOR_INFO_LOSS;

  /* We don't save the Interface 2 ROM at all */
  if( libspectrum_snap_interface2_active( snap ) )
    *out_flags |= LIBSPECTRUM_FLAG_SNAPSHOT_MAJOR_INFO_LOSS;

  /* We don't save the Timex Dock at all */
  if( libspectrum_snap_dock_active( snap ) )
    *out_flags |= LIBSPECTRUM_FLAG_SNAPSHOT_MAJOR_INFO_LOSS;

  /* We don't save custom ROMs at all */
  if( libspectrum_snap_custom_rom( snap ) )
    *out_flags |= LIBSPECTRUM_FLAG_SNAPSHOT_MAJOR_INFO_LOSS;

  /* We don't save AY interfaces  at all */
  if( libspectrum_snap_fuller_box_active( snap ) ||
      libspectrum_snap_melodik_active( snap ) )
    *out_flags |= LIBSPECTRUM_FLAG_SNAPSHOT_MAJOR_INFO_LOSS;

  /* We don't save the Specdrum state at all */
  if( libspectrum_snap_specdrum_active( snap ) )
    *out_flags |= LIBSPECTRUM_FLAG_SNAPSHOT_MAJOR_INFO_LOSS;

  /* We don't save the Spectranet state at all */
  if( libspectrum_snap_spectranet_active( snap ) )
    *out_flags |= LIBSPECTRUM_FLAG_SNAPSHOT_MAJOR_INFO_LOSS;

  /* We don't save the uSource state at all */
  if( libspectrum_snap_usource_active( snap ) )
    *out_flags |= LIBSPECTRUM_FLAG_SNAPSHOT_MAJOR_INFO_LOSS;

  /* We don't save the DISCiPLE state at all */
  if( libspectrum_snap_disciple_active( snap ) )
    *out_flags |= LIBSPECTRUM_FLAG_SNAPSHOT_MAJOR_INFO_LOSS;

  /* We don't save the Didaktik80 state at all */
  if( libspectrum_snap_didaktik80_active( snap ) )
    *out_flags |= LIBSPECTRUM_FLAG_SNAPSHOT_MAJOR_INFO_LOSS;

  /* We don't save the Covox state at all */
  if( libspectrum_snap_covox_active( snap ) )
    *out_flags |= LIBSPECTRUM_FLAG_SNAPSHOT_MAJOR_INFO_LOSS;

  /* We don't save the Multiface state at all */
  if( libspectrum_snap_multiface_active( snap ) )
    *out_flags |= LIBSPECTRUM_FLAG_SNAPSHOT_MAJOR_INFO_LOSS;

  /* We don't save the MMC state at all */
  if( libspectrum_snap_divmmc_active( snap ) )
    *out_flags |= LIBSPECTRUM_FLAG_SNAPSHOT_MAJOR_INFO_LOSS;
  if( libspectrum_snap_zxmmc_active( snap ) )
    *out_flags |= LIBSPECTRUM_FLAG_SNAPSHOT_MAJOR_INFO_LOSS;

  buffer_mem = libspectrum_buffer_alloc();

  switch( libspectrum_snap_machine( snap ) ) {

  case LIBSPECTRUM_MACHINE_48_NTSC:
  case LIBSPECTRUM_MACHINE_TC2048:
  case LIBSPECTRUM_MACHINE_TC2068:
  case LIBSPECTRUM_MACHINE_TS2068:
    *out_flags |= LIBSPECTRUM_FLAG_SNAPSHOT_MAJOR_INFO_LOSS;
    /* Fall through */
  case LIBSPECTRUM_MACHINE_16:
  case LIBSPECTRUM_MACHINE_48:
    error = write_48k_sna( buffer_mem, snap, &snap_sp );
    break;
    
  case LIBSPECTRUM_MACHINE_128:
  case LIBSPECTRUM_MACHINE_128E:
  case LIBSPECTRUM_MACHINE_PENT512:
  case LIBSPECTRUM_MACHINE_PENT1024:
  case LIBSPECTRUM_MACHINE_PLUS2:
  case LIBSPECTRUM_MACHINE_PLUS2A:
  case LIBSPECTRUM_MACHINE_PLUS3:
  case LIBSPECTRUM_MACHINE_PLUS3E:
  case LIBSPECTRUM_MACHINE_SCORP:
  case LIBSPECTRUM_MACHINE_SE:
    *out_flags |= LIBSPECTRUM_FLAG_SNAPSHOT_MAJOR_INFO_LOSS;
    /* Fall through */
  case LIBSPECTRUM_MACHINE_PENT:
    snap_sp = libspectrum_snap_sp ( snap );
    write_128k_sna( buffer_mem, snap );
    break;

  default:
    libspectrum_print_error( LIBSPECTRUM_ERROR_LOGIC,
			     "Emulated machine type is set to 'unknown'!" );
    libspectrum_buffer_free( buffer_mem );
    return LIBSPECTRUM_ERROR_LOGIC;
  }

  if( error ) {
    libspectrum_buffer_free( buffer_mem );
    return error;
  }

  write_header( buffer, snap, snap_sp );
  libspectrum_buffer_write_buffer( buffer, buffer_mem );
  libspectrum_buffer_free( buffer_mem );

  return LIBSPECTRUM_ERROR_NONE;
}

static void
write_header( libspectrum_buffer *buffer, libspectrum_snap *snap,
              libspectrum_word sp )
{
  libspectrum_buffer_write_byte( buffer, libspectrum_snap_i  ( snap ) );
  libspectrum_buffer_write_word( buffer, libspectrum_snap_hl_( snap ) );
  libspectrum_buffer_write_word( buffer, libspectrum_snap_de_( snap ) );
  libspectrum_buffer_write_word( buffer, libspectrum_snap_bc_( snap ) );
  libspectrum_buffer_write_byte( buffer, libspectrum_snap_f_ ( snap ) );
  libspectrum_buffer_write_byte( buffer, libspectrum_snap_a_ ( snap ) );
  libspectrum_buffer_write_word( buffer, libspectrum_snap_hl ( snap ) );
  libspectrum_buffer_write_word( buffer, libspectrum_snap_de ( snap ) );
  libspectrum_buffer_write_word( buffer, libspectrum_snap_bc ( snap ) );
  libspectrum_buffer_write_word( buffer, libspectrum_snap_iy ( snap ) );
  libspectrum_buffer_write_word( buffer, libspectrum_snap_ix ( snap ) );

  libspectrum_buffer_write_byte( buffer, libspectrum_snap_iff2( snap ) ? 0x04 : 0x00 );

  libspectrum_buffer_write_byte( buffer, libspectrum_snap_r ( snap ) );
  libspectrum_buffer_write_byte( buffer, libspectrum_snap_f ( snap ) );
  libspectrum_buffer_write_byte( buffer, libspectrum_snap_a ( snap ) );

  libspectrum_buffer_write_word( buffer, sp );

  libspectrum_buffer_write_byte( buffer, libspectrum_snap_im( snap ) );
  libspectrum_buffer_write_byte( buffer, libspectrum_snap_out_ula( snap ) & 0x07 );
}

static libspectrum_error
write_48k_sna( libspectrum_buffer *buffer, libspectrum_snap *snap,
               libspectrum_word *new_sp )
{
  libspectrum_byte *stack, *memory_base;
  size_t offset;
  libspectrum_word new_stack_address;

  /* Must have somewhere in RAM to store PC */
  if( libspectrum_snap_sp( snap ) < 0x4002 ) {
    libspectrum_print_error( LIBSPECTRUM_ERROR_INVALID,
			     "SP is too low (0x%04x) to stack PC",
			     libspectrum_snap_sp( snap ) );
    return LIBSPECTRUM_ERROR_INVALID;
  }

  offset = libspectrum_buffer_get_data_size( buffer );

  write_page( buffer, snap, 5 );
  write_page( buffer, snap, 2 );
  write_page( buffer, snap, 0 );

  memory_base = libspectrum_buffer_get_data( buffer ) + offset;

  /* Place PC on the stack */
  new_stack_address = libspectrum_snap_sp( snap ) - 2;
  stack = &( memory_base[ new_stack_address - 0x4000 ] );
  libspectrum_write_word( &stack, libspectrum_snap_pc( snap ) );

  /* Store the new value of SP */
  *new_sp = new_stack_address;

  return LIBSPECTRUM_ERROR_NONE;
}

static void
write_128k_sna( libspectrum_buffer *buffer, libspectrum_snap *snap )
{
  size_t i, page;
  
  page = libspectrum_snap_out_128_memoryport( snap ) & 0x07;

  write_page( buffer, snap, 5 );
  write_page( buffer, snap, 2 );
  write_page( buffer, snap, page );

  libspectrum_buffer_write_word( buffer, libspectrum_snap_pc( snap ) );
  libspectrum_buffer_write_byte( buffer,
                                 libspectrum_snap_out_128_memoryport( snap ) );
  libspectrum_buffer_write_byte( buffer, '\0' );

  for( i = 0; i < 8; i++ ) {

    /* Already written pages 5, 2 and whatever's paged in */
    if( i == 5 || i == 2 || i == page ) continue;

    write_page( buffer, snap, i );
  }
}

static void
write_page( libspectrum_buffer *buffer, libspectrum_snap *snap, int page )
{
  libspectrum_byte *ram;

  ram = libspectrum_snap_pages( snap, page );
  if( ram ) {
    libspectrum_buffer_write( buffer, ram, 0x4000 );
  } else {
    libspectrum_buffer_set( buffer, 0xff, 0x4000 );
  }
}
