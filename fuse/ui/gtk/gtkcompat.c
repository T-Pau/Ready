/* gtkcompat.c: various compatibility bits between GTK+ versions
   Copyright (c) 2012-2014 Philip Kendall

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

#include <gtk/gtk.h>

#include "gtkcompat.h"

#if !GTK_CHECK_VERSION( 3, 0, 0 )

GtkWidget *
gtk_box_new( GtkOrientation orientation, gint spacing )
{
  return ( orientation == GTK_ORIENTATION_HORIZONTAL )?
          gtk_hbox_new( FALSE, spacing ) :
          gtk_vbox_new( FALSE, spacing );
}

GtkWidget *
gtk_separator_new( GtkOrientation orientation )
{
  return ( orientation == GTK_ORIENTATION_HORIZONTAL )?
          gtk_hseparator_new() :
          gtk_vseparator_new();
}

GtkWidget *
gtk_scrollbar_new( GtkOrientation orientation, GtkAdjustment *adjustment )
{
  return ( orientation == GTK_ORIENTATION_HORIZONTAL )?
          gtk_hscrollbar_new( adjustment ) :
          gtk_vscrollbar_new( adjustment );
}

#endif                /* #if !GTK_CHECK_VERSION( 3, 0, 0 ) */
