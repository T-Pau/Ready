/* szx.c: Routines for .szx snapshots
   Copyright (c) 1998-2016 Philip Kendall, Fredrick Meunier, Stuart Brady

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

/* Used for passing internal data around */

typedef struct szx_context {

  int swap_af;

} szx_context;

/* The machine numbers used in the .szx format */

typedef enum szx_machine_type {

  SZX_MACHINE_16 = 0,
  SZX_MACHINE_48,
  SZX_MACHINE_128,
  SZX_MACHINE_PLUS2,
  SZX_MACHINE_PLUS2A,
  SZX_MACHINE_PLUS3,
  SZX_MACHINE_PLUS3E,
  SZX_MACHINE_PENTAGON,
  SZX_MACHINE_TC2048,
  SZX_MACHINE_TC2068,
  SZX_MACHINE_SCORPION,
  SZX_MACHINE_SE,
  SZX_MACHINE_TS2068,
  SZX_MACHINE_PENTAGON512,
  SZX_MACHINE_PENTAGON1024,
  SZX_MACHINE_48_NTSC,
  SZX_MACHINE_128KE,

} szx_machine_type;

static const char * const signature = "ZXST";
static const size_t signature_length = 4;

static const libspectrum_byte ZXSTMF_ALTERNATETIMINGS = 1;

static const char * const libspectrum_string = "libspectrum: ";

static const libspectrum_byte SZX_VERSION_MAJOR = 1;
static const libspectrum_byte SZX_VERSION_MINOR = 5;

/* Constants etc for each chunk type */

#define ZXSTBID_CREATOR "CRTR"

#define ZXSTBID_Z80REGS "Z80R"
static const libspectrum_byte ZXSTZF_EILAST = 1;
static const libspectrum_byte ZXSTZF_HALTED = 2;
static const libspectrum_byte ZXSTZF_FSET = 4;

#define ZXSTBID_SPECREGS "SPCR"

#define ZXSTBID_RAMPAGE "RAMP"
static const libspectrum_word ZXSTRF_COMPRESSED = 1;

#define ZXSTBID_AY "AY\0\0"
static const libspectrum_byte ZXSTAYF_FULLERBOX = 1;
static const libspectrum_byte ZXSTAYF_128AY = 2;

#define ZXSTBID_MULTIFACE "MFCE"
static const libspectrum_byte ZXSTMF_PAGEDIN = 1;
static const libspectrum_byte ZXSTMF_COMPRESSED = 2;
static const libspectrum_byte ZXSTMF_SOFTWARELOCKOUT = 4;
static const libspectrum_byte ZXSTMF_REDBUTTONDISABLED = 8;
static const libspectrum_byte ZXSTMF_DISABLED = 16;
static const libspectrum_byte ZXSTMF_16KRAMMODE = 32;
static const libspectrum_byte ZXSTMFM_1 = 0;
static const libspectrum_byte ZXSTMFM_128 = 1;

#define ZXSTBID_USPEECH "USPE"
#define ZXSTBID_SPECDRUM "DRUM"
#define ZXSTBID_ZXTAPE "TAPE"

#define ZXSTBID_KEYBOARD "KEYB"
static const libspectrum_dword ZXSTKF_ISSUE2 = 1;

#define ZXSTBID_JOYSTICK "JOY\0"
static const libspectrum_dword ZXSTJOYF_ALWAYSPORT31 = 1;

typedef enum szx_joystick_type {

  ZXJT_KEMPSTON = 0,
  ZXJT_FULLER,
  ZXJT_CURSOR,
  ZXJT_SINCLAIR1,
  ZXJT_SINCLAIR2,
  ZXJT_SPECTRUMPLUS,
  ZXJT_TIMEX1,
  ZXJT_TIMEX2,
  ZXJT_NONE,

} szx_joystick_type;

#define ZXSTBID_IF2ROM "IF2R"

#define ZXSTBID_MOUSE "AMXM"
typedef enum szx_mouse_type {

  ZXSTM_NONE = 0,
  ZXSTM_AMX,
  ZXSTM_KEMPSTON,

} szx_mouse_type;

#define ZXSTBID_ROM "ROM\0"

#define ZXSTBID_ZXPRINTER "ZXPR"
static const libspectrum_word ZXSTPRF_ENABLED = 1;

#define ZXSTBID_IF1 "IF1\0"
static const libspectrum_word ZXSTIF1F_ENABLED = 1;
static const libspectrum_word ZXSTIF1F_COMPRESSED = 2;
static const libspectrum_word ZXSTIF1F_PAGED = 4;

#define ZXSTBID_MICRODRIVE "MDRV"
#define ZXSTBID_PLUS3DISK "+3\0\0"
#define ZXSTBID_DSKFILE "DSK\0"
#define ZXSTBID_LEC "LEC\0"
/* static const libspectrum_word ZXSTLECF_PAGED = 1; */

#define ZXSTBID_LECRAMPAGE "LCRP"
/* static const libspectrum_word ZXSTLCRPF_COMPRESSED = 1; */

#define ZXSTBID_TIMEXREGS "SCLD"

#define ZXSTBID_BETA128 "B128"
static const libspectrum_dword ZXSTBETAF_CONNECTED = 1;
static const libspectrum_dword ZXSTBETAF_CUSTOMROM = 2;
static const libspectrum_dword ZXSTBETAF_PAGED = 4;
static const libspectrum_dword ZXSTBETAF_AUTOBOOT = 8;
static const libspectrum_dword ZXSTBETAF_SEEKLOWER = 16;
static const libspectrum_dword ZXSTBETAF_COMPRESSED = 32;

#define ZXSTBID_BETADISK "BDSK"
#define ZXSTBID_GS "GS\0\0"
#define ZXSTBID_GSRAMPAGE "GSRP"
#define ZXSTBID_COVOX "COVX"

#define ZXSTBID_DOCK "DOCK"
static const libspectrum_word ZXSTDOCKF_RAM = 2;
static const libspectrum_word ZXSTDOCKF_EXROMDOCK = 4;

#define ZXSTBID_ZXATASP "ZXAT"
static const libspectrum_word ZXSTZXATF_UPLOAD = 1;
static const libspectrum_word ZXSTZXATF_WRITEPROTECT = 2;

#define ZXSTBID_ZXATASPRAMPAGE "ATRP"

#define ZXSTBID_ZXCF "ZXCF"
static const libspectrum_word ZXSTZXCFF_UPLOAD = 1;

#define ZXSTBID_ZXCFRAMPAGE "CFRP"

#define ZXSTBID_PLUSD "PLSD"
static const libspectrum_dword ZXSTPLUSDF_PAGED = 1;
static const libspectrum_dword ZXSTPLUSDF_COMPRESSED = 2;
static const libspectrum_dword ZXSTPLUSDF_SEEKLOWER = 4;
static const libspectrum_byte ZXSTPDRT_GDOS = 0;
/* static const libspectrum_byte ZXSTPDRT_UNIDOS = 1; */
static const libspectrum_byte ZXSTPDRT_CUSTOM = 2;

#define ZXSTBID_PLUSDDISK "PDSK"

#define ZXSTBID_OPUS "OPUS"
static const libspectrum_dword ZXSTOPUSF_PAGED = 1;
static const libspectrum_dword ZXSTOPUSF_COMPRESSED = 2;
static const libspectrum_dword ZXSTOPUSF_SEEKLOWER = 4;
static const libspectrum_dword ZXSTOPUSF_CUSTOMROM = 8;

#define ZXSTBID_OPUSDISK "ODSK"

#define ZXSTBID_SIMPLEIDE "SIDE"
/* static const libspectrum_word ZXSTSIDE_ENABLED = 1; */

#define ZXSTBID_DIVIDE "DIDE"
static const libspectrum_word ZXSTDIVIDE_EPROM_WRITEPROTECT = 1;
static const libspectrum_word ZXSTDIVIDE_PAGED = 2;
static const libspectrum_word ZXSTDIVIDE_COMPRESSED = 4;

#define ZXSTBID_DIVIDERAMPAGE "DIRP"

#define ZXSTBID_DIVMMC "DMMC"
static const libspectrum_word ZXSTDIVMMC_EPROM_WRITEPROTECT = 1;
static const libspectrum_word ZXSTDIVMMC_PAGED = 2;
static const libspectrum_word ZXSTDIVMMC_COMPRESSED = 4;

#define ZXSTBID_DIVMMCRAMPAGE "DMRP"

#define ZXSTBID_SPECTRANET "SNET"
static const libspectrum_word ZXSTSNET_PAGED = 1;
static const libspectrum_word ZXSTSNET_PAGED_VIA_IO = 2;
static const libspectrum_word ZXSTSNET_PROGRAMMABLE_TRAP_ACTIVE = 4;
static const libspectrum_word ZXSTSNET_PROGRAMMABLE_TRAP_MSB = 8;
static const libspectrum_word ZXSTSNET_ALL_DISABLED = 16;
static const libspectrum_word ZXSTSNET_RST8_DISABLED = 32;
static const libspectrum_word ZXSTSNET_DENY_DOWNSTREAM_A15 = 64;
static const libspectrum_word ZXSTSNET_NMI_FLIPFLOP = 128;

#define ZXSTBID_SPECTRANETFLASHPAGE "SNEF"
static const libspectrum_byte ZXSTSNEF_FLASH_COMPRESSED = 1;

#define ZXSTBID_SPECTRANETRAMPAGE "SNER"
static const libspectrum_byte ZXSTSNER_RAM_COMPRESSED = 1;

#define ZXSTBID_PALETTE "PLTT"
static const libspectrum_byte ZXSTPALETTE_ENABLED = 1;

#define ZXSTBID_ZXMMC "ZMMC"

static libspectrum_error
read_chunk( libspectrum_snap *snap, libspectrum_word version,
	    const libspectrum_byte **buffer, const libspectrum_byte *end,
            szx_context *ctx );

typedef libspectrum_error (*read_chunk_fn)( libspectrum_snap *snap,
					    libspectrum_word version,
					    const libspectrum_byte **buffer,
					    const libspectrum_byte *end,
					    size_t data_length,
                                            szx_context *ctx );

static libspectrum_error
write_file_header( libspectrum_buffer *buffer, int *out_flags,
                   libspectrum_snap *snap );

static void
write_crtr_chunk( libspectrum_buffer *buffer, libspectrum_buffer *crtr_data,
                  libspectrum_creator *creator );
static void
write_z80r_chunk( libspectrum_buffer *buffer, libspectrum_buffer *data,
                  libspectrum_snap *snap );
static void
write_spcr_chunk( libspectrum_buffer *buffer, libspectrum_buffer *data,
                  libspectrum_snap *snap );
static void
write_amxm_chunk( libspectrum_buffer *buffer, libspectrum_buffer *data,
                  libspectrum_snap *snap );
static void
write_joy_chunk( libspectrum_buffer *buffer, libspectrum_buffer *data,
                 int *out_flags, libspectrum_snap *snap );
static void
write_keyb_chunk( libspectrum_buffer *buffer, libspectrum_buffer *data,
		  int *out_flags, libspectrum_snap *snap );
static void
write_ram_pages( libspectrum_buffer *buffer, libspectrum_buffer *block_data,
                 libspectrum_snap *snap, int compress );
static void
write_ramp_chunk( libspectrum_buffer *buffer, libspectrum_buffer *block_data,
                  libspectrum_snap *snap, int page, int compress );
static void
write_ram_page( libspectrum_buffer *buffer, libspectrum_buffer *block_data,
                const char *id, const libspectrum_byte *data,
                size_t data_length, int page, int compress, int extra_flags );
static libspectrum_error
write_rom_chunk( libspectrum_buffer *buffer, libspectrum_buffer *block_data,
                 int *out_flags, libspectrum_snap *snap, int compress );
static void
write_ay_chunk( libspectrum_buffer *buffer, libspectrum_buffer *data,
                libspectrum_snap *snap );
static void
write_scld_chunk( libspectrum_buffer *buffer, libspectrum_buffer *data,
                  libspectrum_snap *snap );
static void
write_b128_chunk( libspectrum_buffer *buffer, libspectrum_buffer *data,
                  libspectrum_snap *snap, int compress );
static libspectrum_error
write_opus_chunk( libspectrum_buffer *buffer, libspectrum_buffer *data,
		  libspectrum_snap *snap, int compress  );
static libspectrum_error
write_plsd_chunk( libspectrum_buffer *buffer, libspectrum_buffer *data,
                  libspectrum_snap *snap, int compress  );
static libspectrum_error
write_if1_chunk( libspectrum_buffer *buffer, libspectrum_buffer *data,
		 libspectrum_snap *snap, int compress  );
static void
write_zxat_chunk( libspectrum_buffer *buffer, libspectrum_buffer *data,
                  libspectrum_snap *snap );
static void
write_atrp_chunk( libspectrum_buffer *buffer, libspectrum_buffer *block_data,
		  libspectrum_snap *snap, int page, int compress );
static void
write_zxcf_chunk( libspectrum_buffer *buffer, libspectrum_buffer *data,
		  libspectrum_snap *snap );
static void
write_cfrp_chunk( libspectrum_buffer *buffer, libspectrum_buffer *block_data,
                  libspectrum_snap *snap, int page, int compress );
static void
write_side_chunk( libspectrum_buffer *buffer, libspectrum_buffer *block_data,
		  libspectrum_snap *snap );
static void
write_drum_chunk( libspectrum_buffer *buffer, libspectrum_buffer *data,
                  libspectrum_snap *snap );
static void
write_snet_chunk( libspectrum_buffer *buffer, libspectrum_buffer *data,
                  libspectrum_snap *snap, int compress );
static void
write_snef_chunk( libspectrum_buffer *buffer, libspectrum_buffer *data,
		  libspectrum_snap *snap, int compress );
static void
write_sner_chunk( libspectrum_buffer *buffer, libspectrum_buffer *data,
		  libspectrum_snap *snap, int compress );
static libspectrum_error
write_pltt_chunk( libspectrum_buffer *buffer, libspectrum_buffer *data,
                  libspectrum_snap *snap, int compress );
static libspectrum_error
write_mfce_chunk( libspectrum_buffer *buffer, libspectrum_buffer *data,
		  libspectrum_snap *snap, int compress );
static void
write_zmmc_chunk( libspectrum_buffer *buffer, libspectrum_buffer *data,
		  libspectrum_snap *snap );

#ifdef HAVE_ZLIB_H

static libspectrum_error
write_if2r_chunk( libspectrum_buffer *buffer, libspectrum_buffer *block_data,
                  libspectrum_snap *snap );

#endif				/* #ifdef HAVE_ZLIB_H */

static void
write_dock_chunk( libspectrum_buffer *buffer, libspectrum_buffer *block_data,
		  libspectrum_snap *snap, int exrom_dock,
                  const libspectrum_byte *data, int page, int writeable,
                  int compress );
static libspectrum_error
write_dide_chunk( libspectrum_buffer *buffer, libspectrum_buffer *data,
                  libspectrum_snap *snap, int compress );
static void
write_dirp_chunk( libspectrum_buffer *buffer, libspectrum_buffer *block_data,
                  libspectrum_snap *snap, int page, int compress );
static libspectrum_error
write_dmmc_chunk( libspectrum_buffer *buffer, libspectrum_buffer *data,
                  libspectrum_snap *snap, int compress );
static void
write_dmrp_chunk( libspectrum_buffer *buffer, libspectrum_buffer *block_data,
                  libspectrum_snap *snap, int page, int compress );
static void
write_zxpr_chunk( libspectrum_buffer *buffer, libspectrum_buffer *data,
		  int *out_flags, libspectrum_snap *snap );
static void
write_covx_chunk( libspectrum_buffer *buffer, libspectrum_buffer *data,
                  libspectrum_snap *snap );

static void
write_chunk( libspectrum_buffer *buffer, const char *id,
             libspectrum_buffer *block_data );

static int
compress_data( libspectrum_buffer *dest, const libspectrum_byte *src_data,
               size_t src_data_length, int compress );

