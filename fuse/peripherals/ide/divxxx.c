/* divxxx.c: Shared DivIDE/DivMMC interface routines
   Copyright (c) 2005-2017 Matthew Westcott, Stuart Brady, Philip Kendall

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

   E-mail: Philip Kendall <philip-fuse@shadowmagic.org.uk>

*/

#include <config.h>

#include <string.h>

#include <libspectrum.h>

#include "debugger/debugger.h"
#include "divxxx.h"
#include "machine.h"
#include "periph.h"

static const libspectrum_byte DIVXXX_CONTROL_CONMEM = 0x80;
static const libspectrum_byte DIVXXX_CONTROL_MAPRAM = 0x40;

#define DIVXXX_PAGE_LENGTH 0x2000

struct divxxx_t {
  libspectrum_byte control;

  int active;

  /* automap tracks opcode fetches to entry and exit points to determine
     whether interface memory *would* be paged in at this moment if mapram / wp
     flags allowed it */
  int automap;

  /* True once memory has been allocated for this interface */
  int memory_allocated;

  /* EPROM */
  int eprom_memory_source;
  memory_page memory_map_eprom[ MEMORY_PAGES_IN_8K ];
  libspectrum_byte *eprom;

  /* RAM */
  size_t ram_page_count;
  int ram_memory_source;
  memory_page **memory_map_ram;
  libspectrum_byte **ram;

  /* The debugger paging events for this interface */
  int page_event;
  int unpage_event;

  /* References to the current settings for this interface */
  const int *enabled;
  const int *write_protect;

};

divxxx_t*
divxxx_alloc( const char *eprom_source_name, size_t ram_page_count,
    const char *ram_source_name, const char *event_type_string,
    const int *enabled, const int *write_protect )
{
  size_t i, j;
  divxxx_t *divxxx = libspectrum_new( divxxx_t, 1 );

  divxxx->control = 0;
  divxxx->active = 0;
  divxxx->automap = 0;

  divxxx->memory_allocated = 0;

  divxxx->eprom_memory_source = memory_source_register( eprom_source_name );
  for( i = 0; i < MEMORY_PAGES_IN_8K; i++ ) {
    memory_page *page = &divxxx->memory_map_eprom[i];
    page->source = divxxx->eprom_memory_source;
    page->contended = 0;
    page->page_num = 0;
  }
  divxxx->eprom = NULL;

  divxxx->ram_page_count = ram_page_count;
  divxxx->ram_memory_source = memory_source_register( ram_source_name );
  divxxx->memory_map_ram =
    libspectrum_new( memory_page*, divxxx->ram_page_count );
  for( i = 0; i < divxxx->ram_page_count; i++ ) {
    divxxx->memory_map_ram[i] =
      libspectrum_new( memory_page, MEMORY_PAGES_IN_8K );
    for( j = 0; j < MEMORY_PAGES_IN_8K; j++ ) {
      memory_page *page = &divxxx->memory_map_ram[i][j];
      page->source = divxxx->ram_memory_source;
      page->contended = 0;
      page->page_num = i;
    }
  }
  divxxx->ram = NULL;

  periph_register_paging_events( event_type_string, &divxxx->page_event,
                                 &divxxx->unpage_event );

  divxxx->enabled = enabled;
  divxxx->write_protect = write_protect;

  return divxxx;
}

void
divxxx_free( divxxx_t *divxxx )
{
  size_t i;

  for( i = 0; i < divxxx->ram_page_count; i++ )
    libspectrum_free( divxxx->memory_map_ram[i] );
  libspectrum_free( divxxx->memory_map_ram );
  libspectrum_free( divxxx->ram );

  libspectrum_free( divxxx );
}

libspectrum_byte
divxxx_get_control( divxxx_t *divxxx )
{
  return divxxx->control;
}

int
divxxx_get_active( divxxx_t *divxxx )
{
  return divxxx->active;
}

int
divxxx_get_eprom_memory_source( divxxx_t *divxxx )
{
  return divxxx->eprom_memory_source;
}

memory_page*
divxxx_get_eprom_page( divxxx_t *divxxx, size_t which )
{
  return &divxxx->memory_map_eprom[ which ];
}

libspectrum_byte*
divxxx_get_eprom( divxxx_t *divxxx )
{
  return divxxx->eprom;
}

int
divxxx_get_ram_memory_source( divxxx_t *divxxx )
{
  return divxxx->ram_memory_source;
}

libspectrum_byte*
divxxx_get_ram( divxxx_t *divxxx, size_t which )
{
  return divxxx->ram[ which ];
}

/* DivIDE/DivMMC does not page in immediately on a reset condition (we do that by
   trapping PC instead); however, it needs to perform housekeeping tasks upon
   reset */
