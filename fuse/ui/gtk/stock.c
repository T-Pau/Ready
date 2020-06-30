/* stock.c: 'standard' GTK+ widgets etc
   Copyright (c) 2004 Darren Salt, Philip Kendall
   Copyright (c) 2015 Stuart Brady
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

   Philip Kendall <philip-fuse@shadowmagic.org.uk>

*/

#include <config.h>

#include <string.h>

#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>

#include "compat.h"
#include "gtkcompat.h"
#include "gtkinternals.h"

GtkAccelGroup*
gtkstock_add_accel_group( GtkWidget *widget )
{
  GtkAccelGroup *accel = gtk_accel_group_new();
  gtk_window_add_accel_group( GTK_WINDOW( widget ), accel );
  return accel;
}

static void
add_click( GtkWidget *btn, const gtkstock_button *button, GtkAccelGroup *accel,
	   guint key1, GdkModifierType mod1, guint key2, GdkModifierType mod2 )
{
  if( button->shortcut ) {
    if( button->shortcut != GDK_KEY_VoidSymbol )
      gtk_widget_add_accelerator( btn, "clicked", accel, button->shortcut,
				  button->modifier, 0 );
  } else if( key1 ) {
    gtk_widget_add_accelerator( btn, "clicked", accel, key1, mod1, 0 );
  }

  if( button->shortcut_alt ) {
    if( button->shortcut_alt != GDK_KEY_VoidSymbol )
      gtk_widget_add_accelerator( btn, "clicked", accel, button->shortcut_alt,
				  button->modifier_alt, 0 );
  }
  else if( key2 ) {
    gtk_widget_add_accelerator( btn, "clicked", accel, key2, mod2, 0 );
  }

}

GtkWidget*
gtkstock_create_button( GtkWidget *widget, GtkAccelGroup *accel,
			const gtkstock_button *button )
{
  GtkWidget *btn;
  gboolean link_object = ( button->label[0] == '!' );
  const gchar *button_label = button->label + link_object;

  if( !accel ) accel = gtkstock_add_accel_group (widget);

  if( GTK_IS_DIALOG( widget ) ) {
    btn = gtk_dialog_add_button( GTK_DIALOG( widget ), button_label,
                                 button->response_id );
  } else {
    btn = gtk_button_new_with_mnemonic( button_label );
    gtk_container_add( GTK_CONTAINER( widget ), btn );
  }

  if( button->action ) {
    if( link_object ) {
      g_signal_connect_swapped( G_OBJECT( btn ), "clicked",
				button->action,
				G_OBJECT( button->actiondata ) );
    } else {
      g_signal_connect( G_OBJECT( btn ), "clicked", button->action,
			button->actiondata );
    }
  }

  if( button->destroy )
    g_signal_connect_swapped( G_OBJECT( btn ), "clicked", button->destroy,
			      G_OBJECT( widget ) );

  GtkWidget *icon = NULL;
  if( !strcmp( button->label, "_Close" ) )
    icon = gtk_image_new_from_icon_name( "window-close", GTK_ICON_SIZE_BUTTON );    
  else if( !strcmp( button->label, "_Open" ) )
    icon = gtk_image_new_from_icon_name( "document-open",GTK_ICON_SIZE_BUTTON );
  else if( !strcmp( button->label, "_Save" ) )
    icon = gtk_image_new_from_icon_name( "document-save",GTK_ICON_SIZE_BUTTON );
  else if( !strcmp( button->label, "_Add" ) )
    icon = gtk_image_new_from_icon_name( "list-add", GTK_ICON_SIZE_BUTTON );

#if !GTK_CHECK_VERSION( 3, 0, 0 )
  /* Note: ok/cancel/yes/no icons are not covered by freedesktop's Icon Naming
     Specification 0.8.90 */
  else if( !strcmp( button->label, "_OK" ) )
    icon = gtk_image_new_from_icon_name( GTK_STOCK_OK, GTK_ICON_SIZE_BUTTON );
  else if( !strcmp( button->label, "_Cancel" ) )
    icon = gtk_image_new_from_icon_name( GTK_STOCK_CANCEL,
                                         GTK_ICON_SIZE_BUTTON );
  else if( !strcmp( button->label, "_Yes" ) )
    icon = gtk_image_new_from_icon_name( GTK_STOCK_YES, GTK_ICON_SIZE_BUTTON );
  else if( !strcmp( button->label, "_No" ) )
    icon = gtk_image_new_from_icon_name( GTK_STOCK_NO, GTK_ICON_SIZE_BUTTON );
#endif

  if( icon ) gtk_button_set_image( GTK_BUTTON( btn ), icon );

  add_click( btn, button, accel, 0, 0, 0, 0 );

  return btn;
}

GtkAccelGroup*
gtkstock_create_buttons( GtkWidget *widget, GtkAccelGroup *accel,
			 const gtkstock_button *buttons, size_t count )
{
  if( !accel ) accel = gtkstock_add_accel_group( widget );

  for( ; count; buttons++, count-- )
    gtkstock_create_button( widget, accel, buttons );

  return accel;
}

GtkAccelGroup*
gtkstock_create_ok_cancel( GtkWidget *widget, GtkAccelGroup *accel,
                           GCallback action, gpointer actiondata,
                           GCallback destroy_ok, GCallback destroy_cancel )
{
  gtkstock_button btn[] = {
    { "_Cancel", NULL, NULL, NULL, GDK_KEY_Escape, 0, 0, 0, GTK_RESPONSE_CANCEL},
    { "_OK", NULL, NULL, NULL, 0, 0, 0, 0, GTK_RESPONSE_OK},
  };
  btn[0].destroy = destroy_cancel ? destroy_cancel : NULL;
  btn[1].destroy = destroy_ok ? destroy_ok : NULL;
  btn[1].action = action;
  btn[1].actiondata = actiondata;

  return gtkstock_create_buttons( widget, accel, btn,
				  ARRAY_SIZE( btn ) );
}

GtkAccelGroup*
gtkstock_create_close( GtkWidget *widget, GtkAccelGroup *accel,
		       GCallback destroy, gboolean esconly )
{
  gtkstock_button btn = {
    "_Close", NULL, NULL, (destroy ? destroy : DEFAULT_DESTROY),
    (esconly ? GDK_KEY_VoidSymbol : GDK_KEY_Return), 0, GDK_KEY_Escape, 0,
    GTK_RESPONSE_CLOSE
  };
  return gtkstock_create_buttons( widget, accel, &btn, 1 );
}

GtkWidget*
gtkstock_dialog_new( const gchar *title, GCallback destroy )
{
  GtkWidget *dialog = gtk_dialog_new();
  if( title ) gtk_window_set_title( GTK_WINDOW( dialog ), title );
  /* TODO: allow to keep the dialog after closing for gtk_dialog_run() */
  g_signal_connect( G_OBJECT( dialog ), "delete-event",
		    destroy ? destroy : DEFAULT_DESTROY, NULL );
  if( destroy == NULL ) gtk_window_set_modal( GTK_WINDOW( dialog ), TRUE );
  gtk_window_set_transient_for( GTK_WINDOW( dialog ),
                                GTK_WINDOW( gtkui_window ) );

  return dialog;
}
