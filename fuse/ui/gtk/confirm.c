/* confirm.c: Confirmation dialog box
   Copyright (c) 2000-2015 Philip Kendall, Russell Marks

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

#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>

#include "compat.h"
#include "fuse.h"
#include "gtkcompat.h"
#include "gtkinternals.h"
#include "settings.h"
#include "ui/ui.h"

static void set_confirmed( GtkButton *button, gpointer user_data );
static void set_save( GtkButton *button, gpointer user_data );
static void set_dont_save( GtkButton *button, gpointer user_data );

int
gtkui_confirm( const char *string )
{
  GtkWidget *dialog, *label, *content_area;
  int confirm;

  /* Return value isn't an error code, but signifies whether to undertake
     the action */
  if( !settings_current.confirm_actions ) return 1;

  fuse_emulation_pause();

  confirm = 0;

  dialog = gtkstock_dialog_new( "Fuse - Confirm", NULL );

  label = gtk_label_new( string );
  content_area = gtk_dialog_get_content_area( GTK_DIALOG( dialog ) );
  gtk_box_pack_start( GTK_BOX( content_area ), label, TRUE, TRUE, 5 );

  gtkstock_create_ok_cancel( dialog, NULL, G_CALLBACK( set_confirmed ),
                             &confirm, DEFAULT_DESTROY, DEFAULT_DESTROY );

  gtk_dialog_set_default_response( GTK_DIALOG( dialog ), GTK_RESPONSE_CANCEL );

  gtk_widget_show_all( dialog );
  gtk_main();

  fuse_emulation_unpause();

  return confirm;
}

static void
set_confirmed( GtkButton *button GCC_UNUSED, gpointer user_data )
{
  int *ptr = user_data;

  *ptr = 1;
}

ui_confirm_save_t
ui_confirm_save_specific( const char *message )
{
  GtkWidget *dialog, *label, *content_area;
  ui_confirm_save_t confirm;

  if( !settings_current.confirm_actions ) return UI_CONFIRM_SAVE_DONTSAVE;

  fuse_emulation_pause();

  confirm = UI_CONFIRM_SAVE_CANCEL;

  dialog = gtkstock_dialog_new( "Fuse - Confirm", NULL );
  content_area = gtk_dialog_get_content_area( GTK_DIALOG( dialog ) );

  label = gtk_label_new( message );
  gtk_box_pack_start( GTK_BOX( content_area ), label, TRUE, TRUE, 5 );

  {
    static gtkstock_button btn[] = {
      { "_No", G_CALLBACK( set_dont_save ), NULL, DEFAULT_DESTROY, 0, 0, 0, 0, GTK_RESPONSE_NO },
      { "_Cancel", NULL, NULL, DEFAULT_DESTROY, GDK_KEY_Escape, 0, 0, 0, GTK_RESPONSE_CANCEL },
      { "_Save", G_CALLBACK( set_save ), NULL, DEFAULT_DESTROY, 0, 0, 0, 0, GTK_RESPONSE_YES }
    };
    btn[0].actiondata = btn[2].actiondata = &confirm;
    gtkstock_create_buttons( dialog, NULL, btn, ARRAY_SIZE( btn ) );
  }

  gtk_dialog_set_default_response( GTK_DIALOG( dialog ), GTK_RESPONSE_CANCEL );

  gtk_widget_show_all( dialog );
  gtk_main();

  fuse_emulation_unpause();

  return confirm;
}

int
ui_query( const char *message )
{
  return gtkui_confirm( message );
}

static void
set_save( GtkButton *button GCC_UNUSED, gpointer user_data )
{
  ui_confirm_save_t *ptr = user_data;

  *ptr = UI_CONFIRM_SAVE_SAVE;
}

static void
set_dont_save( GtkButton *button GCC_UNUSED, gpointer user_data )
{
  ui_confirm_save_t *ptr = user_data;

  *ptr = UI_CONFIRM_SAVE_DONTSAVE;
}
