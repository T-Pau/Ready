/* szx.c: SZX test routines
   Copyright (c) 2017 Philip Kendall

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

/* This file contains a number of routines for checking the contents of an SZX
   file. It very deliberately does not use any of the core libspectrum code for
   this as that would defeat the point of a unit test */

#include <config.h>

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "internals.h"
#include "test.h"

typedef struct szx_chunk_t {
  libspectrum_byte *data;
  size_t length;
} szx_chunk_t;

static szx_chunk_t*
find_szx_chunk( libspectrum_buffer *buffer, const char *search )
{
  libspectrum_byte *data;
  size_t data_remaining;
  libspectrum_byte id[5] = {0};
  libspectrum_byte length_buffer[4];
  libspectrum_dword length;

  data = libspectrum_buffer_get_data( buffer );
  data_remaining = libspectrum_buffer_get_data_size( buffer );

  if( data_remaining < 8 ) {
    fprintf( stderr, "SZX file is less than 8 bytes long\n" );
    return NULL;
  }

  /* Skip header */
  data += 8; data_remaining -= 8;

  while( data_remaining > 0 ) {
    if( data_remaining < 8 ) {
      fprintf( stderr, "Chunk is less than 8 bytes long\n" );
      return NULL;
    }

    memcpy( id, data, 4 );
    data += 4; data_remaining -= 4;

    memcpy( length_buffer, data, 4 );
    data += 4; data_remaining -= 4;

    length = length_buffer[0] + (length_buffer[1] << 8) +
      (length_buffer[2] << 16) + (length_buffer[3] << 24);

    if( data_remaining < length ) {
      fprintf( stderr, "Not enough data for chunk\n" );
      return NULL;
    }

    if( !memcmp( id, search, 4 ) ) {
      szx_chunk_t *chunk = libspectrum_malloc( sizeof(*chunk) );
      libspectrum_byte *chunk_data = libspectrum_malloc( length );
      memcpy( chunk_data, data, length );
      chunk->data = chunk_data;
      chunk->length = length;
      return chunk;
    }

    data += length; data_remaining -= length;
  }

  return NULL;
}

static test_return_t
szx_write_block_test_with_flags( const char *id, libspectrum_machine machine,
    int flags, void (*setter)( libspectrum_snap* ),
    libspectrum_byte *expected, size_t expected_length, size_t total_length )
{
  libspectrum_snap *snap;
  libspectrum_buffer *buffer;
  int out_flags;
  szx_chunk_t *chunk;
  test_return_t r = TEST_INCOMPLETE;

  buffer = libspectrum_buffer_alloc();

  snap = libspectrum_snap_alloc();

  libspectrum_snap_set_machine( snap, machine );

  setter( snap );

  libspectrum_szx_write( buffer, &out_flags, snap, NULL, flags );
  libspectrum_snap_free( snap );

  chunk = find_szx_chunk( buffer, id );
  if( !chunk ) {
    fprintf( stderr, "Chunk not found\n" );
    return TEST_FAIL;
  }

  libspectrum_buffer_free( buffer );

  if( chunk->length == total_length ) {
    if( memcmp( chunk->data, expected, expected_length ) ) {
      fprintf( stderr, "Chunk has wrong initial data\n" );
      r = TEST_FAIL;
    } else {
      r = TEST_PASS;
      for( size_t i = expected_length; i < total_length; i++ ) {
        if( chunk->data[i] ) {
          r = TEST_FAIL;
          break;
        }
      }
    }
  } else {
    fprintf( stderr, "Chunk has wrong length\n" );
    r = TEST_FAIL;
  }

  libspectrum_free( chunk->data );
  libspectrum_free( chunk );

  return r;
}

static test_return_t
szx_write_block_test( const char *id, libspectrum_machine machine,
    void (*setter)( libspectrum_snap* ),
    libspectrum_byte *expected, size_t expected_length )
{
  return szx_write_block_test_with_flags( id, machine, 0, setter,
      expected, expected_length, expected_length );
}

