/* memory.c: the GTK+ memory browser
   Copyright (c) 2004-2005 Philip Kendall
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

#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>

#include "compat.h"
#include "fuse.h"
#include "gtkcompat.h"
#include "gtkinternals.h"
#include "memory_pages.h"
#include "menu.h"
#include "ui/ui.h"

#define VIEW_NUM_ROWS 20
#define VIEW_NUM_COLS 16

static libspectrum_word memaddr = 0x0000;

static libspectrum_dword mark_offset = 0xffffffff;

static int cursor_line_number = -1, cursor_char_offset;
static GtkTextBuffer *cursor_buffer;

static GtkTextBuffer *buffer_address, *buffer_hex, *buffer_data;
static GtkAdjustment *adjustment;

static gboolean
textview_wheel_scroll_event( GtkWidget *widget, GdkEvent *event, gpointer user_data )
{
  GtkAdjustment *adjustment = user_data;
  gdouble base, oldbase, base_limit;

  base = oldbase = gtk_adjustment_get_value( adjustment );

  switch( event->scroll.direction )
  {
  case GDK_SCROLL_UP:
    base -= gtk_adjustment_get_page_increment( adjustment ) / 2;
    break;
  case GDK_SCROLL_DOWN:
    base += gtk_adjustment_get_page_increment( adjustment ) / 2;
    break;

#if GTK_CHECK_VERSION( 3, 4, 0 )
  case GDK_SCROLL_SMOOTH:
    {
      static gdouble total_dy = 0;
      gdouble dx, dy, page_size;
      int delta;

      if( gdk_event_get_scroll_deltas( event, &dx, &dy ) ) {
        total_dy += dy;
        page_size = gtk_adjustment_get_page_size( adjustment );
        delta = total_dy * pow( page_size, 2.0 / 3.0 );

        /* Is movement significative? */
        if( delta ) {
          base += delta;
          total_dy = 0;
        }
      }
      break;
    }
#endif

  default:
    return FALSE;
  }

  if( base < 0 ) {
    base = 0;
  } else {
    base_limit = gtk_adjustment_get_upper( adjustment ) -
                 gtk_adjustment_get_page_size( adjustment );
    if( base > base_limit ) base = base_limit;
  }

  if( base != oldbase ) {
    gtk_adjustment_set_value( adjustment, base );
  }

  return TRUE;
}

static gboolean
textview_key_press_event( GtkWidget *widget, GdkEventKey *event, gpointer user_data )
{
  GtkAdjustment *adjustment = user_data;
  GtkTextBuffer *text_buffer;
  GtkTextIter iter;
  GtkTextMark *mark;
  gdouble base, oldbase, base_limit;
  gdouble page_size, step_increment;
  gint line, line_offset;
  int num_rows, line_width;

  base = oldbase = gtk_adjustment_get_value( adjustment );
  page_size = gtk_adjustment_get_page_size( adjustment );
  step_increment = gtk_adjustment_get_step_increment( adjustment );
  num_rows = ( page_size + 1 ) / step_increment;

  text_buffer = gtk_text_view_get_buffer( GTK_TEXT_VIEW( widget ) );

  /* Get line width (includes CR/LF) */
  line_width = gtk_text_buffer_get_char_count( text_buffer ) /
               gtk_text_buffer_get_line_count( text_buffer );

  /* Get row and offset of cursor */
  mark = gtk_text_buffer_get_insert( text_buffer );
  gtk_text_buffer_get_iter_at_mark( text_buffer, &iter, mark );
  line = gtk_text_iter_get_line( &iter );
  line_offset = gtk_text_iter_get_line_offset( &iter );

  switch( event->keyval )
  {

  case GDK_KEY_Left:
    if( line == 0 && line_offset == 0 ) {
      base -= step_increment;
      cursor_buffer = text_buffer;
      cursor_line_number = line;
      cursor_char_offset = line_width;
    }
    break;

  case GDK_KEY_Right:
    if( line == num_rows - 1 && line_offset == line_width ) {
      base += step_increment;
      cursor_buffer = text_buffer;
      cursor_line_number = line;
      cursor_char_offset = 0;
    }
    break;

  case GDK_KEY_Up:
    if( line == 0 ) {
      base -= step_increment;
      cursor_buffer = text_buffer;
      cursor_line_number = 0;
      cursor_char_offset = line_offset;
    }
    break;

  case GDK_KEY_Down:
    if( line == num_rows - 1 ) {
      base += step_increment;
      cursor_buffer = text_buffer;
      cursor_line_number = line;
      cursor_char_offset = line_offset;
    }
    break;

  case GDK_KEY_Page_Up:
    base -= gtk_adjustment_get_page_increment( adjustment );
    cursor_buffer = text_buffer;
    cursor_line_number = line;
    cursor_char_offset = line_offset;
    break;

  case GDK_KEY_Page_Down:
    base += gtk_adjustment_get_page_increment( adjustment );
    cursor_buffer = text_buffer;
    cursor_line_number = line;
    cursor_char_offset = line_offset;
    break;

  default:
    return FALSE;
  }

  if( base < 0 ) {
    base = 0;
  } else {
    base_limit = gtk_adjustment_get_upper( adjustment ) - page_size;
    if( base > base_limit ) base = base_limit;
  }

  if( base != oldbase ) {
    gtk_adjustment_set_value( adjustment, base );

    /* As we are simulating a full-filled text view, we need to move the cursor
       position after page movement/redraw */
    if( cursor_line_number >= 0 && cursor_line_number < VIEW_NUM_ROWS ) {
      gtk_text_buffer_get_iter_at_line_offset( cursor_buffer, &iter,
                                               cursor_line_number,
                                               cursor_char_offset );
      gtk_text_buffer_place_cursor( cursor_buffer, &iter );
      cursor_line_number = -1;
    }

    return TRUE;
  }

  return FALSE;
}

