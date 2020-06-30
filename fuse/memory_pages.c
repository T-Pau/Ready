/* memory_pages.c: Routines for accessing memory
   Copyright (c) 1999-2016 Philip Kendall
   Copyright (c) 2015 Stuart Brady
   Copyright (c) 2016 Fredrick Meunier

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

#include <string.h>
#ifdef HAVE_STRINGS_STRCASECMP
#include <strings.h>
#endif      /* #ifdef HAVE_STRINGS_STRCASECMP */

#include <libspectrum.h>

#include "debugger/debugger.h"
#include "display.h"
#include "fuse.h"
#include "infrastructure/startup_manager.h"
#include "machines/pentagon.h"
#include "machines/spec128.h"
#include "machines/specplus3.h"
#include "memory_pages.h"
#include "module.h"
#include "peripherals/disk/opus.h"
#include "peripherals/spectranet.h"
#include "peripherals/ula.h"
#include "settings.h"
#include "spectrum.h"
#include "ui/ui.h"
#include "utils.h"

/* The various sources of memory available to us */
static GArray *memory_sources;

/* Some "well-known" memory sources */
int memory_source_rom; /* System ROM */
int memory_source_ram; /* System RAM */
int memory_source_dock; /* Timex DOCK */
int memory_source_exrom; /* Timex EXROM */
int memory_source_any; /* Used by the debugger to signify an absolute address */
int memory_source_none; /* No memory attached here */

/* Each RAM chunk accessible by the Z80 */
memory_page memory_map_read[MEMORY_PAGES_IN_64K];
memory_page memory_map_write[MEMORY_PAGES_IN_64K];

/* Standard mappings for the 'normal' RAM */
memory_page memory_map_ram[SPECTRUM_RAM_PAGES * MEMORY_PAGES_IN_16K];

/* Standard mappings for the ROMs */
memory_page memory_map_rom[SPECTRUM_ROM_PAGES * MEMORY_PAGES_IN_16K];

/* Some allocated memory */
typedef struct memory_pool_entry_t {
  int persistent;
  libspectrum_byte *memory;
} memory_pool_entry_t;

/* All the memory we've allocated for this machine */
static GSList *pool;

/* Which RAM page contains the current screen */
int memory_current_screen;

/* Which bits to look at when working out where the screen is */
libspectrum_word memory_screen_mask;

static void memory_from_snapshot( libspectrum_snap *snap );
static void memory_to_snapshot( libspectrum_snap *snap );

static module_info_t memory_module_info = {

  NULL,
  NULL,
  NULL,
  memory_from_snapshot,
  memory_to_snapshot,

};

/* Set up the information about the normal page mappings.
   Memory contention and usable pages vary from machine to machine and must
   be set in the appropriate _reset function */
static int
memory_init( void *context )
{
  size_t i, j;

  memory_sources = g_array_new( FALSE, FALSE, sizeof( const char* ) );

  memory_source_rom = memory_source_register( "ROM" );
  memory_source_ram = memory_source_register( "RAM" );
  memory_source_dock = memory_source_register( "Timex Dock" );
  memory_source_exrom = memory_source_register( "Timex EXROM" );
  memory_source_any = memory_source_register( "Absolute address" );
  memory_source_none = memory_source_register( "None" );

  /* Nothing in the memory pool as yet */
  pool = NULL;

  for( i = 0; i < SPECTRUM_ROM_PAGES; i++ )
    for( j = 0; j < MEMORY_PAGES_IN_16K; j++ ) {
      memory_page *page = &memory_map_rom[i * MEMORY_PAGES_IN_16K + j];
      page->writable = 0;
      page->contended = 0;
      page->source = memory_source_rom;
    }
    
  for( i = 0; i < SPECTRUM_RAM_PAGES; i++ )
    for( j = 0; j < MEMORY_PAGES_IN_16K; j++ ) {
      memory_page *page = &memory_map_ram[i * MEMORY_PAGES_IN_16K + j];
      page->page = &RAM[i][j * MEMORY_PAGE_SIZE];
      page->page_num = i;
      page->offset = j * MEMORY_PAGE_SIZE;
      page->writable = 1;
      page->source = memory_source_ram;
    }

  module_register( &memory_module_info );

  return 0;
}

