/* gtkcompat.h: various compatibility bits between GTK+ versions
   Copyright (c) 2012 Philip Kendall

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

#ifndef FUSE_GTKCOMPAT_H
#define FUSE_GTKCOMPAT_H

#include <gdk/gdk.h>
#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>

#ifndef GTK_TYPE_COMBO_BOX_TEXT

#define GTK_COMBO_BOX_TEXT( X ) GTK_COMBO_BOX( X )
#define gtk_combo_box_text_new() gtk_combo_box_new_text()
#define gtk_combo_box_text_append_text( X, Y ) gtk_combo_box_append_text( X, Y )

#endif				/* #ifndef GTK_TYPE_COMBO_BOX_TEXT */


#if !GTK_CHECK_VERSION( 3, 0, 0 )

GtkWidget *
gtk_box_new( GtkOrientation orientation, gint spacing );

GtkWidget *
gtk_separator_new( GtkOrientation orientation );

GtkWidget *
gtk_scrollbar_new( GtkOrientation orientation, GtkAdjustment *adjustment );

#define gtk_widget_override_font( X, Y ) gtk_widget_modify_font( X, Y )

#endif


#if !GTK_CHECK_VERSION( 2, 22, 0 )

#define GDK_KEY_VoidSymbol      GDK_VoidSymbol
#define GDK_KEY_Tab             GDK_Tab
#define GDK_KEY_Return          GDK_Return
#define GDK_KEY_Escape          GDK_Escape
#define GDK_KEY_space           GDK_space
#define GDK_KEY_quotedbl        GDK_quotedbl
#define GDK_KEY_exclam          GDK_exclam
#define GDK_KEY_numbersign      GDK_numbersign
#define GDK_KEY_dollar          GDK_dollar
#define GDK_KEY_percent         GDK_percent
#define GDK_KEY_ampersand       GDK_ampersand
#define GDK_KEY_apostrophe      GDK_apostrophe
#define GDK_KEY_parenleft       GDK_parenleft
#define GDK_KEY_parenright      GDK_parenright
#define GDK_KEY_asterisk        GDK_asterisk
#define GDK_KEY_plus            GDK_plus
#define GDK_KEY_comma           GDK_comma
#define GDK_KEY_minus           GDK_minus
#define GDK_KEY_period          GDK_period
#define GDK_KEY_slash           GDK_slash
#define GDK_KEY_0               GDK_0
#define GDK_KEY_1               GDK_1
#define GDK_KEY_2               GDK_2
#define GDK_KEY_3               GDK_3
#define GDK_KEY_4               GDK_4
#define GDK_KEY_5               GDK_5
#define GDK_KEY_6               GDK_6
#define GDK_KEY_7               GDK_7
#define GDK_KEY_8               GDK_8
#define GDK_KEY_9               GDK_9
#define GDK_KEY_colon           GDK_colon
#define GDK_KEY_semicolon       GDK_semicolon
#define GDK_KEY_less            GDK_less
#define GDK_KEY_equal           GDK_equal
#define GDK_KEY_greater         GDK_greater
#define GDK_KEY_question        GDK_question
#define GDK_KEY_at              GDK_at
#define GDK_KEY_A               GDK_A
#define GDK_KEY_B               GDK_B
#define GDK_KEY_C               GDK_C
#define GDK_KEY_D               GDK_D
#define GDK_KEY_E               GDK_E
#define GDK_KEY_F               GDK_F
#define GDK_KEY_G               GDK_G
#define GDK_KEY_H               GDK_H
#define GDK_KEY_I               GDK_I
#define GDK_KEY_J               GDK_J
#define GDK_KEY_K               GDK_K
#define GDK_KEY_L               GDK_L
#define GDK_KEY_M               GDK_M
#define GDK_KEY_N               GDK_N
#define GDK_KEY_O               GDK_O
#define GDK_KEY_P               GDK_P
#define GDK_KEY_Q               GDK_Q
#define GDK_KEY_R               GDK_R
#define GDK_KEY_S               GDK_S
#define GDK_KEY_T               GDK_T
#define GDK_KEY_U               GDK_U
#define GDK_KEY_V               GDK_V
#define GDK_KEY_W               GDK_W
#define GDK_KEY_X               GDK_X
#define GDK_KEY_Y               GDK_Y
#define GDK_KEY_Z               GDK_Z
#define GDK_KEY_bracketleft     GDK_bracketleft
#define GDK_KEY_backslash       GDK_backslash
#define GDK_KEY_bracketright    GDK_bracketright
#define GDK_KEY_asciicircum     GDK_asciicircum
#define GDK_KEY_underscore      GDK_underscore
#define GDK_KEY_dead_circumflex GDK_dead_circumflex
#define GDK_KEY_a               GDK_a
#define GDK_KEY_b               GDK_b
#define GDK_KEY_c               GDK_c
#define GDK_KEY_d               GDK_d
#define GDK_KEY_e               GDK_e
#define GDK_KEY_f               GDK_f
#define GDK_KEY_g               GDK_g
#define GDK_KEY_h               GDK_h
#define GDK_KEY_i               GDK_i
#define GDK_KEY_j               GDK_j
#define GDK_KEY_k               GDK_k
#define GDK_KEY_l               GDK_l
#define GDK_KEY_m               GDK_m
#define GDK_KEY_n               GDK_n
#define GDK_KEY_o               GDK_o
#define GDK_KEY_p               GDK_p
#define GDK_KEY_q               GDK_q
#define GDK_KEY_r               GDK_r
#define GDK_KEY_s               GDK_s
#define GDK_KEY_t               GDK_t
#define GDK_KEY_u               GDK_u
#define GDK_KEY_v               GDK_v
#define GDK_KEY_w               GDK_w
#define GDK_KEY_x               GDK_x
#define GDK_KEY_y               GDK_y
#define GDK_KEY_z               GDK_z
#define GDK_KEY_braceleft       GDK_braceleft
#define GDK_KEY_bar             GDK_bar
#define GDK_KEY_braceright      GDK_braceright
#define GDK_KEY_asciitilde      GDK_asciitilde
#define GDK_KEY_BackSpace       GDK_BackSpace
#define GDK_KEY_KP_Enter        GDK_KP_Enter
#define GDK_KEY_Up              GDK_Up
#define GDK_KEY_Down            GDK_Down
#define GDK_KEY_Left            GDK_Left
#define GDK_KEY_Right           GDK_Right
#define GDK_KEY_Insert          GDK_Insert
#define GDK_KEY_Delete          GDK_Delete
#define GDK_KEY_Home            GDK_Home
#define GDK_KEY_End             GDK_End
#define GDK_KEY_Page_Up         GDK_Page_Up
#define GDK_KEY_Page_Down       GDK_Page_Down
#define GDK_KEY_Caps_Lock       GDK_Caps_Lock
#define GDK_KEY_F1              GDK_F1
#define GDK_KEY_F2              GDK_F2
#define GDK_KEY_F3              GDK_F3
#define GDK_KEY_F4              GDK_F4
#define GDK_KEY_F5              GDK_F5
#define GDK_KEY_F6              GDK_F6
#define GDK_KEY_F7              GDK_F7
#define GDK_KEY_F8              GDK_F8
#define GDK_KEY_F9              GDK_F9
#define GDK_KEY_F10             GDK_F10
#define GDK_KEY_F11             GDK_F11
#define GDK_KEY_F12             GDK_F12
#define GDK_KEY_Shift_L         GDK_Shift_L
#define GDK_KEY_Shift_R         GDK_Shift_R
#define GDK_KEY_Control_L       GDK_Control_L
#define GDK_KEY_Control_R       GDK_Control_R
#define GDK_KEY_Alt_L           GDK_Alt_L
#define GDK_KEY_Alt_R           GDK_Alt_R
#define GDK_KEY_Meta_L          GDK_Meta_L
#define GDK_KEY_Meta_R          GDK_Meta_R
#define GDK_KEY_Super_L         GDK_Super_L
#define GDK_KEY_Super_R         GDK_Super_R
#define GDK_KEY_Hyper_L         GDK_Hyper_L
#define GDK_KEY_Hyper_R         GDK_Hyper_R
#define GDK_KEY_Mode_switch     GDK_Mode_switch

#endif				/* #if !GTK_CHECK_VERSION( 2, 22, 0 ) */

#endif				/* #ifndef FUSE_GTKCOMPAT_H */