static libspectrum_error
read_ram_page( libspectrum_byte **data, size_t *page,
	       const libspectrum_byte **buffer, size_t data_length,
	       size_t uncompressed_length, libspectrum_word *flags )
{
#ifdef HAVE_ZLIB_H

  libspectrum_error error;

#endif			/* #ifdef HAVE_ZLIB_H */

  if( data_length < 3 ) {
    libspectrum_print_error( LIBSPECTRUM_ERROR_UNKNOWN,
			     "%s:read_ram_page: length %lu too short",
			     __FILE__, (unsigned long)data_length );
    return LIBSPECTRUM_ERROR_UNKNOWN;
  }

  *flags = libspectrum_read_word( buffer );

  *page = **buffer; (*buffer)++;

  if( *flags & ZXSTRF_COMPRESSED ) {

#ifdef HAVE_ZLIB_H

    error = libspectrum_zlib_inflate( *buffer, data_length - 3, data,
				      &uncompressed_length );
    if( error ) return error;

    *buffer += data_length - 3;

#else			/* #ifdef HAVE_ZLIB_H */

    libspectrum_print_error(
      LIBSPECTRUM_ERROR_UNKNOWN,
      "%s:read_ram_page: zlib needed for decompression\n",
      __FILE__
    );
    return LIBSPECTRUM_ERROR_UNKNOWN;

#endif			/* #ifdef HAVE_ZLIB_H */

  } else {

    if( data_length < 3 + uncompressed_length ) {
      libspectrum_print_error( LIBSPECTRUM_ERROR_UNKNOWN,
			       "%s:read_ram_page: length %lu too short",
			       __FILE__, (unsigned long)data_length );
      return LIBSPECTRUM_ERROR_UNKNOWN;
    }

    *data = libspectrum_new( libspectrum_byte, uncompressed_length );
    memcpy( *data, *buffer, uncompressed_length );
    *buffer += uncompressed_length;

  }

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
read_atrp_chunk( libspectrum_snap *snap, libspectrum_word version GCC_UNUSED,
		 const libspectrum_byte **buffer,
		 const libspectrum_byte *end GCC_UNUSED, size_t data_length,
                 szx_context *ctx GCC_UNUSED )
{
  libspectrum_byte *data;
  size_t page;
  libspectrum_error error;
  libspectrum_word flags;

  error = read_ram_page( &data, &page, buffer, data_length, 0x4000, &flags );
  if( error ) return error;

  if( page >= SNAPSHOT_ZXATASP_PAGES ) {
    libspectrum_print_error( LIBSPECTRUM_ERROR_CORRUPT,
			     "%s:read_atrp_chunk: unknown page number %lu",
			     __FILE__, (unsigned long)page );
    libspectrum_free( data );
    return LIBSPECTRUM_ERROR_CORRUPT;
  }

  libspectrum_snap_set_zxatasp_ram( snap, page, data );

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
read_ay_chunk( libspectrum_snap *snap, libspectrum_word version GCC_UNUSED,
	       const libspectrum_byte **buffer,
	       const libspectrum_byte *end GCC_UNUSED, size_t data_length,
               szx_context *ctx GCC_UNUSED )
{
  size_t i;
  libspectrum_byte flags;

  if( data_length != 18 ) {
    libspectrum_print_error( LIBSPECTRUM_ERROR_UNKNOWN,
			     "szx_read_ay_chunk: unknown length %lu",
			     (unsigned long)data_length );
    return LIBSPECTRUM_ERROR_UNKNOWN;
  }

  flags = **buffer; (*buffer)++;
  libspectrum_snap_set_fuller_box_active( snap, flags & ZXSTAYF_FULLERBOX );
  libspectrum_snap_set_melodik_active( snap, !!( flags & ZXSTAYF_128AY ) );

  libspectrum_snap_set_out_ay_registerport( snap, **buffer ); (*buffer)++;

  for( i = 0; i < 16; i++ ) {
    libspectrum_snap_set_ay_registers( snap, i, **buffer ); (*buffer)++;
  }

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
read_b128_chunk( libspectrum_snap *snap, libspectrum_word version GCC_UNUSED,
		 const libspectrum_byte **buffer,
		 const libspectrum_byte *end GCC_UNUSED, size_t data_length,
                 szx_context *ctx GCC_UNUSED )
{
#ifdef HAVE_ZLIB_H
  libspectrum_error error;
#endif
  libspectrum_byte *rom_data = NULL;
  libspectrum_dword flags;
  const size_t expected_length = 0x4000;

  if( data_length < 10 ) {
    libspectrum_print_error( LIBSPECTRUM_ERROR_UNKNOWN,
			     "szx_read_b128_chunk: length %lu too short",
			     (unsigned long)data_length );
    return LIBSPECTRUM_ERROR_UNKNOWN;
  }

  flags = libspectrum_read_dword( buffer );
  libspectrum_snap_set_beta_active( snap, 1 );
  libspectrum_snap_set_beta_paged( snap, !!( flags & ZXSTBETAF_PAGED ) );
  libspectrum_snap_set_beta_autoboot( snap, !!( flags & ZXSTBETAF_AUTOBOOT ) );
  libspectrum_snap_set_beta_direction( snap,
				       !( flags & ZXSTBETAF_SEEKLOWER ) );

  libspectrum_snap_set_beta_custom_rom( snap,
                                        !!( flags & ZXSTBETAF_CUSTOMROM ) );

  libspectrum_snap_set_beta_drive_count( snap, **buffer ); (*buffer)++;
  libspectrum_snap_set_beta_system( snap, **buffer ); (*buffer)++;
  libspectrum_snap_set_beta_track ( snap, **buffer ); (*buffer)++;
  libspectrum_snap_set_beta_sector( snap, **buffer ); (*buffer)++;
  libspectrum_snap_set_beta_data  ( snap, **buffer ); (*buffer)++;
  libspectrum_snap_set_beta_status( snap, **buffer ); (*buffer)++;

  if( libspectrum_snap_beta_custom_rom( snap ) ) {

    if( flags & ZXSTBETAF_COMPRESSED ) {

#ifdef HAVE_ZLIB_H

      size_t uncompressed_length = 0;

      error = libspectrum_zlib_inflate( *buffer, data_length - 10, &rom_data,
					&uncompressed_length );
      if( error ) return error;

      if( uncompressed_length != expected_length ) {
	libspectrum_print_error( LIBSPECTRUM_ERROR_UNKNOWN,
				 "%s:read_b128_chunk: invalid ROM length "
				 "in compressed file, should be %lu, file "
				 "has %lu",
				 __FILE__,
				 (unsigned long)expected_length,
				 (unsigned long)uncompressed_length );
	return LIBSPECTRUM_ERROR_UNKNOWN;
      }

#else

      libspectrum_print_error(
	LIBSPECTRUM_ERROR_UNKNOWN,
	"%s:read_b128_chunk: zlib needed for decompression\n",
	__FILE__
      );
      return LIBSPECTRUM_ERROR_UNKNOWN;

#endif

    } else {

      if( data_length < 10 + expected_length ) {
        libspectrum_print_error( LIBSPECTRUM_ERROR_UNKNOWN,
				 "%s:read_b128_chunk: length %lu too short, "
				 "expected %lu",
				 __FILE__, (unsigned long)data_length,
				 (unsigned long)( 10UL + expected_length ) );
	return LIBSPECTRUM_ERROR_UNKNOWN;
      }

      rom_data = libspectrum_new( libspectrum_byte, expected_length );
      memcpy( rom_data, *buffer, expected_length );

    }

  }

  libspectrum_snap_set_beta_rom( snap, 0, rom_data );

  /* Skip any extra data (most likely a custom ROM) */
  *buffer += data_length - 10;

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
read_covx_chunk( libspectrum_snap *snap, libspectrum_word version GCC_UNUSED,
		 const libspectrum_byte **buffer,
		 const libspectrum_byte *end GCC_UNUSED, size_t data_length,
		 szx_context *ctx )
{
  if( data_length != 4 ) {
    libspectrum_print_error( LIBSPECTRUM_ERROR_UNKNOWN,
			     "%s:read_covx_chunk: unknown length %lu",
			     __FILE__, (unsigned long)data_length );
    return LIBSPECTRUM_ERROR_UNKNOWN;
  }

  libspectrum_snap_set_covox_dac( snap, *(*buffer)++ );

  libspectrum_snap_set_covox_active( snap, 1 );

  *buffer += 3;			/* Skip 'reserved' data */

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
read_crtr_chunk( libspectrum_snap *snap, libspectrum_word version GCC_UNUSED,
		 const libspectrum_byte **buffer,
		 const libspectrum_byte *end GCC_UNUSED, size_t data_length,
                 szx_context *ctx )
{
  if( data_length < 36 ) {
    libspectrum_print_error( LIBSPECTRUM_ERROR_UNKNOWN,
			     "%s:read_crtr_chunk: length %lu too short",
			     __FILE__, (unsigned long)data_length );
    return LIBSPECTRUM_ERROR_UNKNOWN;
  }

  *buffer += 36;
  data_length -= 36;

  /* This is ugly, but I can't see a better way to do it */
  if( sizeof( libspectrum_byte ) == sizeof( char ) ) {
    char *custom = libspectrum_new( char, data_length + 1 );
    char *libspectrum;

    memcpy( custom, *buffer, data_length );
    custom[data_length] = 0;

    libspectrum = strstr( custom, libspectrum_string );
    if( libspectrum ) {
      int matches, v1, v2, v3;
      libspectrum += strlen( libspectrum_string );
      matches = sscanf( libspectrum, "%d.%d.%d", &v1, &v2, &v3 );
      if( matches == 3 ) {
        if( v1 == 0 ) {
          if( v2 < 5 || ( v2 == 5 && v3 == 0 ) ) {
            ctx->swap_af = 1;
          }
        }
      }
    }

    libspectrum_free( custom );
  }

  *buffer += data_length;

  return LIBSPECTRUM_ERROR_NONE;
}
     
static libspectrum_error
read_opus_chunk( libspectrum_snap *snap, libspectrum_word version GCC_UNUSED,
		 const libspectrum_byte **buffer,
		 const libspectrum_byte *end GCC_UNUSED, size_t data_length,
                 szx_context *ctx GCC_UNUSED )
{
#ifdef HAVE_ZLIB_H
  libspectrum_error error;
#endif
  libspectrum_byte *ram_data = NULL, *rom_data = NULL;
  libspectrum_dword flags;
  size_t disc_ram_length;
  size_t disc_rom_length;
  size_t expected_length = 0x800;

  if( data_length < 23 ) {
    libspectrum_print_error( LIBSPECTRUM_ERROR_UNKNOWN,
			     "szx_read_opus_chunk: length %lu too short",
			     (unsigned long)data_length );
    return LIBSPECTRUM_ERROR_UNKNOWN;
  }

  libspectrum_snap_set_opus_active( snap, 1 );

  flags = libspectrum_read_dword( buffer );
  libspectrum_snap_set_opus_paged( snap, flags & ZXSTOPUSF_PAGED );
  libspectrum_snap_set_opus_direction( snap,
				       !( flags & ZXSTOPUSF_SEEKLOWER ) );

  disc_ram_length = libspectrum_read_dword( buffer );
  disc_rom_length = libspectrum_read_dword( buffer );

  libspectrum_snap_set_opus_custom_rom( snap,
                                        !!( flags & ZXSTOPUSF_CUSTOMROM ) );
  if( libspectrum_snap_opus_custom_rom( snap ) && !disc_rom_length ) {
    libspectrum_print_error( LIBSPECTRUM_ERROR_UNKNOWN,
			     "szx_read_opus_chunk: block flagged as custom "
                             "ROM but there is no custom ROM stored in the "
                             "snapshot" );
    return LIBSPECTRUM_ERROR_UNKNOWN;
  }

  libspectrum_snap_set_opus_control_a( snap, **buffer ); (*buffer)++;
  libspectrum_snap_set_opus_data_reg_a( snap, **buffer ); (*buffer)++;
  libspectrum_snap_set_opus_data_dir_a( snap, **buffer ); (*buffer)++;
  libspectrum_snap_set_opus_control_b( snap, **buffer ); (*buffer)++;
  libspectrum_snap_set_opus_data_reg_b( snap, **buffer ); (*buffer)++;
  libspectrum_snap_set_opus_data_dir_b( snap, **buffer ); (*buffer)++;
  libspectrum_snap_set_opus_drive_count( snap, **buffer ); (*buffer)++;
  libspectrum_snap_set_opus_track ( snap, **buffer ); (*buffer)++;
  libspectrum_snap_set_opus_sector( snap, **buffer ); (*buffer)++;
  libspectrum_snap_set_opus_data  ( snap, **buffer ); (*buffer)++;
  libspectrum_snap_set_opus_status( snap, **buffer ); (*buffer)++;

  if( flags & ZXSTOPUSF_COMPRESSED ) {

#ifdef HAVE_ZLIB_H

    size_t uncompressed_length = 0;

    if( (!libspectrum_snap_opus_custom_rom( snap ) &&
         disc_rom_length != 0 ) ||
        (libspectrum_snap_opus_custom_rom( snap ) &&
         disc_rom_length == 0 ) ) {
      libspectrum_print_error( LIBSPECTRUM_ERROR_UNKNOWN,
			       "%s:read_opus_chunk: invalid ROM length "
                               "in compressed file, should be %lu, file "
                               "has %lu",
			       __FILE__, 
                               0UL,
                               (unsigned long)disc_rom_length );
      return LIBSPECTRUM_ERROR_UNKNOWN;
    }

    if( data_length < 23 + disc_ram_length + disc_rom_length ) {
      libspectrum_print_error( LIBSPECTRUM_ERROR_UNKNOWN,
			       "%s:read_opus_chunk: length %lu too short, "
			       "expected %lu" ,
			       __FILE__, (unsigned long)data_length,
			       (unsigned long)( 23UL + disc_ram_length +
			                               disc_rom_length ) );
      return LIBSPECTRUM_ERROR_UNKNOWN;
    }

    error = libspectrum_zlib_inflate( *buffer, disc_ram_length, &ram_data,
				      &uncompressed_length );
    if( error ) return error;

    if( uncompressed_length != expected_length ) {
      libspectrum_print_error( LIBSPECTRUM_ERROR_UNKNOWN,
			       "%s:read_opus_chunk: invalid RAM length "
                               "in compressed file, should be %lu, file "
                               "has %lu",
			       __FILE__, 
                               (unsigned long)expected_length,
                               (unsigned long)uncompressed_length );
      return LIBSPECTRUM_ERROR_UNKNOWN;
    }

    *buffer += disc_ram_length;

    if( libspectrum_snap_opus_custom_rom( snap ) ) {
      uncompressed_length = 0;

      error = libspectrum_zlib_inflate( *buffer, disc_rom_length, &rom_data,
                                        &uncompressed_length );
      if( error ) return error;

      expected_length = 0x2000;

      if( uncompressed_length != expected_length ) {
        libspectrum_print_error( LIBSPECTRUM_ERROR_UNKNOWN,
                                 "%s:read_opus_chunk: invalid ROM length "
                                 "in compressed file, should be %lu, file "
                                 "has %lu",
                                 __FILE__, 
                                 (unsigned long)expected_length,
                                 (unsigned long)uncompressed_length );
        return LIBSPECTRUM_ERROR_UNKNOWN;
      }

      *buffer += disc_rom_length;
    }

#else			/* #ifdef HAVE_ZLIB_H */

    libspectrum_print_error(
      LIBSPECTRUM_ERROR_UNKNOWN,
      "%s:read_opus_chunk: zlib needed for decompression\n",
      __FILE__
    );
    return LIBSPECTRUM_ERROR_UNKNOWN;

#endif			/* #ifdef HAVE_ZLIB_H */

  } else {

    if( disc_ram_length != expected_length ) {
      libspectrum_print_error( LIBSPECTRUM_ERROR_UNKNOWN,
			       "%s:read_opus_chunk: invalid RAM length "
                               "in uncompressed file, should be %lu, file "
                               "has %lu",
			       __FILE__, 
                               (unsigned long)expected_length,
                               (unsigned long)disc_ram_length );
      return LIBSPECTRUM_ERROR_UNKNOWN;
    }

    expected_length = 0x2000;

    if( (libspectrum_snap_opus_custom_rom( snap ) &&
         disc_rom_length != expected_length ) ||
        (!libspectrum_snap_opus_custom_rom( snap ) &&
         disc_rom_length != 0 ) ) {
      libspectrum_print_error( LIBSPECTRUM_ERROR_UNKNOWN,
			       "%s:read_opus_chunk: invalid ROM length "
                               "in uncompressed file, should be %lu, file "
                               "has %lu",
			       __FILE__, 
                               libspectrum_snap_opus_custom_rom( snap ) ?
                                 (unsigned long)expected_length : 0UL,
                               (unsigned long)disc_rom_length );
      return LIBSPECTRUM_ERROR_UNKNOWN;
    }

    if( data_length < 23 + disc_ram_length + disc_rom_length ) {
      libspectrum_print_error( LIBSPECTRUM_ERROR_UNKNOWN,
			       "%s:read_opus_chunk: length %lu too short, "
			       "expected %lu" ,
			       __FILE__, (unsigned long)data_length,
			       (unsigned long)( 23UL + disc_ram_length + disc_rom_length ) );
      return LIBSPECTRUM_ERROR_UNKNOWN;
    }

    expected_length = 0x800;

    ram_data = libspectrum_new( libspectrum_byte, expected_length );
    memcpy( ram_data, *buffer, expected_length );
    *buffer += expected_length;

    if( libspectrum_snap_opus_custom_rom( snap ) ) {
      expected_length = 0x2000;
      rom_data = libspectrum_new( libspectrum_byte, expected_length );
      memcpy( rom_data, *buffer, expected_length );
      *buffer += expected_length;
    }

  }

  libspectrum_snap_set_opus_ram( snap, 0, ram_data );
  libspectrum_snap_set_opus_rom( snap, 0, rom_data );

  return LIBSPECTRUM_ERROR_NONE;
}
     
static libspectrum_error
read_plsd_chunk( libspectrum_snap *snap, libspectrum_word version GCC_UNUSED,
		  const libspectrum_byte **buffer,
		  const libspectrum_byte *end GCC_UNUSED, size_t data_length,
                 szx_context *ctx GCC_UNUSED )
{
#ifdef HAVE_ZLIB_H
  libspectrum_error error;
#endif
  libspectrum_byte *ram_data = NULL, *rom_data = NULL;
  libspectrum_byte rom_type;
  libspectrum_dword flags;
  size_t disc_ram_length;
  size_t disc_rom_length;
  const size_t expected_length = 0x2000;

  if( data_length < 19 ) {
    libspectrum_print_error( LIBSPECTRUM_ERROR_UNKNOWN,
			     "szx_read_plusd_chunk: length %lu too short",
			     (unsigned long)data_length );
    return LIBSPECTRUM_ERROR_UNKNOWN;
  }

  libspectrum_snap_set_plusd_active( snap, 1 );

  flags = libspectrum_read_dword( buffer );
  libspectrum_snap_set_plusd_paged( snap, flags & ZXSTPLUSDF_PAGED );
  libspectrum_snap_set_plusd_direction( snap,
				       !( flags & ZXSTPLUSDF_SEEKLOWER ) );

  disc_ram_length = libspectrum_read_dword( buffer );
  disc_rom_length = libspectrum_read_dword( buffer );
  rom_type = *(*buffer)++;

  libspectrum_snap_set_plusd_custom_rom( snap, rom_type == ZXSTPDRT_CUSTOM );
  if( libspectrum_snap_plusd_custom_rom( snap ) && !disc_rom_length ) {
    libspectrum_print_error( LIBSPECTRUM_ERROR_UNKNOWN,
			     "szx_read_plusd_chunk: block flagged as custom "
                             "ROM but there is no custom ROM stored in the "
                             "snapshot" );
    return LIBSPECTRUM_ERROR_UNKNOWN;
  }

  libspectrum_snap_set_plusd_control( snap, **buffer ); (*buffer)++;
  libspectrum_snap_set_plusd_drive_count( snap, **buffer ); (*buffer)++;
  libspectrum_snap_set_plusd_track ( snap, **buffer ); (*buffer)++;
  libspectrum_snap_set_plusd_sector( snap, **buffer ); (*buffer)++;
  libspectrum_snap_set_plusd_data  ( snap, **buffer ); (*buffer)++;
  libspectrum_snap_set_plusd_status( snap, **buffer ); (*buffer)++;

  if( flags & ZXSTPLUSDF_COMPRESSED ) {

#ifdef HAVE_ZLIB_H

    size_t uncompressed_length = 0;

    if( (!libspectrum_snap_plusd_custom_rom( snap ) &&
         disc_rom_length != 0 ) ||
        (libspectrum_snap_plusd_custom_rom( snap ) &&
         disc_rom_length == 0 ) ) {
      libspectrum_print_error( LIBSPECTRUM_ERROR_UNKNOWN,
			       "%s:read_plsd_chunk: invalid ROM length "
                               "in compressed file, should be %lu, file "
                               "has %lu",
			       __FILE__, 
                               0UL,
                               (unsigned long)disc_rom_length );
      return LIBSPECTRUM_ERROR_UNKNOWN;
    }

    if( data_length < 19 + disc_ram_length + disc_rom_length ) {
      libspectrum_print_error( LIBSPECTRUM_ERROR_UNKNOWN,
			       "%s:read_plsd_chunk: length %lu too short, "
			       "expected %lu" ,
			       __FILE__, (unsigned long)data_length,
			       (unsigned long)( 19UL + disc_ram_length +
			                               disc_rom_length ) );
      return LIBSPECTRUM_ERROR_UNKNOWN;
    }

    error = libspectrum_zlib_inflate( *buffer, disc_ram_length, &ram_data,
				      &uncompressed_length );
    if( error ) return error;

    if( uncompressed_length != expected_length ) {
      libspectrum_print_error( LIBSPECTRUM_ERROR_UNKNOWN,
			       "%s:read_plsd_chunk: invalid RAM length "
                               "in compressed file, should be %lu, file "
                               "has %lu",
			       __FILE__, 
                               (unsigned long)expected_length,
                               (unsigned long)uncompressed_length );
      return LIBSPECTRUM_ERROR_UNKNOWN;
    }

    *buffer += disc_ram_length;

    if( libspectrum_snap_plusd_custom_rom( snap ) ) {
      uncompressed_length = 0;

      error = libspectrum_zlib_inflate( *buffer, disc_rom_length, &rom_data,
                                        &uncompressed_length );
      if( error ) return error;

      if( uncompressed_length != expected_length ) {
        libspectrum_print_error( LIBSPECTRUM_ERROR_UNKNOWN,
                                 "%s:read_plsd_chunk: invalid ROM length "
                                 "in compressed file, should be %lu, file "
                                 "has %lu",
                                 __FILE__, 
                                 (unsigned long)expected_length,
                                 (unsigned long)uncompressed_length );
        return LIBSPECTRUM_ERROR_UNKNOWN;
      }

      *buffer += disc_rom_length;
    }

#else			/* #ifdef HAVE_ZLIB_H */

    libspectrum_print_error(
      LIBSPECTRUM_ERROR_UNKNOWN,
      "%s:read_plsd_chunk: zlib needed for decompression\n",
      __FILE__
    );
    return LIBSPECTRUM_ERROR_UNKNOWN;

#endif			/* #ifdef HAVE_ZLIB_H */

  } else {

    if( disc_ram_length != expected_length ) {
      libspectrum_print_error( LIBSPECTRUM_ERROR_UNKNOWN,
			       "%s:read_plsd_chunk: invalid RAM length "
                               "in uncompressed file, should be %lu, file "
                               "has %lu",
			       __FILE__, 
                               (unsigned long)expected_length,
                               (unsigned long)disc_ram_length );
      return LIBSPECTRUM_ERROR_UNKNOWN;
    }

    if( (libspectrum_snap_plusd_custom_rom( snap ) &&
         disc_rom_length != expected_length ) ||
        (!libspectrum_snap_plusd_custom_rom( snap ) &&
         disc_rom_length != 0 ) ) {
      libspectrum_print_error( LIBSPECTRUM_ERROR_UNKNOWN,
			       "%s:read_plsd_chunk: invalid ROM length "
                               "in uncompressed file, should be %lu, file "
                               "has %lu",
			       __FILE__, 
                               libspectrum_snap_plusd_custom_rom( snap ) ?
                                 (unsigned long)expected_length : 0UL,
                               (unsigned long)disc_rom_length );
      return LIBSPECTRUM_ERROR_UNKNOWN;
    }

    if( data_length < 19 + disc_ram_length + disc_rom_length ) {
      libspectrum_print_error( LIBSPECTRUM_ERROR_UNKNOWN,
			       "%s:read_plsd_chunk: length %lu too short, "
			       "expected %lu" ,
			       __FILE__, (unsigned long)data_length,
			       (unsigned long)( 19UL + disc_ram_length + disc_rom_length ) );
      return LIBSPECTRUM_ERROR_UNKNOWN;
    }

    ram_data = libspectrum_new( libspectrum_byte, expected_length );
    memcpy( ram_data, *buffer, expected_length );
    *buffer += expected_length;

    if( libspectrum_snap_plusd_custom_rom( snap ) ) {
      rom_data = libspectrum_new( libspectrum_byte, expected_length );
      memcpy( rom_data, *buffer, expected_length );
      *buffer += expected_length;
    }

  }

  libspectrum_snap_set_plusd_ram( snap, 0, ram_data );
  libspectrum_snap_set_plusd_rom( snap, 0, rom_data );

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
read_cfrp_chunk( libspectrum_snap *snap, libspectrum_word version GCC_UNUSED,
		 const libspectrum_byte **buffer,
		 const libspectrum_byte *end GCC_UNUSED, size_t data_length,
                 szx_context *ctx GCC_UNUSED )
{
  libspectrum_byte *data;
  size_t page;
  libspectrum_error error;
  libspectrum_word flags;

  error = read_ram_page( &data, &page, buffer, data_length, 0x4000, &flags );
  if( error ) return error;

  if( page >= SNAPSHOT_ZXCF_PAGES ) {
    libspectrum_print_error( LIBSPECTRUM_ERROR_CORRUPT,
			     "%s:read_cfrp_chunk: unknown page number %lu",
			     __FILE__, (unsigned long)page );
    libspectrum_free( data );
    return LIBSPECTRUM_ERROR_CORRUPT;
  }

  libspectrum_snap_set_zxcf_ram( snap, page, data );

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
read_side_chunk( libspectrum_snap *snap, libspectrum_word version GCC_UNUSED,
		 const libspectrum_byte **buffer,
		 const libspectrum_byte *end GCC_UNUSED, size_t data_length,
                 szx_context *ctx GCC_UNUSED )
{
  if( data_length ) {
    libspectrum_print_error( LIBSPECTRUM_ERROR_UNKNOWN,
			     "%s:read_side_chunk: unknown length %lu",
			     __FILE__, (unsigned long)data_length );
    return LIBSPECTRUM_ERROR_UNKNOWN;
  }

  libspectrum_snap_set_simpleide_active( snap, 1 );

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
read_drum_chunk( libspectrum_snap *snap, libspectrum_word version GCC_UNUSED,
		 const libspectrum_byte **buffer,
		 const libspectrum_byte *end GCC_UNUSED, size_t data_length,
                 szx_context *ctx GCC_UNUSED )
{
  libspectrum_byte volume;

  if( data_length != 1 ) {
    libspectrum_print_error( LIBSPECTRUM_ERROR_UNKNOWN,
			     "%s:read_drum_chunk: unknown length %lu",
			     __FILE__, (unsigned long)data_length );
    return LIBSPECTRUM_ERROR_UNKNOWN;
  }

  volume = *(*buffer)++;

  libspectrum_snap_set_specdrum_dac( snap, volume - 128 );

  libspectrum_snap_set_specdrum_active( snap, 1 );

  return LIBSPECTRUM_ERROR_NONE;
}


static void
add_joystick( libspectrum_snap *snap, libspectrum_joystick type, int inputs )
{
  size_t i;
  size_t num_joysticks = libspectrum_snap_joystick_active_count( snap );

  for( i = 0; i < num_joysticks; i++ ) {
    if( libspectrum_snap_joystick_list( snap, i ) == type ) {
      libspectrum_snap_set_joystick_inputs( snap, i, inputs |
                                  libspectrum_snap_joystick_inputs( snap, i ) );
      return;
    }
  }

  libspectrum_snap_set_joystick_list( snap, num_joysticks, type );
  libspectrum_snap_set_joystick_inputs( snap, num_joysticks, inputs );
  libspectrum_snap_set_joystick_active_count( snap, num_joysticks + 1 );
}

static libspectrum_error
read_joy_chunk( libspectrum_snap *snap, libspectrum_word version,
		 const libspectrum_byte **buffer,
		 const libspectrum_byte *end GCC_UNUSED, size_t data_length,
                 szx_context *ctx GCC_UNUSED )
{
  libspectrum_dword flags;

  if( data_length != 6 ) {
    libspectrum_print_error( LIBSPECTRUM_ERROR_UNKNOWN,
			     "%s:read_joy_chunk: unknown length %lu",
			     __FILE__, (unsigned long)data_length );
    return LIBSPECTRUM_ERROR_UNKNOWN;
  }

  flags = libspectrum_read_dword( buffer );
  if( flags & ZXSTJOYF_ALWAYSPORT31 ) {
    add_joystick( snap, LIBSPECTRUM_JOYSTICK_KEMPSTON,
                  LIBSPECTRUM_JOYSTICK_INPUT_NONE );
  }

  switch( **buffer ) {
  case ZXJT_KEMPSTON:
    add_joystick( snap, LIBSPECTRUM_JOYSTICK_KEMPSTON,
                  LIBSPECTRUM_JOYSTICK_INPUT_JOYSTICK_1 );
    break;
  case ZXJT_FULLER:
    add_joystick( snap, LIBSPECTRUM_JOYSTICK_FULLER,
                  LIBSPECTRUM_JOYSTICK_INPUT_JOYSTICK_1 );
    break;
  case ZXJT_CURSOR:
    add_joystick( snap, LIBSPECTRUM_JOYSTICK_CURSOR,
                  LIBSPECTRUM_JOYSTICK_INPUT_JOYSTICK_1 );
    break;
  case ZXJT_SINCLAIR1:
    add_joystick( snap, LIBSPECTRUM_JOYSTICK_SINCLAIR_1,
                  LIBSPECTRUM_JOYSTICK_INPUT_JOYSTICK_1 );
    break;
  case ZXJT_SINCLAIR2:
    add_joystick( snap, LIBSPECTRUM_JOYSTICK_SINCLAIR_2,
                  LIBSPECTRUM_JOYSTICK_INPUT_JOYSTICK_1 );
    break;
  case ZXJT_TIMEX1:
    add_joystick( snap, LIBSPECTRUM_JOYSTICK_TIMEX_1,
                  LIBSPECTRUM_JOYSTICK_INPUT_JOYSTICK_1 );
    break;
  case ZXJT_TIMEX2:
    add_joystick( snap, LIBSPECTRUM_JOYSTICK_TIMEX_2,
                  LIBSPECTRUM_JOYSTICK_INPUT_JOYSTICK_1 );
    break;
  case ZXJT_NONE:
    break;
  }
  (*buffer)++;

  switch( **buffer ) {
  case ZXJT_KEMPSTON:
    add_joystick( snap, LIBSPECTRUM_JOYSTICK_KEMPSTON,
                  LIBSPECTRUM_JOYSTICK_INPUT_JOYSTICK_2 );
    break;
  case ZXJT_FULLER:
    add_joystick( snap, LIBSPECTRUM_JOYSTICK_FULLER,
                  LIBSPECTRUM_JOYSTICK_INPUT_JOYSTICK_2 );
    break;
  case ZXJT_CURSOR:
    add_joystick( snap, LIBSPECTRUM_JOYSTICK_CURSOR,
                  LIBSPECTRUM_JOYSTICK_INPUT_JOYSTICK_2 );
    break;
  case ZXJT_SINCLAIR1:
    add_joystick( snap, LIBSPECTRUM_JOYSTICK_SINCLAIR_1,
                  LIBSPECTRUM_JOYSTICK_INPUT_JOYSTICK_2 );
    break;
  case ZXJT_SINCLAIR2:
    add_joystick( snap, LIBSPECTRUM_JOYSTICK_SINCLAIR_2,
                  LIBSPECTRUM_JOYSTICK_INPUT_JOYSTICK_2 );
    break;
  case ZXJT_TIMEX1:
    add_joystick( snap, LIBSPECTRUM_JOYSTICK_TIMEX_1,
                  LIBSPECTRUM_JOYSTICK_INPUT_JOYSTICK_2 );
    break;
  case ZXJT_TIMEX2:
    add_joystick( snap, LIBSPECTRUM_JOYSTICK_TIMEX_2,
                  LIBSPECTRUM_JOYSTICK_INPUT_JOYSTICK_2 );
    break;
  case ZXJT_NONE:
    break;
  }
  (*buffer)++;

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
read_keyb_chunk( libspectrum_snap *snap, libspectrum_word version,
		 const libspectrum_byte **buffer,
		 const libspectrum_byte *end GCC_UNUSED, size_t data_length,
                 szx_context *ctx GCC_UNUSED )
{
  size_t expected_length;
  libspectrum_dword flags;

  expected_length = version >= 0x0101 ? 5 : 4;

  if( data_length != expected_length ) {
    libspectrum_print_error( LIBSPECTRUM_ERROR_UNKNOWN,
			     "%s:read_keyb_chunk: unknown length %lu",
			     __FILE__, (unsigned long)data_length );
    return LIBSPECTRUM_ERROR_UNKNOWN;
  }

  flags = libspectrum_read_dword( buffer );
  libspectrum_snap_set_issue2( snap, !!( flags & ZXSTKF_ISSUE2 ) );

  if( expected_length >= 5 ) {
    switch( **buffer ) {
    case ZXJT_KEMPSTON:
      add_joystick( snap, LIBSPECTRUM_JOYSTICK_KEMPSTON,
                    LIBSPECTRUM_JOYSTICK_INPUT_KEYBOARD );
      break;
    case ZXJT_FULLER:
      add_joystick( snap, LIBSPECTRUM_JOYSTICK_FULLER,
                    LIBSPECTRUM_JOYSTICK_INPUT_KEYBOARD );
      break;
    case ZXJT_CURSOR:
      add_joystick( snap, LIBSPECTRUM_JOYSTICK_CURSOR,
                    LIBSPECTRUM_JOYSTICK_INPUT_KEYBOARD );
      break;
    case ZXJT_SINCLAIR1:
      add_joystick( snap, LIBSPECTRUM_JOYSTICK_SINCLAIR_1,
                    LIBSPECTRUM_JOYSTICK_INPUT_KEYBOARD );
      break;
    case ZXJT_SINCLAIR2:
      add_joystick( snap, LIBSPECTRUM_JOYSTICK_SINCLAIR_2,
                    LIBSPECTRUM_JOYSTICK_INPUT_KEYBOARD );
      break;
    case ZXJT_TIMEX1:
      add_joystick( snap, LIBSPECTRUM_JOYSTICK_TIMEX_1,
                    LIBSPECTRUM_JOYSTICK_INPUT_KEYBOARD );
      break;
    case ZXJT_TIMEX2:
      add_joystick( snap, LIBSPECTRUM_JOYSTICK_TIMEX_2,
                    LIBSPECTRUM_JOYSTICK_INPUT_KEYBOARD );
      break;
    case ZXJT_SPECTRUMPLUS: /* Actually, no joystick at all */
      break;
    }
    (*buffer)++;
  }

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
read_amxm_chunk( libspectrum_snap *snap, libspectrum_word version,
		 const libspectrum_byte **buffer,
		 const libspectrum_byte *end GCC_UNUSED, size_t data_length,
                 szx_context *ctx GCC_UNUSED )
{
  if( data_length != 7 ) {
    libspectrum_print_error( LIBSPECTRUM_ERROR_UNKNOWN,
			     "read_amxm_chunk: unknown length %lu",
			     (unsigned long)data_length );
    return LIBSPECTRUM_ERROR_UNKNOWN;
  }

  switch( **buffer ) {
  case ZXSTM_AMX:
    break;
  case ZXSTM_KEMPSTON:
    libspectrum_snap_set_kempston_mouse_active( snap, 1 );
    break;
  case ZXSTM_NONE:
  default:
    break;
  }
  (*buffer)++;

  /* Z80 PIO CTRLA registers for AMX mouse */
  (*buffer)++; (*buffer)++; (*buffer)++;

  /* Z80 PIO CTRLB registers for AMX mouse */
  (*buffer)++; (*buffer)++; (*buffer)++;

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
read_ramp_chunk( libspectrum_snap *snap, libspectrum_word version GCC_UNUSED,
		 const libspectrum_byte **buffer,
		 const libspectrum_byte *end GCC_UNUSED, size_t data_length,
                 szx_context *ctx GCC_UNUSED )
{
  libspectrum_byte *data;
  size_t page;
  libspectrum_error error;
  libspectrum_word flags;


  error = read_ram_page( &data, &page, buffer, data_length, 0x4000, &flags );
  if( error ) return error;

  if( page > 63 ) {
    libspectrum_print_error( LIBSPECTRUM_ERROR_CORRUPT,
			     "%s:read_ramp_chunk: unknown page number %lu",
			     __FILE__, (unsigned long)page );
    libspectrum_free( data );
    return LIBSPECTRUM_ERROR_CORRUPT;
  }

  libspectrum_snap_set_pages( snap, page, data );

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
read_scld_chunk( libspectrum_snap *snap, libspectrum_word version GCC_UNUSED,
		 const libspectrum_byte **buffer,
		 const libspectrum_byte *end GCC_UNUSED, size_t data_length,
                 szx_context *ctx GCC_UNUSED )
{
  if( data_length != 2 ) {
    libspectrum_print_error( LIBSPECTRUM_ERROR_UNKNOWN,
			     "szx_read_scld_chunk: unknown length %lu",
			     (unsigned long)data_length );
    return LIBSPECTRUM_ERROR_UNKNOWN;
  }

  libspectrum_snap_set_out_scld_hsr( snap, **buffer ); (*buffer)++;
  libspectrum_snap_set_out_scld_dec( snap, **buffer ); (*buffer)++;

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
read_spcr_chunk( libspectrum_snap *snap, libspectrum_word version,
		 const libspectrum_byte **buffer,
		 const libspectrum_byte *end GCC_UNUSED, size_t data_length,
                 szx_context *ctx GCC_UNUSED )
{
  libspectrum_byte out_ula;
  int capabilities;

  if( data_length != 8 ) {
    libspectrum_print_error(
      LIBSPECTRUM_ERROR_UNKNOWN,
      "szx_read_spcr_chunk: unknown length %lu", (unsigned long)data_length
    );
    return LIBSPECTRUM_ERROR_UNKNOWN;
  }

  capabilities =
    libspectrum_machine_capabilities( libspectrum_snap_machine( snap ) );

  out_ula = **buffer & 0x07; (*buffer)++;

  libspectrum_snap_set_out_128_memoryport( snap, **buffer ); (*buffer)++;

  if( ( capabilities & LIBSPECTRUM_MACHINE_CAPABILITY_PLUS3_MEMORY )    ||
      ( capabilities & LIBSPECTRUM_MACHINE_CAPABILITY_SCORP_MEMORY )    ||
      ( capabilities & LIBSPECTRUM_MACHINE_CAPABILITY_PENT1024_MEMORY )    )
    libspectrum_snap_set_out_plus3_memoryport( snap, **buffer );
  (*buffer)++;

  if( version >= 0x0101 ) out_ula |= **buffer & 0xf8;
  (*buffer)++;

  libspectrum_snap_set_out_ula( snap, out_ula );

  *buffer += 4;			/* Skip 'reserved' data */

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
read_z80r_chunk( libspectrum_snap *snap, libspectrum_word version,
		 const libspectrum_byte **buffer,
		 const libspectrum_byte *end GCC_UNUSED, size_t data_length,
                 szx_context *ctx )
{
  if( data_length != 37 ) {
    libspectrum_print_error( LIBSPECTRUM_ERROR_UNKNOWN,
			     "szx_read_z80r_chunk: unknown length %lu",
			     (unsigned long)data_length );
    return LIBSPECTRUM_ERROR_UNKNOWN;
  }

  if( ctx->swap_af ) {
    libspectrum_snap_set_a( snap, **buffer ); (*buffer)++;
    libspectrum_snap_set_f( snap, **buffer ); (*buffer)++;
  } else {
    libspectrum_snap_set_f( snap, **buffer ); (*buffer)++;
    libspectrum_snap_set_a( snap, **buffer ); (*buffer)++;
  }

  libspectrum_snap_set_bc  ( snap, libspectrum_read_word( buffer ) );
  libspectrum_snap_set_de  ( snap, libspectrum_read_word( buffer ) );
  libspectrum_snap_set_hl  ( snap, libspectrum_read_word( buffer ) );

  if( ctx->swap_af ) {
    libspectrum_snap_set_a_( snap, **buffer ); (*buffer)++;
    libspectrum_snap_set_f_( snap, **buffer ); (*buffer)++;
  } else {
    libspectrum_snap_set_f_( snap, **buffer ); (*buffer)++;
    libspectrum_snap_set_a_( snap, **buffer ); (*buffer)++;
  }

  libspectrum_snap_set_bc_ ( snap, libspectrum_read_word( buffer ) );
  libspectrum_snap_set_de_ ( snap, libspectrum_read_word( buffer ) );
  libspectrum_snap_set_hl_ ( snap, libspectrum_read_word( buffer ) );

  libspectrum_snap_set_ix  ( snap, libspectrum_read_word( buffer ) );
  libspectrum_snap_set_iy  ( snap, libspectrum_read_word( buffer ) );
  libspectrum_snap_set_sp  ( snap, libspectrum_read_word( buffer ) );
  libspectrum_snap_set_pc  ( snap, libspectrum_read_word( buffer ) );

  libspectrum_snap_set_i   ( snap, **buffer ); (*buffer)++;
  libspectrum_snap_set_r   ( snap, **buffer ); (*buffer)++;
  libspectrum_snap_set_iff1( snap, **buffer ); (*buffer)++;
  libspectrum_snap_set_iff2( snap, **buffer ); (*buffer)++;
  libspectrum_snap_set_im  ( snap, **buffer ); (*buffer)++;

  libspectrum_snap_set_tstates( snap, libspectrum_read_dword( buffer ) );

  if( version >= 0x0101 ) {
    (*buffer)++;		/* Skip chHoldIntReqCycles */
    
    /* Flags */
    libspectrum_snap_set_last_instruction_ei( snap, !!(**buffer & ZXSTZF_EILAST) );
    libspectrum_snap_set_halted( snap, !!(**buffer & ZXSTZF_HALTED) );
    libspectrum_snap_set_last_instruction_set_f( snap, !!(**buffer & ZXSTZF_FSET) );
    (*buffer)++;

    if( version >= 0x0104 ) {
      libspectrum_snap_set_memptr( snap, libspectrum_read_word( buffer ) );
    } else {
      (*buffer)++;		/* Skip the hidden register */
      (*buffer)++;		/* Skip the reserved byte */
    }

  } else {
    *buffer += 4;		/* Skip the reserved dword */
  }

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
read_zxat_chunk( libspectrum_snap *snap, libspectrum_word version GCC_UNUSED,
		 const libspectrum_byte **buffer,
		 const libspectrum_byte *end GCC_UNUSED, size_t data_length,
                 szx_context *ctx GCC_UNUSED )
{
  libspectrum_word flags;

  if( data_length != 8 ) {
    libspectrum_print_error( LIBSPECTRUM_ERROR_UNKNOWN,
			     "%s:read_zxat_chunk: unknown length %lu",
			     __FILE__, (unsigned long)data_length );
    return LIBSPECTRUM_ERROR_UNKNOWN;
  }

  libspectrum_snap_set_zxatasp_active( snap, 1 );

  flags = libspectrum_read_word( buffer );
  libspectrum_snap_set_zxatasp_upload( snap, flags & ZXSTZXATF_UPLOAD );
  libspectrum_snap_set_zxatasp_writeprotect( snap,
    !!( flags & ZXSTZXATF_WRITEPROTECT ) );

  libspectrum_snap_set_zxatasp_port_a( snap, **buffer ); (*buffer)++;
  libspectrum_snap_set_zxatasp_port_b( snap, **buffer ); (*buffer)++;
  libspectrum_snap_set_zxatasp_port_c( snap, **buffer ); (*buffer)++;
  libspectrum_snap_set_zxatasp_control( snap, **buffer ); (*buffer)++;
  libspectrum_snap_set_zxatasp_pages( snap, **buffer ); (*buffer)++;
  libspectrum_snap_set_zxatasp_current_page( snap, **buffer ); (*buffer)++;

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
read_zxcf_chunk( libspectrum_snap *snap, libspectrum_word version GCC_UNUSED,
		 const libspectrum_byte **buffer,
		 const libspectrum_byte *end GCC_UNUSED, size_t data_length,
                 szx_context *ctx GCC_UNUSED )
{
  libspectrum_word flags;

  if( data_length != 4 ) {
    libspectrum_print_error( LIBSPECTRUM_ERROR_UNKNOWN,
			     "read_zxcf_chunk: unknown length %lu",
			     (unsigned long)data_length );
    return LIBSPECTRUM_ERROR_UNKNOWN;
  }

  libspectrum_snap_set_zxcf_active( snap, 1 );

  flags = libspectrum_read_word( buffer );
  libspectrum_snap_set_zxcf_upload( snap, flags & ZXSTZXCFF_UPLOAD );

  libspectrum_snap_set_zxcf_memctl( snap, **buffer ); (*buffer)++;
  libspectrum_snap_set_zxcf_pages( snap, **buffer ); (*buffer)++;

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
read_if1_chunk( libspectrum_snap *snap, libspectrum_word version GCC_UNUSED,
		const libspectrum_byte **buffer,
		const libspectrum_byte *end GCC_UNUSED, size_t data_length,
                szx_context *ctx GCC_UNUSED )
{
  libspectrum_word flags;
  libspectrum_word expected_length;
  libspectrum_byte *rom_data = NULL; 

  if( data_length < 40 ) {
    libspectrum_print_error(
      LIBSPECTRUM_ERROR_UNKNOWN,
      "read_if1_chunk: length %lu too short", (unsigned long)data_length
    );
    return LIBSPECTRUM_ERROR_UNKNOWN;
  }

  flags = libspectrum_read_word( buffer );
  libspectrum_snap_set_interface1_drive_count( snap, **buffer ); (*buffer)++;
  *buffer += 3;		/* Skip reserved byte space */
  *buffer += sizeof( libspectrum_dword ) * 8; /* Skip reserved dword space */
  expected_length = libspectrum_read_word( buffer );

  libspectrum_snap_set_interface1_active( snap, flags & ZXSTIF1F_ENABLED );

  libspectrum_snap_set_interface1_paged( snap, !!( flags & ZXSTIF1F_PAGED ) );

  if( expected_length ) {
    if( expected_length != 0x2000 && expected_length != 0x4000 ) {
        libspectrum_print_error( LIBSPECTRUM_ERROR_UNKNOWN,
                                 "%s:read_if1_chunk: invalid ROM length "
                                 "in file, should be 8192 or 16384 bytes, "
                                 "file has %lu",
                                 __FILE__, 
                                 (unsigned long)expected_length );
      return LIBSPECTRUM_ERROR_UNKNOWN;
    }

    libspectrum_snap_set_interface1_custom_rom( snap, 1 );

    if( flags & ZXSTIF1F_COMPRESSED ) {

#ifdef HAVE_ZLIB_H

      size_t uncompressed_length = 0;

      libspectrum_error error =
              libspectrum_zlib_inflate( *buffer, data_length - 40, &rom_data,
                                        &uncompressed_length );
      if( error ) return error;

      if( uncompressed_length != expected_length ) {
        libspectrum_print_error( LIBSPECTRUM_ERROR_UNKNOWN,
                                 "%s:read_if1_chunk: invalid ROM length "
                                 "in compressed file, should be %lu, file "
                                 "has %lu",
                                 __FILE__, 
                                 (unsigned long)expected_length,
                                 (unsigned long)uncompressed_length );
        return LIBSPECTRUM_ERROR_UNKNOWN;
      }

      libspectrum_snap_set_interface1_rom( snap, 0, rom_data );
      libspectrum_snap_set_interface1_rom_length( snap, 0,
                                                  uncompressed_length );

      *buffer += data_length - 40;

#else			/* #ifdef HAVE_ZLIB_H */

      libspectrum_print_error(
        LIBSPECTRUM_ERROR_UNKNOWN,
        "%s:read_if1_chunk: zlib needed for decompression\n",
        __FILE__
      );
      return LIBSPECTRUM_ERROR_UNKNOWN;

#endif			/* #ifdef HAVE_ZLIB_H */

    } else {

      if( data_length < 40 + expected_length ) {
        libspectrum_print_error( LIBSPECTRUM_ERROR_UNKNOWN,
                                 "%s:read_if1_chunk: length %lu too short, "
                                 "expected %lu" ,
                                 __FILE__, (unsigned long)data_length,
                                 (unsigned long)40 + expected_length );
        return LIBSPECTRUM_ERROR_UNKNOWN;
      }

      rom_data = libspectrum_new( libspectrum_byte, expected_length );
      memcpy( rom_data, *buffer, expected_length );

      libspectrum_snap_set_interface1_rom_length( snap, 0, expected_length );

      *buffer += expected_length;

    }
  }

  return LIBSPECTRUM_ERROR_NONE;
}

static void
szx_set_custom_rom( libspectrum_snap *snap, int page_no,
                    libspectrum_byte *rom_data, libspectrum_word length )
{
  if( length ) {
    libspectrum_byte *page = libspectrum_new( libspectrum_byte, length );
    memcpy( page, rom_data, length );

    libspectrum_snap_set_roms( snap, page_no, page );
    libspectrum_snap_set_rom_length( snap, page_no, length );
  }
}

static libspectrum_error
szx_extract_roms( libspectrum_snap *snap, libspectrum_byte *rom_data,
                  libspectrum_dword length, libspectrum_dword expected_length )
{
  int i;
  const size_t standard_rom_length = 0x4000;
  size_t num_16k_roms, additional_rom_length;

  if( length != expected_length ) {
    libspectrum_print_error( LIBSPECTRUM_ERROR_UNKNOWN,
                             "%s:szx_extract_roms: invalid ROM length %u, "
                             "expected %u",
                             __FILE__, length, expected_length );
    return LIBSPECTRUM_ERROR_UNKNOWN;
  }

  num_16k_roms = length / standard_rom_length;
  additional_rom_length = length % standard_rom_length;

  for( i = 0; i < num_16k_roms; i++ )
    szx_set_custom_rom( snap, i, rom_data + (i * standard_rom_length), standard_rom_length );

  /* Timex 2068 machines have a 16k and an 8k ROM, all other machines just have
     multiples of 16k ROMs */
  if( additional_rom_length )
    szx_set_custom_rom( snap, i, rom_data + (i * standard_rom_length), additional_rom_length );

  libspectrum_snap_set_custom_rom_pages( snap, num_16k_roms +
                                         !!additional_rom_length );

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
read_rom_chunk( libspectrum_snap *snap, libspectrum_word version GCC_UNUSED,
		const libspectrum_byte **buffer,
		const libspectrum_byte *end GCC_UNUSED, size_t data_length,
                 szx_context *ctx GCC_UNUSED )
{
  libspectrum_word flags;
  libspectrum_dword expected_length;
  libspectrum_byte *rom_data = NULL; 
  libspectrum_error retval = LIBSPECTRUM_ERROR_NONE;

  if( data_length < 6 ) {
    libspectrum_print_error(
      LIBSPECTRUM_ERROR_UNKNOWN,
      "read_rom_chunk: length %lu too short", (unsigned long)data_length
    );
    return LIBSPECTRUM_ERROR_UNKNOWN;
  }

  flags = libspectrum_read_word( buffer );
  expected_length = libspectrum_read_dword( buffer );

  if( flags & ZXSTRF_COMPRESSED ) {

#ifdef HAVE_ZLIB_H

    size_t uncompressed_length = 0;

    libspectrum_error error =
            libspectrum_zlib_inflate( *buffer, data_length - 6, &rom_data,
                                      &uncompressed_length );
    if( error ) return error;

    if( uncompressed_length != expected_length ) {
      libspectrum_print_error( LIBSPECTRUM_ERROR_UNKNOWN,
                               "%s:read_rom_chunk: invalid ROM length "
                               "in compressed file, should be %lu, file "
                               "has %lu",
                               __FILE__, 
                               (unsigned long)expected_length,
                               (unsigned long)uncompressed_length );
      return LIBSPECTRUM_ERROR_UNKNOWN;
    }

    *buffer += data_length - 6;

#else			/* #ifdef HAVE_ZLIB_H */

    libspectrum_print_error(
      LIBSPECTRUM_ERROR_UNKNOWN,
      "%s:read_rom_chunk: zlib needed for decompression\n",
      __FILE__
    );
    return LIBSPECTRUM_ERROR_UNKNOWN;

#endif			/* #ifdef HAVE_ZLIB_H */

  } else {

    if( data_length < 6 + expected_length ) {
      libspectrum_print_error( LIBSPECTRUM_ERROR_UNKNOWN,
                               "%s:read_rom_chunk: length %lu too short, "
                               "expected %lu" ,
                               __FILE__, (unsigned long)data_length,
                               (unsigned long)6 + expected_length );
      return LIBSPECTRUM_ERROR_UNKNOWN;
    }

    rom_data = libspectrum_new( libspectrum_byte, expected_length );
    memcpy( rom_data, *buffer, expected_length );
    *buffer += expected_length;
  }

  libspectrum_snap_set_custom_rom( snap, 1 );

  switch ( libspectrum_snap_machine( snap ) ) {
  case LIBSPECTRUM_MACHINE_16:
  case LIBSPECTRUM_MACHINE_48:
  case LIBSPECTRUM_MACHINE_TC2048:
    retval = szx_extract_roms( snap, rom_data, expected_length, 0x4000 );
    break;
  case LIBSPECTRUM_MACHINE_128:
  case LIBSPECTRUM_MACHINE_PLUS2:
  case LIBSPECTRUM_MACHINE_SE:
    retval = szx_extract_roms( snap, rom_data, expected_length, 0x8000 );
    break;
  case LIBSPECTRUM_MACHINE_PLUS2A:
  case LIBSPECTRUM_MACHINE_PLUS3:
  case LIBSPECTRUM_MACHINE_PLUS3E:
    retval = szx_extract_roms( snap, rom_data, expected_length, 0x10000 );
    break;
  case LIBSPECTRUM_MACHINE_PENT:
    /* FIXME: This is a conflict with Fuse - szx specs say Pentagon 128k snaps
       will total 32k, Fuse also has the 'gluck.rom' */
    retval = szx_extract_roms( snap, rom_data, expected_length, 0x8000 );
    break;
  case LIBSPECTRUM_MACHINE_TC2068:
  case LIBSPECTRUM_MACHINE_TS2068:
    retval = szx_extract_roms( snap, rom_data, expected_length, 0x6000 );
    break;
  case LIBSPECTRUM_MACHINE_SCORP:
  case LIBSPECTRUM_MACHINE_PENT512:
  case LIBSPECTRUM_MACHINE_PENT1024:
    retval = szx_extract_roms( snap, rom_data, expected_length, 0x10000 );
    break;
  default:
    libspectrum_print_error( LIBSPECTRUM_ERROR_UNKNOWN,
                             "%s:read_rom_chunk: don't know correct custom ROM "
                             "length for this machine",
                             __FILE__ );
    retval = LIBSPECTRUM_ERROR_UNKNOWN;
    break;
  }

  libspectrum_free( rom_data );

  return retval;
}

static libspectrum_error
read_zxpr_chunk( libspectrum_snap *snap, libspectrum_word version,
		 const libspectrum_byte **buffer,
		 const libspectrum_byte *end GCC_UNUSED, size_t data_length,
                 szx_context *ctx GCC_UNUSED )
{
  libspectrum_word flags;

  if( data_length != 2 ) {
    libspectrum_print_error( LIBSPECTRUM_ERROR_UNKNOWN,
			     "%s:read_zxpr_chunk: unknown length %lu",
			     __FILE__, (unsigned long)data_length );
    return LIBSPECTRUM_ERROR_UNKNOWN;
  }

  flags = libspectrum_read_word( buffer );
  libspectrum_snap_set_zx_printer_active( snap, flags & ZXSTPRF_ENABLED );

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
read_if2r_chunk( libspectrum_snap *snap, libspectrum_word version GCC_UNUSED,
		 const libspectrum_byte **buffer,
		 const libspectrum_byte *end GCC_UNUSED, size_t data_length,
                 szx_context *ctx GCC_UNUSED )
{

#ifdef HAVE_ZLIB_H

  libspectrum_byte *buffer2;

  size_t uncompressed_length;

  libspectrum_error error;

  if( data_length < 4 ) {
    libspectrum_print_error(
      LIBSPECTRUM_ERROR_UNKNOWN,
      "read_if2r_chunk: length %lu too short", (unsigned long)data_length
    );
    return LIBSPECTRUM_ERROR_UNKNOWN;
  }

  /* Skip the compressed length as we never actually use it - bug? */
  libspectrum_read_dword( buffer );

  uncompressed_length = 0x4000;

  error = libspectrum_zlib_inflate( *buffer, data_length - 4, &buffer2,
                                    &uncompressed_length );
  if( error ) return error;

  *buffer += data_length - 4;

  libspectrum_snap_set_interface2_active( snap, 1 );

  libspectrum_snap_set_interface2_rom( snap, 0, buffer2 );

  return LIBSPECTRUM_ERROR_NONE;

#else			/* #ifdef HAVE_ZLIB_H */

  libspectrum_print_error(
    LIBSPECTRUM_ERROR_UNKNOWN,
    "%s:read_if2r_chunk: zlib needed for decompression\n",
    __FILE__
  );
  return LIBSPECTRUM_ERROR_UNKNOWN;

#endif			/* #ifdef HAVE_ZLIB_H */

}

static libspectrum_error
read_dock_chunk( libspectrum_snap *snap, libspectrum_word version GCC_UNUSED,
		 const libspectrum_byte **buffer,
		 const libspectrum_byte *end GCC_UNUSED, size_t data_length,
                 szx_context *ctx GCC_UNUSED )
{
  libspectrum_byte *data;
  size_t page;
  libspectrum_error error;
  libspectrum_word flags;
  libspectrum_byte writeable;

  error = read_ram_page( &data, &page, buffer, data_length, 0x2000, &flags );
  if( error ) return error;

  if( page > 7 ) {
    libspectrum_print_error( LIBSPECTRUM_ERROR_CORRUPT,
			     "%s:read_dock_chunk: unknown page number %ld",
			     __FILE__, (unsigned long)page );
    libspectrum_free( data );
    return LIBSPECTRUM_ERROR_CORRUPT;
  }

  libspectrum_snap_set_dock_active( snap, 1 );

  writeable = flags & ZXSTDOCKF_RAM;

  if( flags & ZXSTDOCKF_EXROMDOCK ) {
    libspectrum_snap_set_dock_ram( snap, page, writeable );
    libspectrum_snap_set_dock_cart( snap, page, data );
  } else {
    libspectrum_snap_set_exrom_ram( snap, page, writeable );
    libspectrum_snap_set_exrom_cart( snap, page, data );
  }

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
read_divxxx_chunk( libspectrum_snap *snap, const libspectrum_byte **buffer,
		   size_t data_length,
                   void (*set_active)( libspectrum_snap*, int ),
                   void (*set_eprom_writeprotect)( libspectrum_snap*, int ),
                   libspectrum_word eprom_writeprotect_flag,
                   void (*set_paged)( libspectrum_snap*, int ),
                   libspectrum_word paged_flag,
                   void (*set_control)( libspectrum_snap*, libspectrum_byte ),
                   void (*set_page_count)( libspectrum_snap*, size_t ),
                   libspectrum_word compressed_flag,
                   void (*set_eprom)( libspectrum_snap*, int, libspectrum_byte* ) )
{
#ifdef HAVE_ZLIB_H
  libspectrum_error error;
#endif
  libspectrum_word flags;
  libspectrum_byte *eprom_data = NULL;
  const size_t expected_length = 0x2000;

  if( data_length < 4 ) {
    libspectrum_print_error( LIBSPECTRUM_ERROR_UNKNOWN,
			     "%s:read_divxxx_chunk: unknown length %lu",
			     __FILE__, (unsigned long)data_length );
    return LIBSPECTRUM_ERROR_UNKNOWN;
  }

  flags = libspectrum_read_word( buffer );
  set_active( snap, 1 );
  set_eprom_writeprotect( snap, !!(flags & eprom_writeprotect_flag ) );
  set_paged( snap, !!(flags & paged_flag) );

  set_control( snap, **buffer ); (*buffer)++;
  set_page_count( snap, **buffer ); (*buffer)++;

  if( flags & compressed_flag ) {

#ifdef HAVE_ZLIB_H

    size_t uncompressed_length = 0;

    error = libspectrum_zlib_inflate( *buffer, data_length - 4, &eprom_data,
                                      &uncompressed_length );
    if( error ) return error;

    if( uncompressed_length != expected_length ) {
      libspectrum_print_error( LIBSPECTRUM_ERROR_UNKNOWN,
                               "%s:read_divxxx_chunk: invalid EPROM length "
                               "in compressed file, should be %lu, file "
                               "has %lu",
                               __FILE__,
                               (unsigned long)expected_length,
                               (unsigned long)uncompressed_length );
      return LIBSPECTRUM_ERROR_UNKNOWN;
    }

    *buffer += data_length - 4;

#else

    libspectrum_print_error(
      LIBSPECTRUM_ERROR_UNKNOWN,
      "%s:read_divxxx_chunk: zlib needed for decompression\n",
      __FILE__
    );
    return LIBSPECTRUM_ERROR_UNKNOWN;

#endif

  } else {

    if( data_length < 4 + expected_length ) {
      libspectrum_print_error( LIBSPECTRUM_ERROR_UNKNOWN,
                               "%s:read_divxxx_chunk: length %lu too short, "
                               "expected %lu",
                               __FILE__, (unsigned long)data_length,
                               (unsigned long)( 4UL + expected_length ) );
      return LIBSPECTRUM_ERROR_UNKNOWN;
    }

    eprom_data = libspectrum_new( libspectrum_byte, expected_length );
    memcpy( eprom_data, *buffer, expected_length );

    *buffer += expected_length;

  }

  set_eprom( snap, 0, eprom_data );

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
read_dide_chunk( libspectrum_snap *snap, libspectrum_word version GCC_UNUSED,
		 const libspectrum_byte **buffer,
		 const libspectrum_byte *end GCC_UNUSED, size_t data_length,
                 szx_context *ctx GCC_UNUSED )
{
  return read_divxxx_chunk( snap, buffer, data_length,
                            libspectrum_snap_set_divide_active,
                            libspectrum_snap_set_divide_eprom_writeprotect,
                            ZXSTDIVIDE_EPROM_WRITEPROTECT,
                            libspectrum_snap_set_divide_paged,
                            ZXSTDIVIDE_PAGED,
                            libspectrum_snap_set_divide_control,
                            libspectrum_snap_set_divide_pages,
                            ZXSTDIVIDE_COMPRESSED,
                            libspectrum_snap_set_divide_eprom );
}

static libspectrum_error
read_dmmc_chunk( libspectrum_snap *snap, libspectrum_word version GCC_UNUSED,
		 const libspectrum_byte **buffer,
		 const libspectrum_byte *end GCC_UNUSED, size_t data_length,
                 szx_context *ctx GCC_UNUSED )
{
  return read_divxxx_chunk( snap, buffer, data_length,
                            libspectrum_snap_set_divmmc_active,
                            libspectrum_snap_set_divmmc_eprom_writeprotect,
                            ZXSTDIVMMC_EPROM_WRITEPROTECT,
                            libspectrum_snap_set_divmmc_paged,
                            ZXSTDIVMMC_PAGED,
                            libspectrum_snap_set_divmmc_control,
                            libspectrum_snap_set_divmmc_pages,
                            ZXSTDIVMMC_COMPRESSED,
                            libspectrum_snap_set_divmmc_eprom );
}

static libspectrum_error
read_divxxx_ram_chunk( libspectrum_snap *snap, const libspectrum_byte **buffer,
                       size_t data_length, size_t page_count,
                       void (*set_ram)( libspectrum_snap*, int, libspectrum_byte* ) )
{
  libspectrum_byte *data;
  size_t page;
  libspectrum_error error;
  libspectrum_word flags;

  error = read_ram_page( &data, &page, buffer, data_length, 0x2000, &flags );
  if( error ) return error;

  if( page >= page_count ) {
    libspectrum_print_error( LIBSPECTRUM_ERROR_CORRUPT,
			     "%s:read_divxxx_ram_chunk: unknown page number %lu",
			     __FILE__, (unsigned long)page );
    libspectrum_free( data );
    return LIBSPECTRUM_ERROR_CORRUPT;
  }

  set_ram( snap, page, data );

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
read_dirp_chunk( libspectrum_snap *snap, libspectrum_word version GCC_UNUSED,
		 const libspectrum_byte **buffer,
		 const libspectrum_byte *end GCC_UNUSED, size_t data_length,
                 szx_context *ctx GCC_UNUSED )
{
  return read_divxxx_ram_chunk( snap, buffer, data_length,
                                SNAPSHOT_DIVIDE_PAGES,
                                libspectrum_snap_set_divide_ram );
}

static libspectrum_error
read_dmrp_chunk( libspectrum_snap *snap, libspectrum_word version GCC_UNUSED,
		 const libspectrum_byte **buffer,
		 const libspectrum_byte *end GCC_UNUSED, size_t data_length,
                 szx_context *ctx GCC_UNUSED )
{
  return read_divxxx_ram_chunk( snap, buffer, data_length,
                                SNAPSHOT_DIVMMC_PAGES,
                                libspectrum_snap_set_divmmc_ram );
}

static libspectrum_error
read_snet_memory( libspectrum_snap *snap, const libspectrum_byte **buffer,
  int compressed, size_t *data_remaining,
  void (*setter)(libspectrum_snap*, int, libspectrum_byte*) )
{
  size_t data_length;
  libspectrum_byte *data_out;
  const libspectrum_byte *data;

  if( *data_remaining < 4 ) {
    libspectrum_print_error( LIBSPECTRUM_ERROR_UNKNOWN,
      "%s:read_snet_memory: not enough data for length", __FILE__ );
    return LIBSPECTRUM_ERROR_UNKNOWN;
  }

  data_length = libspectrum_read_dword( buffer );
  *data_remaining -= 4;

  if( *data_remaining < data_length ) {
    libspectrum_print_error( LIBSPECTRUM_ERROR_UNKNOWN,
      "%s:read_snet_memory: not enough data", __FILE__ );
    return LIBSPECTRUM_ERROR_UNKNOWN;
  }
  *data_remaining -= data_length;

  if( compressed ) {

#ifdef HAVE_ZLIB_H
    libspectrum_error error;
    size_t uncompressed_length = 0;
    libspectrum_byte *uncompressed_data;

    error = libspectrum_zlib_inflate( *buffer, data_length, &uncompressed_data,
        &uncompressed_length );
    if( error ) return error;

    *buffer += data_length;

    if( uncompressed_length != 0x20000 ) {
      libspectrum_print_error( LIBSPECTRUM_ERROR_UNKNOWN,
          "%s:read_snet_memory: data decompressed to %lu but should be 0x20000",
          __FILE__, (unsigned long)uncompressed_length );
      libspectrum_free( uncompressed_data );
      return LIBSPECTRUM_ERROR_UNKNOWN;
    }

    data = uncompressed_data;

#else

    libspectrum_print_error( LIBSPECTRUM_ERROR_UNKNOWN,
        "%s:read_snet_memory: zlib needed for decompression\n", __FILE__ );
    return LIBSPECTRUM_ERROR_UNKNOWN;

#endif

  } else {
    if( data_length != 0x20000 ) {
      libspectrum_print_error( LIBSPECTRUM_ERROR_UNKNOWN,
          "%s:read_snet_memory: data has length %lu but should be 0x20000",
          __FILE__, (unsigned long)data_length );
      return LIBSPECTRUM_ERROR_NONE;
    }

    data = *buffer;
    *buffer += data_length;
  }

  data_out = libspectrum_new( libspectrum_byte, 0x20000 );
  memcpy( data_out, data, 0x20000 );
  setter( snap, 0, data_out );

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
read_snet_chunk( libspectrum_snap *snap, libspectrum_word version GCC_UNUSED,
		 const libspectrum_byte **buffer,
		 const libspectrum_byte *end GCC_UNUSED, size_t data_length,
                 szx_context *ctx GCC_UNUSED )
{
  libspectrum_word flags;
  libspectrum_byte *w5100;

  if( data_length < 54 ) {
    libspectrum_print_error(
      LIBSPECTRUM_ERROR_UNKNOWN,
      "read_snet_chunk: length %lu too short", (unsigned long)data_length
    );
    return LIBSPECTRUM_ERROR_UNKNOWN;
  }

  libspectrum_snap_set_spectranet_active( snap, 1 );

  flags = libspectrum_read_word( buffer );
  libspectrum_snap_set_spectranet_paged( snap, !!( flags & ZXSTSNET_PAGED ) );
  libspectrum_snap_set_spectranet_paged_via_io( snap,
    !!( flags & ZXSTSNET_PAGED_VIA_IO ) );
  libspectrum_snap_set_spectranet_programmable_trap_active( snap,
    !!( flags & ZXSTSNET_PROGRAMMABLE_TRAP_ACTIVE ) );
  libspectrum_snap_set_spectranet_programmable_trap_msb( snap,
    !!( flags & ZXSTSNET_PROGRAMMABLE_TRAP_MSB ) );
  libspectrum_snap_set_spectranet_all_traps_disabled( snap,
    !!( flags & ZXSTSNET_ALL_DISABLED ) );
  libspectrum_snap_set_spectranet_rst8_trap_disabled( snap,
    !!( flags & ZXSTSNET_RST8_DISABLED ) );
  libspectrum_snap_set_spectranet_deny_downstream_a15( snap,
    !!( flags & ZXSTSNET_DENY_DOWNSTREAM_A15 ) );
  libspectrum_snap_set_spectranet_nmi_flipflop( snap,
    !!( flags & ZXSTSNET_NMI_FLIPFLOP ) );

  libspectrum_snap_set_spectranet_page_a( snap, **buffer ); (*buffer)++;
  libspectrum_snap_set_spectranet_page_b( snap, **buffer ); (*buffer)++;

  libspectrum_snap_set_spectranet_programmable_trap( snap,
    libspectrum_read_word( buffer ) );

  w5100 = libspectrum_new( libspectrum_byte, 0x30 );
  libspectrum_snap_set_spectranet_w5100( snap, 0, w5100 );
  memcpy( w5100, *buffer, 0x30 );
  (*buffer) += 0x30;

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
read_snef_chunk( libspectrum_snap *snap, libspectrum_word version GCC_UNUSED,
		 const libspectrum_byte **buffer,
		 const libspectrum_byte *end GCC_UNUSED, size_t data_length,
                 szx_context *ctx GCC_UNUSED )
{
  libspectrum_byte flags;
  int flash_compressed;
  libspectrum_error error;
  size_t data_remaining;

  if( data_length < 5 ) {
    libspectrum_print_error(
      LIBSPECTRUM_ERROR_UNKNOWN,
      "read_snef_chunk: length %lu too short", (unsigned long)data_length
    );
    return LIBSPECTRUM_ERROR_UNKNOWN;
  }

  flags = **buffer; (*buffer)++;
  flash_compressed = flags & ZXSTSNEF_FLASH_COMPRESSED;

  data_remaining = data_length - 1;

  error = read_snet_memory( snap, buffer, flash_compressed, &data_remaining,
    libspectrum_snap_set_spectranet_flash );
  if( error )
    return error;

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
read_sner_chunk( libspectrum_snap *snap, libspectrum_word version GCC_UNUSED,
		 const libspectrum_byte **buffer,
		 const libspectrum_byte *end GCC_UNUSED, size_t data_length,
                 szx_context *ctx GCC_UNUSED )
{
  libspectrum_byte flags;
  int ram_compressed;
  libspectrum_error error;
  size_t data_remaining;

  if( data_length < 5 ) {
    libspectrum_print_error(
      LIBSPECTRUM_ERROR_UNKNOWN,
      "read_sner_chunk: length %lu too short", (unsigned long)data_length
    );
    return LIBSPECTRUM_ERROR_UNKNOWN;
  }

  flags = **buffer; (*buffer)++;
  ram_compressed = flags & ZXSTSNER_RAM_COMPRESSED;

  data_remaining = data_length - 1;

  error = read_snet_memory( snap, buffer, ram_compressed, &data_remaining,
    libspectrum_snap_set_spectranet_ram );
  if( error )
    return error;

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
read_pltt_chunk( libspectrum_snap *snap, libspectrum_word version GCC_UNUSED,
                 const libspectrum_byte **buffer,
                 const libspectrum_byte *end GCC_UNUSED, size_t data_length,
                 szx_context *ctx GCC_UNUSED )
{
  libspectrum_byte flags;
  libspectrum_byte *palette;

  if( data_length < 66 ) {
    libspectrum_print_error(
      LIBSPECTRUM_ERROR_UNKNOWN,
      "read_pltt_chunk: length %lu too short", (unsigned long)data_length
    );
    return LIBSPECTRUM_ERROR_UNKNOWN;
  }

  flags = **buffer; (*buffer)++;

  libspectrum_snap_set_ulaplus_active( snap, 1 );
  libspectrum_snap_set_ulaplus_palette_enabled( snap, flags & ZXSTPALETTE_ENABLED );
  libspectrum_snap_set_ulaplus_current_register( snap, **buffer ); (*buffer)++;

  palette = libspectrum_new( libspectrum_byte, 64 );
  libspectrum_snap_set_ulaplus_palette( snap, 0, palette );
  memcpy( palette, *buffer, 64 );
  (*buffer) += 64;

  /* Specifications previous to v1.1a didn't have this register */
  if( data_length > 66 ) {
    libspectrum_snap_set_ulaplus_ff_register( snap, **buffer );
    (*buffer)++;
  }

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
read_mfce_chunk( libspectrum_snap *snap, libspectrum_word version GCC_UNUSED,
                 const libspectrum_byte **buffer,
                 const libspectrum_byte *end GCC_UNUSED, size_t data_length,
                 szx_context *ctx GCC_UNUSED )
{
#ifdef HAVE_ZLIB_H
  libspectrum_error error;
#endif
  libspectrum_byte *ram_data = NULL;
  libspectrum_byte multiface_model;
  libspectrum_byte flags;
  int capabilities;
  size_t expected_ram_length, disc_ram_length;

  if( data_length < 2 ) {
    libspectrum_print_error(
      LIBSPECTRUM_ERROR_UNKNOWN,
      "read_mfce_chunk: length %lu too short", (unsigned long)data_length
    );
    return LIBSPECTRUM_ERROR_UNKNOWN;
  }

  libspectrum_snap_set_multiface_active( snap, 1 );

  multiface_model = **buffer; (*buffer)++;

  if( multiface_model == ZXSTMFM_1 )
    libspectrum_snap_set_multiface_model_one( snap, 1 );
  else if ( multiface_model == ZXSTMFM_128 ) {
    capabilities =
      libspectrum_machine_capabilities( libspectrum_snap_machine( snap ) );

    if( capabilities & LIBSPECTRUM_MACHINE_CAPABILITY_PLUS3_MEMORY )
      libspectrum_snap_set_multiface_model_3( snap, 1 );
    else
      libspectrum_snap_set_multiface_model_128( snap, 1 );
  }

  flags = **buffer; (*buffer)++;
  libspectrum_snap_set_multiface_paged( snap, !!( flags & ZXSTMF_PAGEDIN ) );
  libspectrum_snap_set_multiface_software_lockout( snap,
    !!( flags & ZXSTMF_SOFTWARELOCKOUT ) );
  libspectrum_snap_set_multiface_red_button_disabled( snap,
    !!( flags & ZXSTMF_REDBUTTONDISABLED ) );
  libspectrum_snap_set_multiface_disabled( snap,
    !!( flags & ZXSTMF_DISABLED ) );

  expected_ram_length = flags & ZXSTMF_16KRAMMODE ? 0x4000 : 0x2000;
  disc_ram_length = data_length - 2;

  if( flags & ZXSTMF_COMPRESSED ) {

#ifdef HAVE_ZLIB_H

    size_t uncompressed_length = 0;

    error = libspectrum_zlib_inflate( *buffer, disc_ram_length, &ram_data,
                                      &uncompressed_length );
    if( error ) return error;

    if( uncompressed_length != expected_ram_length ) {
      libspectrum_print_error( LIBSPECTRUM_ERROR_UNKNOWN,
                               "%s:read_mfce_chunk: invalid RAM length "
                               "in compressed file, should be %lu, file "
                               "has %lu",
                               __FILE__,
                               (unsigned long)expected_ram_length,
                               (unsigned long)uncompressed_length );
      return LIBSPECTRUM_ERROR_UNKNOWN;
    }

    *buffer += disc_ram_length;

#else			/* #ifdef HAVE_ZLIB_H */

    libspectrum_print_error(
      LIBSPECTRUM_ERROR_UNKNOWN,
      "%s:read_mfce_chunk: zlib needed for decompression\n",
      __FILE__
    );
    return LIBSPECTRUM_ERROR_UNKNOWN;

#endif			/* #ifdef HAVE_ZLIB_H */

  } else {

    if( disc_ram_length != expected_ram_length ) {
      libspectrum_print_error( LIBSPECTRUM_ERROR_UNKNOWN,
                               "%s:read_mfce_chunk: invalid RAM length "
                               "in uncompressed file, should be %lu, file "
                               "has %lu",
                               __FILE__,
                               (unsigned long)expected_ram_length,
                               (unsigned long)disc_ram_length );
      return LIBSPECTRUM_ERROR_UNKNOWN;
    }

    ram_data = libspectrum_new( libspectrum_byte, expected_ram_length );
    memcpy( ram_data, *buffer, expected_ram_length );
    *buffer += expected_ram_length;
  }

  libspectrum_snap_set_multiface_ram( snap, 0, ram_data );
  libspectrum_snap_set_multiface_ram_length( snap, 0, expected_ram_length );

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
read_zmmc_chunk( libspectrum_snap *snap, libspectrum_word version GCC_UNUSED,
                 const libspectrum_byte **buffer,
                 const libspectrum_byte *end GCC_UNUSED, size_t data_length,
                 szx_context *ctx GCC_UNUSED )
{
  if( data_length ) {
    libspectrum_print_error( LIBSPECTRUM_ERROR_UNKNOWN,
                             "%s:read_zmmc_chunk: unknown length %lu",
                             __FILE__, (unsigned long)data_length );
    return LIBSPECTRUM_ERROR_UNKNOWN;
  }

  libspectrum_snap_set_zxmmc_active( snap, 1 );

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
skip_chunk( libspectrum_snap *snap GCC_UNUSED,
	    libspectrum_word version GCC_UNUSED,
	    const libspectrum_byte **buffer,
	    const libspectrum_byte *end GCC_UNUSED,
	    size_t data_length,
            szx_context *ctx GCC_UNUSED )
{
  *buffer += data_length;
  return LIBSPECTRUM_ERROR_NONE;
}

struct read_chunk_t {

  const char *id;
  read_chunk_fn function;

};

static struct read_chunk_t read_chunks[] = {

  { ZXSTBID_AY,		         read_ay_chunk   },
  { ZXSTBID_BETA128,	         read_b128_chunk },
  { ZXSTBID_BETADISK,	         skip_chunk      },
  { ZXSTBID_COVOX,	         read_covx_chunk },
  { ZXSTBID_CREATOR,	         read_crtr_chunk },
  { ZXSTBID_DIVIDE,	         read_dide_chunk },
  { ZXSTBID_DIVIDERAMPAGE,       read_dirp_chunk },
  { ZXSTBID_DIVMMC,	         read_dmmc_chunk },
  { ZXSTBID_DIVMMCRAMPAGE,       read_dmrp_chunk },
  { ZXSTBID_DOCK,	         read_dock_chunk },
  { ZXSTBID_DSKFILE,	         skip_chunk      },
  { ZXSTBID_LEC,                 skip_chunk      },
  { ZXSTBID_LECRAMPAGE,          skip_chunk      },
  { ZXSTBID_GS,		         skip_chunk      },
  { ZXSTBID_GSRAMPAGE,	         skip_chunk      },
  { ZXSTBID_IF1,	         read_if1_chunk  },
  { ZXSTBID_IF2ROM,	         read_if2r_chunk },
  { ZXSTBID_JOYSTICK,	         read_joy_chunk  },
  { ZXSTBID_KEYBOARD,	         read_keyb_chunk },
  { ZXSTBID_MICRODRIVE,	         skip_chunk      },
  { ZXSTBID_MOUSE,	         read_amxm_chunk },
  { ZXSTBID_MULTIFACE,	         read_mfce_chunk },
  { ZXSTBID_OPUS,	         read_opus_chunk },
  { ZXSTBID_OPUSDISK,	         skip_chunk      },
  { ZXSTBID_PALETTE,	         read_pltt_chunk },
  { ZXSTBID_PLUS3DISK,	         skip_chunk      },
  { ZXSTBID_PLUSD,	         read_plsd_chunk },
  { ZXSTBID_PLUSDDISK,	         skip_chunk      },
  { ZXSTBID_RAMPAGE,	         read_ramp_chunk },
  { ZXSTBID_ROM,	         read_rom_chunk  },
  { ZXSTBID_SIMPLEIDE,	         read_side_chunk },
  { ZXSTBID_SPECDRUM,	         read_drum_chunk },
  { ZXSTBID_SPECREGS,	         read_spcr_chunk },
  { ZXSTBID_SPECTRANET,          read_snet_chunk },
  { ZXSTBID_SPECTRANETFLASHPAGE, read_snef_chunk },
  { ZXSTBID_SPECTRANETRAMPAGE,   read_sner_chunk },
  { ZXSTBID_TIMEXREGS,	         read_scld_chunk },
  { ZXSTBID_USPEECH,	         skip_chunk      },
  { ZXSTBID_Z80REGS,	         read_z80r_chunk },
  { ZXSTBID_ZXATASPRAMPAGE,      read_atrp_chunk },
  { ZXSTBID_ZXATASP,	         read_zxat_chunk },
  { ZXSTBID_ZXCF,	         read_zxcf_chunk },
  { ZXSTBID_ZXCFRAMPAGE,         read_cfrp_chunk },
  { ZXSTBID_ZXMMC,	         read_zmmc_chunk },
  { ZXSTBID_ZXPRINTER,	         read_zxpr_chunk },
  { ZXSTBID_ZXTAPE,	         skip_chunk      },

};

static libspectrum_error
read_chunk_header( char *id, libspectrum_dword *data_length, 
		   const libspectrum_byte **buffer,
		   const libspectrum_byte *end )
{
  if( end - *buffer < 8 ) {
    libspectrum_print_error(
      LIBSPECTRUM_ERROR_CORRUPT,
      "szx_read_chunk_header: not enough data for chunk header"
    );
    return LIBSPECTRUM_ERROR_CORRUPT;
  }

  memcpy( id, *buffer, 4 ); id[4] = '\0'; *buffer += 4;
  *data_length = libspectrum_read_dword( buffer );

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
read_chunk( libspectrum_snap *snap, libspectrum_word version,
	    const libspectrum_byte **buffer, const libspectrum_byte *end,
            szx_context *ctx )
{
  char id[5];
  libspectrum_dword data_length;
  libspectrum_error error;
  size_t i; int done;

  error = read_chunk_header( id, &data_length, buffer, end );
  if( error ) return error;

  if( end - *buffer < data_length ) {
    libspectrum_print_error(
      LIBSPECTRUM_ERROR_CORRUPT,
      "szx_read_chunk: chunk length goes beyond end of file"
    );
    return LIBSPECTRUM_ERROR_CORRUPT;
  }

  done = 0;

  for( i = 0; !done && i < ARRAY_SIZE( read_chunks ); i++ ) {

    if( !memcmp( id, read_chunks[i].id, 4 ) ) {
      error = read_chunks[i].function( snap, version, buffer, end,
				       data_length, ctx );
      if( error ) return error;
      done = 1;
    }

  }

  if( !done ) {
    libspectrum_print_error( LIBSPECTRUM_ERROR_UNKNOWN,
			     "szx_read_chunk: unknown chunk id '%s'", id );
    *buffer += data_length;
  }

  return LIBSPECTRUM_ERROR_NONE;
}

libspectrum_error
libspectrum_szx_read( libspectrum_snap *snap, const libspectrum_byte *buffer,
		      size_t length )
{
  libspectrum_word version;
  libspectrum_byte machine;
  libspectrum_byte flags;

  libspectrum_error error;
  const libspectrum_byte *end = buffer + length;
  szx_context *ctx;

  if( end - buffer < 8 ) {
    libspectrum_print_error(
      LIBSPECTRUM_ERROR_CORRUPT,
      "libspectrum_szx_read: not enough data for SZX header"
    );
    return LIBSPECTRUM_ERROR_CORRUPT;
  }

  if( memcmp( buffer, signature, signature_length ) ) {
    libspectrum_print_error(
      LIBSPECTRUM_ERROR_SIGNATURE,
      "libspectrum_szx_read: wrong signature"
    );
    return LIBSPECTRUM_ERROR_SIGNATURE;
  }
  buffer += signature_length;

  version = (*buffer++) << 8; version |= *buffer++;

  machine = *buffer++;

  switch( machine ) {

  case SZX_MACHINE_16:
    libspectrum_snap_set_machine( snap, LIBSPECTRUM_MACHINE_16 );
    break;

  case SZX_MACHINE_48:
    libspectrum_snap_set_machine( snap, LIBSPECTRUM_MACHINE_48 );
    break;

  case SZX_MACHINE_48_NTSC:
    libspectrum_snap_set_machine( snap, LIBSPECTRUM_MACHINE_48_NTSC );
    break;

  case SZX_MACHINE_128:
    libspectrum_snap_set_machine( snap, LIBSPECTRUM_MACHINE_128 );
    break;

  case SZX_MACHINE_PLUS2:
    libspectrum_snap_set_machine( snap, LIBSPECTRUM_MACHINE_PLUS2 );
    break;

  case SZX_MACHINE_PLUS2A:
    libspectrum_snap_set_machine( snap, LIBSPECTRUM_MACHINE_PLUS2A );
    break;

  case SZX_MACHINE_PLUS3:
    libspectrum_snap_set_machine( snap, LIBSPECTRUM_MACHINE_PLUS3 );
    break;

  case SZX_MACHINE_PLUS3E:
    libspectrum_snap_set_machine( snap, LIBSPECTRUM_MACHINE_PLUS3E );
    break;

  case SZX_MACHINE_PENTAGON:
    libspectrum_snap_set_machine( snap, LIBSPECTRUM_MACHINE_PENT );
    break;

  case SZX_MACHINE_TC2048:
    libspectrum_snap_set_machine( snap, LIBSPECTRUM_MACHINE_TC2048 );
    break;

  case SZX_MACHINE_TC2068:
    libspectrum_snap_set_machine( snap, LIBSPECTRUM_MACHINE_TC2068 );
    break;

  case SZX_MACHINE_TS2068:
    libspectrum_snap_set_machine( snap, LIBSPECTRUM_MACHINE_TS2068 );
    break;

  case SZX_MACHINE_SCORPION:
    libspectrum_snap_set_machine( snap, LIBSPECTRUM_MACHINE_SCORP );
    break;

  case SZX_MACHINE_SE:
    libspectrum_snap_set_machine( snap, LIBSPECTRUM_MACHINE_SE );
    break;

  case SZX_MACHINE_PENTAGON512:
    libspectrum_snap_set_machine( snap, LIBSPECTRUM_MACHINE_PENT512 );
    break;

  case SZX_MACHINE_PENTAGON1024:
    libspectrum_snap_set_machine( snap, LIBSPECTRUM_MACHINE_PENT1024 );
    break;

  case SZX_MACHINE_128KE:
    libspectrum_snap_set_machine( snap, LIBSPECTRUM_MACHINE_128E );
    break;

  default:
    libspectrum_print_error(
      LIBSPECTRUM_ERROR_UNKNOWN,
      "libspectrum_szx_read: unknown machine type %d", (int)*buffer
    );
    return LIBSPECTRUM_ERROR_UNKNOWN;
  }

  flags = *buffer++;

  switch( machine ) {

  case SZX_MACHINE_16:
  case SZX_MACHINE_48:
  case SZX_MACHINE_48_NTSC:
  case SZX_MACHINE_128:
    libspectrum_snap_set_late_timings( snap, flags & ZXSTMF_ALTERNATETIMINGS );
    break;

  default:
    break;
  }

  ctx = libspectrum_new( szx_context, 1 );
  ctx->swap_af = 0;

  while( buffer < end ) {
    error = read_chunk( snap, version, &buffer, end, ctx );
    if( error ) {
      libspectrum_free( ctx );
      return error;
    }
  }

  libspectrum_free( ctx );
  return LIBSPECTRUM_ERROR_NONE;
}

libspectrum_error
libspectrum_szx_write( libspectrum_buffer *buffer, int *out_flags,
                       libspectrum_snap *snap, libspectrum_creator *creator,
                       int in_flags )
{
  int capabilities, compress;
  libspectrum_error error;
  size_t i;
  libspectrum_buffer *block_data;

  *out_flags = 0;

  /* We don't save the uSource state at all */
  if( libspectrum_snap_usource_active( snap ) )
    *out_flags |= LIBSPECTRUM_FLAG_SNAPSHOT_MAJOR_INFO_LOSS;

  /* We don't save the DISCiPLE state at all */
  if( libspectrum_snap_disciple_active( snap ) )
    *out_flags |= LIBSPECTRUM_FLAG_SNAPSHOT_MAJOR_INFO_LOSS;

  /* We don't save the Didaktik80 state at all */
  if( libspectrum_snap_didaktik80_active( snap ) )
    *out_flags |= LIBSPECTRUM_FLAG_SNAPSHOT_MAJOR_INFO_LOSS;

  capabilities =
    libspectrum_machine_capabilities( libspectrum_snap_machine( snap ) );

  compress = !( in_flags & LIBSPECTRUM_FLAG_SNAPSHOT_NO_COMPRESSION );

  error = write_file_header( buffer, out_flags, snap );
  if( error ) return error;

  block_data = libspectrum_buffer_alloc();

  if( creator ) {
    write_crtr_chunk( buffer, block_data, creator );
  }

  write_z80r_chunk( buffer, block_data, snap );
  write_spcr_chunk( buffer, block_data, snap );
  write_joy_chunk( buffer, block_data, out_flags, snap );
  write_keyb_chunk( buffer, block_data, out_flags, snap );

  if( libspectrum_snap_custom_rom( snap ) ) {
    error = write_rom_chunk( buffer, block_data, out_flags, snap, compress );
    if( error ) {
      libspectrum_buffer_free( block_data );
      return error;
    }
  }

  write_ram_pages( buffer, block_data, snap, compress );

  if( libspectrum_snap_fuller_box_active( snap ) ||
      libspectrum_snap_melodik_active( snap ) ||
      capabilities & LIBSPECTRUM_MACHINE_CAPABILITY_AY ) {
    write_ay_chunk( buffer, block_data, snap );
  }

  if( capabilities & ( LIBSPECTRUM_MACHINE_CAPABILITY_TIMEX_MEMORY |
                       LIBSPECTRUM_MACHINE_CAPABILITY_SE_MEMORY ) ) {
    write_scld_chunk( buffer, block_data, snap );
  }

  if( libspectrum_snap_beta_active( snap ) ) {
    write_b128_chunk( buffer, block_data, snap, compress );
  }

  if( libspectrum_snap_zxatasp_active( snap ) ) {
    write_zxat_chunk( buffer, block_data, snap );

    for( i = 0; i < libspectrum_snap_zxatasp_pages( snap ); i++ ) {
      write_atrp_chunk( buffer, block_data, snap, i, compress );
    }
  }

  if( libspectrum_snap_zxcf_active( snap ) ) {
    write_zxcf_chunk( buffer, block_data, snap );

    for( i = 0; i < libspectrum_snap_zxcf_pages( snap ); i++ ) {
      write_cfrp_chunk( buffer, block_data, snap, i, compress );
    }
  }

  if( libspectrum_snap_interface2_active( snap ) ) {
#ifdef HAVE_ZLIB_H
    error = write_if2r_chunk( buffer, block_data, snap );
    if( error ) {
      libspectrum_buffer_free( block_data );
      return error;
    }
#else
    /* IF2R blocks only support writing compressed images */
    *out_flags |= LIBSPECTRUM_FLAG_SNAPSHOT_MAJOR_INFO_LOSS;
#endif                         /* #ifdef HAVE_ZLIB_H */
  }

  if( libspectrum_snap_dock_active( snap ) ) {
    for( i = 0; i < 8; i++ ) {
      if( libspectrum_snap_exrom_cart( snap, i ) ) {
        write_dock_chunk( buffer, block_data, snap, 0,
                          libspectrum_snap_exrom_cart( snap, i ), i,
                          libspectrum_snap_exrom_ram( snap, i ),
                          compress );
      }
      if( libspectrum_snap_dock_cart( snap, i ) ) {
        write_dock_chunk( buffer, block_data, snap, 1,
                          libspectrum_snap_dock_cart( snap, i ), i,
                          libspectrum_snap_dock_ram( snap, i ),
                          compress );
      }
    }
  }

  if( libspectrum_snap_interface1_active( snap ) ) {
    error = write_if1_chunk( buffer, block_data, snap, compress );
    if( error ) {
      libspectrum_buffer_free( block_data );
      return error;
    }
  }

  if( libspectrum_snap_opus_active( snap ) ) {
    error = write_opus_chunk( buffer, block_data, snap, compress );
    if( error ) {
      libspectrum_buffer_free( block_data );
      return error;
    }
  }

  if( libspectrum_snap_plusd_active( snap ) ) {
    error = write_plsd_chunk( buffer, block_data, snap, compress );
    if( error ) {
      libspectrum_buffer_free( block_data );
      return error;
    }
  }

  if( libspectrum_snap_kempston_mouse_active( snap ) ) {
    write_amxm_chunk( buffer, block_data, snap );
  }

  if( libspectrum_snap_simpleide_active( snap ) ) {
    write_side_chunk( buffer, block_data, snap );
  }

  if( libspectrum_snap_specdrum_active( snap ) ) {
    write_drum_chunk( buffer, block_data, snap );
  }

  if( libspectrum_snap_divide_active( snap ) ) {
    error = write_dide_chunk( buffer, block_data, snap, compress );
    if( error ) {
      libspectrum_buffer_free( block_data );
      return error;
    }

    for( i = 0; i < libspectrum_snap_divide_pages( snap ); i++ ) {
      write_dirp_chunk( buffer, block_data, snap, i, compress );
    }
  }

  if( libspectrum_snap_divmmc_active( snap ) ) {
    error = write_dmmc_chunk( buffer, block_data, snap, compress );
    if( error ) {
      libspectrum_buffer_free( block_data );
      return error;
    }

    for( i = 0; i < libspectrum_snap_divmmc_pages( snap ); i++ ) {
      write_dmrp_chunk( buffer, block_data, snap, i, compress );
    }
  }

  if( libspectrum_snap_spectranet_active( snap ) ) {
    write_snet_chunk( buffer, block_data, snap, compress );
    write_snef_chunk( buffer, block_data, snap, compress );
    write_sner_chunk( buffer, block_data, snap, compress );
  }

  write_zxpr_chunk( buffer, block_data, out_flags, snap );

  if( libspectrum_snap_covox_active( snap ) ) {
    write_covx_chunk( buffer, block_data, snap );
  }

  if( libspectrum_snap_multiface_active( snap ) ) {
    error = write_mfce_chunk( buffer, block_data, snap, compress );
    if( error ) return error;
  }
    
  if( libspectrum_snap_ulaplus_active( snap ) ) {
    error = write_pltt_chunk( buffer, block_data, snap, compress );
    if( error ) return error;
  }

  if( libspectrum_snap_zxmmc_active( snap ) ) {
    write_zmmc_chunk( buffer, block_data, snap );
  }

  libspectrum_buffer_free( block_data );

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
write_file_header( libspectrum_buffer *buffer, int *out_flags,
                   libspectrum_snap *snap )
{
  libspectrum_byte flags;

  libspectrum_buffer_write( buffer, signature, 4 );
  
  libspectrum_buffer_write_byte( buffer, SZX_VERSION_MAJOR );
  libspectrum_buffer_write_byte( buffer, SZX_VERSION_MINOR );

  switch( libspectrum_snap_machine( snap ) ) {

  case LIBSPECTRUM_MACHINE_16:     flags = SZX_MACHINE_16; break;
  case LIBSPECTRUM_MACHINE_48:     flags = SZX_MACHINE_48; break;
  case LIBSPECTRUM_MACHINE_48_NTSC: flags = SZX_MACHINE_48_NTSC; break;
  case LIBSPECTRUM_MACHINE_128:    flags = SZX_MACHINE_128; break;
  case LIBSPECTRUM_MACHINE_128E:    flags = SZX_MACHINE_128KE; break;
  case LIBSPECTRUM_MACHINE_PLUS2:  flags = SZX_MACHINE_PLUS2; break;
  case LIBSPECTRUM_MACHINE_PLUS2A: flags = SZX_MACHINE_PLUS2A; break;
  case LIBSPECTRUM_MACHINE_PLUS3:  flags = SZX_MACHINE_PLUS3; break;
  case LIBSPECTRUM_MACHINE_PLUS3E: flags = SZX_MACHINE_PLUS3E; break;
  case LIBSPECTRUM_MACHINE_PENT:   flags = SZX_MACHINE_PENTAGON; break;
  case LIBSPECTRUM_MACHINE_TC2048: flags = SZX_MACHINE_TC2048; break;
  case LIBSPECTRUM_MACHINE_TC2068: flags = SZX_MACHINE_TC2068; break;
  case LIBSPECTRUM_MACHINE_TS2068: flags = SZX_MACHINE_TS2068; break;
  case LIBSPECTRUM_MACHINE_SCORP:  flags = SZX_MACHINE_SCORPION; break;
  case LIBSPECTRUM_MACHINE_SE:     flags = SZX_MACHINE_SE; break;
  case LIBSPECTRUM_MACHINE_PENT512: flags = SZX_MACHINE_PENTAGON512; break;
  case LIBSPECTRUM_MACHINE_PENT1024: flags = SZX_MACHINE_PENTAGON1024; break;

  default:
    libspectrum_print_error( LIBSPECTRUM_ERROR_LOGIC,
			     "Emulated machine type is set to 'unknown'!" );
    return LIBSPECTRUM_ERROR_LOGIC;
  }
  libspectrum_buffer_write_byte( buffer, flags );

  /* Flags byte */
  flags = 0;
  if( libspectrum_snap_late_timings( snap ) ) flags |= ZXSTMF_ALTERNATETIMINGS;
  libspectrum_buffer_write_byte( buffer, flags );

  return LIBSPECTRUM_ERROR_NONE;
}

static void
write_crtr_chunk( libspectrum_buffer *buffer, libspectrum_buffer *crtr_data,
                  libspectrum_creator *creator )
{
  size_t custom_length;

  libspectrum_buffer_write( crtr_data, libspectrum_creator_program( creator ),
                            32 );
  libspectrum_buffer_write_word( crtr_data, libspectrum_creator_major( creator ) );
  libspectrum_buffer_write_word( crtr_data, libspectrum_creator_minor( creator ) );

  custom_length = libspectrum_creator_custom_length( creator );
  if( custom_length ) {
    libspectrum_buffer_write( crtr_data, libspectrum_creator_custom( creator ),
                              custom_length );
  }

  write_chunk( buffer, ZXSTBID_CREATOR, crtr_data );
}

static void
write_z80r_chunk( libspectrum_buffer *buffer, libspectrum_buffer *data,
                  libspectrum_snap *snap )
{
  libspectrum_dword tstates;
  libspectrum_byte flags, tstates_remaining;

  libspectrum_buffer_write_byte( data, libspectrum_snap_f   ( snap ) );
  libspectrum_buffer_write_byte( data, libspectrum_snap_a   ( snap ) );
  libspectrum_buffer_write_word( data, libspectrum_snap_bc  ( snap ) );
  libspectrum_buffer_write_word( data, libspectrum_snap_de  ( snap ) );
  libspectrum_buffer_write_word( data, libspectrum_snap_hl  ( snap ) );

  libspectrum_buffer_write_byte( data, libspectrum_snap_f_  ( snap ) );
  libspectrum_buffer_write_byte( data, libspectrum_snap_a_  ( snap ) );
  libspectrum_buffer_write_word( data, libspectrum_snap_bc_ ( snap ) );
  libspectrum_buffer_write_word( data, libspectrum_snap_de_ ( snap ) );
  libspectrum_buffer_write_word( data, libspectrum_snap_hl_ ( snap ) );

  libspectrum_buffer_write_word( data, libspectrum_snap_ix  ( snap ) );
  libspectrum_buffer_write_word( data, libspectrum_snap_iy  ( snap ) );
  libspectrum_buffer_write_word( data, libspectrum_snap_sp  ( snap ) );
  libspectrum_buffer_write_word( data, libspectrum_snap_pc  ( snap ) );

  libspectrum_buffer_write_byte( data, libspectrum_snap_i   ( snap ) );
  libspectrum_buffer_write_byte( data, libspectrum_snap_r   ( snap ) );
  libspectrum_buffer_write_byte( data, libspectrum_snap_iff1( snap ) );
  libspectrum_buffer_write_byte( data, libspectrum_snap_iff2( snap ) );
  libspectrum_buffer_write_byte( data, libspectrum_snap_im  ( snap ) );

  tstates = libspectrum_snap_tstates( snap );

  libspectrum_buffer_write_dword( data, tstates );

  /* Number of tstates remaining in which an interrupt can occur */
  if( tstates < 48 ) {
    tstates_remaining = (unsigned char)(48 - tstates);
  } else {
    tstates_remaining = '\0';
  }
  libspectrum_buffer_write_byte( data, tstates_remaining );

  flags = '\0';
  if( libspectrum_snap_last_instruction_ei( snap ) ) flags |= ZXSTZF_EILAST;
  if( libspectrum_snap_halted( snap ) ) flags |= ZXSTZF_HALTED;
  if( libspectrum_snap_last_instruction_set_f( snap ) ) flags |= ZXSTZF_FSET;
  libspectrum_buffer_write_byte( data, flags );

  libspectrum_buffer_write_word( data, libspectrum_snap_memptr( snap ) );

  write_chunk( buffer, ZXSTBID_Z80REGS, data );
}

static void
write_spcr_chunk( libspectrum_buffer *buffer, libspectrum_buffer *data,
                  libspectrum_snap *snap )
{
  int capabilities;

  capabilities =
    libspectrum_machine_capabilities( libspectrum_snap_machine( snap ) );

  /* Border colour */
  libspectrum_buffer_write_byte( data,
                                 libspectrum_snap_out_ula( snap ) & 0x07 );

  if( capabilities & LIBSPECTRUM_MACHINE_CAPABILITY_128_MEMORY ) {
    libspectrum_buffer_write_byte( data,
                                 libspectrum_snap_out_128_memoryport( snap ) );
  } else {
    libspectrum_buffer_write_byte( data, '\0' );
  }
  
  if( capabilities & LIBSPECTRUM_MACHINE_CAPABILITY_PLUS3_MEMORY    || 
      capabilities & LIBSPECTRUM_MACHINE_CAPABILITY_SCORP_MEMORY    ||
      capabilities & LIBSPECTRUM_MACHINE_CAPABILITY_PENT1024_MEMORY    ) {
    libspectrum_buffer_write_byte( data,
                               libspectrum_snap_out_plus3_memoryport( snap ) );
  } else {
    libspectrum_buffer_write_byte( data, '\0' );
  }

  libspectrum_buffer_write_byte( data, libspectrum_snap_out_ula( snap ) );

  /* Reserved bytes */
  libspectrum_buffer_write_dword( data, 0 );

  write_chunk( buffer, ZXSTBID_SPECREGS, data );
}

static void
write_joystick( libspectrum_buffer *buffer, int *out_flags,
                libspectrum_snap *snap, const int connection )
{
  size_t num_joysticks = libspectrum_snap_joystick_active_count( snap );
  int found = 0;
  int i;
  libspectrum_byte type = ZXJT_NONE;

  for( i = 0; i < num_joysticks; i++ ) {
    if( libspectrum_snap_joystick_inputs( snap, i ) & connection ) {
      switch( libspectrum_snap_joystick_list( snap, i ) ) {
      case LIBSPECTRUM_JOYSTICK_CURSOR:
        if( !found ) { type = ZXJT_CURSOR; found = 1; }
        else *out_flags |= LIBSPECTRUM_FLAG_SNAPSHOT_MINOR_INFO_LOSS;
        break;
      case LIBSPECTRUM_JOYSTICK_KEMPSTON:
        if( !found ) { type = ZXJT_KEMPSTON; found = 1; }
        else *out_flags |= LIBSPECTRUM_FLAG_SNAPSHOT_MINOR_INFO_LOSS;
        break;
      case LIBSPECTRUM_JOYSTICK_SINCLAIR_1:
        if( !found ) { type = ZXJT_SINCLAIR1; found = 1; }
        else *out_flags |= LIBSPECTRUM_FLAG_SNAPSHOT_MINOR_INFO_LOSS;
        break;
      case LIBSPECTRUM_JOYSTICK_SINCLAIR_2:
        if( !found ) { type = ZXJT_SINCLAIR2; found = 1; }
        else *out_flags |= LIBSPECTRUM_FLAG_SNAPSHOT_MINOR_INFO_LOSS;
        break;
      case LIBSPECTRUM_JOYSTICK_TIMEX_1:
        if( !found ) { type = ZXJT_TIMEX1; found = 1; }
        else *out_flags |= LIBSPECTRUM_FLAG_SNAPSHOT_MINOR_INFO_LOSS;
        break;
      case LIBSPECTRUM_JOYSTICK_TIMEX_2:
        if( !found ) { type = ZXJT_TIMEX2; found = 1; }
        else *out_flags |= LIBSPECTRUM_FLAG_SNAPSHOT_MINOR_INFO_LOSS;
        break;
      case LIBSPECTRUM_JOYSTICK_FULLER:
        if( !found ) { type = ZXJT_FULLER; found = 1; }
        else *out_flags |= LIBSPECTRUM_FLAG_SNAPSHOT_MINOR_INFO_LOSS;
        break;
      case LIBSPECTRUM_JOYSTICK_NONE: /* Shouldn't happen */
      default:
        type = ZXJT_NONE;
        break;
      }
    }
  }

  libspectrum_buffer_write_byte( buffer, type );
}

static void
write_joy_chunk( libspectrum_buffer *buffer, libspectrum_buffer *data,
                 int *out_flags, libspectrum_snap *snap )
{
  libspectrum_dword flags;
  size_t num_joysticks = libspectrum_snap_joystick_active_count( snap );
  int i;

  flags = 0;
  for( i = 0; i < num_joysticks; i++ ) {
    if( libspectrum_snap_joystick_list( snap, i ) ==
        LIBSPECTRUM_JOYSTICK_KEMPSTON )
      flags |= ZXSTJOYF_ALWAYSPORT31;
  }
  libspectrum_buffer_write_dword( data, flags );

  write_joystick( data, out_flags, snap, LIBSPECTRUM_JOYSTICK_INPUT_JOYSTICK_1 );
  write_joystick( data, out_flags, snap, LIBSPECTRUM_JOYSTICK_INPUT_JOYSTICK_2 );

  write_chunk( buffer, ZXSTBID_JOYSTICK, data );
}

static void
write_amxm_chunk( libspectrum_buffer *buffer, libspectrum_buffer *data,
                  libspectrum_snap *snap )
{
  if( libspectrum_snap_kempston_mouse_active( snap ) )
    libspectrum_buffer_write_byte( data, ZXSTM_KEMPSTON );
  else
    libspectrum_buffer_write_byte( data, ZXSTM_NONE );

  /* Z80 PIO CTRLA registers for AMX mouse */
  libspectrum_buffer_write_byte( data, '\0' );
  libspectrum_buffer_write_byte( data, '\0' );
  libspectrum_buffer_write_byte( data, '\0' );

  /* Z80 PIO CTRLB registers for AMX mouse */
  libspectrum_buffer_write_byte( data, '\0' );
  libspectrum_buffer_write_byte( data, '\0' );
  libspectrum_buffer_write_byte( data, '\0' );

  write_chunk( buffer, ZXSTBID_MOUSE, data );
}

static void
write_keyb_chunk( libspectrum_buffer *buffer, libspectrum_buffer *data,
		  int *out_flags, libspectrum_snap *snap )
{
  libspectrum_dword flags;

  flags = 0;
  if( libspectrum_snap_issue2( snap ) ) flags |= ZXSTKF_ISSUE2;

  libspectrum_buffer_write_dword( data, flags );

  write_joystick( data, out_flags, snap, LIBSPECTRUM_JOYSTICK_INPUT_KEYBOARD );

  write_chunk( buffer, ZXSTBID_KEYBOARD, data );
}
  
static void
write_zxpr_chunk( libspectrum_buffer *buffer, libspectrum_buffer *data,
		  int *out_flags, libspectrum_snap *snap )
{
  libspectrum_word flags;

  flags = 0;
  if( libspectrum_snap_zx_printer_active( snap ) ) flags |= ZXSTPRF_ENABLED;

  libspectrum_buffer_write_word( data, flags );

  write_chunk( buffer, ZXSTBID_ZXPRINTER, data );
}
  
static libspectrum_error
write_rom_chunk( libspectrum_buffer *buffer, libspectrum_buffer *block_data,
                 int *out_flags, libspectrum_snap *snap, int compress )
{
  size_t i, data_length = 0;
  libspectrum_buffer *data, *rom_buffer;
  int flags = 0;
  int use_compression;

  for( i = 0; i< libspectrum_snap_custom_rom_pages( snap ); i++ ) {
    data_length += libspectrum_snap_rom_length( snap, i );
  }

  /* Check that we have the expected number of ROMs per the machine type */
  switch( libspectrum_snap_machine( snap ) ) {

  case LIBSPECTRUM_MACHINE_16:
  case LIBSPECTRUM_MACHINE_48:
  case LIBSPECTRUM_MACHINE_48_NTSC:
  case LIBSPECTRUM_MACHINE_TC2048:
    /* 1 ROM = 16k */
    if( ( libspectrum_snap_custom_rom_pages( snap ) != 1 ||
          data_length != 0x4000 ) ) {
      *out_flags |= LIBSPECTRUM_FLAG_SNAPSHOT_MAJOR_INFO_LOSS;
      return LIBSPECTRUM_ERROR_NONE;
    }
    break;
  case LIBSPECTRUM_MACHINE_128:
  case LIBSPECTRUM_MACHINE_128E:
  case LIBSPECTRUM_MACHINE_PENT:
  case LIBSPECTRUM_MACHINE_PLUS2:
  case LIBSPECTRUM_MACHINE_SE:
    /* 2 ROMs = 32k */
    if( ( libspectrum_snap_custom_rom_pages( snap ) != 2 ||
          data_length != 0x8000 ) ) {
      *out_flags |= LIBSPECTRUM_FLAG_SNAPSHOT_MAJOR_INFO_LOSS;
      return LIBSPECTRUM_ERROR_NONE;
    }
    break;
  case LIBSPECTRUM_MACHINE_PLUS2A:
  case LIBSPECTRUM_MACHINE_PLUS3:
  case LIBSPECTRUM_MACHINE_PLUS3E:
    /* 4 ROMs = 64k */
    if( ( libspectrum_snap_custom_rom_pages( snap ) != 4 ||
          data_length != 0x10000 ) ) {
      *out_flags |= LIBSPECTRUM_FLAG_SNAPSHOT_MAJOR_INFO_LOSS;
      return LIBSPECTRUM_ERROR_NONE;
    }
    break;
  case LIBSPECTRUM_MACHINE_PENT512:
  case LIBSPECTRUM_MACHINE_PENT1024:
    /* 3 ROMs = 48k */
    if( ( libspectrum_snap_custom_rom_pages( snap ) != 3 ||
          data_length != 0xc000 ) ) {
      *out_flags |= LIBSPECTRUM_FLAG_SNAPSHOT_MAJOR_INFO_LOSS;
      return LIBSPECTRUM_ERROR_NONE;
    }
    break;
  case LIBSPECTRUM_MACHINE_TC2068:
  case LIBSPECTRUM_MACHINE_TS2068:
    /* 2 ROMs = 24k */
    if( ( libspectrum_snap_custom_rom_pages( snap ) != 2 ||
          data_length != 0x6000 ) ) {
      *out_flags |= LIBSPECTRUM_FLAG_SNAPSHOT_MAJOR_INFO_LOSS;
      return LIBSPECTRUM_ERROR_NONE;
    }
    break;
  case LIBSPECTRUM_MACHINE_SCORP:
    /* 4 ROMs = 64k */
    if( ( libspectrum_snap_custom_rom_pages( snap ) != 4 ||
          data_length != 0x10000 ) ) {
      *out_flags |= LIBSPECTRUM_FLAG_SNAPSHOT_MAJOR_INFO_LOSS;
      return LIBSPECTRUM_ERROR_NONE;
    }
    break;

  case LIBSPECTRUM_MACHINE_UNKNOWN:
    libspectrum_print_error( LIBSPECTRUM_ERROR_LOGIC,
			     "Emulated machine type is set to 'unknown'!" );
    return LIBSPECTRUM_ERROR_LOGIC;
  }

  data = libspectrum_buffer_alloc();

  /* Copy the rom data into a single block ready for putting in the szx */
  for( i = 0; i< libspectrum_snap_custom_rom_pages( snap ); i++ ) {
    libspectrum_buffer_write( data, libspectrum_snap_roms( snap, i ),
                              libspectrum_snap_rom_length( snap, i ) );
  }

  rom_buffer = libspectrum_buffer_alloc();
  use_compression = compress_data( rom_buffer,
                                   libspectrum_buffer_get_data( data ),
                                   libspectrum_buffer_get_data_size( data ),
                                   compress );

  if( use_compression ) flags |= ZXSTRF_COMPRESSED;
  libspectrum_buffer_write_word( block_data, flags );
  libspectrum_buffer_write_dword( block_data,
                                  libspectrum_buffer_get_data_size( data ) );

  libspectrum_buffer_write_buffer( block_data, rom_buffer );

  libspectrum_buffer_free( data );
  libspectrum_buffer_free( rom_buffer );

  write_chunk( buffer, ZXSTBID_ROM, block_data );

  return LIBSPECTRUM_ERROR_NONE;
}

static void
write_ram_pages( libspectrum_buffer *buffer, libspectrum_buffer *block_data,
                 libspectrum_snap *snap, int compress )
{
  libspectrum_machine machine;
  int i, capabilities; 

  machine = libspectrum_snap_machine( snap );
  capabilities = libspectrum_machine_capabilities( machine );

  write_ramp_chunk( buffer, block_data, snap, 5, compress );

  if( machine != LIBSPECTRUM_MACHINE_16 ) {
    write_ramp_chunk( buffer, block_data, snap, 2, compress );
    write_ramp_chunk( buffer, block_data, snap, 0, compress );
  }

  if( capabilities & LIBSPECTRUM_MACHINE_CAPABILITY_128_MEMORY ) {
    write_ramp_chunk( buffer, block_data, snap, 1, compress );
    write_ramp_chunk( buffer, block_data, snap, 3, compress );
    write_ramp_chunk( buffer, block_data, snap, 4, compress );
    write_ramp_chunk( buffer, block_data, snap, 6, compress );
    write_ramp_chunk( buffer, block_data, snap, 7, compress );

    if( capabilities & LIBSPECTRUM_MACHINE_CAPABILITY_SCORP_MEMORY ) {
      for( i = 8; i < 16; i++ ) {
        write_ramp_chunk( buffer, block_data, snap, i, compress );
      }
    } else if( capabilities & LIBSPECTRUM_MACHINE_CAPABILITY_PENT512_MEMORY ) {
      for( i = 8; i < 32; i++ ) {
        write_ramp_chunk( buffer, block_data, snap, i, compress );
      }

      if( capabilities & LIBSPECTRUM_MACHINE_CAPABILITY_PENT1024_MEMORY ) {
	for( i = 32; i < 64; i++ ) {
	  write_ramp_chunk( buffer, block_data, snap, i, compress );
	}
      }
    }

  }

  if( capabilities & LIBSPECTRUM_MACHINE_CAPABILITY_SE_MEMORY ) {
    write_ramp_chunk( buffer, block_data, snap, 8, compress );
  }
}

static void
write_ramp_chunk( libspectrum_buffer *buffer, libspectrum_buffer *block_data,
                  libspectrum_snap *snap, int page, int compress )
{
  const libspectrum_byte *data = libspectrum_snap_pages( snap, page );

  write_ram_page( buffer, block_data, ZXSTBID_RAMPAGE, data, 0x4000, page,
                  compress, 0x00 );
}

static void
write_ram_page( libspectrum_buffer *buffer, libspectrum_buffer *block_data,
                const char *id, const libspectrum_byte *data,
                size_t data_length, int page, int compress, int extra_flags )
{
  libspectrum_buffer *data_buffer;
  int use_compression;

  if( !data ) return;

  data_buffer = libspectrum_buffer_alloc();
  use_compression = compress_data( data_buffer, data, data_length, compress );

  if( use_compression ) extra_flags |= ZXSTRF_COMPRESSED;

  libspectrum_buffer_write_word( block_data, extra_flags );

  libspectrum_buffer_write_byte( block_data, (libspectrum_byte)page );

  libspectrum_buffer_write_buffer( block_data, data_buffer );

  libspectrum_buffer_free( data_buffer );

  write_chunk( buffer, id, block_data );
}

static void
write_ay_chunk( libspectrum_buffer *buffer, libspectrum_buffer *data,
                libspectrum_snap *snap )
{
  size_t i;
  libspectrum_byte flags;

  flags = 0;
  if( libspectrum_snap_fuller_box_active( snap ) ) flags |= ZXSTAYF_FULLERBOX;
  if( libspectrum_snap_melodik_active( snap ) ) flags |= ZXSTAYF_128AY;
  libspectrum_buffer_write_byte( data, flags );

  libspectrum_buffer_write_byte( data,
                                 libspectrum_snap_out_ay_registerport( snap ) );

  for( i = 0; i < 16; i++ )
    libspectrum_buffer_write_byte( data,
                                   libspectrum_snap_ay_registers( snap, i ) );

  write_chunk( buffer, ZXSTBID_AY, data );
}

static void
write_scld_chunk( libspectrum_buffer *buffer, libspectrum_buffer *data,
                  libspectrum_snap *snap )
{
  libspectrum_buffer_write_byte( data, libspectrum_snap_out_scld_hsr( snap ) );
  libspectrum_buffer_write_byte( data, libspectrum_snap_out_scld_dec( snap ) );

  write_chunk( buffer, ZXSTBID_TIMEXREGS, data );
}

static void
write_b128_chunk( libspectrum_buffer *buffer, libspectrum_buffer *data,
                  libspectrum_snap *snap, int compress )
{
  libspectrum_byte *rom_data = NULL;
  libspectrum_buffer *rom_buffer = NULL;
  libspectrum_word beta_rom_length = 0;
  libspectrum_dword flags;
  int use_compression = 0;

  if( libspectrum_snap_beta_custom_rom( snap ) ) {
    rom_data = libspectrum_snap_beta_rom( snap, 0 );
    beta_rom_length = 0x4000;

    rom_buffer = libspectrum_buffer_alloc();
    use_compression = compress_data( rom_buffer, rom_data,
                                     beta_rom_length, compress );
  }

  flags = ZXSTBETAF_CONNECTED;	/* Betadisk interface connected */
  if( libspectrum_snap_beta_paged( snap ) ) flags |= ZXSTBETAF_PAGED;
  if( libspectrum_snap_beta_autoboot( snap ) ) flags |= ZXSTBETAF_AUTOBOOT;
  if( !libspectrum_snap_beta_direction( snap ) ) flags |= ZXSTBETAF_SEEKLOWER;
  if( libspectrum_snap_beta_custom_rom( snap ) ) flags |= ZXSTBETAF_CUSTOMROM;
  if( use_compression ) flags |= ZXSTBETAF_COMPRESSED;
  libspectrum_buffer_write_dword( data, flags );

  libspectrum_buffer_write_byte( data, libspectrum_snap_beta_drive_count( snap ) );
  libspectrum_buffer_write_byte( data, libspectrum_snap_beta_system( snap ) );
  libspectrum_buffer_write_byte( data, libspectrum_snap_beta_track ( snap ) );
  libspectrum_buffer_write_byte( data, libspectrum_snap_beta_sector( snap ) );
  libspectrum_buffer_write_byte( data, libspectrum_snap_beta_data  ( snap ) );
  libspectrum_buffer_write_byte( data, libspectrum_snap_beta_status( snap ) );

  if( libspectrum_snap_beta_custom_rom( snap ) && rom_data ) {
    libspectrum_buffer_write_buffer( data, rom_buffer );
  }

  write_chunk( buffer, ZXSTBID_BETA128, data );

  if( rom_buffer ) libspectrum_buffer_free( rom_buffer );
}

static int
compress_data( libspectrum_buffer *dest, const libspectrum_byte *src_data,
               size_t src_data_length, int compress )
{
  libspectrum_byte *compressed_data = NULL;
  int use_compression = 0;

#ifdef HAVE_ZLIB_H

  if( src_data && compress ) {

    libspectrum_error error;
    size_t compressed_length;

    error = libspectrum_zlib_compress( src_data, src_data_length,
				       &compressed_data, &compressed_length );

    if( error == LIBSPECTRUM_ERROR_NONE &&
        ( compress & LIBSPECTRUM_FLAG_SNAPSHOT_ALWAYS_COMPRESS ||
          compressed_length < src_data_length ) ) {
      use_compression = 1;
      src_data = compressed_data;
      src_data_length = compressed_length;
    }

  }

#endif				/* #ifdef HAVE_ZLIB_H */

  libspectrum_buffer_write( dest, src_data, src_data_length );

  if( compressed_data ) libspectrum_free( compressed_data );

  return use_compression;
}

static libspectrum_error
write_if1_chunk( libspectrum_buffer *buffer, libspectrum_buffer *data,
		 libspectrum_snap *snap, int compress  )
{
  libspectrum_byte *rom_data = NULL; 
  libspectrum_buffer *rom_buffer;
  libspectrum_word disk_rom_length = 0;
  libspectrum_word uncompressed_rom_length = 0;
  libspectrum_word flags = 0;
  int use_compression;
  int i;
  libspectrum_byte drive_count = 8;

  if( libspectrum_snap_interface1_custom_rom( snap ) ) {
    if( !(libspectrum_snap_interface1_rom_length( snap, 0 ) == 0x2000 ||
          libspectrum_snap_interface1_rom_length( snap, 0 ) == 0x4000 )) {
      libspectrum_print_error( LIBSPECTRUM_ERROR_LOGIC,
                               "Interface 1 custom ROM must be 8192 or 16384 "
                               "bytes, supplied ROM is %lu bytes",
                               (unsigned long)
			       libspectrum_snap_interface1_rom_length(
                                 snap, 0 ) );
      return LIBSPECTRUM_ERROR_LOGIC;
    }
    rom_data = libspectrum_snap_interface1_rom( snap, 0 );
    if( rom_data == NULL ) {
      libspectrum_print_error( LIBSPECTRUM_ERROR_LOGIC,
                              "Interface 1 custom ROM specified to be %lu "
                              "bytes but NULL pointer provided",
                              (unsigned long)
                              libspectrum_snap_interface1_rom_length(
                                 snap, 0 ) );
      return LIBSPECTRUM_ERROR_LOGIC;
    }
    uncompressed_rom_length = disk_rom_length =
      libspectrum_snap_interface1_rom_length( snap, 0 );
  }

  rom_buffer = libspectrum_buffer_alloc();
  use_compression = compress_data( rom_buffer, rom_data,
                                   uncompressed_rom_length, compress );

  flags |= ZXSTIF1F_ENABLED;
  if( libspectrum_snap_interface1_paged( snap ) ) flags |= ZXSTIF1F_PAGED;
  if( use_compression ) flags |= ZXSTIF1F_COMPRESSED;
  libspectrum_buffer_write_word( data, flags );

  if( libspectrum_snap_interface1_drive_count( snap ) )
    drive_count = libspectrum_snap_interface1_drive_count( snap );
  else
    drive_count = 8;	/* guess 8 drives connected */
  libspectrum_buffer_write_byte( data, drive_count );

  /* Skip 'reserved' data */
  for( i = 0; i < 3; i++ ) libspectrum_buffer_write_byte( data, 0 );

  /* Skip 'reserved' data */
  for( i = 0; i < 8; i++ ) libspectrum_buffer_write_dword( data, 0 );

  libspectrum_buffer_write_word( data, uncompressed_rom_length );

  if( libspectrum_snap_interface1_custom_rom( snap ) &&
      libspectrum_buffer_is_not_empty( rom_buffer ) ) {
    libspectrum_buffer_write_buffer( data, rom_buffer );
  }

  libspectrum_buffer_free( rom_buffer );

  write_chunk( buffer, ZXSTBID_IF1, data );

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
write_opus_chunk( libspectrum_buffer *buffer, libspectrum_buffer *data,
		  libspectrum_snap *snap, int compress  )
{
  libspectrum_byte *rom_data, *ram_data; 
  libspectrum_buffer *rom_buffer, *ram_buffer;
  size_t disk_rom_length, disk_ram_length;
  libspectrum_dword flags = 0;
  int rom_compression = 0, ram_compression = 0;

  rom_data = libspectrum_snap_opus_rom( snap, 0 );
  if( rom_data == NULL ) {
    libspectrum_print_error( LIBSPECTRUM_ERROR_LOGIC,
                            "Opus ROM must be 8192 bytes but NULL pointer "
                            "provided" );
    return LIBSPECTRUM_ERROR_LOGIC;
  }
  disk_rom_length = 0x2000;
  ram_data = libspectrum_snap_opus_ram( snap, 0 );
  disk_ram_length = 0x800;

  rom_buffer = libspectrum_buffer_alloc();
  rom_compression = compress_data( rom_buffer, rom_data, disk_rom_length,
                                   compress );
  ram_buffer = libspectrum_buffer_alloc();
  ram_compression = compress_data( ram_buffer, ram_data, disk_ram_length,
                                   compress );

  /* If we wanted to compress, only use a compressed buffer if both were
     compressed as we only have one flag */
  if( compress && !( rom_compression && ram_compression ) ) {
    libspectrum_buffer_clear( rom_buffer );
    libspectrum_buffer_write( rom_buffer, rom_data, disk_rom_length );
    libspectrum_buffer_clear( ram_buffer );
    libspectrum_buffer_write( ram_buffer, ram_data, disk_ram_length );
  }

  if( libspectrum_snap_opus_paged( snap ) ) flags |= ZXSTOPUSF_PAGED;
  if( rom_compression && ram_compression ) flags |= ZXSTOPUSF_COMPRESSED;
  if( !libspectrum_snap_opus_direction( snap ) ) flags |= ZXSTOPUSF_SEEKLOWER;
  if( libspectrum_snap_opus_custom_rom( snap ) ) flags |= ZXSTOPUSF_CUSTOMROM;
  libspectrum_buffer_write_dword( data, flags );

  libspectrum_buffer_write_dword( data,
                              libspectrum_buffer_get_data_size( ram_buffer ) );
  if( libspectrum_snap_opus_custom_rom( snap ) ) {
    libspectrum_buffer_write_dword( data,
                              libspectrum_buffer_get_data_size( rom_buffer ) );
  } else {
    libspectrum_buffer_write_dword( data, 0 );
  }
  libspectrum_buffer_write_byte( data, libspectrum_snap_opus_control_a( snap ) );
  libspectrum_buffer_write_byte( data, libspectrum_snap_opus_data_reg_a( snap ) );
  libspectrum_buffer_write_byte( data, libspectrum_snap_opus_data_dir_a( snap ) );
  libspectrum_buffer_write_byte( data, libspectrum_snap_opus_control_b( snap ) );
  libspectrum_buffer_write_byte( data, libspectrum_snap_opus_data_reg_b( snap ) );
  libspectrum_buffer_write_byte( data, libspectrum_snap_opus_data_dir_b( snap ) );
  libspectrum_buffer_write_byte( data, libspectrum_snap_opus_drive_count( snap ) );
  libspectrum_buffer_write_byte( data, libspectrum_snap_opus_track  ( snap ) );
  libspectrum_buffer_write_byte( data, libspectrum_snap_opus_sector ( snap ) );
  libspectrum_buffer_write_byte( data, libspectrum_snap_opus_data   ( snap ) );
  libspectrum_buffer_write_byte( data, libspectrum_snap_opus_status ( snap ) );

  libspectrum_buffer_write_buffer( data, ram_buffer );

  if( libspectrum_snap_opus_custom_rom( snap ) ) {
    libspectrum_buffer_write_buffer( data, rom_buffer );
  }

  write_chunk( buffer, ZXSTBID_OPUS, data );

  libspectrum_buffer_free( rom_buffer );
  libspectrum_buffer_free( ram_buffer );

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
write_plsd_chunk( libspectrum_buffer *buffer, libspectrum_buffer *data,
                  libspectrum_snap *snap, int compress  )
{
  libspectrum_byte *rom_data, *ram_data; 
  libspectrum_buffer *rom_buffer, *ram_buffer;
  size_t disk_rom_length, disk_ram_length;
  libspectrum_dword flags = 0;
  int rom_compression = 0, ram_compression = 0;

  rom_data = libspectrum_snap_plusd_rom( snap, 0 );
  if( rom_data == NULL ) {
    libspectrum_print_error( LIBSPECTRUM_ERROR_LOGIC,
                            "+D ROM must be 8192 bytes but NULL pointer "
                            "provided" );
    return LIBSPECTRUM_ERROR_LOGIC;
  }
  disk_rom_length = 0x2000;
  ram_data = libspectrum_snap_plusd_ram( snap, 0 );
  disk_ram_length = 0x2000;

  rom_buffer = libspectrum_buffer_alloc();
  rom_compression = compress_data( rom_buffer, rom_data, disk_rom_length,
                                   compress );
  ram_buffer = libspectrum_buffer_alloc();
  ram_compression = compress_data( ram_buffer, ram_data, disk_ram_length,
                                   compress );

  /* If we wanted to compress, only use a compressed buffer if both were
     compressed as we only have one flag */
  if( compress && !( rom_compression && ram_compression ) ) {
    libspectrum_buffer_clear( rom_buffer );
    libspectrum_buffer_write( rom_buffer, rom_data, disk_rom_length );
    libspectrum_buffer_clear( ram_buffer );
    libspectrum_buffer_write( ram_buffer, ram_data, disk_ram_length );
  }

  if( libspectrum_snap_plusd_paged( snap ) ) flags |= ZXSTPLUSDF_PAGED;
  if( rom_compression && ram_compression ) flags |= ZXSTPLUSDF_COMPRESSED;
  if( !libspectrum_snap_plusd_direction( snap ) ) flags |= ZXSTPLUSDF_SEEKLOWER;
  libspectrum_buffer_write_dword( data, flags );

  libspectrum_buffer_write_dword( data,
                              libspectrum_buffer_get_data_size( ram_buffer ) );
  if( libspectrum_snap_plusd_custom_rom( snap ) ) {
    libspectrum_buffer_write_dword( data,
                              libspectrum_buffer_get_data_size( rom_buffer ) );
    libspectrum_buffer_write_byte( data, ZXSTPDRT_CUSTOM );
  } else {
    libspectrum_buffer_write_dword( data, 0 );
    libspectrum_buffer_write_byte( data, ZXSTPDRT_GDOS );
  }
  libspectrum_buffer_write_byte( data, libspectrum_snap_plusd_control( snap ) );
  libspectrum_buffer_write_byte( data, libspectrum_snap_plusd_drive_count( snap ) );
  libspectrum_buffer_write_byte( data, libspectrum_snap_plusd_track  ( snap ) );
  libspectrum_buffer_write_byte( data, libspectrum_snap_plusd_sector ( snap ) );
  libspectrum_buffer_write_byte( data, libspectrum_snap_plusd_data   ( snap ) );
  libspectrum_buffer_write_byte( data, libspectrum_snap_plusd_status ( snap ) );

  libspectrum_buffer_write_buffer( data, ram_buffer );

  if( libspectrum_snap_plusd_custom_rom( snap ) ) {
    libspectrum_buffer_write_buffer( data, rom_buffer );
  }

  write_chunk( buffer, ZXSTBID_PLUSD, data );

  libspectrum_buffer_free( rom_buffer );
  libspectrum_buffer_free( ram_buffer );

  return LIBSPECTRUM_ERROR_NONE;
}

static void
write_zxat_chunk( libspectrum_buffer *buffer, libspectrum_buffer *data,
                  libspectrum_snap *snap )
{
  libspectrum_word flags;

  flags = 0;
  if( libspectrum_snap_zxatasp_upload ( snap ) ) flags |= ZXSTZXATF_UPLOAD;
  if( libspectrum_snap_zxatasp_writeprotect( snap ) )
    flags |= ZXSTZXATF_WRITEPROTECT;
  libspectrum_buffer_write_word( data, flags );

  libspectrum_buffer_write_byte( data, libspectrum_snap_zxatasp_port_a( snap ) );
  libspectrum_buffer_write_byte( data, libspectrum_snap_zxatasp_port_b( snap ) );
  libspectrum_buffer_write_byte( data, libspectrum_snap_zxatasp_port_c( snap ) );
  libspectrum_buffer_write_byte( data, libspectrum_snap_zxatasp_control( snap ) );
  libspectrum_buffer_write_byte( data, libspectrum_snap_zxatasp_pages( snap ) );
  libspectrum_buffer_write_byte( data, libspectrum_snap_zxatasp_current_page( snap ) );

  write_chunk( buffer, ZXSTBID_ZXATASP, data );
}

static void
write_atrp_chunk( libspectrum_buffer *buffer, libspectrum_buffer *block_data,
		  libspectrum_snap *snap, int page, int compress )
{
  const libspectrum_byte *data = libspectrum_snap_zxatasp_ram( snap, page );

  write_ram_page( buffer, block_data, ZXSTBID_ZXATASPRAMPAGE, data, 0x4000,
                  page, compress, 0x00 );
}

static void
write_zxcf_chunk( libspectrum_buffer *buffer, libspectrum_buffer *data,
		  libspectrum_snap *snap )
{
  libspectrum_word flags;

  flags = 0;
  if( libspectrum_snap_zxcf_upload( snap ) ) flags |= ZXSTZXCFF_UPLOAD;
  libspectrum_buffer_write_word( data, flags );

  libspectrum_buffer_write_byte( data, libspectrum_snap_zxcf_memctl( snap ) );
  libspectrum_buffer_write_byte( data, libspectrum_snap_zxcf_pages( snap ) );

  write_chunk( buffer, ZXSTBID_ZXCF, data );
}

static void
write_cfrp_chunk( libspectrum_buffer *buffer, libspectrum_buffer *block_data,
                  libspectrum_snap *snap, int page, int compress )
{
  const libspectrum_byte *data = libspectrum_snap_zxcf_ram( snap, page );

  write_ram_page( buffer, block_data, ZXSTBID_ZXCFRAMPAGE, data, 0x4000, page,
                  compress, 0x00 );
}

#ifdef HAVE_ZLIB_H

static libspectrum_error
write_if2r_chunk( libspectrum_buffer *buffer, libspectrum_buffer *block_data,
                  libspectrum_snap *snap )
{
  libspectrum_error error;
  libspectrum_byte *data, *compressed_data;
  size_t data_length, compressed_length;

  data = libspectrum_snap_interface2_rom( snap, 0 ); data_length = 0x4000;
  compressed_data = NULL;

  error = libspectrum_zlib_compress( data, data_length,
                                     &compressed_data, &compressed_length );
  if( error ) return error;

  libspectrum_buffer_write_dword( block_data, compressed_length );
  libspectrum_buffer_write( block_data, compressed_data, compressed_length );

  write_chunk( buffer, ZXSTBID_IF2ROM, block_data );

  if( compressed_data ) libspectrum_free( compressed_data );

  return LIBSPECTRUM_ERROR_NONE;
}

#endif                         /* #ifdef HAVE_ZLIB_H */

static void
write_dock_chunk( libspectrum_buffer *buffer, libspectrum_buffer *block_data,
		  libspectrum_snap *snap, int exrom_dock,
                  const libspectrum_byte *data, int page, int writeable,
                  int compress )
{
  libspectrum_byte extra_flags = 0;

  if( writeable  ) extra_flags |= ZXSTDOCKF_RAM;
  if( exrom_dock ) extra_flags |= ZXSTDOCKF_EXROMDOCK;

  write_ram_page( buffer, block_data, ZXSTBID_DOCK, data, 0x2000, page,
                  compress, extra_flags );
}

static void
write_side_chunk( libspectrum_buffer *buffer, libspectrum_buffer *block_data,
		  libspectrum_snap *snap )
{
  write_chunk( buffer, ZXSTBID_SIMPLEIDE, NULL );
}

static void
write_drum_chunk( libspectrum_buffer *buffer, libspectrum_buffer *data,
                  libspectrum_snap *snap )
{
  libspectrum_buffer_write_byte( data,
                                 libspectrum_snap_specdrum_dac( snap ) + 128 );

  write_chunk( buffer, ZXSTBID_SPECDRUM, data );
}

static void
write_covx_chunk( libspectrum_buffer *buffer, libspectrum_buffer *data,
                  libspectrum_snap *snap )
{
  libspectrum_buffer_write_byte( data, libspectrum_snap_covox_dac( snap ) );

  /* Write 'reserved' data */
  libspectrum_buffer_write_byte( data, 0 );
  libspectrum_buffer_write_byte( data, 0 );
  libspectrum_buffer_write_byte( data, 0 );

  write_chunk( buffer, ZXSTBID_COVOX, data );
}

static libspectrum_error
write_divxxx_chunk( libspectrum_buffer *buffer, libspectrum_buffer *data,
                    libspectrum_snap *snap, int compress,
                    const char *id,
                    libspectrum_byte* (*get_eprom)( libspectrum_snap*, int ),
                    int (*get_eprom_wp)( libspectrum_snap* ),
                    libspectrum_word eprom_wp_flag,
                    int (*get_paged)( libspectrum_snap* ),
                    libspectrum_word paged_flag,
                    libspectrum_word compressed_flag,
                    libspectrum_byte (*get_control)( libspectrum_snap* ),
                    size_t (*get_page_count)( libspectrum_snap* ) )
{
  libspectrum_byte *eprom_data = NULL;
  libspectrum_buffer *eprom_buffer;
  libspectrum_word flags = 0;
  libspectrum_word uncompressed_eprom_length = 0;
  int use_compression;

  eprom_data = get_eprom( snap, 0 );
  if( !eprom_data ) {
    libspectrum_print_error( LIBSPECTRUM_ERROR_LOGIC, "EPROM data is missing" );
    return LIBSPECTRUM_ERROR_LOGIC;
  }
  uncompressed_eprom_length = 0x2000;

  eprom_buffer = libspectrum_buffer_alloc();
  use_compression = compress_data( eprom_buffer, eprom_data,
                                   uncompressed_eprom_length, compress );

  if( get_eprom_wp( snap ) ) flags |= eprom_wp_flag;
  if( get_paged( snap ) ) flags |= paged_flag;
  if( use_compression ) flags |= compressed_flag;
  libspectrum_buffer_write_word( data, flags );

  libspectrum_buffer_write_byte( data, get_control( snap ) );
  libspectrum_buffer_write_byte( data, get_page_count( snap ) );

  libspectrum_buffer_write_buffer( data, eprom_buffer );

  write_chunk( buffer, id, data );

  libspectrum_buffer_free( eprom_buffer );

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
write_dide_chunk( libspectrum_buffer *buffer, libspectrum_buffer *data,
                  libspectrum_snap *snap, int compress )
{
  return write_divxxx_chunk(
      buffer, data, snap, compress, ZXSTBID_DIVIDE,
      libspectrum_snap_divide_eprom,
      libspectrum_snap_divide_eprom_writeprotect,
      ZXSTDIVIDE_EPROM_WRITEPROTECT,
      libspectrum_snap_divide_paged, ZXSTDIVIDE_PAGED,
      ZXSTDIVIDE_COMPRESSED,
      libspectrum_snap_divide_control,
      libspectrum_snap_divide_pages );
}

static libspectrum_error
write_dmmc_chunk( libspectrum_buffer *buffer, libspectrum_buffer *data,
                  libspectrum_snap *snap, int compress )
{
  return write_divxxx_chunk(
      buffer, data, snap, compress, ZXSTBID_DIVMMC,
      libspectrum_snap_divmmc_eprom,
      libspectrum_snap_divmmc_eprom_writeprotect,
      ZXSTDIVMMC_EPROM_WRITEPROTECT,
      libspectrum_snap_divmmc_paged, ZXSTDIVMMC_PAGED,
      ZXSTDIVMMC_COMPRESSED,
      libspectrum_snap_divmmc_control,
      libspectrum_snap_divmmc_pages );
}

static void
write_divxxx_ram_chunk( libspectrum_buffer *buffer,
                        libspectrum_buffer *block_data, libspectrum_snap *snap,
                        int page, int compress,
                        libspectrum_byte* (*get_data)( libspectrum_snap*, int ),
                        const char *id )
{
  const libspectrum_byte *data = get_data( snap, page );
  write_ram_page( buffer, block_data, id, data, 0x2000, page, compress, 0x00 );
}

static void
write_dirp_chunk( libspectrum_buffer *buffer, libspectrum_buffer *block_data,
                  libspectrum_snap *snap, int page, int compress )
{
  write_divxxx_ram_chunk( buffer, block_data, snap, page, compress,
                          libspectrum_snap_divide_ram, ZXSTBID_DIVIDERAMPAGE );
}

static void
write_dmrp_chunk( libspectrum_buffer *buffer, libspectrum_buffer *block_data,
                  libspectrum_snap *snap, int page, int compress )
{
  write_divxxx_ram_chunk( buffer, block_data, snap, page, compress,
                          libspectrum_snap_divmmc_ram, ZXSTBID_DIVMMCRAMPAGE );
}

static void
write_snet_chunk( libspectrum_buffer *buffer, libspectrum_buffer *data,
                  libspectrum_snap *snap, int compress )
{
  libspectrum_word flags = 0;

  if( libspectrum_snap_spectranet_paged( snap ) )
    flags |= ZXSTSNET_PAGED;
  if( libspectrum_snap_spectranet_paged_via_io( snap ) )
    flags |= ZXSTSNET_PAGED_VIA_IO;
  if( libspectrum_snap_spectranet_programmable_trap_active( snap ) )
    flags |= ZXSTSNET_PROGRAMMABLE_TRAP_ACTIVE;
  if( libspectrum_snap_spectranet_programmable_trap_msb( snap ) )
    flags |= ZXSTSNET_PROGRAMMABLE_TRAP_MSB;
  if( libspectrum_snap_spectranet_all_traps_disabled( snap ) )
    flags |= ZXSTSNET_ALL_DISABLED;
  if( libspectrum_snap_spectranet_rst8_trap_disabled( snap ) )
    flags |= ZXSTSNET_RST8_DISABLED;
  if( libspectrum_snap_spectranet_deny_downstream_a15( snap ) )
    flags |= ZXSTSNET_DENY_DOWNSTREAM_A15;
  if( libspectrum_snap_spectranet_nmi_flipflop( snap ) )
    flags |= ZXSTSNET_NMI_FLIPFLOP;
  libspectrum_buffer_write_word( data, flags );

  libspectrum_buffer_write_byte( data,
                                 libspectrum_snap_spectranet_page_a( snap ) );
  libspectrum_buffer_write_byte( data,
                                 libspectrum_snap_spectranet_page_b( snap ) );

  libspectrum_buffer_write_word( data,
    libspectrum_snap_spectranet_programmable_trap( snap ) );

  libspectrum_buffer_write( data, libspectrum_snap_spectranet_w5100( snap, 0 ),
                            0x30 );

  write_chunk( buffer, ZXSTBID_SPECTRANET, data );
}

static void
write_snef_chunk( libspectrum_buffer *buffer, libspectrum_buffer *data,
		  libspectrum_snap *snap, int compress )
{
  size_t flash_length;
  libspectrum_byte *flash_data;
  libspectrum_buffer *flash_buffer;
  int flash_compressed = 0;
  libspectrum_byte flags = 0;

  flash_data = libspectrum_snap_spectranet_flash( snap, 0 );
  flash_length = 0x20000;

  flash_buffer = libspectrum_buffer_alloc();
  flash_compressed = compress_data( flash_buffer, flash_data, flash_length,
                                    compress );

  if( flash_compressed )
    flags |= ZXSTSNEF_FLASH_COMPRESSED;
  libspectrum_buffer_write_byte( data, flags );

  libspectrum_buffer_write_dword( data,
                            libspectrum_buffer_get_data_size( flash_buffer ) );
  libspectrum_buffer_write_buffer( data, flash_buffer );

  libspectrum_buffer_free( flash_buffer );

  write_chunk( buffer, ZXSTBID_SPECTRANETFLASHPAGE, data );
}

static void
write_sner_chunk( libspectrum_buffer *buffer, libspectrum_buffer *data,
		  libspectrum_snap *snap, int compress )
{
  size_t ram_length;
  libspectrum_byte *ram_data;
  libspectrum_buffer *ram_buffer;
  int ram_compressed = 0;
  libspectrum_byte flags = 0;

  ram_data = libspectrum_snap_spectranet_ram( snap, 0 );
  ram_length = 0x20000;

  ram_buffer = libspectrum_buffer_alloc();
  ram_compressed = compress_data( ram_buffer, ram_data,
                                  ram_length, compress );

  if( ram_compressed )
    flags |= ZXSTSNER_RAM_COMPRESSED;
  libspectrum_buffer_write_byte( data, flags );

  libspectrum_buffer_write_dword( data,
                              libspectrum_buffer_get_data_size( ram_buffer ) );
  libspectrum_buffer_write_buffer( data, ram_buffer );

  write_chunk( buffer, ZXSTBID_SPECTRANETRAMPAGE, data );

  libspectrum_buffer_free( ram_buffer );
}

static libspectrum_error
write_mfce_chunk( libspectrum_buffer *buffer, libspectrum_buffer *data,
		  libspectrum_snap *snap, int compress )
{
  libspectrum_byte *ram_data;
  libspectrum_buffer *ram_buffer;
  size_t ram_length;
  libspectrum_byte flags = 0;
  libspectrum_byte model;
  int use_compression = 0;

  ram_length = libspectrum_snap_multiface_ram_length( snap, 0 );
  if( ram_length != 0x2000 && ram_length != 0x4000 ) {
      libspectrum_print_error( LIBSPECTRUM_ERROR_LOGIC,
                               "%s:write_mfce_chunk: invalid RAM length, "
                               "should be 8192 or 16384 bytes, "
                               "provided snap has %lu",
                               __FILE__,
                               (unsigned long)ram_length );
    return LIBSPECTRUM_ERROR_LOGIC;
  }
  ram_data = libspectrum_snap_multiface_ram( snap, 0 );
  if( ram_data == NULL ) {
    libspectrum_print_error( LIBSPECTRUM_ERROR_LOGIC,
                            "Multiface RAM specified to be %lu "
                            "bytes but NULL pointer provided in snap",
                            (unsigned long)ram_length );
    return LIBSPECTRUM_ERROR_LOGIC;
  }

  ram_buffer = libspectrum_buffer_alloc();
  use_compression = compress_data( ram_buffer, ram_data, ram_length, compress );

  if( libspectrum_snap_multiface_model_one( snap ) )
    model = ZXSTMFM_1;
  else
    model = ZXSTMFM_128;
  libspectrum_buffer_write_byte( data, model );

  if( libspectrum_snap_multiface_paged( snap ) ) flags |= ZXSTMF_PAGEDIN;
  if( use_compression ) flags |= ZXSTMF_COMPRESSED;
  if( libspectrum_snap_multiface_software_lockout( snap ) )
    flags |= ZXSTMF_SOFTWARELOCKOUT;
  if( libspectrum_snap_multiface_red_button_disabled( snap ) )
    flags |= ZXSTMF_REDBUTTONDISABLED;
  if( libspectrum_snap_multiface_disabled( snap ) ) flags |= ZXSTMF_DISABLED;
  if( ram_length == 0x4000 ) flags |= ZXSTMF_16KRAMMODE;
  libspectrum_buffer_write_byte( data, flags );

  libspectrum_buffer_write_buffer( data, ram_buffer );

  write_chunk( buffer, ZXSTBID_MULTIFACE, data );

  libspectrum_buffer_free( ram_buffer );

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
write_pltt_chunk( libspectrum_buffer *buffer, libspectrum_buffer *data,
		  libspectrum_snap *snap, int compress )
{
  libspectrum_byte flags = 0;

  if( libspectrum_snap_ulaplus_palette_enabled( snap ) )
    flags |= ZXSTPALETTE_ENABLED;
  libspectrum_buffer_write_byte( data, flags );

  libspectrum_buffer_write_byte( data,
      libspectrum_snap_ulaplus_current_register( snap ) );

  libspectrum_buffer_write( data,
      libspectrum_snap_ulaplus_palette( snap, 0 ), 64 );

  libspectrum_buffer_write_byte( data,
      libspectrum_snap_ulaplus_ff_register( snap ) );

  write_chunk( buffer, ZXSTBID_PALETTE, data );

  return LIBSPECTRUM_ERROR_NONE;
}

static void
write_zmmc_chunk( libspectrum_buffer *buffer, libspectrum_buffer *data,
                  libspectrum_snap *snap )
{
  write_chunk( buffer, ZXSTBID_ZXMMC, NULL );
}

static void
write_chunk( libspectrum_buffer *buffer, const char *id,
             libspectrum_buffer *block_data )
{
  size_t data_length = libspectrum_buffer_get_data_size( block_data );
  libspectrum_buffer_write( buffer, id, 4 );
  libspectrum_buffer_write_dword( buffer, data_length );
  libspectrum_buffer_write_buffer( buffer, block_data );
  libspectrum_buffer_clear( block_data );
}
