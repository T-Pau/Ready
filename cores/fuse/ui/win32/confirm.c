/* confirm.c: Confirmation dialog box
   Copyright (c) 2008 Marek Januszewski

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

#include <windows.h>

#include "fuse.h"
#include "win32internals.h"
#include "settings.h"
#include "ui/ui.h"

int
win32ui_confirm( const char *string )
{
  int confirm;
  int result;

  /* Return value isn't an error code, but signifies whether to undertake
     the action */
  if( !settings_current.confirm_actions ) return 1;

  fuse_emulation_pause();

  confirm = 0;

  result = MessageBox( fuse_hWnd, string, "Fuse - Confirm",
                       MB_OKCANCEL | MB_ICONQUESTION );
  if( result == IDOK ) confirm = 1;

  fuse_emulation_unpause();

  return confirm;
}

ui_confirm_save_t
ui_confirm_save_specific( const char *message )
{
  ui_confirm_save_t confirm;
  int result;

  if( !settings_current.confirm_actions ) return UI_CONFIRM_SAVE_DONTSAVE;

  fuse_emulation_pause();

  confirm = UI_CONFIRM_SAVE_CANCEL;

  result = MessageBox( fuse_hWnd, message, "Fuse - Confirm",
                       MB_YESNOCANCEL | MB_ICONQUESTION );
  switch( result )
  {
  case IDYES:
    confirm = UI_CONFIRM_SAVE_SAVE;
    break;
  case IDNO:
    confirm = UI_CONFIRM_SAVE_DONTSAVE;
    break;
  }

  fuse_emulation_unpause();

  return confirm;
}

int
ui_query( const char *message )
{
  return win32ui_confirm( message );
}