static void
memory_pool_free_entry( gpointer data, gpointer user_data GCC_UNUSED )
{
  memory_pool_entry_t *entry = data;
  libspectrum_free( entry->memory );
  libspectrum_free( entry );
}

/* Tidy-up function called at end of emulation */
static void
memory_end( void )
{
  int i;
  char *description;

  /* Free all the memory we've allocated for this machine */
  if( pool ) {
    g_slist_foreach( pool, memory_pool_free_entry, NULL );
    g_slist_free( pool );
    pool = NULL;
  }

  /* Free memory source types */
  if( memory_sources ) {
    for( i = 0; i < memory_sources->len; i++ ) {
      description = g_array_index( memory_sources, char *, i );
      libspectrum_free( description );
    }

    g_array_free( memory_sources, TRUE );
    memory_sources = NULL;
  }
}

void
memory_register_startup( void )
{
  startup_manager_module dependencies[] = { STARTUP_MANAGER_MODULE_SETUID };
  startup_manager_register( STARTUP_MANAGER_MODULE_MEMORY, dependencies,
                            ARRAY_SIZE( dependencies ), memory_init, NULL,
                            memory_end );
}

int
memory_source_register( const char *description )
{
  const char *copy = utils_safe_strdup( description );

  g_array_append_val( memory_sources, copy );

  return memory_sources->len - 1;
}

const char*
memory_source_description( int source )
{
  return g_array_index( memory_sources, const char*, source );
}

int
memory_source_find( const char *description )
{
  int i, source = -1;

  for( i = 0; i < memory_sources->len; i++ ) {
    const char *found = g_array_index( memory_sources, const char*, i );
    if( !strcasecmp( description, found ) ) {
      source = i;
      break;
    }
  }

  return source;
}

/* Allocate some memory from the pool */
libspectrum_byte*
memory_pool_allocate( size_t length )
{
  return memory_pool_allocate_persistent( length, 0 );
}

libspectrum_byte*
memory_pool_allocate_persistent( size_t length, int persistent )
{
  memory_pool_entry_t *entry;
  libspectrum_byte *memory;

  memory = libspectrum_new( libspectrum_byte, length );

  entry = libspectrum_new( memory_pool_entry_t, 1 );

  entry->persistent = persistent;
  entry->memory = memory;

  pool = g_slist_prepend( pool, entry );

  return memory;
}

static gint
find_non_persistent( gconstpointer data, gconstpointer user_data GCC_UNUSED )
{
  const memory_pool_entry_t *entry = data;
  return entry->persistent;
}

/* Free all non-persistent memory in the pool */
void
memory_pool_free( void )
{
  GSList *ptr;

  while( ( ptr = g_slist_find_custom( pool, NULL, find_non_persistent ) ) != NULL )
  {
    memory_pool_entry_t *entry = ptr->data;
    libspectrum_free( entry->memory );
    pool = g_slist_remove( pool, entry );
    libspectrum_free( entry );
  }
}

/* Set contention for 16K of RAM */
void
memory_ram_set_16k_contention( int page_num, int contended )
{
  int i;

  for( i = 0; i < MEMORY_PAGES_IN_16K; i++ )
    memory_map_ram[ page_num * MEMORY_PAGES_IN_16K + i ].contended = contended;
}

/* Map 16K of memory */
void
memory_map_16k( libspectrum_word address, memory_page source[], int page_num )
{
  memory_map_16k_read_write( address, source, page_num, 1, 1 );
}

/* Map 16K of memory for either reading, writing or both */
void
memory_map_16k_read_write( libspectrum_word address, memory_page source[],
		           int page_num, int map_read, int map_write )
{
  memory_map_8k_read_write( address, source, page_num * 2, map_read,
		            map_write );
  memory_map_8k_read_write( address + 0x2000, source, page_num * 2 + 1,
		            map_read, map_write );
}

/* Map 8K of memory */
void
memory_map_8k( libspectrum_word address, memory_page source[], int page_num )
{
  memory_map_8k_read_write( address, source, page_num, 1, 1 );
}