static test_return_t
szx_write_uncompressed_block_test( const char *id, libspectrum_machine machine,
    void (*setter)( libspectrum_snap* ),
    libspectrum_byte *expected, size_t expected_length, size_t total_length )
{
  return szx_write_block_test_with_flags( id, machine,
      LIBSPECTRUM_FLAG_SNAPSHOT_NO_COMPRESSION, setter,
      expected, expected_length, total_length );
}

static void
z80r_setter( libspectrum_snap *snap )
{
  libspectrum_snap_set_a( snap, 0xc4 );
  libspectrum_snap_set_f( snap, 0x1f );
  libspectrum_snap_set_bc( snap, 0x0306 );
  libspectrum_snap_set_de( snap, 0x06e4 );
  libspectrum_snap_set_hl( snap, 0x0154 );

  libspectrum_snap_set_a_( snap, 0x69 );
  libspectrum_snap_set_f_( snap, 0x07 );
  libspectrum_snap_set_bc_( snap, 0xe7dc );
  libspectrum_snap_set_de_( snap, 0xc3d0 );
  libspectrum_snap_set_hl_( snap, 0xdccb );

  libspectrum_snap_set_ix( snap, 0x8ba3 );
  libspectrum_snap_set_iy( snap, 0x1c13 );
  libspectrum_snap_set_sp( snap, 0xf86d );
  libspectrum_snap_set_pc( snap, 0xc81e );

  libspectrum_snap_set_i( snap, 0x19 );
  libspectrum_snap_set_r( snap, 0x84 );
  libspectrum_snap_set_iff1( snap, 1 );
  libspectrum_snap_set_iff2( snap, 0 );
  libspectrum_snap_set_im( snap, 2 );

  libspectrum_snap_set_tstates( snap, 40 );

  libspectrum_snap_set_last_instruction_ei( snap, 1 );
  libspectrum_snap_set_halted( snap, 0 );
  libspectrum_snap_set_last_instruction_set_f( snap, 1 );

  libspectrum_snap_set_memptr( snap, 0xdc03 );
}

static libspectrum_byte
test_31_expected[] = {
  0x1f, 0xc4, 0x06, 0x03, 0xe4, 0x06, 0x54, 0x01, /* AF, BC, DE, HL */
  0x07, 0x69, 0xdc, 0xe7, 0xd0, 0xc3, 0xcb, 0xdc, /* AF', BC', DE', HL' */
  0xa3, 0x8b, 0x13, 0x1c, 0x6d, 0xf8, 0x1e, 0xc8, /* IX, IY, SP, PC */
  0x19, 0x84, 0x01, 0x00, 0x02, /* I, R, IFF1, IFF2, IM */
  0x28, 0x00, 0x00, 0x00, 0x08, /* tstates, tstates until /INT goes high */
  0x05, /* flags */
  0x03, 0xdc /* MEMPTR */
};

test_return_t
test_31( void )
{
  return szx_write_block_test( "Z80R", LIBSPECTRUM_MACHINE_48, z80r_setter,
      test_31_expected, ARRAY_SIZE(test_31_expected) );
}

static void
spcr_setter( libspectrum_snap *snap )
{
  libspectrum_snap_set_out_ula( snap, 0xfa );
  libspectrum_snap_set_out_128_memoryport( snap, 0x6f );
  libspectrum_snap_set_out_plus3_memoryport( snap, 0x28 );
}

static libspectrum_byte
test_32_expected[] = {
  0x02, 0x6f, 0x28, 0xfa, /* Border, 128, +3, ULA */
  0x00, 0x00, 0x00, 0x00 /* Reserved */
};

test_return_t
test_32( void )
{
  return szx_write_block_test( "SPCR", LIBSPECTRUM_MACHINE_PLUS3, spcr_setter,
      test_32_expected, ARRAY_SIZE(test_32_expected) );
}

