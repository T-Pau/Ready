/* periph.c: code for handling peripherals
   Copyright (c) 2005-2016 Philip Kendall
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

#include <libspectrum.h>

#include "debugger/debugger.h"
#include "event.h"
#include "fuse.h"
#include "periph.h"
#include "peripherals/if1.h"
#include "peripherals/multiface.h"
#include "peripherals/ula.h"
#include "rzx.h"
#include "settings.h"
#include "ui/ui.h"

/*
 * General peripheral list handling routines
 */

typedef struct periph_private_t {
  /* Can this peripheral ever be present on the currently emulated machine? */
  periph_present present;
  /* Is this peripheral currently active? */
  int active; 
  /* The actual peripheral data */
  const periph_t *periph;
} periph_private_t;

/* All the peripherals we know about */
static GHashTable *peripherals = NULL;

/* Wrapper to pair up a port response with the peripheral it came from */
typedef struct periph_port_private_t {
  /* The peripheral this came from */
  periph_type type;
  /* The port response */
  periph_port_t port;
} periph_port_private_t;

/* The list of currently active ports */
static GSList *ports = NULL;

/* The strings used for debugger events */
static const char * const page_event_string = "page",
  * const unpage_event_string = "unpage";

/* Place one port response in the list of currently active ones */
static void
port_register( periph_type type, const periph_port_t *port )
{
  periph_port_private_t *private;

  private = libspectrum_new( periph_port_private_t, 1 );

  private->type = type;
  private->port = *port;

  ports = g_slist_append( ports, private );
}

/* Register a peripheral with the system */
void
periph_register( periph_type type, const periph_t *periph )
{
  periph_private_t *private;

  if( !peripherals )
    peripherals = g_hash_table_new_full( NULL, NULL, NULL, libspectrum_free );

  private = libspectrum_new( periph_private_t, 1 );

  private->present = PERIPH_PRESENT_NEVER;
  private->active = 0;
  private->periph = periph;

  g_hash_table_insert( peripherals, GINT_TO_POINTER( type ), private );
}

/* Get the data about one peripheral */
static gint
find_by_type( gconstpointer data, gconstpointer user_data )
{
  const periph_port_private_t *periph = data;
  periph_type type = GPOINTER_TO_INT( user_data );
  return periph->type - type;
}

/* Set whether a peripheral can be present on this machine or not */
void
periph_set_present( periph_type type, periph_present present )
{
  periph_private_t *type_data = g_hash_table_lookup( peripherals, GINT_TO_POINTER( type ) );
  if( type_data ) type_data->present = present;
}

/* Mark a specific peripheral as (in)active */
int
periph_activate_type( periph_type type, int active )
{
  periph_private_t *private = g_hash_table_lookup( peripherals, GINT_TO_POINTER( type ) );
  if( !private || private->active == active ) return 0;

  private->active = active;

  if( active ) {
    const periph_port_t *ptr;
    if( private->periph->activate )
      private->periph->activate();
    for( ptr = private->periph->ports; ptr && ptr->mask != 0; ptr++ )
      port_register( type, ptr );
  } else {
    GSList *found;
    while( ( found = g_slist_find_custom( ports, GINT_TO_POINTER( type ), find_by_type ) ) != NULL )
      ports = g_slist_remove( ports, found->data );
  }

  return 1;
}

/* Is a specific peripheral active at the moment? */
int
periph_is_active( periph_type type )
{
  periph_private_t *type_data = g_hash_table_lookup( peripherals, GINT_TO_POINTER( type ) );
  return type_data ? type_data->active : 0;
}

/* Work out whether a peripheral is present on this machine, and mark it
   (in)active as appropriate */
static void
set_activity( gpointer key, gpointer value, gpointer user_data )
{
  periph_type type = GPOINTER_TO_INT( key );
  periph_private_t *private = value;
  int active = 0;
  int *needs_hard_reset = (int *)user_data;

  switch ( private->present ) {
  case PERIPH_PRESENT_NEVER: active = 0; break;
  case PERIPH_PRESENT_OPTIONAL:
    active = private->periph->option ? *(private->periph->option) : 0; break;
  case PERIPH_PRESENT_ALWAYS: active = 1; break;
  }

  *needs_hard_reset = 
    ( periph_activate_type( type, active ) && private->periph->hard_reset ) ||
    *needs_hard_reset;
}

/* Work out whether a peripheral needs a hard reset without (de)activate */
static void
get_hard_reset( gpointer key, gpointer value, gpointer user_data )
{
  periph_private_t *private = value;
  int active = 0;
  int *machine_hard_reset = (int *)user_data;
  int periph_hard_reset = 0;

  switch ( private->present ) {
  case PERIPH_PRESENT_NEVER: active = 0; break;
  case PERIPH_PRESENT_OPTIONAL:
    active = private->periph->option ? *(private->periph->option) : 0; break;
  case PERIPH_PRESENT_ALWAYS: active = 1; break;
  }

  periph_hard_reset = ( private && ( private->active != active ) &&
                        private->periph->hard_reset );

  *machine_hard_reset = ( periph_hard_reset || *machine_hard_reset );
}

