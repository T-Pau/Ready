/* snapshot.c: Snapshot handling routines
   Copyright (c) 2001-2009 Philip Kendall, Darren Salt

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

/* Some flags which may be given to libspectrum_snap_write() */
const int LIBSPECTRUM_FLAG_SNAPSHOT_NO_COMPRESSION = 1 << 0;
const int LIBSPECTRUM_FLAG_SNAPSHOT_ALWAYS_COMPRESS = 1 << 1;

/* Some flags which may be returned from libspectrum_snap_write() */
const int LIBSPECTRUM_FLAG_SNAPSHOT_MINOR_INFO_LOSS = 1 << 0;
const int LIBSPECTRUM_FLAG_SNAPSHOT_MAJOR_INFO_LOSS = 1 << 1;

/* Initialise a libspectrum_snap structure */
libspectrum_snap*
libspectrum_snap_alloc( void )
{
  libspectrum_snap *snap;
  size_t i;

  snap = libspectrum_snap_alloc_internal();

  libspectrum_snap_set_a   ( snap, 0x00 );
  libspectrum_snap_set_f   ( snap, 0x00 );
  libspectrum_snap_set_bc  ( snap, 0x0000 );
  libspectrum_snap_set_de  ( snap, 0x0000 );
  libspectrum_snap_set_hl  ( snap, 0x0000 );

  libspectrum_snap_set_a_  ( snap, 0x00 );
  libspectrum_snap_set_f_  ( snap, 0x00 );
  libspectrum_snap_set_bc_ ( snap, 0x0000 );
  libspectrum_snap_set_de_ ( snap, 0x0000 );
  libspectrum_snap_set_hl_ ( snap, 0x0000 );

  libspectrum_snap_set_ix  ( snap, 0x0000 );
  libspectrum_snap_set_iy  ( snap, 0x0000 );
  libspectrum_snap_set_i   ( snap, 0x00 );
  libspectrum_snap_set_r   ( snap, 0x00 );
  libspectrum_snap_set_sp  ( snap, 0x0000 );
  libspectrum_snap_set_pc  ( snap, 0x0000 );
  libspectrum_snap_set_memptr( snap, 0x0000 );

  libspectrum_snap_set_iff1( snap, 1 );
  libspectrum_snap_set_iff2( snap, 1 );
  libspectrum_snap_set_im  ( snap, 1 );

  libspectrum_snap_set_halted( snap, 0 );
  libspectrum_snap_set_last_instruction_ei( snap, 0 );
  libspectrum_snap_set_last_instruction_set_f( snap, 0 );

  libspectrum_snap_set_custom_rom( snap, 0 );
  libspectrum_snap_set_custom_rom_pages( snap, 0 );
  for( i = 0; i < 4; i++ ) {
    libspectrum_snap_set_roms( snap, i, NULL );
    libspectrum_snap_set_rom_length( snap, i, 0 );
  }

  for( i = 0; i < SNAPSHOT_RAM_PAGES; i++ )
    libspectrum_snap_set_pages( snap, i, NULL );
  for( i = 0; i < SNAPSHOT_SLT_PAGES; i++ ) {
    libspectrum_snap_set_slt( snap, i, NULL );
    libspectrum_snap_set_slt_length( snap, i, 0 );
  }
  libspectrum_snap_set_slt_screen( snap, NULL );
  libspectrum_snap_set_slt_screen_level( snap, 0 );

  libspectrum_snap_set_out_ula( snap, 0x00 );
  libspectrum_snap_set_tstates( snap, 69664 );
  libspectrum_snap_set_out_128_memoryport( snap, 0x07 );

  libspectrum_snap_set_out_ay_registerport( snap, 0x0e );
  for( i = 0; i < 16; i++ ) libspectrum_snap_set_ay_registers( snap, i, 0 );

  libspectrum_snap_set_out_plus3_memoryport( snap, 0x08 );

  libspectrum_snap_set_out_scld_hsr( snap, 0x00 );
  libspectrum_snap_set_out_scld_dec( snap, 0x00 );

  libspectrum_snap_set_interface1_active( snap, 0 );
  libspectrum_snap_set_interface1_paged( snap, 0 );
  libspectrum_snap_set_interface1_drive_count( snap, 0 );
  libspectrum_snap_set_interface1_custom_rom( snap, 0 );
  libspectrum_snap_set_interface1_rom( snap, 0, NULL );
  libspectrum_snap_set_interface1_rom_length( snap, 0, 0 );

  libspectrum_snap_set_beta_active( snap, 0 );
  libspectrum_snap_set_beta_paged( snap, 0 );
  libspectrum_snap_set_beta_autoboot( snap, 0 );
  libspectrum_snap_set_beta_drive_count( snap, 0 );
  libspectrum_snap_set_beta_custom_rom( snap, 0 );
  libspectrum_snap_set_beta_direction( snap, 0 );
  libspectrum_snap_set_beta_system( snap, 0 );
  libspectrum_snap_set_beta_track ( snap, 0 );
  libspectrum_snap_set_beta_sector( snap, 0 );
  libspectrum_snap_set_beta_data  ( snap, 0 );
  libspectrum_snap_set_beta_status( snap, 0 );
  libspectrum_snap_set_beta_rom( snap, 0, NULL );

  libspectrum_snap_set_plusd_active( snap, 0 );
  libspectrum_snap_set_plusd_paged( snap, 0 );
  libspectrum_snap_set_plusd_drive_count( snap, 0 );
  libspectrum_snap_set_plusd_custom_rom( snap, 0 );
  libspectrum_snap_set_plusd_direction( snap, 0 );
  libspectrum_snap_set_plusd_control( snap, 0 );
  libspectrum_snap_set_plusd_track ( snap, 0 );
  libspectrum_snap_set_plusd_sector( snap, 0 );
  libspectrum_snap_set_plusd_data  ( snap, 0 );
  libspectrum_snap_set_plusd_status( snap, 0 );
  libspectrum_snap_set_plusd_rom( snap, 0, NULL );
  libspectrum_snap_set_plusd_ram( snap, 0, NULL );

  libspectrum_snap_set_opus_active( snap, 0 );
  libspectrum_snap_set_opus_paged ( snap, 0 );
  libspectrum_snap_set_opus_drive_count( snap, 0 );
  libspectrum_snap_set_opus_custom_rom( snap, 0 );
  libspectrum_snap_set_opus_direction ( snap, 0 );
  libspectrum_snap_set_opus_track ( snap, 0 );
  libspectrum_snap_set_opus_sector( snap, 0 );
  libspectrum_snap_set_opus_data  ( snap, 0 );
  libspectrum_snap_set_opus_status( snap, 0 );
  libspectrum_snap_set_opus_data_reg_a( snap, 0 );
  libspectrum_snap_set_opus_data_dir_a( snap, 0 );
  libspectrum_snap_set_opus_control_a ( snap, 0 );
  libspectrum_snap_set_opus_data_reg_b( snap, 0 );
  libspectrum_snap_set_opus_data_dir_b( snap, 0 );
  libspectrum_snap_set_opus_control_b ( snap, 0 );
  libspectrum_snap_set_opus_rom( snap, 0, NULL );
  libspectrum_snap_set_opus_ram( snap, 0, NULL );

  libspectrum_snap_set_zxatasp_active( snap, 0 );
  libspectrum_snap_set_zxatasp_upload( snap, 0 );
  libspectrum_snap_set_zxatasp_writeprotect( snap, 0 );
  libspectrum_snap_set_zxatasp_port_a( snap, 0 );
  libspectrum_snap_set_zxatasp_port_b( snap, 0 );
  libspectrum_snap_set_zxatasp_port_c( snap, 0 );
  libspectrum_snap_set_zxatasp_control( snap, 0 );
  libspectrum_snap_set_zxatasp_pages( snap, 0 );
  libspectrum_snap_set_zxatasp_current_page( snap, 0 );
  for( i = 0; i < SNAPSHOT_ZXATASP_PAGES; i++ )
    libspectrum_snap_set_zxatasp_ram( snap, i, NULL );

  libspectrum_snap_set_zxcf_active( snap, 0 );
  libspectrum_snap_set_zxcf_upload( snap, 0 );
  libspectrum_snap_set_zxcf_memctl( snap, 0x00 );
  libspectrum_snap_set_zxcf_pages( snap, 0 );
  for( i = 0; i < SNAPSHOT_ZXCF_PAGES; i++ )
    libspectrum_snap_set_zxcf_ram( snap, i, NULL );

  libspectrum_snap_set_interface2_active( snap, 0 );
  libspectrum_snap_set_interface2_rom( snap, 0, NULL );

  libspectrum_snap_set_dock_active( snap, 0 );
  for( i = 0; i < SNAPSHOT_DOCK_EXROM_PAGES; i++ ) {
    libspectrum_snap_set_exrom_ram( snap, i, 0 );
    libspectrum_snap_set_exrom_cart( snap, i, NULL );
    libspectrum_snap_set_dock_ram( snap, i, 0 );
    libspectrum_snap_set_dock_cart( snap, i, NULL );
  }

  libspectrum_snap_set_issue2( snap, 0 );

  libspectrum_snap_set_joystick_active_count( snap, 0 );
  for( i = 0; i < SNAPSHOT_JOYSTICKS; i++ ) {
    libspectrum_snap_set_joystick_list( snap, i, LIBSPECTRUM_JOYSTICK_NONE );
    libspectrum_snap_set_joystick_inputs( snap, i, 0 );
  }

  libspectrum_snap_set_kempston_mouse_active( snap, 0 );

  libspectrum_snap_set_simpleide_active( snap, 0 );

  libspectrum_snap_set_specdrum_active( snap, 0 );
  libspectrum_snap_set_specdrum_dac( snap, 0 );

  libspectrum_snap_set_divide_active( snap, 0 );
  libspectrum_snap_set_divide_eprom_writeprotect( snap, 0 );
  libspectrum_snap_set_divide_paged( snap, 0 );
  libspectrum_snap_set_divide_control( snap, 0 );
  libspectrum_snap_set_divide_pages( snap, 0 );
  libspectrum_snap_set_divide_eprom( snap, 0, NULL );
  for( i = 0; i < SNAPSHOT_DIVIDE_PAGES; i++ ) {
    libspectrum_snap_set_divide_ram( snap, i, NULL );
  }

  libspectrum_snap_set_divmmc_active( snap, 0 );
  libspectrum_snap_set_divmmc_eprom_writeprotect( snap, 0 );
  libspectrum_snap_set_divmmc_paged( snap, 0 );
  libspectrum_snap_set_divmmc_control( snap, 0 );
  libspectrum_snap_set_divmmc_pages( snap, 0 );
  libspectrum_snap_set_divmmc_eprom( snap, 0, NULL );
  for( i = 0; i < SNAPSHOT_DIVMMC_PAGES; i++ ) {
    libspectrum_snap_set_divmmc_ram( snap, i, NULL );
  }

  libspectrum_snap_set_fuller_box_active( snap, 0 );

  libspectrum_snap_set_melodik_active( snap, 0 );

  libspectrum_snap_set_spectranet_active( snap, 0 );
  libspectrum_snap_set_spectranet_all_traps_disabled( snap, 0 );
  libspectrum_snap_set_spectranet_rst8_trap_disabled( snap, 0 );
  libspectrum_snap_set_spectranet_w5100( snap, 0, NULL );
  libspectrum_snap_set_spectranet_flash( snap, 0, NULL );
  libspectrum_snap_set_spectranet_ram( snap, 0, NULL );
  
  libspectrum_snap_set_late_timings( snap, 0 );
  
  libspectrum_snap_set_zx_printer_active( snap, 0 );

  libspectrum_snap_set_usource_active( snap, 0 );
  libspectrum_snap_set_usource_paged( snap, 0 );
  libspectrum_snap_set_usource_custom_rom( snap, 0 );
  libspectrum_snap_set_usource_rom( snap, 0, NULL );
  libspectrum_snap_set_usource_rom_length( snap, 0, 0 );

  libspectrum_snap_set_disciple_active( snap, 0 );
  libspectrum_snap_set_disciple_paged( snap, 0 );
  libspectrum_snap_set_disciple_inhibit_button( snap, 0 );
  libspectrum_snap_set_disciple_drive_count( snap, 0 );
  libspectrum_snap_set_disciple_custom_rom( snap, 0 );
  libspectrum_snap_set_disciple_direction( snap, 0 );
  libspectrum_snap_set_disciple_control( snap, 0 );
  libspectrum_snap_set_disciple_track ( snap, 0 );
  libspectrum_snap_set_disciple_sector( snap, 0 );
  libspectrum_snap_set_disciple_data  ( snap, 0 );
  libspectrum_snap_set_disciple_status( snap, 0 );
  libspectrum_snap_set_disciple_rom( snap, 0, NULL );
  libspectrum_snap_set_disciple_rom_length( snap, 0, 0 );
  libspectrum_snap_set_disciple_ram( snap, 0, NULL );

  libspectrum_snap_set_didaktik80_active( snap, 0 );
  libspectrum_snap_set_didaktik80_paged( snap, 0 );
  libspectrum_snap_set_didaktik80_drive_count( snap, 0 );
  libspectrum_snap_set_didaktik80_custom_rom( snap, 0 );
  libspectrum_snap_set_didaktik80_direction( snap, 0 );
  libspectrum_snap_set_didaktik80_aux( snap, 0 );
  libspectrum_snap_set_didaktik80_track ( snap, 0 );
  libspectrum_snap_set_didaktik80_sector( snap, 0 );
  libspectrum_snap_set_didaktik80_data  ( snap, 0 );
  libspectrum_snap_set_didaktik80_status( snap, 0 );
  libspectrum_snap_set_didaktik80_rom( snap, 0, NULL );
  libspectrum_snap_set_didaktik80_rom_length( snap, 0, 0 );
  libspectrum_snap_set_didaktik80_ram( snap, 0, NULL );

  libspectrum_snap_set_covox_active( snap, 0 );
  libspectrum_snap_set_covox_dac( snap, 0 );

  libspectrum_snap_set_ulaplus_active( snap, 0 );
  libspectrum_snap_set_ulaplus_palette_enabled( snap, 0 );
  libspectrum_snap_set_ulaplus_current_register( snap, 0 );
  libspectrum_snap_set_ulaplus_palette( snap, 0, NULL );
  libspectrum_snap_set_ulaplus_ff_register( snap, 0 );

  libspectrum_snap_set_multiface_active( snap, 0 );
  libspectrum_snap_set_multiface_paged( snap, 0 );
  libspectrum_snap_set_multiface_model_one( snap, 0 );
  libspectrum_snap_set_multiface_model_128( snap, 0 );
  libspectrum_snap_set_multiface_model_3( snap, 0 );
  libspectrum_snap_set_multiface_disabled( snap, 0 );
  libspectrum_snap_set_multiface_software_lockout( snap, 0 );
  libspectrum_snap_set_multiface_red_button_disabled( snap, 0 );
  libspectrum_snap_set_multiface_ram( snap, 0, NULL );
  libspectrum_snap_set_multiface_ram_length( snap, 0, 0 );

  libspectrum_snap_set_zxmmc_active( snap, 0 );

  return snap;
}

