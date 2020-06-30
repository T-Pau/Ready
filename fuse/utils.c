/* utils.c: some useful helper functions
   Copyright (c) 1999-2012 Philip Kendall
   Copyright (c) 2015 Stuart Brady
   Copyright (c) 2015 Gergely Szasz
   Copyright (c) 2015 Fredrick Meunier
   Copyright (c) 2016 BogDan Vatra
   Copyright (c) 2016 Sergio Baldoví

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

#include <errno.h>
#ifdef HAVE_LIBGEN_H
#include <libgen.h>
#endif				/* #ifdef HAVE_LIBGEN_H */
#include <string.h>
#include <ui/ui.h>
#include <unistd.h>

#include <libspectrum.h>

#include "fuse.h"
#include "machines/specplus3.h"
#include "memory_pages.h"
#include "peripherals/dck.h"
#include "peripherals/ide/divide.h"
#include "peripherals/ide/divmmc.h"
#include "peripherals/ide/simpleide.h"
#include "peripherals/ide/zxatasp.h"
#include "peripherals/ide/zxcf.h"
#include "peripherals/ide/zxmmc.h"
#include "peripherals/if1.h"
#include "peripherals/if2.h"
#include "pokefinder/pokemem.h"
#include "rzx.h"
#include "screenshot.h"
#include "settings.h"
#include "snapshot.h"
#include "tape.h"
#include "utils.h"

static void init_path_context( path_context *ctx, utils_aux_type type );

static int networking_init_count = 0;

/* Open `filename' and do something sensible with it; autoload tapes
   if `autoload' is true and return the type of file found in `type' */