static void
update_display( libspectrum_word base )
{
  size_t i, j;
  char buffer2[ 8 ];
  char buffer3;
  GtkTextIter iter_address, iter_hex, iter_data, start, end;

  memaddr = base;

  gtk_text_buffer_get_bounds( buffer_address, &start, &end );
  gtk_text_buffer_delete( buffer_address, &start, &end );
  gtk_text_buffer_get_bounds( buffer_hex, &start, &end );
  gtk_text_buffer_delete( buffer_hex, &start, &end );
  gtk_text_buffer_get_bounds( buffer_data, &start, &end );
  gtk_text_buffer_delete( buffer_data, &start, &end );

  gtk_text_buffer_get_start_iter( buffer_address, &iter_address );
  gtk_text_buffer_get_start_iter( buffer_hex, &iter_hex );
  gtk_text_buffer_get_start_iter( buffer_data, &iter_data );

  for( i = 0; i < VIEW_NUM_ROWS; i++ ) {
    if( i > 0 ) {
      gtk_text_buffer_insert( buffer_address, &iter_address, "\n", -1 );
      gtk_text_buffer_insert( buffer_hex, &iter_hex, "\n", -1 );
      gtk_text_buffer_insert( buffer_data, &iter_data, "\n", -1 );
    }

    snprintf( buffer2, 8, "%04X", base );
    gtk_text_buffer_insert( buffer_address, &iter_address, buffer2, -1 );

    for( j = 0; j < VIEW_NUM_COLS; j++, base++ ) {
      if( j > 0 )
        gtk_text_buffer_insert( buffer_hex, &iter_hex, " ", -1 );

      libspectrum_byte b = readbyte_internal( base );
      snprintf( buffer2, 4, "%02X", b );

      buffer3 = ( b >= 32 && b < 127 ) ? b : '.';

      if( base != mark_offset ) {
        gtk_text_buffer_insert( buffer_hex, &iter_hex, buffer2, -1 );
        gtk_text_buffer_insert( buffer_data, &iter_data, &buffer3, 1 );
      } else {
        gtk_text_buffer_insert_with_tags_by_name( buffer_hex, &iter_hex,
          buffer2, -1, "background_yellow", NULL );
        gtk_text_buffer_insert_with_tags_by_name( buffer_data, &iter_data,
          &buffer3, 1, "background_yellow", NULL );
      }
    }
  }

  gtk_text_buffer_get_bounds( buffer_address, &start, &end );
  gtk_text_buffer_apply_tag_by_name( buffer_address, "monospace", &start, &end );
  gtk_text_buffer_get_bounds( buffer_hex, &start, &end );
  gtk_text_buffer_apply_tag_by_name( buffer_hex, "monospace", &start, &end );
  gtk_text_buffer_get_bounds( buffer_data, &start, &end );
  gtk_text_buffer_apply_tag_by_name( buffer_data, "monospace", &start, &end );
}