/* Free all memory used by a libspectrum_snap structure (destructor...) */
libspectrum_error
libspectrum_snap_free( libspectrum_snap *snap )
{
  size_t i;

  for( i = 0; i < 4; i++ )
    libspectrum_free( libspectrum_snap_roms( snap, i ) );

  for( i = 0; i < SNAPSHOT_RAM_PAGES; i++ )
    libspectrum_free( libspectrum_snap_pages( snap, i ) );

  for( i = 0; i < SNAPSHOT_SLT_PAGES; i++ )
    libspectrum_free( libspectrum_snap_slt( snap, i ) );

  libspectrum_free( libspectrum_snap_slt_screen( snap ) );

  for( i = 0; i < SNAPSHOT_ZXCF_PAGES; i++ )
    libspectrum_free( libspectrum_snap_zxcf_ram( snap, i ) );

  libspectrum_free( libspectrum_snap_interface2_rom( snap, 0 ) );

  for( i = 0; i < SNAPSHOT_DOCK_EXROM_PAGES; i++ ) {
    libspectrum_free( libspectrum_snap_dock_cart( snap, i ) );
    libspectrum_free( libspectrum_snap_exrom_cart( snap, i ) );
  }

  if( libspectrum_snap_beta_rom( snap, 0 ) )
    libspectrum_free( libspectrum_snap_beta_rom( snap, 0 ) );

  if( libspectrum_snap_plusd_rom( snap, 0 ) )
    libspectrum_free( libspectrum_snap_plusd_rom( snap, 0 ) );
  if( libspectrum_snap_plusd_ram( snap, 0 ) )
    libspectrum_free( libspectrum_snap_plusd_ram( snap, 0 ) );

  if( libspectrum_snap_interface1_rom( snap, 0 ) )
    libspectrum_free( libspectrum_snap_interface1_rom( snap, 0 ) );

  if( libspectrum_snap_spectranet_w5100( snap, 0 ) )
    libspectrum_free( libspectrum_snap_spectranet_w5100( snap, 0 ) );
  if( libspectrum_snap_spectranet_flash( snap, 0 ) )
    libspectrum_free( libspectrum_snap_spectranet_flash( snap, 0 ) );
  if( libspectrum_snap_spectranet_ram( snap, 0 ) )
    libspectrum_free( libspectrum_snap_spectranet_ram( snap, 0 ) );

  if( libspectrum_snap_usource_rom( snap, 0 ) )
    libspectrum_free( libspectrum_snap_usource_rom( snap, 0 ) );

  if( libspectrum_snap_disciple_rom( snap, 0 ) )
    libspectrum_free( libspectrum_snap_disciple_rom( snap, 0 ) );
  if( libspectrum_snap_disciple_ram( snap, 0 ) )
    libspectrum_free( libspectrum_snap_disciple_ram( snap, 0 ) );

  if( libspectrum_snap_didaktik80_rom( snap, 0 ) )
    libspectrum_free( libspectrum_snap_didaktik80_rom( snap, 0 ) );
  if( libspectrum_snap_didaktik80_ram( snap, 0 ) )
    libspectrum_free( libspectrum_snap_didaktik80_ram( snap, 0 ) );

  libspectrum_free( libspectrum_snap_divide_eprom( snap, 0 ) );
  for( i = 0; i < SNAPSHOT_DIVIDE_PAGES; i++ )
    libspectrum_free( libspectrum_snap_divide_ram( snap, i ) );

  libspectrum_free( libspectrum_snap_divmmc_eprom( snap, 0 ) );
  for( i = 0; i < SNAPSHOT_DIVMMC_PAGES; i++ )
    libspectrum_free( libspectrum_snap_divmmc_ram( snap, i ) );

  if( libspectrum_snap_ulaplus_palette( snap, 0 ) )
    libspectrum_free( libspectrum_snap_ulaplus_palette( snap, 0 ) );

  if( libspectrum_snap_multiface_ram( snap, 0 ) )
    libspectrum_free( libspectrum_snap_multiface_ram( snap, 0 ) );

  libspectrum_free( snap );

  return LIBSPECTRUM_ERROR_NONE;
}