static void
joy_setter( libspectrum_snap *snap )
{
  libspectrum_snap_set_joystick_active_count( snap, 2 );
  libspectrum_snap_set_joystick_list( snap, 0, LIBSPECTRUM_JOYSTICK_KEMPSTON );
  libspectrum_snap_set_joystick_inputs( snap, 0,
      LIBSPECTRUM_JOYSTICK_INPUT_JOYSTICK_1 );
  libspectrum_snap_set_joystick_list( snap, 1, LIBSPECTRUM_JOYSTICK_SINCLAIR_1 );
  libspectrum_snap_set_joystick_inputs( snap, 1,
      LIBSPECTRUM_JOYSTICK_INPUT_JOYSTICK_2 );
}

static libspectrum_byte
test_33_expected[] = {
  0x01, 0x00, 0x00, 0x00, /* Flags */
  0x00, 0x03 /* Joystick 1 = Kempston, Joystick 2 = Sinclair 1 */
};

test_return_t
test_33( void )
{
  return szx_write_block_test( "JOY\0", LIBSPECTRUM_MACHINE_48, joy_setter,
      test_33_expected, ARRAY_SIZE(test_33_expected) );
}

static void
keyb_setter( libspectrum_snap *snap )
{
  libspectrum_snap_set_issue2( snap, 1 );
  libspectrum_snap_set_joystick_active_count( snap, 1 );
  libspectrum_snap_set_joystick_list( snap, 0, LIBSPECTRUM_JOYSTICK_CURSOR );
  libspectrum_snap_set_joystick_inputs( snap, 0,
      LIBSPECTRUM_JOYSTICK_INPUT_KEYBOARD );
}

static libspectrum_byte
test_34_expected[] = {
  0x01, 0x00, 0x00, 0x00, /* Flags */
  0x02 /* Cursor joystick */
};

test_return_t
test_34( void )
{
  return szx_write_block_test( "KEYB", LIBSPECTRUM_MACHINE_48, keyb_setter,
      test_34_expected, ARRAY_SIZE(test_34_expected) );
}

static void
zxpr_setter( libspectrum_snap *snap )
{
  libspectrum_snap_set_zx_printer_active( snap, 1 );
}

static libspectrum_byte
test_35_expected[] = {
  0x01, 0x00 /* Flags */
};

test_return_t
test_35( void )
{
  return szx_write_block_test( "ZXPR", LIBSPECTRUM_MACHINE_48, zxpr_setter,
      test_35_expected, ARRAY_SIZE(test_35_expected) );
}

static libspectrum_byte
ay_registers_data[] = {
  0x73, 0x03, 0xb1, 0x00, 0xbb, 0x0c, 0x19, 0x0f,
  0x1e, 0x07, 0x11, 0x71, 0x6c, 0x0a, 0x2b, 0x41
};

static void
ay_setter( libspectrum_snap *snap )
{
  size_t i;

  libspectrum_snap_set_fuller_box_active( snap, 1 );
  libspectrum_snap_set_melodik_active( snap, 0 );
  libspectrum_snap_set_out_ay_registerport( snap, 0x08 );

  for( i = 0; i < 16; i++ ) {
    libspectrum_snap_set_ay_registers( snap, i, ay_registers_data[i] );
  }
}

static libspectrum_byte
test_36_expected[] = {
  0x01, /* Flags */
  0x08, /* Register port */
  0x73, 0x03, 0xb1, 0x00, 0xbb, 0x0c, 0x19, 0x0f, /* Registers 0x00 - 0x07 */
  0x1e, 0x07, 0x11, 0x71, 0x6c, 0x0a, 0x2b, 0x41 /* Register 0x08 - 0x0f */
};

test_return_t
test_36( void )
{
  return szx_write_block_test( "AY\0\0", LIBSPECTRUM_MACHINE_48, ay_setter,
      test_36_expected, ARRAY_SIZE(test_36_expected) );
}

static void
scld_setter( libspectrum_snap *snap )
{
  libspectrum_snap_set_out_scld_hsr( snap, 0x49 );
  libspectrum_snap_set_out_scld_dec( snap, 0x9d );
}

static libspectrum_byte
test_37_expected[] = {
  0x49, 0x9d
};

test_return_t
test_37( void )
{
  return szx_write_block_test( "SCLD", LIBSPECTRUM_MACHINE_TC2048, scld_setter,
      test_37_expected, ARRAY_SIZE(test_37_expected) );
}