/* Map 8K of memory for either reading, writing or both */
void
memory_map_8k_read_write( libspectrum_word address, memory_page source[],
		          int page_num, int map_read, int map_write )
{
  memory_map_4k_read_write( address, source, page_num * 2, map_read, map_write );
  memory_map_4k_read_write( address + 0x1000, source, page_num * 2 + 1,
		  	    map_read, map_write );
}

/* Map 4K of memory for either reading, writing or both */
void
memory_map_4k_read_write( libspectrum_word address, memory_page source[],
		          int page_num, int map_read, int map_write )
{
  memory_map_2k_read_write( address, source, page_num * 2, map_read, map_write );
  memory_map_2k_read_write( address + 0x0800, source, page_num * 2 + 1,
		  	    map_read, map_write );
}

/* Map 2K of memory for either reading, writing or both */
void
memory_map_2k_read_write( libspectrum_word address, memory_page source[],
		          int page_num, int map_read, int map_write )
{
  int i;

  for( i = 0; i < MEMORY_PAGES_IN_2K; i++ ) {
    int page_offset = ( address >> MEMORY_PAGE_SIZE_LOGARITHM ) + i;
    memory_page *page = &source[ page_num * MEMORY_PAGES_IN_2K + i ];
    if( map_read ) memory_map_read[ page_offset ] = *page;
    if( map_write ) memory_map_write[ page_offset ] = *page;
  }
}

/* Map one page of memory */
void
memory_map_page( memory_page *source[], int page_num )
{
  memory_map_read[ page_num ] = memory_map_write[ page_num ] =
    *source[ page_num ];
}

/* Page in 16k from /ROMCS */
void
memory_map_romcs_full( memory_page source[] )
{
  memory_map_16k( 0x0000, source, 0 );
}

/* Page in 8K from /ROMCS */
void
memory_map_romcs_8k( libspectrum_word address, memory_page source[] )
{
  memory_map_8k( address, source, 0 );
}

/* Page in 4K from /ROMCS */
void
memory_map_romcs_4k( libspectrum_word address, memory_page source[] )
{
  memory_map_4k_read_write( address, source, 0, 1, 1 );
}

/* Page in 2K from /ROMCS */
void
memory_map_romcs_2k( libspectrum_word address, memory_page source[] )
{
  memory_map_2k_read_write( address, source, 0, 1, 1 );
}

libspectrum_byte
readbyte( libspectrum_word address )
{
  libspectrum_word bank;
  memory_page *mapping;

  bank = address >> MEMORY_PAGE_SIZE_LOGARITHM;
  mapping = &memory_map_read[ bank ];

  if( debugger_mode != DEBUGGER_MODE_INACTIVE )
    debugger_check( DEBUGGER_BREAKPOINT_TYPE_READ, address );

  if( mapping->contended ) tstates += ula_contention[ tstates ];
  tstates += 3;

  if( opus_active && address >= 0x2800 && address < 0x3800 )
    return opus_read( address );

  if( spectranet_paged ) {
    if( spectranet_w5100_paged_a && address >= 0x1000 && address < 0x2000 )
      return spectranet_w5100_read( mapping, address );
    if( spectranet_w5100_paged_b && address >= 0x2000 && address < 0x3000 )
      return spectranet_w5100_read( mapping, address );
  }

  return mapping->page[ address & MEMORY_PAGE_SIZE_MASK ];
}

void
writebyte( libspectrum_word address, libspectrum_byte b )
{
  libspectrum_word bank;
  memory_page *mapping;

  bank = address >> MEMORY_PAGE_SIZE_LOGARITHM;
  mapping = &memory_map_write[ bank ];

  if( debugger_mode != DEBUGGER_MODE_INACTIVE )
    debugger_check( DEBUGGER_BREAKPOINT_TYPE_WRITE, address );

  if( mapping->contended ) tstates += ula_contention[ tstates ];

  tstates += 3;

  writebyte_internal( address, b );
}

