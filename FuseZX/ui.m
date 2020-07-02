/* ios.c: User Interface Routines for Integration with Ready
 Copyright (c) 2017 Philip Kendall
 Copyright (C) 2020 Dieter Baron
 
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
 
 E-mail: philip-fuse@shadowmagic.org.uk, dillo@nih.at
 
 */

#include <config.h>

#include "fuse.h"
#include "keyboard.h"
#include "ui/ui.h"

#include "FuseThread.h"

keysyms_map_t keysyms_map[] = {
    { 0, 0 } /* End marker */
};

scaler_type
menu_get_scaler( scaler_available_fn selector )
{
    /* No scaler selected */
    return SCALER_NUM;
}


int
menu_select_roms_with_title( const char *title, size_t start, size_t count,
                            int is_peripheral )
{
    /* No error */
    return 0;
}

void
ui_breakpoints_updated( void )
{
    /* Do nothing */
}

ui_confirm_save_t
ui_confirm_save_specific( const char *message )
{
    return UI_CONFIRM_SAVE_DONTSAVE;
}

ui_confirm_joystick_t
ui_confirm_joystick( libspectrum_joystick libspectrum_type, int inputs )
{
    return UI_CONFIRM_JOYSTICK_NONE;
}

int
ui_debugger_activate( void )
{
    /* No error */
    return 0;
}

int
ui_debugger_deactivate( int interruptable )
{
    /* No error */
    return 0;
}

int
ui_debugger_disassemble( libspectrum_word addr )
{
    /* No error */
    return 0;
}

int
ui_debugger_update( void )
{
    /* No error */
    return 0;
}

int
ui_end( void )
{
    /* No error */
    return 0;
}

int
ui_error_specific( ui_error_level severity, const char *message )
{
    /* No error */
    return 0;
}

int
ui_event( void )
{
    [fuseThread.delegate handleEvents];
    return 0;
}

char*
ui_get_open_filename( const char *title )
{
    /* No filename */
    return NULL;
}

int
ui_get_rollback_point( GSList *points )
{
    /* No rollback point */
    return -1;
}

char*
ui_get_save_filename( const char *title )
{
    /* No filename */
    return NULL;
}

int
ui_init( int *argc, char ***argv )
{
    /* No error */
    return 0;
}

int
ui_menu_item_set_active( const char *path, int active )
{
    /* No error */
    return 0;
}

int
ui_mouse_grab( int startup )
{
    /* Successful grab */
    return 1;
}

int
ui_mouse_release( int suspend )
{
    /* No error */
    return 0;
}

void
ui_pokemem_selector( const char *filename )
{
    /* Do nothing */
}

int
ui_query( const char *message )
{
    /* Query confirmed */
    return 1;
}

int
ui_statusbar_update( ui_statusbar_item item, ui_statusbar_state state )
{
    /* TODO: implement */
    return 0;
}

int
ui_statusbar_update_speed( float speed )
{
    /* No error */
    return 0;
}

int
ui_tape_browser_update( ui_tape_browser_update_type change,
                       libspectrum_tape_block *block )
{
    /* No error */
    return 0;
}

int
ui_widgets_reset( void )
{
    /* No error */
    return 0;
}
