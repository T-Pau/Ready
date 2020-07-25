/* pokemem.c: The poke memory widget
   Copyright (c) 2011 Philip Kendall, Sergio Baldoví
   Copyright (c) 2015 Adrien Destugues

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
#include <stdio.h>
#include <string.h>

#include "fuse.h"
#include "menu.h"
#include "pokefinder/pokemem.h"
#include "ui/ui.h"
#include "widget.h"
#include "widget_internals.h"

typedef struct entry_t {
    int checked;
    trainer_t *trainer;
} entry_t;

const char *pokemem_title = "Poke memory";
const unsigned int page_size = 16;
GArray *store = NULL;
int selected = -1;
int highlight_line = 0;
unsigned int menu_left_edge_x;
unsigned int menu_width;
unsigned int pokemem_count = 0;
unsigned int top_index = 0;

unsigned int widget_pokemem_calculate_width( void );
void widget_pokemem_store_new( void );
void widget_pokemem_store_add( gpointer data, gpointer user_data );
void widget_pokemem_print_list( unsigned int left_edge, unsigned int width );
int  widget_pokemem_print_trainer( unsigned int left_edge, unsigned int width,
                                   int number, int disabled, int checked,
                                   const char *string );
void widget_pokemem_update_line( unsigned int left_edge, unsigned int width,
                                 int index );

int  widget_pokemem_trainer_click( int index );
void widget_pokemem_ask_value( trainer_t *trainer );
int  widget_pokemem_add_custom_poke( void );
void widget_pokemem_apply_pokes( void );

void
ui_pokemem_selector( const char *filename )
{
  fuse_emulation_pause();

  pokemem_read_from_file( filename );
  menu_machine_pokememory( 0 );

  fuse_emulation_unpause();
}

int widget_pokemem_finish( widget_finish_state finished )
{
  if( finished == WIDGET_FINISHED_OK ) {
    widget_pokemem_apply_pokes();
  }

  if( store ) {
    g_array_free( store, TRUE );
    store = NULL;
  }

  pokemem_count = 0;

  return 0;
}

int
widget_pokemem_draw( void *data GCC_UNUSED )
{
  if( !store ) {
    pokemem_autoload_pokfile();
    widget_pokemem_store_new();
  }

  menu_width = widget_pokemem_calculate_width();
  menu_left_edge_x = DISPLAY_WIDTH_COLS / 2 - menu_width / 2;

  widget_dialog_with_border( menu_left_edge_x, 2, menu_width, page_size + 4 );
  widget_printstring( menu_left_edge_x * 8 + 2, 16, WIDGET_COLOUR_TITLE,
                      pokemem_title );

  widget_pokemem_print_list( menu_left_edge_x, menu_width );

  widget_printstring( menu_left_edge_x * 8 + 8, ( page_size + 4 ) * 8,
                      WIDGET_COLOUR_FOREGROUND, "\x0A" "A\x01" "dd" );

  widget_display_lines( 2, page_size + 4 );

  return 0;
}

unsigned int
widget_pokemem_calculate_width( void )
{
  unsigned int i, width, max_width, text_width;
  entry_t *entry;
  trainer_t *trainer;

  if( !store ) return 25;

  max_width = 0;

  for( i = 0; i < pokemem_count; i++ ) {
    entry = &g_array_index( store, entry_t, i );
    trainer = entry->trainer;

    text_width = widget_stringwidth( trainer->name ) + 3 * 8;
    if( text_width > max_width ) max_width = text_width;
  }

  width = ( max_width + 16 ) / 8;
  if( width < 25 )
    width = 25;
  else if( width > 32 )
    width = 32;

  return width;
}

void
widget_pokemem_print_list( unsigned int left_edge, unsigned int width )
{
  char buf[32];
  unsigned int i, page_limit;
  entry_t *entry;
  trainer_t *trainer;

  if( store && pokemem_count ) {
    page_limit = top_index + page_size;

    for( i = top_index; i < pokemem_count && i < page_limit; i++ ) {
      entry = &g_array_index( store, entry_t, i );
      trainer = entry->trainer;

      snprintf( buf, sizeof( buf ), "%s", trainer->name );
      widget_pokemem_print_trainer( left_edge, width, i - top_index,
                                    trainer->disabled, entry->checked, buf );
    }

    if( top_index ) widget_up_arrow( left_edge, 3, WIDGET_COLOUR_FOREGROUND );

    if( i < pokemem_count )
      widget_down_arrow( left_edge, page_size + 2, WIDGET_COLOUR_FOREGROUND );
  }

  widget_display_lines( 3, page_size );
}

int
widget_pokemem_print_trainer( unsigned int left_edge, unsigned int width,
                              int number, int disabled, int checked,
                              const char *string )
{
  char buffer[128];
  size_t l, w;
  int colour = WIDGET_COLOUR_BACKGROUND;
  int x;
  int y;

  if( number == highlight_line ) colour = WIDGET_COLOUR_HIGHLIGHT;
  widget_rectangle( left_edge * 8 + 1, number * 8 + 24, width * 8 - 2, 1 * 8,
                    colour );

  snprintf( buffer, sizeof( buffer ), "%s", string );
  l = strlen( buffer );

  if( l >= sizeof( buffer ) )
    l = sizeof( buffer ) - 1;
  while( ( w = widget_substringwidth( string, l ) ) >= ( left_edge+width-2 )*8 )
    --l;
  buffer[l] = '\0';
  w = widget_printstring( left_edge * 8 + 9, number * 8 + 24,
                          WIDGET_COLOUR_FOREGROUND, buffer ) - 1;
  while( ( w += 3 ) < ( left_edge + width - 1 ) * 8 - 2 )
    widget_putpixel( w, number * 8 + 31, 0 );

  /* print check */
  x = ( left_edge + width - 2 ) * 8 - 2;
  y = number * 8 + 24;

  widget_rectangle( x, y, 8, 8, colour );
  widget_print_checkbox( x, y,
                         ( disabled )? WIDGET_COLOUR_FOREGROUND : colour,
                         checked );
  widget_display_rasters( y, 8 );

  return 0;
}