/* Read in a snapshot, optionally guessing what type it is */
libspectrum_error
libspectrum_snap_read( libspectrum_snap *snap, const libspectrum_byte *buffer,
		       size_t length, libspectrum_id_t type,
		       const char *filename )
{
  libspectrum_id_t raw_type;
  libspectrum_class_t class;
  libspectrum_byte *new_buffer;
  libspectrum_error error;

  /* If we don't know what sort of file this is, make a best guess */
  if( type == LIBSPECTRUM_ID_UNKNOWN ) {
    error = libspectrum_identify_file( &type, filename, buffer, length );
    if( error ) return error;

    /* If we still can't identify it, give up */
    if( type == LIBSPECTRUM_ID_UNKNOWN ) {
      libspectrum_print_error(
        LIBSPECTRUM_ERROR_UNKNOWN,
	"libspectrum_snap_read: couldn't identify file"
      );
      return LIBSPECTRUM_ERROR_UNKNOWN;
    }
  }

  error = libspectrum_identify_class( &class, type );
  if( error ) return error;

  if( class != LIBSPECTRUM_CLASS_SNAPSHOT ) {
    libspectrum_print_error( LIBSPECTRUM_ERROR_CORRUPT,
			     "libspectrum_snap_read: not a snapshot file" );
    return LIBSPECTRUM_ERROR_CORRUPT;
  }

  /* Find out if this file needs decompression */
  new_buffer = NULL;

  error = libspectrum_identify_file_raw( &raw_type, filename, buffer, length );
  if( error ) return error;

  error = libspectrum_identify_class( &class, raw_type );
  if( error ) return error;

  if( class == LIBSPECTRUM_CLASS_COMPRESSED ) {

    size_t new_length;

    error = libspectrum_uncompress_file( &new_buffer, &new_length, NULL,
					 raw_type, buffer, length, NULL );
    if( error ) return error;
    buffer = new_buffer; length = new_length;
  }

  switch( type ) {

  case LIBSPECTRUM_ID_SNAPSHOT_PLUSD:
    error = libspectrum_plusd_read( snap, buffer, length ); break;

  case LIBSPECTRUM_ID_SNAPSHOT_SNA:
    error = internal_sna_read( snap, buffer, length ); break;

  case LIBSPECTRUM_ID_SNAPSHOT_SNP:
    error = libspectrum_snp_read( snap, buffer, length ); break;

  case LIBSPECTRUM_ID_SNAPSHOT_SP:
    error = libspectrum_sp_read( snap, buffer, length ); break;

  case LIBSPECTRUM_ID_SNAPSHOT_SZX:
    error = libspectrum_szx_read( snap, buffer, length ); break;

  case LIBSPECTRUM_ID_SNAPSHOT_Z80:
    error = internal_z80_read( snap, buffer, length ); break;

  case LIBSPECTRUM_ID_SNAPSHOT_ZXS:
    error = libspectrum_zxs_read( snap, buffer, length ); break;

  default:
    libspectrum_print_error( LIBSPECTRUM_ERROR_LOGIC,
			     "libspectrum_snap_read: unknown snapshot type %d",
			     type );
    libspectrum_free( new_buffer );
    return LIBSPECTRUM_ERROR_LOGIC;
  }

  libspectrum_free( new_buffer );
  return error;
}