static void
scroller( GtkAdjustment *adjustment, gpointer user_data )
{
  libspectrum_word base;

  /* Drop the low bits before displaying anything */
  base = gtk_adjustment_get_value( adjustment );
  base &= 0xfff0;

  update_display( base );
}

#if GTK_CHECK_VERSION( 3, 6, 0 )
static void
goto_offset( GtkWidget *widget GCC_UNUSED, gpointer user_data GCC_UNUSED )
{
  long offset;
  const gchar *entry;
  char *endptr;
  int base_num;

  if( gtk_entry_get_text_length( GTK_ENTRY( widget ) ) == 0 )
     return;

  /* Parse address */
  entry = gtk_entry_get_text( GTK_ENTRY( widget ) );
  errno = 0;
  base_num = ( g_str_has_prefix( entry, "0x" ) )? 16 : 10;
  offset = strtol( entry, &endptr, base_num );

  /* Validate address */
  if( errno || offset < 0 || offset > 65535 || endptr == entry ||
      *endptr != '\0' ) {
    return;
  }

  mark_offset = offset;
  gtk_adjustment_set_value( adjustment, offset );
}
#endif

void
menu_machine_memorybrowser( GtkAction *gtk_action GCC_UNUSED,
                            gpointer data GCC_UNUSED )
{
  GtkWidget *dialog, *content_area, *scrollbar, *label, *offset;
  GtkWidget *box, *box_address, *box_hex, *box_data, *box_data_horizontal;
  GtkAccelGroup *accel_group;
  GtkWidget *view_address, *view_hex, *view_data;
  GtkTextTagTable *tag_table;
  GtkTextTag *tag;

  fuse_emulation_pause();

  dialog = gtkstock_dialog_new( "Fuse - Memory Browser", NULL );
  content_area = gtk_dialog_get_content_area( GTK_DIALOG( dialog ) );

  /* Keyboard shortcuts */
  accel_group = gtk_accel_group_new();
  gtk_window_add_accel_group( GTK_WINDOW( dialog ), accel_group );

  gtkstock_create_close( dialog, accel_group, NULL, TRUE );

#if GTK_CHECK_VERSION( 3, 6, 0 )
  /* Go to offset */
  box = gtk_box_new( GTK_ORIENTATION_HORIZONTAL, 8 );
  offset = gtk_search_entry_new();
  /*
   * Entry is max 6 chars wide ("0xXXXX") but the GTK widget adds
   * a search icon and "clear contents" icon, which between them
   * take up about 6 char's widths. So it needs to be wider.
   */
  gtk_entry_set_width_chars( GTK_ENTRY( offset ), 15 );
  gtk_entry_set_max_length( GTK_ENTRY( offset ), 6 );
  gtk_box_pack_end( GTK_BOX( box ), offset, FALSE, FALSE, 0 );

  label = gtk_label_new( "Go to offset" );
  gtk_box_pack_end( GTK_BOX( box ), label, FALSE, FALSE, 0 );

  gtk_box_pack_start( GTK_BOX( content_area ), box, FALSE, FALSE, 8 );

  g_signal_connect( G_OBJECT( offset ), "activate",
                    G_CALLBACK( goto_offset ), NULL );
#endif

  /* Create text buffers */
  tag_table = gtk_text_tag_table_new();
  tag = gtk_text_tag_new( "monospace" );
  g_object_set( tag, "family", "monospace", NULL );
  gtk_text_tag_table_add( tag_table, tag );

  tag = gtk_text_tag_new( "background_yellow" );
  g_object_set( tag, "background", "yellow",
                     "background-full-height", TRUE, NULL );
  gtk_text_tag_table_add( tag_table, tag );

  buffer_address = gtk_text_buffer_new( tag_table );
  buffer_hex = gtk_text_buffer_new( tag_table );
  buffer_data = gtk_text_buffer_new( tag_table );

  /* Labels */
  box = gtk_box_new( GTK_ORIENTATION_HORIZONTAL, 8 );
  box_address = gtk_box_new( GTK_ORIENTATION_VERTICAL, 0 );
  gtk_box_pack_start( GTK_BOX( box ), box_address, FALSE, FALSE, 0 );
  box_hex = gtk_box_new( GTK_ORIENTATION_VERTICAL, 0 );
  gtk_box_pack_start( GTK_BOX( box ), box_hex, FALSE, FALSE, 0 );
  box_data = gtk_box_new( GTK_ORIENTATION_VERTICAL, 0 );
  gtk_box_pack_start( GTK_BOX( box ), box_data, FALSE, FALSE, 0 );
  gtk_box_pack_start( GTK_BOX( content_area ), box, FALSE, FALSE, 0 );

  label = gtk_label_new( "Address" );
  gtk_box_pack_start( GTK_BOX( box_address ), label, FALSE, FALSE, 0 );
  label = gtk_label_new( "Hex" );
  gtk_box_pack_start( GTK_BOX( box_hex ), label, FALSE, FALSE, 0 );
  label = gtk_label_new( "Data" );
  gtk_box_pack_start( GTK_BOX( box_data ), label, FALSE, FALSE, 0 );

  /* Add views */
  view_address = gtk_text_view_new_with_buffer( buffer_address );
  gtk_text_view_set_editable( GTK_TEXT_VIEW( view_address ), FALSE );
  gtk_text_view_set_left_margin( GTK_TEXT_VIEW( view_address ), 1 );
  gtk_text_view_set_right_margin( GTK_TEXT_VIEW( view_address ), 1 );
  view_hex = gtk_text_view_new_with_buffer( buffer_hex );
  gtk_text_view_set_editable( GTK_TEXT_VIEW( view_hex ), FALSE );
  gtk_text_view_set_left_margin( GTK_TEXT_VIEW( view_hex ), 1 );
  gtk_text_view_set_right_margin( GTK_TEXT_VIEW( view_hex ), 1 );
  view_data = gtk_text_view_new_with_buffer( buffer_data );
  gtk_text_view_set_editable( GTK_TEXT_VIEW( view_data ), FALSE );
  gtk_text_view_set_left_margin( GTK_TEXT_VIEW( view_data ), 1 );
  gtk_text_view_set_right_margin( GTK_TEXT_VIEW( view_data ), 1 );

#if GTK_CHECK_VERSION( 3, 16, 0 )
  gtk_text_view_set_monospace( GTK_TEXT_VIEW( view_address ), TRUE );
  gtk_text_view_set_monospace( GTK_TEXT_VIEW( view_hex ), TRUE );
  gtk_text_view_set_monospace( GTK_TEXT_VIEW( view_data ), TRUE );
#endif

  gtk_box_pack_start( GTK_BOX( box_address ), view_address, FALSE, FALSE, 0 );
  gtk_box_pack_start( GTK_BOX( box_hex ), view_hex, FALSE, FALSE, 0 );
  box_data_horizontal = gtk_box_new( GTK_ORIENTATION_HORIZONTAL, 0 );
  gtk_box_pack_start( GTK_BOX( box_data ), box_data_horizontal, FALSE, FALSE, 0 );
  gtk_box_pack_start( GTK_BOX( box_data_horizontal ), view_data, FALSE, FALSE, 0 );

  /* Scroll */
  adjustment = GTK_ADJUSTMENT(
    gtk_adjustment_new( memaddr, 0x0000, 0xffff, VIEW_NUM_COLS,
                        ( VIEW_NUM_ROWS * VIEW_NUM_COLS ) / 2,
                        ( VIEW_NUM_ROWS * VIEW_NUM_COLS ) - 1 ) );
  g_signal_connect( adjustment, "value-changed", G_CALLBACK( scroller ), NULL );
  scrollbar = gtk_scrollbar_new( GTK_ORIENTATION_VERTICAL, adjustment );
  gtk_box_pack_start( GTK_BOX( box_data_horizontal ), scrollbar, FALSE, FALSE, 0 );

  /* Allow scroll on text views */
  g_signal_connect( view_address, "scroll-event",
                    G_CALLBACK( textview_wheel_scroll_event ), adjustment );
  g_signal_connect( view_hex, "scroll-event",
                    G_CALLBACK( textview_wheel_scroll_event ), adjustment );
  g_signal_connect( view_data, "scroll-event",
                    G_CALLBACK( textview_wheel_scroll_event ), adjustment );
  g_signal_connect( view_address, "key-press-event",
                    G_CALLBACK( textview_key_press_event ), adjustment );
  g_signal_connect( view_hex, "key-press-event",
                    G_CALLBACK( textview_key_press_event ), adjustment );
  g_signal_connect( view_data, "key-press-event",
                    G_CALLBACK( textview_key_press_event ), adjustment );

  update_display( memaddr );

  gtk_widget_show_all( dialog );
  gtk_main();

  fuse_emulation_unpause();

  return;
}