static void
zxat_setter( libspectrum_snap *snap )
{
  libspectrum_snap_set_zxatasp_active( snap, 1 );

  libspectrum_snap_set_zxatasp_upload( snap, 1 );
  libspectrum_snap_set_zxatasp_writeprotect( snap, 0 );
  libspectrum_snap_set_zxatasp_port_a( snap, 0xab );
  libspectrum_snap_set_zxatasp_port_b( snap, 0x8c );
  libspectrum_snap_set_zxatasp_port_c( snap, 0x82 );
  libspectrum_snap_set_zxatasp_control( snap, 0xd8 );
  libspectrum_snap_set_zxatasp_pages( snap, 0x18 );
  libspectrum_snap_set_zxatasp_current_page( snap, 0x11 );
}

static libspectrum_byte
test_38_expected[] = {
  0x01, 0x00, /* Flags */
  0xab, 0x8c, 0x82, 0xd8, /* Ports */
  0x18, 0x11 /* Page count and current page */
};

test_return_t
test_38( void )
{
  return szx_write_block_test( "ZXAT", LIBSPECTRUM_MACHINE_48, zxat_setter,
      test_38_expected, ARRAY_SIZE(test_38_expected) );
}

static void
zxcf_setter( libspectrum_snap *snap )
{
  libspectrum_snap_set_zxcf_active( snap, 1 );

  libspectrum_snap_set_zxcf_upload( snap, 1 );
  libspectrum_snap_set_zxcf_memctl( snap, 0x37 );
  libspectrum_snap_set_zxcf_pages( snap, 0x55 );
}

static libspectrum_byte
test_39_expected[] = {
  0x01, 0x00, /* Flags */
  0x37, /* Memory control */
  0x55 /* Page count */
};

test_return_t
test_39( void )
{
  return szx_write_block_test( "ZXCF", LIBSPECTRUM_MACHINE_48, zxcf_setter,
      test_39_expected, ARRAY_SIZE(test_39_expected) );
}

static void
amxm_setter( libspectrum_snap *snap )
{
  libspectrum_snap_set_kempston_mouse_active( snap, 1 );
}

static libspectrum_byte
test_40_expected[] = {
  0x02, /* Kempston mouse */
  0x00, 0x00, 0x00, /* AMX mouse CTRLA registers */
  0x00, 0x00, 0x00 /* AMX mouse CTRLB registers */
};

test_return_t
test_40( void )
{
  return szx_write_block_test( "AMXM", LIBSPECTRUM_MACHINE_48, amxm_setter,
      test_40_expected, ARRAY_SIZE(test_40_expected) );
}

static void
side_setter( libspectrum_snap *snap )
{
  libspectrum_snap_set_simpleide_active( snap, 1 );
}

static libspectrum_byte
empty_chunk_expected[] = { /* Empty */ };

test_return_t
test_41( void )
{
  return szx_write_block_test( "SIDE", LIBSPECTRUM_MACHINE_48, side_setter,
      empty_chunk_expected, ARRAY_SIZE(empty_chunk_expected) );
}

static void
drum_setter( libspectrum_snap *snap )
{
  libspectrum_snap_set_specdrum_active( snap, 1 );
  libspectrum_snap_set_specdrum_dac( snap, -0x3b );
}

static libspectrum_byte
test_42_expected[] = {
  0x45 /* DAC + 128 */
};

test_return_t
test_42( void )
{
  return szx_write_block_test( "DRUM", LIBSPECTRUM_MACHINE_48, drum_setter,
      test_42_expected, ARRAY_SIZE(test_42_expected) );
}

static void
covx_setter( libspectrum_snap *snap )
{
  libspectrum_snap_set_covox_active( snap, 1 );
  libspectrum_snap_set_covox_dac( snap, 0xc0 );
}

static libspectrum_byte
test_43_expected[] = {
  0xc0, /* DAC */
  0x00, 0x00, 0x00 /* Reserved */
};

