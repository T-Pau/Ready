/* pokefinder.c: help with finding pokes
   Copyright (c) 2003-2004 Philip Kendall

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

#include <libspectrum.h>

#include "machine.h"
#include "memory_pages.h"
#include "pokefinder.h"
#include "spectrum.h"

libspectrum_byte pokefinder_possible[ MEMORY_PAGES_IN_16K * SPECTRUM_RAM_PAGES ][ MEMORY_PAGE_SIZE ];
libspectrum_byte pokefinder_impossible[ MEMORY_PAGES_IN_16K * SPECTRUM_RAM_PAGES ][ MEMORY_PAGE_SIZE / 8 ];
size_t pokefinder_count;

void
pokefinder_clear( void )
{
  size_t page, max_page;

  max_page = MEMORY_PAGES_IN_16K * machine_current->ram.valid_pages;
  pokefinder_count = 0;
  for( page = 0; page < MEMORY_PAGES_IN_16K * SPECTRUM_RAM_PAGES; ++page )
    if( page < max_page && memory_map_ram[page].writable ) {
      pokefinder_count += MEMORY_PAGE_SIZE;
      memcpy( pokefinder_possible[page], memory_map_ram[page].page, MEMORY_PAGE_SIZE );
      memset( pokefinder_impossible[page], 0, MEMORY_PAGE_SIZE / 8 );
    } else
      memset( pokefinder_impossible[page], 255, MEMORY_PAGE_SIZE / 8 );
}

int
pokefinder_search( libspectrum_byte value )
{
  size_t page, offset;

  for( page = 0; page < MEMORY_PAGES_IN_16K * SPECTRUM_RAM_PAGES; page++ ) {
    memory_page *mapping = &memory_map_ram[ page ];

    for( offset = 0; offset < MEMORY_PAGE_SIZE; offset++ ) {
      if( pokefinder_impossible[page][offset/8] & 1 << (offset & 7) ) continue;

      if( mapping->page[offset] != value ) {
	pokefinder_impossible[page][offset/8] |= 1 << (offset & 7);
	pokefinder_count--;
      }
    }
  }

  return 0;
}

int
pokefinder_incremented( void )
{
  size_t page, offset;

  for( page = 0; page < MEMORY_PAGES_IN_16K * SPECTRUM_RAM_PAGES; page++ ) {
    memory_page *mapping = &memory_map_ram[ page ];

    for( offset = 0; offset < MEMORY_PAGE_SIZE; offset++ ) {
      if( pokefinder_impossible[page][offset/8] & 1 << (offset & 7) ) continue;

      if( mapping->page[offset] > pokefinder_possible[page][offset] ) {
        pokefinder_possible[page][offset] = mapping->page[offset];
      } else {
	pokefinder_impossible[page][offset/8] |= 1 << (offset & 7);
	pokefinder_count--;
      }

    }
  }

  return 0;
}

int
pokefinder_decremented( void )
{
  size_t page, offset;

  for( page = 0; page < MEMORY_PAGES_IN_16K * SPECTRUM_RAM_PAGES; page++ ) {
    memory_page *mapping = &memory_map_ram[ page ];

    for( offset = 0; offset < MEMORY_PAGE_SIZE; offset++ ) {
      if( pokefinder_impossible[page][offset/8] & 1 << (offset & 7) ) continue;

      if( mapping->page[offset] < pokefinder_possible[page][offset] ) {
        pokefinder_possible[page][offset] = mapping->page[offset];
      } else {
	pokefinder_impossible[page][offset/8] |= 1 << (offset & 7);
	pokefinder_count--;
      }

    }
  }

  return 0;
}