void
widget_pokemem_update_line( unsigned int left_edge, unsigned int width,
                            int index )
{
  char buf[32];
  entry_t *entry;
  trainer_t *trainer;

  if( !store ) return;

  entry = &g_array_index( store, entry_t, index );
  trainer = entry->trainer;

  snprintf( buf, sizeof( buf ), "%s", trainer->name );
  widget_pokemem_print_trainer( left_edge, width, index - top_index,
                                trainer->disabled, entry->checked, buf );

  /* First row in page */
  if( top_index && index == top_index )
    widget_up_arrow( left_edge, 3, WIDGET_COLOUR_FOREGROUND );

  /* Last row in page */
  if( top_index + page_size < pokemem_count && 
      ( index - top_index == page_size - 1 ) )
    widget_down_arrow( left_edge, page_size + 2, WIDGET_COLOUR_FOREGROUND );
}

void
widget_pokemem_keyhandler( input_key key )
{
  int new_selected;

  new_selected = selected;

  switch ( key ) {
  case INPUT_KEY_Return: /* Do pokes */
  case INPUT_KEY_KP_Enter:
  case INPUT_JOYSTICK_FIRE_1:
    widget_end_all( WIDGET_FINISHED_OK );
    break;

  case INPUT_KEY_Escape: /* Close widget */
  case INPUT_JOYSTICK_FIRE_2:
    widget_end_widget( WIDGET_FINISHED_CANCEL );
    return;

  case INPUT_KEY_a: /* Add poke */
    if( !widget_pokemem_add_custom_poke() ) new_selected = pokemem_count - 1;
    break;

  case INPUT_KEY_Home:
    new_selected = 0;
    break;

  case INPUT_KEY_End:
    new_selected = pokemem_count - 1;
    break;

  case INPUT_KEY_Page_Up:
    new_selected = ( selected > page_size )? selected - page_size : 0;
    break;

  case INPUT_KEY_Page_Down:
    new_selected = selected + page_size;
    if( new_selected >= pokemem_count ) new_selected = pokemem_count - 1;
    break;

  case INPUT_KEY_Up:
  case INPUT_KEY_7:
  case INPUT_JOYSTICK_UP:
    if( selected ) new_selected = selected - 1;
    break;

  case INPUT_KEY_Down:
  case INPUT_KEY_6:
  case INPUT_JOYSTICK_DOWN:
    if( selected + 1 < pokemem_count ) new_selected = selected + 1;
    break;

  case INPUT_KEY_space:
  case INPUT_KEY_8:
  case INPUT_JOYSTICK_RIGHT:
    if( !widget_pokemem_trainer_click( selected ) )
      widget_pokemem_update_line( menu_left_edge_x, menu_width, selected );
    return;

  default:;
  }

  if( store && new_selected != selected ) {
    if( new_selected < top_index ) {
      top_index = new_selected;
      highlight_line = new_selected - top_index;
      widget_pokemem_print_list( menu_left_edge_x, menu_width );
    }
    else if( new_selected >= top_index + page_size ) {
      top_index = new_selected - page_size + 1;
      highlight_line = new_selected - top_index;
      widget_pokemem_print_list( menu_left_edge_x, menu_width );
    } else {
      /* Otherwise, print the current trainer uninverted and the
         new current trainer inverted */
      highlight_line = new_selected - top_index;
      if( selected >= 0 )
        widget_pokemem_update_line( menu_left_edge_x, menu_width, selected );
      widget_pokemem_update_line( menu_left_edge_x, menu_width, new_selected );
      widget_display_lines( 3, page_size );
    }

    selected = new_selected;
  }
}