test_return_t
test_43( void )
{
  return szx_write_block_test( "COVX", LIBSPECTRUM_MACHINE_48, covx_setter,
      test_43_expected, ARRAY_SIZE(test_43_expected) );
}

static void
zmmc_setter( libspectrum_snap *snap )
{
  libspectrum_snap_set_zxmmc_active( snap, 1 );
}

test_return_t
test_57( void )
{
  return szx_write_block_test( "ZMMC", LIBSPECTRUM_MACHINE_48, zmmc_setter,
      empty_chunk_expected, ARRAY_SIZE(empty_chunk_expected) );
}

static void
ramp_setter( libspectrum_snap *snap )
{
  libspectrum_byte *ram = libspectrum_malloc0_n( 1, 0x4000 );
  libspectrum_snap_set_pages( snap, 0, ram );
}

static libspectrum_byte
empty_ram_page_expected[] = {
  0x01, 0x00, /* Flags */
  0x00, /* Page number */
  /* 16 Kb of zeros compressed */
  0x78, 0xda, 0xed, 0xc1, 0x31, 0x01, 0x00, 0x00,
  0x00, 0xc2, 0xa0, 0xf5, 0x4f, 0x6d, 0x0c, 0x1f,
  0xa0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x80, 0xb7, 0x01, 0x40, 0x00, 0x00, 0x01
};

test_return_t
test_59( void )
{
  return szx_write_block_test( "RAMP", LIBSPECTRUM_MACHINE_48, ramp_setter,
      empty_ram_page_expected, ARRAY_SIZE(empty_ram_page_expected) );
}

static void
atrp_setter( libspectrum_snap *snap )
{
  libspectrum_byte *ram;

  libspectrum_snap_set_zxatasp_active( snap, 1 );
  libspectrum_snap_set_zxatasp_pages( snap, 1 );

  ram = libspectrum_malloc0_n( 1, 0x4000 );
  libspectrum_snap_set_zxatasp_ram( snap, 0, ram );
}

test_return_t
test_61( void )
{
  return szx_write_block_test( "ATRP", LIBSPECTRUM_MACHINE_48, atrp_setter,
      empty_ram_page_expected, ARRAY_SIZE(empty_ram_page_expected) );
}

static void
cfrp_setter( libspectrum_snap *snap )
{
  libspectrum_byte *ram;

  libspectrum_snap_set_zxcf_active( snap, 1 );
  libspectrum_snap_set_zxcf_pages( snap, 1 );

  ram = libspectrum_malloc0_n( 1, 0x4000 );
  libspectrum_snap_set_zxcf_ram( snap, 0, ram );
}

test_return_t
test_62( void )
{
  return szx_write_block_test( "CFRP", LIBSPECTRUM_MACHINE_48, cfrp_setter,
      empty_ram_page_expected, ARRAY_SIZE(empty_ram_page_expected) );
}

static libspectrum_byte
empty_ram_page_start[] = {
  0x00, 0x00, /* Flags */
  0x00, /* Page number */
  /* Followed by 16 Kb of uncompressed zeros */
};

test_return_t
test_65( void )
{
  return szx_write_uncompressed_block_test( "RAMP", LIBSPECTRUM_MACHINE_48,
      ramp_setter, empty_ram_page_start, ARRAY_SIZE(empty_ram_page_start),
      ARRAY_SIZE(empty_ram_page_start) + 0x4000 );
}

test_return_t
test_66( void )
{
  return szx_write_uncompressed_block_test( "ATRP", LIBSPECTRUM_MACHINE_48,
      atrp_setter, empty_ram_page_start, ARRAY_SIZE(empty_ram_page_start),
      ARRAY_SIZE(empty_ram_page_start) + 0x4000 );
}

test_return_t
test_67( void )
{
  return szx_write_uncompressed_block_test( "CFRP", LIBSPECTRUM_MACHINE_48,
      cfrp_setter, empty_ram_page_start, ARRAY_SIZE(empty_ram_page_start),
      ARRAY_SIZE(empty_ram_page_start) + 0x4000 );
}

