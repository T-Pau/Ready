/* aosound.c: libao sound I/O
   Copyright (c) 2004,2013 Gergely Szasz, Philip Kendall
   Copyright (c) 2015 Sergio Baldoví

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

   Philip: philip-fuse@shadowmagic.org.uk

   Gergely: szaszg@hu.inter.net

*/

#include <config.h>

#include <string.h>

#include <ao/ao.h>

#include "settings.h"
#include "sound.h"
#include "ui/ui.h"
#include "utils.h"

static ao_device *dev_for_ao;
static int sixteenbit = 1;
static char *filename = NULL;
static const char * const default_filename = "fuse-sound.ao";
static int first_init = 1;

static void
driver_error( void )
{
  switch( errno ) {
  case AO_ENODRIVER:
    ui_error( UI_ERROR_ERROR,
	      "ao: no driver corresponds to driver_id." );
    break;
  case AO_ENOTLIVE:
    ui_error( UI_ERROR_ERROR,
	      "ao: driver is not a live output device." );
    break;
  case AO_ENOTFILE:
    ui_error( UI_ERROR_ERROR,
	      "ao: driver is not a file output driver." );
    break;
  case AO_EBADOPTION:
    ui_error( UI_ERROR_ERROR,
	      "ao: a valid option key has an invalid value." );
    break;
  case AO_EOPENDEVICE:
    ui_error( UI_ERROR_ERROR,
	      "ao: cannot open output device." );
    break;
  case AO_EOPENFILE:
    ui_error( UI_ERROR_ERROR,
	      "ao: cannot open output file '%s'.", filename );
    break;
  case AO_EFILEEXISTS:
    ui_error( UI_ERROR_ERROR, "ao: output file '%s' already exists.",
	      filename );
    break;
  case AO_EFAIL:
    ui_error( UI_ERROR_ERROR, "ao: unspecified error." );
  }
}

static int
parse_driver_options( const char *device, int *driver_id, ao_option **options )
{
  char *mutable, *option, *key, *value;

  /* Get a copy of the device string we can modify */
  if( !device || *device == '\0' )
    return 1;

  mutable = utils_safe_strdup( device );

  /* First, find the device name */
  option = strchr( mutable, ':' );
  if( option ) *option++ = '\0';

  if( *mutable )				/* ! \0 */
    *driver_id = ao_driver_id( mutable );

  /* Now parse any further options */
  while( option ) {

    key = option;

    option = strchr( option, ',' );
    if( option ) *option++ = '\0';

    value = strchr( key, '=' );
    if( value ) *value++ = '\0';

    if( strcmp( key, "file" ) == 0 ) {
      filename = utils_safe_strdup( value );
    } else if( key && value) {
      ao_append_option( options, key, value );
    } else if ( first_init ) {
	ui_error( UI_ERROR_ERROR, "ignoring badly formed libao option (%s%s)",
		key ? key : "", value ? value : "" );
    }

  }

  libspectrum_free( mutable );
  return 0;
}

int
sound_lowlevel_init( const char *device, int *freqptr, int *stereoptr )
{
  ao_option *options = NULL;
  ao_info *driver_info = NULL;
  int driver_id = -1;
  static ao_sample_format format = { .bits = 0 };

  /* To prevent recursive errors */
  static int sound_lowlevel_init_in_progress = 0;

  int error;

  if( sound_lowlevel_init_in_progress ) return 0;

  sound_lowlevel_init_in_progress = 1;

  ao_initialize();

  error = parse_driver_options( device, &driver_id, &options );
  if( error ) {
    settings_current.sound = 0;
    sound_lowlevel_init_in_progress = 0;
    return error;
  }

  if( driver_id == -1 )
    driver_id = ao_default_driver_id();

  if( driver_id == -1 ) {
    ui_error( UI_ERROR_ERROR, "ao: driver '%s' unknown",
              device );
    settings_current.sound = 0;
    sound_lowlevel_init_in_progress = 0;
    return 1;
  }

  driver_info = ao_driver_info( driver_id );
  
  if( driver_info->type == AO_TYPE_FILE &&
      format.bits != 0 ) {	/* OK. We not want to trunc the file :-) */
    ui_error( UI_ERROR_WARNING, "ao: must truncate audio file '%s'",
	      filename );
  }

  format.channels = *stereoptr ? 2 : 1;
  format.rate = *freqptr;
  format.bits = settings_current.sound_force_8bit ? 8 : 16;
  format.byte_format = AO_FMT_LITTLE;
  sixteenbit = settings_current.sound_force_8bit ? 0 : 1;
  
  if( driver_info->type == AO_TYPE_LIVE ) {

    dev_for_ao = ao_open_live( driver_id, &format, options);

  } else {

    if( !filename ) filename = (char *)default_filename;
    dev_for_ao = ao_open_file( driver_id, filename, 1, &format, options);

  }

  if( !dev_for_ao ) {
    driver_error();
    settings_current.sound = 0;
    sound_lowlevel_init_in_progress = 0;
    return 1;
  }

  ao_free_options( options );
  
  sound_lowlevel_init_in_progress = 0;
  first_init = 0;

  return 0;
}

void
sound_lowlevel_end( void )
{
  if( filename != default_filename ) libspectrum_free( filename );
  ao_close(dev_for_ao);
  ao_shutdown();
}

void
sound_lowlevel_frame( libspectrum_signed_word *data, int len )
{
  static signed char buf8[4096];
  void *data8 = data;

  len <<= 1;	/* now in bytes */

  if( !sixteenbit ) {
    libspectrum_signed_word *src;
    signed char *dst;
    int f;

    src = data;
    dst = buf8;
    len >>= 1;
    for( f = 0; f < len; f++)
      *dst++ = ( int )( ( *src++ ) / 256 );
  
    data8 = buf8;
  }

  ao_play( dev_for_ao, data8, len );
}