void
widget_pokemem_store_new( void )
{
  if( !trainer_list ) return;

  store = g_array_new( FALSE, FALSE, sizeof( entry_t ) );

  if( store ) {
    g_slist_foreach( trainer_list, widget_pokemem_store_add, NULL );
    pokemem_count = store->len;
  }

  /* Adjust default selection to current data */
  if( pokemem_count == 0 ) {
    selected = -1;
    top_index = 0;
    highlight_line = 0;
  }
  else if( selected > pokemem_count ) {
    selected = 0;
    top_index = 0;
    highlight_line = 0;
  }
}

void
widget_pokemem_store_add( gpointer data, gpointer user_data GCC_UNUSED )
{
  trainer_t *trainer = data;
  entry_t entry;

  if( !trainer ) return;

  /* Append a new row and fill data */
  entry.checked = trainer->active;
  entry.trainer = trainer;
  g_array_append_vals( store, &entry, 1 );
}

void
widget_pokemem_apply_pokes( void )
{
  entry_t *entry;
  trainer_t *trainer;
  unsigned int i;

  if( !store ) return;

  for( i = 0; i < store->len; i++ ) {
    entry = &g_array_index( store, entry_t, i );
    trainer = entry->trainer;

    if( entry->checked ) {
      pokemem_trainer_activate( trainer );
    } else {
      pokemem_trainer_deactivate( trainer );
    }
  }
}

int
widget_pokemem_trainer_click( int index )
{
  entry_t *entry;
  trainer_t *trainer;

  if( !store ) return 1;

  /* Disable incomplete trainers or active without restore value */
  entry = &g_array_index( store, entry_t, index );
  trainer = entry->trainer;
  if( trainer->disabled ) return 1;

  /* Toggle current selection */
  entry->checked = !entry->checked;
  widget_pokemem_update_line( menu_left_edge_x, menu_width, selected );

  /* Request user for custom value */
  if( entry->checked && trainer->ask_value ) {
    widget_pokemem_ask_value( trainer );
  }

  return 0;
}

void
widget_pokemem_ask_value( trainer_t *trainer )
{
  int value;
  widget_text_t text_data;

  text_data.title = "Enter trainer value";
  text_data.allow = WIDGET_INPUT_DIGIT;
  text_data.max_length = 3;
  snprintf( text_data.text, sizeof( text_data.text ), "%d", trainer->value );
  widget_do_text( &text_data );

  if( widget_text_text ) {
    value = atoi( widget_text_text );
    trainer->value = ( value > 255 )? 0 : value;
  }
}

int
widget_pokemem_add_custom_poke( void )
{
  long b, a, v;
  trainer_t *trainer;
  entry_t entry;
  widget_text_t text_data;
  char *endptr;

  /* Bank */
  memset( &text_data, 0, sizeof( widget_text_t ) );
  text_data.title = "Enter bank (optional)";
  text_data.allow = WIDGET_INPUT_DIGIT;
  text_data.max_length = 1;
  if( widget_do_text( &text_data ) ) return 1;

  if( !widget_text_text ) return 1;

  errno = 0;
  b = strtol( widget_text_text, &endptr, 10 );

  if( errno || b < 0 || b > 8 ) {
    ui_error( UI_ERROR_ERROR, "Invalid bank: use an integer from 0 to 8" );
    return 1;
  }

  if( endptr == widget_text_text ) b = 8; /* ignore bank by default */

  /* Address */
  text_data.title = "Enter address / offset";
  text_data.max_length = 5;
  if( widget_do_text( &text_data ) ) return 1;

  if( !widget_text_text ) return 1;

  errno = 0;
  a = strtol( widget_text_text, &endptr, 10 );

  if( errno || a < 0 || a > 65535 || endptr == widget_text_text ) {
    ui_error( UI_ERROR_ERROR,
              "Invalid address: use an integer from 0 to 65535" );
    return 1;
  }

  if( b == 8 && a < 16384 ) {
    ui_error( UI_ERROR_ERROR,
              "Invalid address: use an integer from 16384 to 65535" );
    return 1;
  }

  /* Value */
  text_data.title = "Enter value";
  text_data.max_length = 3;
  if( widget_do_text( &text_data ) ) return 1;

  if( !widget_text_text ) return 1;

  errno = 0;
  v = strtol( widget_text_text, &endptr, 10 );

  if( errno || v < 0 || v > 256 || endptr == widget_text_text ) {
    ui_error( UI_ERROR_ERROR, "Invalid value: use an integer from 0 to 256" );
    return 1;
  }

  trainer = pokemem_trainer_list_add( b, a, v );
  if( !trainer ) {
    ui_error( UI_ERROR_ERROR, "Cannot add trainer" );
    return 1;
  }

  /* Append a new row and fill data */
  entry.trainer = trainer;
  entry.checked = trainer->active;

  if( !trainer->active && !trainer->disabled && !trainer->ask_value ) {
    entry.checked = 1;
  }

  if( !store ) {
    store = g_array_new( FALSE, FALSE, sizeof( entry_t ) );
    if( !store ) return 1;
  }

  g_array_append_vals( store, &entry, 1 );
  pokemem_count = store->len;

  return 0;
}