static void
disable_optional( gpointer key, gpointer value, gpointer user_data )
{
  periph_private_t *private = value;

  switch ( private->present ) {
  case PERIPH_PRESENT_NEVER:
  case PERIPH_PRESENT_OPTIONAL:
    if( private->periph->option ) *(private->periph->option) = 0;
    break;
  default: break;
  }
}

/* Free the memory used by a peripheral-port response pair */
static void
free_peripheral( gpointer data, gpointer user_data GCC_UNUSED )
{
  periph_port_private_t *private = data;
  libspectrum_free( private );
}

/* Make a peripheral as being never present on this machine */
static void
set_type_inactive( gpointer key, gpointer value, gpointer user_data )
{
  periph_private_t *type_data = value;
  type_data->present = PERIPH_PRESENT_NEVER;
  type_data->active = 0;
}

/* Mark all peripherals as being never present on this machine */
static void
set_types_inactive( void )
{
  g_hash_table_foreach( peripherals, set_type_inactive, NULL );
}

/* Empty out the list of peripherals */
void
periph_clear( void )
{
  g_slist_foreach( ports, free_peripheral, NULL );
  g_slist_free( ports );
  ports = NULL;
  set_types_inactive();
}

/* Tidy-up function called at end of emulation */
void
periph_end( void )
{
  g_slist_foreach( ports, free_peripheral, NULL );
  g_slist_free( ports );
  ports = NULL;

  g_hash_table_destroy( peripherals );
  peripherals = NULL;
}

/*
 * The actual routines to read and write a port
 */

/* Internal type used for passing to read_peripheral and write_peripheral */
struct peripheral_data_t {

  libspectrum_word port;

  libspectrum_byte attached;
  libspectrum_byte value;
};

/* Read a byte from a port, taking the appropriate time */
libspectrum_byte
readport( libspectrum_word port )
{
  libspectrum_byte b;

  ula_contend_port_early( port );
  ula_contend_port_late( port );
  b = readport_internal( port );

  /* Very ugly to put this here, but unless anything else needs this
     "writeback" mechanism, no point producing a general framework */
  if( ( port & 0x8002 ) == 0 &&
      ( machine_current->machine == LIBSPECTRUM_MACHINE_128   ||
	machine_current->machine == LIBSPECTRUM_MACHINE_PLUS2    ) )
    writeport_internal( 0x7ffd, b );

  tstates++;

  return b;
}

/* Read a byte from a specific port response */
static void
read_peripheral( gpointer data, gpointer user_data )
{
  periph_port_private_t *private = data;
  struct peripheral_data_t *callback_info = user_data;
  libspectrum_byte last_attached;

  periph_port_t *port = &( private->port );

  if( port->read &&
      ( ( callback_info->port & port->mask ) == port->value ) ) {
    last_attached = callback_info->attached;
    callback_info->value &= (   port->read( callback_info->port,
					    &( callback_info->attached ) )
			      | last_attached );
  }
}

/* Read a byte from a port, taking no time */
libspectrum_byte
readport_internal( libspectrum_word port )
{
  struct peripheral_data_t callback_info;

  /* Trigger the debugger if wanted */
  if( debugger_mode != DEBUGGER_MODE_INACTIVE )
    debugger_check( DEBUGGER_BREAKPOINT_TYPE_PORT_READ, port );

  /* If we're doing RZX playback, get a byte from the RZX file */
  if( rzx_playback ) {

    libspectrum_error error;
    libspectrum_byte value;

    error = libspectrum_rzx_playback( rzx, &value );
    if( error ) {
      rzx_stop_playback( 1 );

      /* Add a null event to mean we pick up the RZX state change in
	 z80_do_opcodes() */
      event_add( tstates, event_type_null );
      return readport_internal( port );
    }

    return value;
  }

  /* If we're not doing RZX playback, get the byte normally */
  callback_info.port = port;
  callback_info.attached = 0x00;
  callback_info.value = 0xff;

  g_slist_foreach( ports, read_peripheral, &callback_info );

  if( callback_info.attached != 0xff )
    callback_info.value =
      periph_merge_floating_bus( callback_info.value, callback_info.attached,
                                 machine_current->unattached_port() );

  /* If we're RZX recording, store this byte */
  if( rzx_recording ) rzx_store_byte( callback_info.value );

  return callback_info.value;
}

/* Merge the read value with the floating bus. Deliberately doesn't take
   a callback_info structure to enable it to be unit tested */
libspectrum_byte
periph_merge_floating_bus( libspectrum_byte value, libspectrum_byte attached,
			   libspectrum_byte floating_bus )
{
  return value & (floating_bus | attached);
}

/* Write a byte to a port, taking the appropriate time */
void
writeport( libspectrum_word port, libspectrum_byte b )
{
  ula_contend_port_early( port );
  writeport_internal( port, b );
  ula_contend_port_late( port ); tstates++;
}

