/* memory_pages.h: memory access routines
   Copyright (c) 2003-2016 Philip Kendall
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

#ifndef FUSE_MEMORY_PAGES_H
#define FUSE_MEMORY_PAGES_H

#include <libspectrum.h>

/* Register a new memory source */
int memory_source_register( const char *description );

/* Get the description for a given source */
const char *memory_source_description( int source );

/* Get the source for a given description */
int memory_source_find( const char *description );

/* Pre-created memory sources */
extern int memory_source_rom; /* System ROM */
extern int memory_source_ram; /* System RAM */
extern int memory_source_dock; /* Timex DOCK */
extern int memory_source_exrom; /* Timex EXROM */
extern int memory_source_any; /* Used by the debugger to signify an absolute address */
extern int memory_source_none; /* No memory attached here */

typedef struct memory_page {

  libspectrum_byte *page;	/* The data for this page */
  int writable;			/* Can we write to this data? */
  int contended;		/* Are reads/writes to this page contended? */

  int source;	                /* Where did this page come from? */
  int save_to_snapshot;         /* Set if this page should be saved snapshots
                                   (set only if this page would not normally be
                                    saved; things like RAM are always saved) */

  int page_num;			/* Which page from the source */
  libspectrum_word offset;	/* How far into the page this chunk starts */

} memory_page;

/* A memory page will be 1 << (this many) bytes in size
   ie 12 => 4 KB, 13 => 8 KB, 14 => 16 KB
   Note: we now rely on 2KB page size so this should no longer be changed
   without a full review of all relevant code and changes will be required to
   match
 */
#define MEMORY_PAGE_SIZE_LOGARITHM 11

/* The actual size of a memory page */
#define MEMORY_PAGE_SIZE ( 1 << MEMORY_PAGE_SIZE_LOGARITHM )

/* The mask to use to select the bits within a page */
#define MEMORY_PAGE_SIZE_MASK ( MEMORY_PAGE_SIZE - 1 )

/* The number of memory pages in 64K
   This calculation is equivalent to 2^16 / MEMORY_PAGE_SIZE */
#define MEMORY_PAGES_IN_64K ( 1 << ( 16 - MEMORY_PAGE_SIZE_LOGARITHM ) )

/* The number of memory pages in 16K */
#define MEMORY_PAGES_IN_16K ( 1 << ( 14 - MEMORY_PAGE_SIZE_LOGARITHM ) )

/* The number of memory pages in 8K */
#define MEMORY_PAGES_IN_8K ( 1 << ( 13 - MEMORY_PAGE_SIZE_LOGARITHM ) )

/* The number of memory pages in 4K */
#define MEMORY_PAGES_IN_4K ( 1 << ( 12 - MEMORY_PAGE_SIZE_LOGARITHM ) )

/* The number of memory pages in 2K */
#define MEMORY_PAGES_IN_2K ( 1 << ( 11 - MEMORY_PAGE_SIZE_LOGARITHM ) )

/* The number of memory pages in 14K */
#define MEMORY_PAGES_IN_14K ( MEMORY_PAGES_IN_16K - MEMORY_PAGES_IN_2K )

/* The number of memory pages in 12K */
#define MEMORY_PAGES_IN_12K ( MEMORY_PAGES_IN_16K - MEMORY_PAGES_IN_4K )

/* Each RAM chunk accessible by the Z80 */
extern memory_page memory_map_read[MEMORY_PAGES_IN_64K];
extern memory_page memory_map_write[MEMORY_PAGES_IN_64K];

/* The number of 16Kb RAM pages we support: 1040 Kb needed for the Pentagon 1024 */
#define SPECTRUM_RAM_PAGES 65

/* The maximum number of 16Kb ROMs we support */
#define SPECTRUM_ROM_PAGES 4

extern memory_page memory_map_ram[SPECTRUM_RAM_PAGES * MEMORY_PAGES_IN_16K];
extern memory_page memory_map_rom[SPECTRUM_ROM_PAGES * MEMORY_PAGES_IN_16K];

/* Which RAM page contains the current screen */
extern int memory_current_screen;

/* Which bits to look at when working out where the screen is */
extern libspectrum_word memory_screen_mask;

void memory_register_startup( void );
libspectrum_byte *memory_pool_allocate( size_t length );
libspectrum_byte *memory_pool_allocate_persistent( size_t length,
                                                   int persistent );
void memory_pool_free( void );

/* Map in alternate bank if ROMCS is set */
void memory_romcs_map( void );

/* Have we loaded any custom ROMs? */
int memory_custom_rom( void );

/* Reset any memory configuration that may have changed in the machine
   configuration */
void memory_reset( void );

/* Set contention for 16K of RAM */
void memory_ram_set_16k_contention( int page_num, int contended );

/* Map 16K of memory */
void memory_map_16k( libspectrum_word address, memory_page source[],
  int page_num );

/* Map 16K of memory for either reading, writing or both */
void memory_map_16k_read_write( libspectrum_word address, memory_page source[],
  int page_num, int map_read, int map_write );

/* Map 8K of memory */
void memory_map_8k( libspectrum_word address, memory_page source[],
  int page_num );

/* Map 8K of memory for either reading, writing or both */
void memory_map_8k_read_write( libspectrum_word address, memory_page source[],
  int page_num, int map_read, int map_write );

/* Map 4K of memory for either reading, writing or both */
void memory_map_4k_read_write( libspectrum_word address, memory_page source[],
  int page_num, int map_read, int map_write );

/* Map 2K of memory for either reading, writing or both */
void memory_map_2k_read_write( libspectrum_word address, memory_page source[],
  int page_num, int map_read, int map_write );

/* Map one page of memory */
void memory_map_page( memory_page *source[], int page_num );

/* Page in all 16K from /ROMCS */
void memory_map_romcs_full( memory_page source[] );

/* Page in 8K from /ROMCS */
void memory_map_romcs_8k( libspectrum_word address, memory_page source[] );

/* Page in 4K from /ROMCS */
void memory_map_romcs_4k( libspectrum_word address, memory_page source[] );

/* Page in 2K from /ROMCS */
void memory_map_romcs_2k( libspectrum_word address, memory_page source[] );

libspectrum_byte readbyte( libspectrum_word address );

/* Use a macro for performance in the main core, but a function for
   flexibility in the core tester */

#ifndef CORETEST

#define readbyte_internal( address ) \
  memory_map_read[ (libspectrum_word)(address) >> MEMORY_PAGE_SIZE_LOGARITHM ].page[ (address) & MEMORY_PAGE_SIZE_MASK ]

#else				/* #ifndef CORETEST */

libspectrum_byte readbyte_internal( libspectrum_word address );

#endif				/* #ifndef CORETEST */

void writebyte( libspectrum_word address, libspectrum_byte b );
void writebyte_internal( libspectrum_word address, libspectrum_byte b );

typedef void (*memory_display_dirty_fn)( libspectrum_word address,
                                         libspectrum_byte b );
extern memory_display_dirty_fn memory_display_dirty;

void memory_display_dirty_sinclair( libspectrum_word address,
                                    libspectrum_byte b );
void memory_display_dirty_pentagon_16_col( libspectrum_word address,
                                           libspectrum_byte b );

typedef enum trap_type {
  CHECK_TAPE_ROM,
  CHECK_48K_ROM
} trap_type;

/* Check whether we're actually in the right ROM when a tape or other traps hit */
extern int trap_check_rom( trap_type type );

#endif				/* #ifndef FUSE_MEMORY_PAGES_H */