int
utils_open_file( const char *filename, int autoload,
		 libspectrum_id_t *type_ptr)
{
  utils_file file;
  libspectrum_id_t type;
  libspectrum_class_t class;
  int error;

  error = 0;
  if( rzx_recording ) error = rzx_stop_recording();
  if( rzx_playback  ) error = rzx_stop_playback( 1 );
  if( error ) return error;

  /* Read the file into a buffer */
  if( utils_read_file( filename, &file ) ) return 1;

  /* See if we can work out what it is */
  if( libspectrum_identify_file_with_class( &type, &class, filename,
					    file.buffer, file.length ) ) {
    utils_close_file( &file );
    return 1;
  }

  switch( class ) {
    
  case LIBSPECTRUM_CLASS_UNKNOWN:
    ui_error( UI_ERROR_ERROR, "utils_open_file: couldn't identify `%s'",
	      filename );
    utils_close_file( &file );
    return 1;

  case LIBSPECTRUM_CLASS_RECORDING:
    error = rzx_start_playback_from_buffer( file.buffer, file.length );
    break;

  case LIBSPECTRUM_CLASS_SNAPSHOT:
    error = snapshot_read_buffer( file.buffer, file.length, type );
    pokemem_find_pokfile( filename );
    break;

  case LIBSPECTRUM_CLASS_TAPE:
    error = tape_read_buffer( file.buffer, file.length, type, filename,
			      autoload );
    pokemem_find_pokfile( filename );
    break;

  case LIBSPECTRUM_CLASS_DISK_PLUS3:
    if( !( machine_current->capabilities &
	   LIBSPECTRUM_MACHINE_CAPABILITY_PLUS3_DISK ) ) {
      error = machine_select( LIBSPECTRUM_MACHINE_PLUS3 ); if( error ) break;
    }

    error = specplus3_disk_insert( SPECPLUS3_DRIVE_A, filename, autoload );
    break;

  case LIBSPECTRUM_CLASS_DISK_DIDAKTIK:

    error = didaktik80_disk_insert( DIDAKTIK80_DRIVE_A, filename, autoload );
    break;

  case LIBSPECTRUM_CLASS_DISK_PLUSD:

    if( periph_is_active( PERIPH_TYPE_DISCIPLE ) )
      error = disciple_disk_insert( DISCIPLE_DRIVE_1, filename, autoload );
    else
      error = plusd_disk_insert( PLUSD_DRIVE_1, filename, autoload );
    break;

  case LIBSPECTRUM_CLASS_DISK_OPUS:

    error = opus_disk_insert( OPUS_DRIVE_1, filename, autoload );
    break;

  case LIBSPECTRUM_CLASS_DISK_TRDOS:

    if( !( machine_current->capabilities &
	   LIBSPECTRUM_MACHINE_CAPABILITY_TRDOS_DISK ) &&
        !periph_is_active( PERIPH_TYPE_BETA128 ) ) {
      error = machine_select( LIBSPECTRUM_MACHINE_SCORP ); if( error ) break;
    }

    /* Check that we actually got a Beta capable machine to insert the disk */
    if( ( machine_current->capabilities & 
          LIBSPECTRUM_MACHINE_CAPABILITY_TRDOS_DISK ) ||
        periph_is_active( PERIPH_TYPE_BETA128 ) ) {
      error = beta_disk_insert( BETA_DRIVE_A, filename, autoload );
    }
    break;

  case LIBSPECTRUM_CLASS_DISK_GENERIC:
    if( machine_current->machine == LIBSPECTRUM_MACHINE_PLUS3 ||
        machine_current->machine == LIBSPECTRUM_MACHINE_PLUS2A )
      error = specplus3_disk_insert( SPECPLUS3_DRIVE_A, filename, autoload );
    else if( machine_current->machine == LIBSPECTRUM_MACHINE_PENT ||
          machine_current->machine == LIBSPECTRUM_MACHINE_PENT512 ||
          machine_current->machine == LIBSPECTRUM_MACHINE_PENT1024 ||
          machine_current->machine == LIBSPECTRUM_MACHINE_SCORP )
      error = beta_disk_insert( BETA_DRIVE_A, filename, autoload );
    else
      if( periph_is_active( PERIPH_TYPE_BETA128 ) )
        error = beta_disk_insert( BETA_DRIVE_A, filename, autoload );
      else if( periph_is_active( PERIPH_TYPE_DISCIPLE ) )
        error = disciple_disk_insert( DISCIPLE_DRIVE_1, filename, autoload );
      else if( periph_is_active( PERIPH_TYPE_PLUSD ) )
        error = plusd_disk_insert( PLUSD_DRIVE_1, filename, autoload );
    break;

  case LIBSPECTRUM_CLASS_CARTRIDGE_IF2:
    error = if2_insert( filename );
    break;

  case LIBSPECTRUM_CLASS_MICRODRIVE:
    error = if1_mdr_insert( -1, filename );
    break;

  case LIBSPECTRUM_CLASS_CARTRIDGE_TIMEX:
    if( !( machine_current->capabilities &
	   LIBSPECTRUM_MACHINE_CAPABILITY_TIMEX_DOCK ) ) {
      error = machine_select( LIBSPECTRUM_MACHINE_TC2068 ); if( error ) break;
    }
    /* Check that we actually got a Dock capable machine to insert the cart */
    if( machine_current->capabilities &
	   LIBSPECTRUM_MACHINE_CAPABILITY_TIMEX_DOCK ) {
      error = dck_insert( filename );
    }
    break;

  case LIBSPECTRUM_CLASS_HARDDISK:
    if( !settings_current.simpleide_active &&
	!settings_current.zxatasp_active   &&
	!settings_current.divide_enabled   &&
	!settings_current.divmmc_enabled   &&
	!settings_current.zxmmc_enabled    &&
	!settings_current.zxcf_active         ) {
      settings_current.zxcf_active = 1;
      periph_update();
    }

    if( settings_current.zxcf_active ) {
      error = zxcf_insert( filename );
    } else if( settings_current.zxatasp_active ) {
      error = zxatasp_insert( filename, LIBSPECTRUM_IDE_MASTER );
    } else if( settings_current.simpleide_active ) {
      error = simpleide_insert( filename, LIBSPECTRUM_IDE_MASTER );
    } else if( settings_current.divide_enabled ) {
      error = divide_insert( filename, LIBSPECTRUM_IDE_MASTER );
    } else if( settings_current.zxmmc_enabled ) {
      error = zxmmc_insert( filename );
    } else {
      error = divmmc_insert( filename );
    }
    if( error ) return error;
    
    break;

  case LIBSPECTRUM_CLASS_AUXILIARY:
    if( type == LIBSPECTRUM_ID_AUX_POK ) {
      ui_pokemem_selector( filename );
    }
    break;

  default:
    ui_error( UI_ERROR_ERROR, "utils_open_file: unknown class %d", type );
    error = 1;
    break;
  }

  if( error ) { utils_close_file( &file ); return error; }

  utils_close_file( &file );

  if( type_ptr ) *type_ptr = type;

  return 0;
}

/* Request a snapshot file from the user and it */
int
utils_open_snap( void )
{
  char *filename;
  int error;

  filename = ui_get_open_filename( "Fuse - Load Snapshot" );
  if( !filename ) return -1;

  error = snapshot_read( filename );
  libspectrum_free( filename );
  return error;
}

/* Find the auxiliary file called `filename'; returns a fd for the
   file on success, -1 if it couldn't find the file */
static compat_fd
utils_find_auxiliary_file( const char *filename, utils_aux_type type )
{
  compat_fd fd;

  char path[ PATH_MAX ];

  /* If given an absolute path, just look there */
  if( compat_is_absolute_path( filename ) )
    return compat_file_open( filename, 0 );

  /* Otherwise look in some likely locations */
  if( utils_find_file_path( filename, path, type ) ) {
    return COMPAT_FILE_OPEN_FAILED;
  }

  fd = compat_file_open( path, 0 );

  if( fd != COMPAT_FILE_OPEN_FAILED ) return fd;

  /* Give up. Couldn't find this file */
  return COMPAT_FILE_OPEN_FAILED;
}


