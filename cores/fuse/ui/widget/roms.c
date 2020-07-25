/* roms.c: select ROMs widget
   Copyright (c) 2003-2014 Philip Kendall
   Copyright (c) 2015 Stuart Brady

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

#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "fuse.h"
#include "settings.h"
#include "widget_internals.h"

static settings_info *widget_settings;
static widget_roms_info *info;

static size_t first_rom, rom_count;
int is_peripheral;

static void print_rom( int which );

int
widget_roms_draw( void *data )
{
  int i;
  char buffer[32];
  char key[] = "\x0A ";

  if( data ) info = data;

  /* Get a copy of the current settings */
  if( !info->initialised ) {

    widget_settings = malloc( sizeof( settings_info ) );
    if( !widget_settings ) {
      ui_error( UI_ERROR_ERROR, "out of memory at %s:%d", __FILE__, __LINE__ );
      return 1;
    }

    memset( widget_settings, 0, sizeof( settings_info ) );
    settings_copy( widget_settings, &settings_current );

    info->initialised = 1;
  }

  first_rom = info->start;
  rom_count = info->count;
  is_peripheral = info->is_peripheral;

  /* Blank the main display area */
  widget_dialog_with_border( 1, 2, 30, rom_count + 2 );

  widget_printstring( 10, 16, WIDGET_COLOUR_TITLE, info->title );
  widget_display_lines( 2, rom_count + 2 );

  for( i=0; i < info->count; i++ ) {

    snprintf( buffer, sizeof( buffer ), "ROM %d:", i );
    key[1] = 'A' + i;
    widget_printstring_right( 24, i*8+24, WIDGET_COLOUR_FOREGROUND, key );
    widget_printstring( 28, i*8+24, WIDGET_COLOUR_FOREGROUND, buffer );

    print_rom( i );
  }

  return 0;
}

static void
print_rom( int which )
{
  const char *setting;

  setting = *( settings_get_rom_setting( widget_settings,
					 which + first_rom, is_peripheral ) );
  while( widget_stringwidth( setting ) >= 232 - 68 )
    ++setting;

  widget_rectangle( 68, which * 8 + 24, 232 - 68, 8,
		    WIDGET_COLOUR_BACKGROUND );
  widget_printstring (68, which * 8 + 24,
				   WIDGET_COLOUR_FOREGROUND, setting );
  widget_display_rasters( which * 8 + 24, 8 );
}

void
widget_roms_keyhandler( input_key key )
{
  switch( key ) {

#if 0
  case INPUT_KEY_Resize:	/* Fake keypress used on window resize */
    widget_roms_draw( NULL );
    break;
#endif

  case INPUT_KEY_Escape:
    widget_end_widget( WIDGET_FINISHED_CANCEL );
    return;

  case INPUT_KEY_Return:
  case INPUT_KEY_KP_Enter:
    widget_end_all( WIDGET_FINISHED_OK );
    return;

  default:	/* Keep gcc happy */
    break;

  }

  if( key >= INPUT_KEY_a && key <= INPUT_KEY_z &&
      key - INPUT_KEY_a < (ptrdiff_t)rom_count ) {

    char **setting;
    char buf[32];
    widget_filesel_data data;

    key -= INPUT_KEY_a;

    snprintf( buf, sizeof( buf ), "%s - ROM %d", info->title, key );

    data.exit_all_widgets = 0;
    data.title = buf;
    widget_do_fileselector( &data );
    if( !widget_filesel_name ) return;

    setting = settings_get_rom_setting( widget_settings, key + first_rom,
					is_peripheral );
    settings_set_string( setting, widget_filesel_name );

    print_rom( key );
  }
}

int
widget_roms_finish( widget_finish_state finished )
{
  if( finished == WIDGET_FINISHED_OK ) {
    settings_copy( &settings_current, widget_settings );
  }

  settings_free( widget_settings ); free( widget_settings );
  return 0;
}