static test_return_t
szx_read_block_test_with_template( const char *id, const char *template,
    int (*check_fn)( libspectrum_snap* ) )
{
  char filename[ 256 ];
  libspectrum_byte *buffer = NULL;
  size_t filesize = 0;
  libspectrum_snap *snap;
  int failed = 0;

  snprintf( filename, 256, template, id );

  if( read_file( &buffer, &filesize, filename ) ) return TEST_INCOMPLETE;

  snap = libspectrum_snap_alloc();

  if( libspectrum_snap_read( snap, buffer, filesize, LIBSPECTRUM_ID_UNKNOWN,
			     filename ) != LIBSPECTRUM_ERROR_NONE ) {
    fprintf( stderr, "%s: error reading `%s'\n", progname, filename );
    libspectrum_snap_free( snap );
    libspectrum_free( buffer );
    return TEST_INCOMPLETE;
  }

  libspectrum_free( buffer );

  failed = check_fn( snap );

  libspectrum_snap_free( snap );

  return failed ? TEST_FAIL : TEST_PASS;
}

static test_return_t
szx_read_block_test( const char *id, int (*check_fn)( libspectrum_snap* ) )
{
  return szx_read_block_test_with_template( id,
      STATIC_TEST_PATH( "szx-chunks/%s.szx" ), check_fn );
}

static test_return_t
szx_read_block_from_compressed_snap_test( const char *id,
    int (*check_fn)( libspectrum_snap* ) )
{
  return szx_read_block_test_with_template( id,
      STATIC_TEST_PATH( "szx-chunks/%s-uncompressed.szx.gz" ), check_fn );
}

static int
test_44_check( libspectrum_snap *snap )
{
  int failed = 0;

  if( libspectrum_snap_a( snap ) != 0xc4 ) failed = 1;

  if( libspectrum_snap_a( snap ) != 0xc4 ) failed = 1;
  if( libspectrum_snap_f( snap ) != 0x1f ) failed = 1;
  if( libspectrum_snap_bc( snap ) != 0x0306 ) failed = 1;
  if( libspectrum_snap_de( snap ) != 0x06e4 ) failed = 1;
  if( libspectrum_snap_hl( snap ) != 0x0154 ) failed = 1;

  if( libspectrum_snap_a_( snap ) != 0x69 ) failed = 1;
  if( libspectrum_snap_f_( snap ) != 0x07 ) failed = 1;
  if( libspectrum_snap_bc_( snap ) != 0xe7dc ) failed = 1;
  if( libspectrum_snap_de_( snap ) != 0xc3d0 ) failed = 1;
  if( libspectrum_snap_hl_( snap ) != 0xdccb ) failed = 1;

  if( libspectrum_snap_ix( snap ) != 0x8ba3 ) failed = 1;
  if( libspectrum_snap_iy( snap ) != 0x1c13 ) failed = 1;
  if( libspectrum_snap_sp( snap ) != 0xf86d ) failed = 1;
  if( libspectrum_snap_pc( snap ) != 0xc81e ) failed = 1;

  if( libspectrum_snap_i( snap ) != 0x19 ) failed = 1;
  if( libspectrum_snap_r( snap ) != 0x84 ) failed = 1;
  if( libspectrum_snap_iff1( snap ) != 1 ) failed = 1;
  if( libspectrum_snap_iff2( snap ) != 0 ) failed = 1;
  if( libspectrum_snap_im( snap ) != 2 ) failed = 1;

  if( libspectrum_snap_tstates( snap ) != 40 ) failed = 1;

  if( libspectrum_snap_last_instruction_ei( snap ) != 1 ) failed = 1;
  if( libspectrum_snap_halted( snap ) != 0 ) failed = 1;
  if( libspectrum_snap_last_instruction_set_f( snap ) != 1 ) failed = 1;

  if( libspectrum_snap_memptr( snap ) != 0xdc03 ) failed = 1;

  return failed;
}

test_return_t
test_44( void )
{
  return szx_read_block_test( "Z80R", test_44_check );
}

