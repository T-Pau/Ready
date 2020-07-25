/* kempmouse.c: Kempston mouse emulation
   Copyright (c) 2004-2016 Darren Salt, Fredrick Meunier, Philip Kendall
   Copyright (c) 2015 Stuart Brady
   Copyright (c) 2016 Sergio Baldoví

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

   E-mail: linux@youmustbejoking.demon.co.uk

*/

#include <config.h>

#include <libspectrum.h>

#include "infrastructure/startup_manager.h"
#include "kempmouse.h"
#include "module.h"
#include "periph.h"
#include "settings.h"
#include "ui/ui.h"

static void kempmouse_snapshot_enabled( libspectrum_snap *snap );
static void kempmouse_to_snapshot( libspectrum_snap *snap );

static module_info_t kempmouse_module_info = {

  /* .reset = */ NULL,
  /* .romcs = */ NULL,
  /* .snapshot_enabled = */ kempmouse_snapshot_enabled,
  /* .snapshot_from = */ NULL,
  /* .snapshot_to = */ kempmouse_to_snapshot,

};

static struct {
  struct { libspectrum_byte x, y; } pos;
  libspectrum_byte buttons;
} kempmouse = { {0, 0}, 255 };

#define READ(name,item) \
  static libspectrum_byte \
  read_##name( libspectrum_word port GCC_UNUSED, libspectrum_byte *attached ) \
  { \
    *attached = 0xff; /* TODO: check this */ \
    return kempmouse.item; \
  }

READ( buttons, buttons );
READ( x_pos, pos.x );
READ( y_pos, pos.y );

static const periph_port_t kempmouse_ports[] = {
  /* _we_ require b0 set */
  { 0x0121, 0x0001, read_buttons, NULL },
  { 0x0521, 0x0101, read_x_pos, NULL },
  { 0x0521, 0x0501, read_y_pos, NULL },
};

static const periph_t kempmouse_periph = {
  /* .option = */ &settings_current.kempston_mouse,
  /* .ports = */ kempmouse_ports,
  /* .hard_reset = */ 1,
  /* .activate = */ NULL,
};

static int
kempmouse_init( void *context )
{
  module_register( &kempmouse_module_info );
  periph_register( PERIPH_TYPE_KEMPSTON_MOUSE, &kempmouse_periph );

  return 0;
}

void
kempmouse_register_startup( void )
{
  startup_manager_module dependencies[] = { STARTUP_MANAGER_MODULE_SETUID };
  startup_manager_register( STARTUP_MANAGER_MODULE_KEMPMOUSE, dependencies,
                            ARRAY_SIZE( dependencies ), kempmouse_init, NULL,
                            NULL );
}

void
kempmouse_update( int dx, int dy, int btn, int down )
{
  kempmouse.pos.x += dx;
  kempmouse.pos.y -= dy;
  if( btn != -1 ) {
    if( down )
      kempmouse.buttons &= ~(1 << btn);
    else
      kempmouse.buttons |= 1 << btn;
  }
}

static void
kempmouse_snapshot_enabled( libspectrum_snap *snap )
{
  if( libspectrum_snap_kempston_mouse_active( snap ) )
    settings_current.kempston_mouse = 1;
}

static void
kempmouse_to_snapshot( libspectrum_snap *snap )
{
  libspectrum_snap_set_kempston_mouse_active( snap,
                                              settings_current.kempston_mouse );
}