void
memory_display_dirty_pentagon_16_col( libspectrum_word address,
                                      libspectrum_byte b )
{
  libspectrum_word bank = address >> MEMORY_PAGE_SIZE_LOGARITHM;
  memory_page *mapping = &memory_map_write[ bank ];
  libspectrum_word offset = address & MEMORY_PAGE_SIZE_MASK;
  libspectrum_byte *memory = mapping->page;

  /* The offset into the 16Kb RAM page (as opposed to the 2Kb chunk) */
  libspectrum_word offset2 = offset + mapping->offset;

  /* If this is a write to the current screen areas (and it actually changes
     the destination), redraw that bit.
     The trick here is that we need to check the home bank screen areas in
     page 5 and 4 (if screen 1 is in use), and page 7 & 6 (if screen 2 is in
     use) and both the standard and ALTDFILE areas of those pages
   */
  if( mapping->source == memory_source_ram && 
      ( ( memory_current_screen  == 5 &&
          ( mapping->page_num == 5 || mapping->page_num == 4 ) ) ||
        ( memory_current_screen  == 7 &&
          ( mapping->page_num == 7 || mapping->page_num == 6 ) ) ) &&
      ( offset2 & 0xdfff ) < 0x1b00 &&
      memory[ offset ] != b )
    display_dirty_pentagon_16_col( offset2 );
}

void
memory_display_dirty_sinclair( libspectrum_word address, libspectrum_byte b ) \
{
  libspectrum_word bank = address >> MEMORY_PAGE_SIZE_LOGARITHM;
  memory_page *mapping = &memory_map_write[ bank ];
  libspectrum_word offset = address & MEMORY_PAGE_SIZE_MASK;
  libspectrum_byte *memory = mapping->page;

  /* The offset into the 16Kb RAM page (as opposed to the 2Kb chunk) */
  libspectrum_word offset2 = offset + mapping->offset;

  /* If this is a write to the current screen (and it actually changes
     the destination), redraw that bit */
  if( mapping->source == memory_source_ram && 
      mapping->page_num == memory_current_screen &&
      ( offset2 & memory_screen_mask ) < 0x1b00 &&
      memory[ offset ] != b )
    display_dirty( offset2 );
}

memory_display_dirty_fn memory_display_dirty;

void
writebyte_internal( libspectrum_word address, libspectrum_byte b )
{
  libspectrum_word bank = address >> MEMORY_PAGE_SIZE_LOGARITHM;
  memory_page *mapping = &memory_map_write[ bank ];
  
  if( spectranet_paged ) {
    /* all writes need to be parsed by the flash rom emulation */
    spectranet_flash_rom_write(address, b);
    
    if( spectranet_w5100_paged_a && address >= 0x1000 && address < 0x2000 ) {
      spectranet_w5100_write( mapping, address, b );
      return;
    }
    if( spectranet_w5100_paged_b && address >= 0x2000 && address < 0x3000 ) {
      spectranet_w5100_write( mapping, address, b );
      return;
    }
  }

  if( opus_active && address >= 0x2800 && address < 0x3800 ) {
    opus_write( address, b );
  } else if( mapping->writable ||
             (mapping->source != memory_source_none &&
              settings_current.writable_roms) ) {
    libspectrum_word offset = address & MEMORY_PAGE_SIZE_MASK;
    libspectrum_byte *memory = mapping->page;

    memory_display_dirty( address, b );

    memory[ offset ] = b;
  }
}

void
memory_romcs_map( void )
{
  /* Nothing changes if /ROMCS is not set */
  if( !machine_current->ram.romcs ) return;

  /* FIXME: what should we do if more than one of these devices is
     active? What happen in the real situation? e.g. if1+if2 with cartridge?
     
     OK. in the Interface 1 service manual: p.: 1.2 par.: 1.3.1
       All the additional software needed in IC2 (the if1 ROM). IC2 enable
       is discussed in paragraph 1.2.2 above. In addition to control from
       IC1 (the if1 ULA), the ROM maybe disabled by a device connected to
       the (if1's) expansion connector J1. ROMCS2 from (B25), for example,
       Interface 2 connected to J1 would disable both ROM IC2 (if1 ROM) and
       the Spectrum ROM, via isolating diodes D10 and D9 respectively.
     
     All comment in parenthesis added by me (Gergely Szasz).
     The ROMCS2 (B25 conn) in Interface 1 J1 edge connector is in the
     same position than ROMCS (B25 conn) in the Spectrum edge connector.
     
   */

  module_romcs();
}

