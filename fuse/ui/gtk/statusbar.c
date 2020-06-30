/* statusbar.c: routines for updating the status bar
   Copyright (c) 2003-2004 Philip Kendall

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

#include <stdio.h>

#include <gtk/gtk.h>

#include "gtkcompat.h"
#include "gtkinternals.h"
#include "ui/ui.h"

static GtkWidget *status_bar;

static GdkPixbuf
  *pixbuf_tape_inactive,  *pixbuf_tape_active,
  *pixbuf_mdr_inactive,   *pixbuf_mdr_active,
  *pixbuf_disk_inactive,  *pixbuf_disk_active,
  *pixbuf_pause_inactive, *pixbuf_pause_active,
  *pixbuf_mouse_inactive, *pixbuf_mouse_active;

static GtkWidget
  *microdrive_status,	/* Is any microdrive motor running? */
  *disk_status,		/* Is the disk motor running? */
  *mouse_status,	/* Have we grabbed the mouse? */
  *pause_status,	/* Is emulation paused (via the menu option)? */
  *tape_status,		/* Is the tape running? */
  *speed_status,	/* How fast are we running? */
  *machine_name;	/* What machine is being emulated? */

int
gtkstatusbar_create( GtkBox *parent )
{
  GtkWidget *separator;

  status_bar = gtk_box_new( GTK_ORIENTATION_HORIZONTAL, 5 );
  gtk_box_pack_start( parent, status_bar, FALSE, FALSE, 3 );

  /* FIXME: unref these pixbuf on statusbar destroy */
  pixbuf_tape_inactive =
    gdk_pixbuf_new_from_xpm_data( gtkpixmap_tape_inactive );
  pixbuf_tape_active =
    gdk_pixbuf_new_from_xpm_data( gtkpixmap_tape_active );

  pixbuf_mdr_inactive =
    gdk_pixbuf_new_from_xpm_data( gtkpixmap_mdr_inactive );
  pixbuf_mdr_active =
    gdk_pixbuf_new_from_xpm_data( gtkpixmap_mdr_active );

  pixbuf_disk_inactive =
    gdk_pixbuf_new_from_xpm_data( gtkpixmap_disk_inactive );
  pixbuf_disk_active =
    gdk_pixbuf_new_from_xpm_data( gtkpixmap_disk_active );

  pixbuf_pause_inactive =
    gdk_pixbuf_new_from_xpm_data( gtkpixmap_pause_inactive );
  pixbuf_pause_active =
    gdk_pixbuf_new_from_xpm_data( gtkpixmap_pause_active );

  pixbuf_mouse_inactive =
    gdk_pixbuf_new_from_xpm_data( gtkpixmap_mouse_inactive );
  pixbuf_mouse_active =
    gdk_pixbuf_new_from_xpm_data( gtkpixmap_mouse_active );

  speed_status = gtk_label_new( "100%" );
  gtk_label_set_width_chars( GTK_LABEL( speed_status ), 8 );
  gtk_box_pack_end( GTK_BOX( status_bar ), speed_status, FALSE, FALSE, 0 );

  separator = gtk_separator_new( GTK_ORIENTATION_VERTICAL );
  gtk_box_pack_end( GTK_BOX( status_bar ), separator, FALSE, FALSE, 0 );

  tape_status = gtk_image_new_from_pixbuf( pixbuf_tape_inactive );
  gtk_box_pack_end( GTK_BOX( status_bar ), tape_status, FALSE, FALSE, 0 );

  microdrive_status = gtk_image_new_from_pixbuf( pixbuf_mdr_inactive );
  gtk_box_pack_end( GTK_BOX( status_bar ), microdrive_status, FALSE, FALSE,
		    0 );

  disk_status = gtk_image_new_from_pixbuf( pixbuf_disk_inactive );
  gtk_box_pack_end( GTK_BOX( status_bar ), disk_status, FALSE, FALSE, 0 );

  pause_status = gtk_image_new_from_pixbuf( pixbuf_pause_inactive );
  gtk_box_pack_end( GTK_BOX( status_bar ), pause_status, FALSE, FALSE, 0 );

  mouse_status = gtk_image_new_from_pixbuf( pixbuf_mouse_inactive );
  gtk_box_pack_end( GTK_BOX( status_bar ), mouse_status, FALSE, FALSE, 0 );

  separator = gtk_separator_new( GTK_ORIENTATION_VERTICAL );
  gtk_box_pack_end( GTK_BOX( status_bar ), separator, FALSE, FALSE, 0 );

  machine_name = gtk_label_new( NULL );
  gtk_box_pack_start( GTK_BOX( status_bar ), machine_name, FALSE, FALSE, 0 );

  return 0;
}