static int
test_45_check( libspectrum_snap *snap )
{
  int failed = 0;

  if( libspectrum_snap_out_ula( snap ) != 0xfa ) failed = 1;
  if( libspectrum_snap_out_128_memoryport( snap ) != 0x6f ) failed = 1;
  if( libspectrum_snap_out_plus3_memoryport( snap ) != 0x28 ) failed = 1;

  return failed;
}

test_return_t
test_45( void )
{
  return szx_read_block_test( "SPCR", test_45_check );
}

static int
test_46_check( libspectrum_snap *snap )
{
  int failed = 0;

  if( libspectrum_snap_joystick_active_count( snap ) != 2 ) failed = 1;
  if( libspectrum_snap_joystick_list( snap, 0 ) != LIBSPECTRUM_JOYSTICK_KEMPSTON ) failed = 1;
  if( libspectrum_snap_joystick_inputs( snap, 0 ) != LIBSPECTRUM_JOYSTICK_INPUT_JOYSTICK_1 ) failed = 1;
  if( libspectrum_snap_joystick_list( snap, 1 ) != LIBSPECTRUM_JOYSTICK_SINCLAIR_1 ) failed = 1;
  if( libspectrum_snap_joystick_inputs( snap, 1 ) != LIBSPECTRUM_JOYSTICK_INPUT_JOYSTICK_2 ) failed = 1;

  return failed;
}

test_return_t
test_46( void )
{
  return szx_read_block_test( "JOY", test_46_check );
}

static int
test_47_check( libspectrum_snap *snap )
{
  int failed = 0;

  if( libspectrum_snap_issue2( snap ) != 1 ) failed = 1;
  if( libspectrum_snap_joystick_active_count( snap ) != 1 ) failed = 1;
  if( libspectrum_snap_joystick_list( snap, 0 ) != LIBSPECTRUM_JOYSTICK_CURSOR ) failed = 1;
  if( libspectrum_snap_joystick_inputs( snap, 0 ) != LIBSPECTRUM_JOYSTICK_INPUT_KEYBOARD ) failed = 1;

  return failed;
}

test_return_t
test_47( void )
{
  return szx_read_block_test( "KEYB", test_47_check );
}

static int
test_48_check( libspectrum_snap *snap )
{
  return libspectrum_snap_zx_printer_active( snap ) != 1;
}

test_return_t
test_48( void )
{
  return szx_read_block_test( "ZXPR", test_48_check );
}

static int
test_49_check( libspectrum_snap *snap )
{
  int failed = 0;
  size_t i;

  if( libspectrum_snap_fuller_box_active( snap ) != 1 ) failed = 1;
  if( libspectrum_snap_melodik_active( snap ) != 0 ) failed = 1;
  if( libspectrum_snap_out_ay_registerport( snap ) != 0x08 ) failed = 1;

  for( i = 0; i < 16; i++ ) {
    if( libspectrum_snap_ay_registers( snap, i ) != ay_registers_data[i] ) failed = 1;
  }

  return failed;
}

test_return_t
test_49( void )
{
  return szx_read_block_test( "AY", test_49_check );
}

static int
test_50_check( libspectrum_snap *snap )
{
  int failed = 0;

  if( libspectrum_snap_out_scld_hsr( snap ) != 0x49 ) failed = 1;
  if( libspectrum_snap_out_scld_dec( snap ) != 0x9d ) failed = 1;

  return failed;
}

test_return_t
test_50( void )
{
  return szx_read_block_test( "SCLD", test_50_check );
}

static int
test_51_check( libspectrum_snap *snap )
{
  int failed = 0;

  if( libspectrum_snap_zxatasp_active( snap ) != 1 ) failed = 1;
  if( libspectrum_snap_zxatasp_upload( snap ) != 1 ) failed = 1;
  if( libspectrum_snap_zxatasp_writeprotect( snap ) != 0 ) failed = 1;
  if( libspectrum_snap_zxatasp_port_a( snap ) != 0xab ) failed = 1;
  if( libspectrum_snap_zxatasp_port_b( snap ) != 0x8c ) failed = 1;
  if( libspectrum_snap_zxatasp_port_c( snap ) != 0x82 ) failed = 1;
  if( libspectrum_snap_zxatasp_control( snap ) != 0xd8 ) failed = 1;
  if( libspectrum_snap_zxatasp_pages( snap ) != 0x18 ) failed = 1;
  if( libspectrum_snap_zxatasp_current_page( snap ) != 0x11 ) failed = 1;

  return failed;
}