void
divxxx_reset( divxxx_t *divxxx, int hard_reset )
{
  int i;

  divxxx->active = 0;

  if( !*divxxx->enabled ) return;

  if( hard_reset ) {
    divxxx->control = 0;

    if( divxxx->ram ) {
      for( i = 0; i < divxxx->ram_page_count; i++ )
        memset( divxxx->ram[i], 0, DIVXXX_PAGE_LENGTH );
    }
  } else {
    divxxx->control &= DIVXXX_CONTROL_MAPRAM;
  }
  divxxx->automap = 0;
  divxxx_refresh_page_state( divxxx );
}

void
divxxx_activate( divxxx_t *divxxx )
{
  if( !divxxx->memory_allocated ) {
    int i, j;
    libspectrum_byte *memory =
      memory_pool_allocate_persistent( divxxx->ram_page_count * DIVXXX_PAGE_LENGTH, 1 );

    divxxx->ram = libspectrum_new( libspectrum_byte*, divxxx->ram_page_count );

    for( i = 0; i < divxxx->ram_page_count; i++ ) {
      divxxx->ram[i] = memory + i * DIVXXX_PAGE_LENGTH;
      for( j = 0; j < MEMORY_PAGES_IN_8K; j++ ) {
        memory_page *page = &divxxx->memory_map_ram[i][j];
        page->page = divxxx->ram[i] + j * MEMORY_PAGE_SIZE;
        page->offset = j * MEMORY_PAGE_SIZE;
      }
    }

    divxxx->eprom = memory_pool_allocate_persistent( DIVXXX_PAGE_LENGTH, 1 );
    memset( divxxx->eprom, 0xff, DIVXXX_PAGE_LENGTH );
    for( i = 0; i < MEMORY_PAGES_IN_8K; i++ ) {
      memory_page *page = divxxx_get_eprom_page( divxxx, i );
      page->page = divxxx->eprom + i * MEMORY_PAGE_SIZE;
      page->offset = i * MEMORY_PAGE_SIZE;
    }

    divxxx->memory_allocated = 1;
  }
}

void
divxxx_control_write( divxxx_t *divxxx, libspectrum_byte data )
{
  int old_mapram;

  /* MAPRAM bit cannot be reset, only set */
  old_mapram = divxxx->control & DIVXXX_CONTROL_MAPRAM;
  divxxx_control_write_internal( divxxx, data | old_mapram );
}

void
divxxx_control_write_internal( divxxx_t *divxxx, libspectrum_byte data )
{
  divxxx->control = data;
  divxxx_refresh_page_state( divxxx );
}

void
divxxx_set_automap( divxxx_t *divxxx, int automap )
{
  divxxx->automap = automap;
  divxxx_refresh_page_state( divxxx );
}

void
divxxx_refresh_page_state( divxxx_t *divxxx )
{
  if( divxxx->control & DIVXXX_CONTROL_CONMEM ) {
    /* always paged in if conmem enabled */
    divxxx_page( divxxx );
  } else if( *divxxx->write_protect
    || ( divxxx->control & DIVXXX_CONTROL_MAPRAM ) ) {
    /* automap in effect */
    if( divxxx->automap ) {
      divxxx_page( divxxx );
    } else {
      divxxx_unpage( divxxx );
    }
  } else {
    divxxx_unpage( divxxx );
  }
}

void
divxxx_memory_map( divxxx_t *divxxx )
{
  int i;
  int upper_ram_page;
  int lower_page_writable, upper_page_writable;
  memory_page *lower_page, *upper_page;

  if( !divxxx->active ) return;

  upper_ram_page = divxxx->control & (divxxx->ram_page_count - 1);
  
  if( divxxx->control & DIVXXX_CONTROL_CONMEM ) {
    lower_page = divxxx->memory_map_eprom;
    lower_page_writable = !*divxxx->write_protect;
    upper_page = divxxx->memory_map_ram[ upper_ram_page ];
    upper_page_writable = 1;
  } else {
    if( divxxx->control & DIVXXX_CONTROL_MAPRAM ) {
      lower_page = divxxx->memory_map_ram[3];
      lower_page_writable = 0;
      upper_page = divxxx->memory_map_ram[ upper_ram_page ];
      upper_page_writable = ( upper_ram_page != 3 );
    } else {
      lower_page = divxxx->memory_map_eprom;
      lower_page_writable = 0;
      upper_page = divxxx->memory_map_ram[ upper_ram_page ];
      upper_page_writable = 1;
    }
  }

  for( i = 0; i < MEMORY_PAGES_IN_8K; i++ ) {
    lower_page[i].writable = lower_page_writable;
    upper_page[i].writable = upper_page_writable;
  }

  memory_map_romcs_8k( 0x0000, lower_page );
  memory_map_romcs_8k( 0x2000, upper_page );
}

void
divxxx_page( divxxx_t *divxxx )
{
  divxxx->active = 1;
  machine_current->ram.romcs = 1;
  machine_current->memory_map();

  debugger_event( divxxx->page_event );
}

void
divxxx_unpage( divxxx_t *divxxx )
{
  divxxx->active = 0;
  machine_current->ram.romcs = 0;
  machine_current->memory_map();

  debugger_event( divxxx->unpage_event );
}