int
gtkstatusbar_get_height( void )
{
  GtkAllocation alloc;

  gtk_widget_get_allocation( status_bar, &alloc );

  return alloc.height + 6;        /* status bar + vbox padding */
}

int
gtkstatusbar_set_visibility( int visible )
{
  gtkdisplay_update_geometry();

  if( visible ) {
    gtk_widget_show( status_bar );
  } else {
    gtk_widget_hide( status_bar );
  }

  return 0;
}

void
gtkstatusbar_update_machine( const char *name )
{
  gtk_label_set_text( GTK_LABEL( machine_name ), name );
}

int
ui_statusbar_update( ui_statusbar_item item, ui_statusbar_state state )
{
  GdkPixbuf *which;

  switch( item ) {

  case UI_STATUSBAR_ITEM_DISK:
    switch( state ) {
    case UI_STATUSBAR_STATE_NOT_AVAILABLE:
      gtk_widget_hide( disk_status ); break;
    case UI_STATUSBAR_STATE_ACTIVE:
      gtk_widget_show( disk_status );
      gtk_image_set_from_pixbuf( GTK_IMAGE( disk_status ), pixbuf_disk_active );
      break;
    default:
      gtk_widget_show( disk_status );
      gtk_image_set_from_pixbuf( GTK_IMAGE( disk_status ),
                                 pixbuf_disk_inactive );
      break;
    }      
    return 0;

  case UI_STATUSBAR_ITEM_MOUSE:
    which = ( state == UI_STATUSBAR_STATE_ACTIVE ?
              pixbuf_mouse_active : pixbuf_mouse_inactive );
    gtk_image_set_from_pixbuf( GTK_IMAGE( mouse_status ), which );
    return 0;

  case UI_STATUSBAR_ITEM_PAUSED:
    which = ( state == UI_STATUSBAR_STATE_ACTIVE ?
              pixbuf_pause_active : pixbuf_pause_inactive );
    gtk_image_set_from_pixbuf( GTK_IMAGE( pause_status ), which );
    return 0;

  case UI_STATUSBAR_ITEM_MICRODRIVE:
    switch( state ) {
    case UI_STATUSBAR_STATE_NOT_AVAILABLE:
      gtk_widget_hide( microdrive_status ); break;
    case UI_STATUSBAR_STATE_ACTIVE:
      gtk_widget_show( microdrive_status );
      gtk_image_set_from_pixbuf( GTK_IMAGE( microdrive_status ),
                                 pixbuf_mdr_active );
      break;
    default:
      gtk_widget_show( microdrive_status );
      gtk_image_set_from_pixbuf( GTK_IMAGE( microdrive_status ),
                                 pixbuf_mdr_inactive );
      break;
    }      
    return 0;

  case UI_STATUSBAR_ITEM_TAPE:
    which = ( state == UI_STATUSBAR_STATE_ACTIVE ?
              pixbuf_tape_active : pixbuf_tape_inactive );
    gtk_image_set_from_pixbuf( GTK_IMAGE( tape_status ), which );

    return 0;

  }

  ui_error( UI_ERROR_ERROR, "Attempt to update unknown statusbar item %d",
	    item );
  return 1;
}

int
ui_statusbar_update_speed( float speed )
{
  char buffer[8];

  snprintf( buffer, 8, "%3.0f%%", speed );
  gtk_label_set_text( GTK_LABEL( speed_status ), buffer );

  return 0;
}