/* Write a byte to a specific port response */
static void
write_peripheral( gpointer data, gpointer user_data )
{
  periph_port_private_t *private = data;
  struct peripheral_data_t *callback_info = user_data;

  periph_port_t *port = &( private->port );
  
  if( port->write &&
      ( ( callback_info->port & port->mask ) == port->value ) )
    port->write( callback_info->port, callback_info->value );
}

/* Write a byte to a port, taking no time */
void
writeport_internal( libspectrum_word port, libspectrum_byte b )
{
  struct peripheral_data_t callback_info;

  /* Trigger the debugger if wanted */
  if( debugger_mode != DEBUGGER_MODE_INACTIVE )
    debugger_check( DEBUGGER_BREAKPOINT_TYPE_PORT_WRITE, port );

  callback_info.port = port;
  callback_info.value = b;
  
  g_slist_foreach( ports, write_peripheral, &callback_info );
}

/*
 * The more Fuse-specific peripheral handling routines
 */

static void
update_cartridge_menu( void )
{
  int cartridge, dock, if2;

  dock = machine_current->capabilities &
         LIBSPECTRUM_MACHINE_CAPABILITY_TIMEX_DOCK;
  if2 = periph_is_active( PERIPH_TYPE_INTERFACE2 );

  cartridge = dock || if2;

  ui_menu_activate( UI_MENU_ITEM_MEDIA_CARTRIDGE, cartridge );
  ui_menu_activate( UI_MENU_ITEM_MEDIA_CARTRIDGE_DOCK, dock );
  ui_menu_activate( UI_MENU_ITEM_MEDIA_CARTRIDGE_IF2, if2 );
}

static void
update_ide_menu( void )
{
  int ide, simpleide, zxatasp, zxcf, divide, divmmc, zxmmc;

  simpleide = settings_current.simpleide_active;
  zxatasp = settings_current.zxatasp_active;
  zxcf = settings_current.zxcf_active;
  divide = settings_current.divide_enabled;
  divmmc = settings_current.divmmc_enabled;
  zxmmc = settings_current.zxmmc_enabled;

  ide = simpleide || zxatasp || zxcf || divide || divmmc || zxmmc;

  ui_menu_activate( UI_MENU_ITEM_MEDIA_IDE, ide );
  ui_menu_activate( UI_MENU_ITEM_MEDIA_IDE_SIMPLE8BIT, simpleide );
  ui_menu_activate( UI_MENU_ITEM_MEDIA_IDE_ZXATASP, zxatasp );
  ui_menu_activate( UI_MENU_ITEM_MEDIA_IDE_ZXCF, zxcf );
  ui_menu_activate( UI_MENU_ITEM_MEDIA_IDE_DIVIDE, divide );
  ui_menu_activate( UI_MENU_ITEM_MEDIA_IDE_DIVMMC, divmmc );
  ui_menu_activate( UI_MENU_ITEM_MEDIA_IDE_ZXMMC, zxmmc );
}

static void
update_peripherals_status( void )
{
  ui_menu_activate( UI_MENU_ITEM_MEDIA_IF1,
                    periph_is_active( PERIPH_TYPE_INTERFACE1 ) );
  ui_menu_activate( UI_MENU_ITEM_MEDIA_CARTRIDGE_IF2,
                    periph_is_active( PERIPH_TYPE_INTERFACE2 ) );

  update_cartridge_menu();
  update_ide_menu();
  if1_update_menu();
  multiface_status_update();
  specplus3_765_update_fdd();
}

void
periph_disable_optional( void )
{
  if( ui_mouse_present && ui_mouse_grabbed ) {
    ui_mouse_grabbed = ui_mouse_release( 1 );
  }

  g_hash_table_foreach( peripherals, disable_optional, NULL );

  update_peripherals_status();
}

int
periph_update( void )
{
  int needs_hard_reset = 0;

  if( ui_mouse_present ) {
    if( settings_current.kempston_mouse ) {
      if( !ui_mouse_grabbed ) ui_mouse_grabbed = ui_mouse_grab( 1 );
    } else {
      if(  ui_mouse_grabbed ) ui_mouse_grabbed = ui_mouse_release( 1 );
    }
  }

  g_hash_table_foreach( peripherals, set_activity, &needs_hard_reset );

  update_peripherals_status();
  machine_current->memory_map();

  return needs_hard_reset;
}

void
periph_posthook( void )
{
  if( periph_update() ) {
    machine_reset( 1 );
  }
}

int
periph_postcheck( void )
{
  int needs_hard_reset = 0;

  /* Detect if a hard reset is needed without (de)activating peripherals */
  g_hash_table_foreach( peripherals, get_hard_reset, &needs_hard_reset );

  return needs_hard_reset;
}

/* Register debugger page/unpage events for a peripheral */
void
periph_register_paging_events( const char *type_string, int *page_event,
			       int *unpage_event )
{
  *page_event = debugger_event_register( type_string, page_event_string );
  *unpage_event = debugger_event_register( type_string, unpage_event_string );
}