int
utils_find_file_path( const char *filename, char *ret_path,
		      utils_aux_type type )
{
  path_context ctx;

  /* If given an absolute path, just look there */
  if( compat_is_absolute_path( filename ) ) {
    strncpy( ret_path, filename, PATH_MAX );
    ret_path[ PATH_MAX - 1 ] = '\0';
    return 0;
  }

  /* Otherwise look in some likely locations */
  init_path_context( &ctx, type );

  while( compat_get_next_path( &ctx ) ) {
    int bytes_written;
#ifdef AMIGA
    bytes_written = snprintf( ret_path, PATH_MAX, "%s%s", ctx.path, filename );
#else
    bytes_written = snprintf( ret_path, PATH_MAX, "%s" FUSE_DIR_SEP_STR "%s",
        ctx.path, filename );
#endif
    if( bytes_written < PATH_MAX && compat_file_exists(ret_path) ) return 0;
  }

  return 1;
}

/* Something similar to that described at
   http://www.chiark.greenend.org.uk/~sgtatham/coroutines.html was
   _very_ tempting here...
*/
static void
init_path_context( path_context *ctx, utils_aux_type type )
{
  ctx->state = 0;
  ctx->type = type;
}

int
utils_read_file( const char *filename, utils_file *file )
{
  compat_fd fd;

  int error;

  fd = compat_file_open( filename, 0 );
  if( fd == COMPAT_FILE_OPEN_FAILED ) {
    ui_error( UI_ERROR_ERROR, "couldn't open '%s': %s", filename,
	      strerror( errno ) );
    return 1;
  }

  error = utils_read_fd( fd, filename, file );
  if( error ) return error;

  return 0;
}

int
utils_read_fd( compat_fd fd, const char *filename, utils_file *file )
{
  file->length = compat_file_get_length( fd );
  if( file->length == -1 ) return 1;

  file->buffer = libspectrum_new( unsigned char, file->length );

  if( compat_file_read( fd, file ) ) {
    libspectrum_free( file->buffer );
    compat_file_close( fd );
    return 1;
  }

  if( compat_file_close( fd ) ) {
    ui_error( UI_ERROR_ERROR, "Couldn't close '%s': %s", filename,
	      strerror( errno ) );
    libspectrum_free( file->buffer );
    return 1;
  }

  return 0;
}

void
utils_close_file( utils_file *file )
{
  libspectrum_free( file->buffer );
}

int utils_write_file( const char *filename, const unsigned char *buffer,
		      size_t length )
{
  compat_fd fd;

  fd = compat_file_open( filename, 1 );
  if( fd == COMPAT_FILE_OPEN_FAILED ) {
    ui_error( UI_ERROR_ERROR, "couldn't open `%s' for writing: %s\n",
    	      filename, strerror( errno ) );
    return 1;
  }

  if( compat_file_write( fd, buffer, length ) ) {
    compat_file_close( fd );
    return 1;
  }

  if( compat_file_close( fd ) ) return 1;

  return 0;
}

int
utils_read_auxiliary_file( const char *filename, utils_file *file,
                           utils_aux_type type )
{
  int error;
  compat_fd fd;

  fd = utils_find_auxiliary_file( filename, type );
  if( fd == COMPAT_FILE_OPEN_FAILED ) return -1;

  error = utils_read_fd( fd, filename, file );
  if( error ) return error;

  return 0;
}

int
utils_read_screen( const char *filename, utils_file *screen )
{
  int error;

  error = utils_read_auxiliary_file( filename, screen, UTILS_AUXILIARY_LIB );
  if( error == -1 ) {
    ui_error( UI_ERROR_ERROR, "couldn't find screen picture ('%s')",
	      filename );
    return 1;
  }

  if( error ) return error;

  if( screen->length != STANDARD_SCR_SIZE ) {
    utils_close_file( screen );
    ui_error( UI_ERROR_ERROR, "screen picture ('%s') is not %d bytes long",
	      filename, STANDARD_SCR_SIZE );
    return 1;
  }

  return 0;
}

char*
utils_safe_strdup( const char *src )
{
  char *dest = NULL;
  if( src ) {
    size_t length = strlen( src ) + 1;
    dest = libspectrum_new( char, length );
    memcpy( dest, src, length );
  }
  return dest;
}

int
utils_save_binary( libspectrum_word start, size_t length,
                   const char *filename )
{
  libspectrum_byte *buffer;
  size_t i;
  int error = 0;

  buffer = libspectrum_new( libspectrum_byte, length );

  for( i = 0; i < length; i++ )
    buffer[ i ] = readbyte_internal( start + i );

  error = utils_write_file( filename, buffer, length );

  libspectrum_free( buffer );

  return error;
}

void
utils_networking_init( void )
{
#ifdef HAVE_SOCKETS

  if( !networking_init_count )
    compat_socket_networking_init();

#endif

  networking_init_count++;
}

void
utils_networking_end( void )
{
  networking_init_count--;

#ifdef HAVE_SOCKETS
  
  if( !networking_init_count )
    compat_socket_networking_end();

#endif
}