static void
memory_from_snapshot( libspectrum_snap *snap )
{
  size_t i;
  int capabilities = machine_current->capabilities;

  if( capabilities & LIBSPECTRUM_MACHINE_CAPABILITY_PENT1024_MEMORY ) {
    pentagon1024_memoryport_write( 0x7ffd,
			      libspectrum_snap_out_128_memoryport( snap ) );
    pentagon1024_v22_memoryport_write( 0xeff7,
                              libspectrum_snap_out_plus3_memoryport( snap ) );
  } else {
    if( capabilities & LIBSPECTRUM_MACHINE_CAPABILITY_128_MEMORY )
      spec128_memoryport_write( 0x7ffd,
                                libspectrum_snap_out_128_memoryport( snap ) );

    if( ( capabilities & LIBSPECTRUM_MACHINE_CAPABILITY_PLUS3_MEMORY ) ||
        ( capabilities & LIBSPECTRUM_MACHINE_CAPABILITY_SCORP_MEMORY )    )
      specplus3_memoryport2_write_internal(
        0x1ffd, libspectrum_snap_out_plus3_memoryport( snap )
      );
  }

  for( i = 0; i < 64; i++ )
    if( libspectrum_snap_pages( snap, i ) )
      memcpy( RAM[i], libspectrum_snap_pages( snap, i ), 0x4000 );

  if( libspectrum_snap_custom_rom( snap ) ) {
    for( i = 0; i < libspectrum_snap_custom_rom_pages( snap ) && i < 4; i++ ) {
      if( libspectrum_snap_roms( snap, i ) ) {
        machine_load_rom_bank_from_buffer( memory_map_rom, i,
          libspectrum_snap_roms( snap, i ),
          libspectrum_snap_rom_length( snap, i ), 1 );
      }
    }
  }
}

static void
write_rom_to_snap( libspectrum_snap *snap, int *current_rom_num,
                   libspectrum_byte **current_rom, size_t *rom_length )
{
  libspectrum_snap_set_roms( snap, *current_rom_num, *current_rom );
  libspectrum_snap_set_rom_length( snap, *current_rom_num, *rom_length );
  (*current_rom_num)++;
  *current_rom = NULL;
}

/* Look at all ROM entries, to see if any are marked as being custom ROMs */
int
memory_custom_rom( void )
{
  size_t i;

  for( i = 0; i < SPECTRUM_ROM_PAGES * MEMORY_PAGES_IN_16K; i++ )
    if( memory_map_rom[ i ].save_to_snapshot )
      return 1;

  return 0;
}

/* Reset all ROM entries to being non-custom ROMs */
void
memory_reset( void )
{
  size_t i;

  for( i = 0; i < SPECTRUM_ROM_PAGES * MEMORY_PAGES_IN_16K; i++ )
    memory_map_rom[ i ].save_to_snapshot = 0;
}

static void
memory_rom_to_snapshot( libspectrum_snap *snap )
{
  libspectrum_byte *current_rom = NULL;
  int current_page_num = -1;
  int current_rom_num = 0;
  size_t rom_length = 0;
  size_t i;

  /* If we have custom ROMs trigger writing all roms to the snap */
  if( !memory_custom_rom() ) return;

  libspectrum_snap_set_custom_rom( snap, 1 );

  /* write all ROMs to the snap */
  for( i = 0; i < SPECTRUM_ROM_PAGES * MEMORY_PAGES_IN_16K; i++ ) {
    if( memory_map_rom[ i ].page ) {
      if( current_page_num != memory_map_rom[ i ].page_num ) {
        if( current_rom )
          write_rom_to_snap( snap, &current_rom_num, &current_rom, &rom_length );

        /* Start a new ROM image */
        rom_length = MEMORY_PAGE_SIZE;
        current_rom = libspectrum_new( libspectrum_byte, rom_length );

        memcpy( current_rom, memory_map_rom[ i ].page, MEMORY_PAGE_SIZE );
        current_page_num = memory_map_rom[ i ].page_num;
      } else {
        /* Extend the current ROM image */
        current_rom = libspectrum_renew( libspectrum_byte, current_rom,
                                         rom_length + MEMORY_PAGE_SIZE );

        memcpy( current_rom + rom_length, memory_map_rom[ i ].page,
                MEMORY_PAGE_SIZE );
        rom_length += MEMORY_PAGE_SIZE;
      }
    }
  }

  if( current_rom )
    write_rom_to_snap( snap, &current_rom_num, &current_rom, &rom_length );

  libspectrum_snap_set_custom_rom_pages( snap, current_rom_num );
}