test_return_t
test_51( void )
{
  return szx_read_block_test( "ZXAT", test_51_check );
}

static int
test_52_check( libspectrum_snap *snap )
{
  int failed = 0;

  if( libspectrum_snap_zxcf_active( snap ) != 1 ) failed = 1;
  if( libspectrum_snap_zxcf_upload( snap ) != 1 ) failed = 1;
  if( libspectrum_snap_zxcf_memctl( snap ) != 0x37 ) failed = 1;
  if( libspectrum_snap_zxcf_pages( snap ) != 0x55 ) failed = 1;

  return failed;
}

test_return_t
test_52( void )
{
  return szx_read_block_test( "ZXCF", test_52_check );
}

static int
test_53_check( libspectrum_snap *snap )
{
  return libspectrum_snap_kempston_mouse_active( snap ) != 1;
}

test_return_t
test_53( void )
{
  return szx_read_block_test( "AMXM", test_53_check );
}

static int
test_54_check( libspectrum_snap *snap )
{
  return libspectrum_snap_simpleide_active( snap ) != 1;
}

test_return_t
test_54( void )
{
  return szx_read_block_test( "SIDE", test_54_check );
}

static int
test_55_check( libspectrum_snap *snap )
{
  int failed = 0;

  if( libspectrum_snap_specdrum_active( snap ) != 1 ) failed = 1;
  if( libspectrum_snap_specdrum_dac( snap ) != -0x3b ) failed = 1;

  return failed;
}

test_return_t
test_55( void )
{
  return szx_read_block_test( "DRUM", test_55_check );
}

static int
test_56_check( libspectrum_snap *snap )
{
  int failed = 0;

  if( libspectrum_snap_covox_active( snap ) != 1 ) failed = 1;
  if( libspectrum_snap_covox_dac( snap ) != 0xc0 ) failed = 1;

  return failed;
}

test_return_t
test_56( void )
{
  return szx_read_block_test( "COVX", test_56_check );
}

static int
test_58_check( libspectrum_snap *snap )
{
  return libspectrum_snap_zxmmc_active( snap ) != 1;
}

test_return_t
test_58( void )
{
  return szx_read_block_test( "ZMMC", test_58_check );
}

static int
empty_ram_page_check( libspectrum_snap *snap,
    libspectrum_byte* (*get_ram_page)( libspectrum_snap*, int ) )
{
  int failed = 0;
  size_t i;

  libspectrum_byte *page = get_ram_page( snap, 0 );
  if( page ) {
    for( i = 0; i < 0x4000; i++ ) {
      if( page[i] ) {
        failed = 1;
        break;
      }
    }
  } else {
    failed = 1;
  }

  return failed;
}

static int
ramp_check( libspectrum_snap *snap )
{
  return empty_ram_page_check( snap, libspectrum_snap_pages );
}

test_return_t
test_60( void )
{
  return szx_read_block_test( "RAMP", ramp_check );
}

static int
atrp_check( libspectrum_snap *snap )
{
  return empty_ram_page_check( snap, libspectrum_snap_zxatasp_ram );
}

test_return_t
test_63( void )
{
  return szx_read_block_test( "ATRP", atrp_check );
}

static int
cfrp_check( libspectrum_snap *snap )
{
  return empty_ram_page_check( snap, libspectrum_snap_zxcf_ram );
}

test_return_t
test_64( void )
{
  return szx_read_block_test( "CFRP", cfrp_check );
}

test_return_t
test_68( void )
{
  return szx_read_block_from_compressed_snap_test( "RAMP", ramp_check );
}

test_return_t
test_69( void )
{
  return szx_read_block_from_compressed_snap_test( "ATRP", atrp_check );
}

test_return_t
test_70( void )
{
  return szx_read_block_from_compressed_snap_test( "CFRP", cfrp_check );
}