libspectrum_error
libspectrum_snap_write( libspectrum_byte **buffer, size_t *length,
			int *out_flags, libspectrum_snap *snap,
			libspectrum_id_t type, libspectrum_creator *creator,
			int in_flags )
{
  libspectrum_byte *ptr = *buffer;
  libspectrum_buffer *new_buffer = libspectrum_buffer_alloc();
  libspectrum_error error =
    libspectrum_snap_write_buffer( new_buffer, out_flags, snap, type, creator,
                                   in_flags );
  libspectrum_buffer_append( buffer, length, &ptr, new_buffer );
  libspectrum_buffer_free( new_buffer );
  return error;
}

libspectrum_error
libspectrum_snap_write_buffer( libspectrum_buffer *buffer, int *out_flags,
                               libspectrum_snap *snap, libspectrum_id_t type,
                               libspectrum_creator *creator, int in_flags )
{
  libspectrum_class_t class;
  libspectrum_error error;

  error = libspectrum_identify_class( &class, type );
  if( error ) return error;

  if( class != LIBSPECTRUM_CLASS_SNAPSHOT ) {
    libspectrum_print_error( LIBSPECTRUM_ERROR_INVALID,
			     "libspectrum_snap_write: not a snapshot format" );
    return LIBSPECTRUM_ERROR_INVALID;
  }

  switch( type ) {

  case LIBSPECTRUM_ID_SNAPSHOT_SNA:
    error = libspectrum_sna_write( buffer, out_flags, snap, in_flags );
    break;

  case LIBSPECTRUM_ID_SNAPSHOT_SZX:
    error = libspectrum_szx_write( buffer, out_flags, snap, creator, in_flags );
    break;

  case LIBSPECTRUM_ID_SNAPSHOT_Z80:
    error = libspectrum_z80_write2( buffer, out_flags, snap, in_flags );
    break;

  default:
    libspectrum_print_error( LIBSPECTRUM_ERROR_UNKNOWN,
			     "libspectrum_snap_write: format not supported" );
    error = LIBSPECTRUM_ERROR_UNKNOWN;

  }

  return error;
}