static void
memory_to_snapshot( libspectrum_snap *snap )
{
  size_t i;
  libspectrum_byte *buffer;

  libspectrum_snap_set_out_128_memoryport( snap,
					   machine_current->ram.last_byte );
  libspectrum_snap_set_out_plus3_memoryport( snap,
					     machine_current->ram.last_byte2 );

  for( i = 0; i < 64; i++ ) {
    if( RAM[i] != NULL ) {

      buffer = libspectrum_new( libspectrum_byte, 0x4000 );

      memcpy( buffer, RAM[i], 0x4000 );
      libspectrum_snap_set_pages( snap, i, buffer );
    }
  }

  memory_rom_to_snapshot( snap );
}

/* Check whether we're actually in the right ROM when a tape or other traps
   hit */
int
trap_check_rom( trap_type type )
{
  if( plusd_available && plusd_active )
    return 0;		/* +D must not be active */

  if( disciple_available && disciple_active )
    return 0;		/* DISCiPLE must not be active */

  if( opus_available && opus_active )
    return 0;		/* Opus must not be active */

  switch( machine_current->machine ) {
  case LIBSPECTRUM_MACHINE_16:
  case LIBSPECTRUM_MACHINE_48:
  case LIBSPECTRUM_MACHINE_48_NTSC:
  case LIBSPECTRUM_MACHINE_SE:
  case LIBSPECTRUM_MACHINE_TC2048:
    return 1;		/* Always OK here */

  case LIBSPECTRUM_MACHINE_TC2068:
  case LIBSPECTRUM_MACHINE_TS2068:
    if( type == CHECK_TAPE_ROM )
      /* OK if we're in the EXROM (location of the tape routines) */
      return( memory_map_read[0].source == memory_source_exrom );
    else
      /* OK if we're in the min TS2068 ROM */
      return( machine_current->ram.current_rom == 0 );

  case LIBSPECTRUM_MACHINE_128:
  case LIBSPECTRUM_MACHINE_PLUS2:
    /* OK if we're in ROM 1 */
    return( machine_current->ram.current_rom == 1 );

  case LIBSPECTRUM_MACHINE_PLUS2A:
  case LIBSPECTRUM_MACHINE_PLUS3:
  case LIBSPECTRUM_MACHINE_PLUS3E:
    /* OK if we're not in a 64Kb RAM configuration and we're in
       ROM 3 */
    return( ! machine_current->ram.special &&
            machine_current->ram.current_rom == 3 );

  case LIBSPECTRUM_MACHINE_128E:
    /* OK if we're not in a 64Kb RAM configuration and we're in
       either ROM 1 or ROM 3 (which are the same) */
    return( ! machine_current->ram.special &&
            ( machine_current->ram.current_rom == 1 ||
              machine_current->ram.current_rom == 3    ));

  case LIBSPECTRUM_MACHINE_PENT:
  case LIBSPECTRUM_MACHINE_PENT512:
  case LIBSPECTRUM_MACHINE_PENT1024:
  case LIBSPECTRUM_MACHINE_SCORP:
    /* OK if we're in ROM 1 and the Beta disk interface is not active */
    return( machine_current->ram.current_rom == 1 && !beta_active );

  case LIBSPECTRUM_MACHINE_UNKNOWN:	/* should never happen */
    ui_error( UI_ERROR_ERROR,
              "trap_check_rom: machine type is LIBSPECTRUM_MACHINE_UNKNOWN" );
    fuse_abort();

  }

  ui_error( UI_ERROR_ERROR, "trap_check_rom: unknown machine type %d",
            machine_current->machine );
  fuse_abort();
}