/* Given a 48K memory dump `data', place it into the
   appropriate bits of `snap' for a 48K machine */
libspectrum_error
libspectrum_split_to_48k_pages( libspectrum_snap *snap,
				const libspectrum_byte* data )
{
  libspectrum_byte *buffer[3];
  size_t i;

  /* If any of the three pages are already occupied, barf */
  if( libspectrum_snap_pages( snap, 5 ) ||
      libspectrum_snap_pages( snap, 2 ) ||
      libspectrum_snap_pages( snap, 0 )    ) {
    libspectrum_print_error(
      LIBSPECTRUM_ERROR_LOGIC,
      "libspectrum_split_to_48k_pages: RAM page already in use"
    );
    return LIBSPECTRUM_ERROR_LOGIC;
  }

  for( i = 0; i < 3; i++ )
    buffer[i] = libspectrum_new( libspectrum_byte, 0x4000 );

  libspectrum_snap_set_pages( snap, 5, buffer[0] );
  libspectrum_snap_set_pages( snap, 2, buffer[1] );
  libspectrum_snap_set_pages( snap, 0, buffer[2] );

  /* Finally, do the copies... */
  memcpy( libspectrum_snap_pages( snap, 5 ), &data[0x0000], 0x4000 );
  memcpy( libspectrum_snap_pages( snap, 2 ), &data[0x4000], 0x4000 );
  memcpy( libspectrum_snap_pages( snap, 0 ), &data[0x8000], 0x4000 );

  return LIBSPECTRUM_ERROR_NONE;
}
