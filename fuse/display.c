/* display.c: Routines for printing the Spectrum screen
   Copyright (c) 1999-2015 Philip Kendall, Thomas Harte, Witold Filipczyk
                           and Fredrick Meunier

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

#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "display.h"
#include "fuse.h"
#include "infrastructure/startup_manager.h"
#include "machine.h"
#include "movie.h"
#include "peripherals/scld.h"
#include "rectangle.h"
#include "screenshot.h"
#include "settings.h"
#include "spectrum.h"
#include "ui/ui.h"
#include "ui/uidisplay.h"

/* Set once we have initialised the UI */
int display_ui_initialised = 0;

/* The current border colour */
libspectrum_byte display_lores_border;
libspectrum_byte display_hires_border;
libspectrum_byte display_last_border;

/* Stores the pixel, attribute and SCLD screen mode information used to
   draw each 8x1 group of pixels (including border) last frame */
libspectrum_dword
display_last_screen[ DISPLAY_SCREEN_WIDTH_COLS * DISPLAY_SCREEN_HEIGHT ];

/* Offsets as to where the data and the attributes for each pixel
   line start */
libspectrum_word display_line_start[ DISPLAY_HEIGHT ];
libspectrum_word display_attr_start[ DISPLAY_HEIGHT ];

/* If you write to the byte at display_dirty_?table[n+0x4000], then
   the eight pixels starting at (8*xtable[n],ytable[n]) must be
   replotted */
static libspectrum_word
  display_dirty_ytable[ DISPLAY_WIDTH_COLS * DISPLAY_HEIGHT ];
static libspectrum_word
  display_dirty_xtable[ DISPLAY_WIDTH_COLS * DISPLAY_HEIGHT ];

/* If you write to the byte at display_dirty_?table2[n+0x5800], then
   the 64 pixels starting at (8*xtable2[n],ytable2[n]) must be
   replotted */
static libspectrum_word
  display_dirty_ytable2[ DISPLAY_WIDTH_COLS * DISPLAY_HEIGHT_ROWS ];
static libspectrum_word
  display_dirty_xtable2[ DISPLAY_WIDTH_COLS * DISPLAY_HEIGHT_ROWS ];

/* The number of frames mod 32 that have elapsed.
    0<=d_f_c<16 => Flashing characters are normal
   16<=d_f_c<32 => Flashing characters are reversed
*/
static int display_frame_count;
static int display_flash_reversed;

/* Which eight-pixel chunks on each line (including border) need to
   be redisplayed. Bit 0 corresponds to pixels 0-7, bit 39 to
   pixels 311-319. */
static libspectrum_qword display_is_dirty[ DISPLAY_SCREEN_HEIGHT ];

/* Which eight-pixel chunks on each line may need to be redisplayed. Bit 0
   corresponds to pixels 0-7, bit 31 to pixels 248-255. */
static libspectrum_dword display_maybe_dirty[ DISPLAY_HEIGHT ];

/* This value signifies that the entire line must be redisplayed */
static libspectrum_qword display_all_dirty;

/* Used to signify that we're redrawing the entire screen */
static int display_redraw_all;

/* The last point at which we updated the screen display */
int critical_region_x = 0, critical_region_y = 0;

/* The border colour changes which have occurred in this frame */
struct border_change_t {
  int x, y;
  int colour;
};

display_dirty_fn display_dirty;
display_write_if_dirty_fn display_write_if_dirty;

static struct border_change_t border_change_end_sentinel =
  { DISPLAY_SCREEN_WIDTH_COLS, DISPLAY_SCREEN_HEIGHT - 1, 0 };

/* The current border colour */
int current_border[ DISPLAY_SCREEN_HEIGHT ][ DISPLAY_SCREEN_WIDTH_COLS ];

static void display_dirty8( libspectrum_word address );
static void display_dirty64( libspectrum_word address );

static void display_get_attr( int x, int y,
			      libspectrum_byte *ink, libspectrum_byte *paper);

static int border_changes_last = 0;
static struct border_change_t *border_changes = NULL;

static struct border_change_t *
alloc_change(void)
{
  static int border_changes_size = 0;

  if( border_changes_size == border_changes_last ) {
    border_changes_size += 10;
    border_changes = libspectrum_renew( struct border_change_t,
                                        border_changes, border_changes_size );
  }
  return border_changes + border_changes_last++; 
}

static int
add_border_sentinel( void )
{
  struct border_change_t *sentinel = alloc_change();

  sentinel->x = sentinel->y = 0;
  sentinel->colour = scld_last_dec.name.hires ?
                            display_hires_border : display_lores_border;

  return 0;
}

int
display_init( int *argc, char ***argv )
{
  int i, j, k, x, y;
  int error;

  if(ui_init(argc, argv))
    return 1;

  /* Set up the 'all pixels must be refreshed' marker */
  display_all_dirty = 0;
  for( i = 0; i < DISPLAY_SCREEN_WIDTH_COLS; i++ )
    display_all_dirty = ( display_all_dirty << 1 ) | 0x01;

  for(i=0;i<3;i++)
    for(j=0;j<8;j++)
      for(k=0;k<8;k++)
	display_line_start[ (64*i) + (8*j) + k ] =
	  32 * ( (64*i) + j + (k*8) );

  for(y=0;y<DISPLAY_HEIGHT;y++) {
    display_attr_start[y]=6144 + (32*(y/8));
  }

  for(y=0;y<DISPLAY_HEIGHT;y++)
    for(x=0;x<DISPLAY_WIDTH_COLS;x++) {
      display_dirty_ytable[ display_line_start[y]+x ] = y;
      display_dirty_xtable[ display_line_start[y]+x ] = x;
    }

  for(y=0;y<DISPLAY_HEIGHT_ROWS;y++)
    for(x=0;x<DISPLAY_WIDTH_COLS;x++) {
      display_dirty_ytable2[ (32*y) + x ] = y * 8;
      display_dirty_xtable2[ (32*y) + x ] = x;
    }

  display_frame_count=0; display_flash_reversed=0;

  display_refresh_all();

  border_changes_last = 0;
  if( border_changes ) {
    libspectrum_free( border_changes );
  }
  border_changes = NULL;
  error = add_border_sentinel(); if( error ) return error;
  display_last_border = scld_last_dec.name.hires ?
                            display_hires_border : display_lores_border;

  return 0;
}

static int
display_init_wrapper( void *context )
{
  display_startup_context *typed_context =
    (display_startup_context*) context;

  return display_init( typed_context->argc, typed_context->argv );
}

void
display_register_startup( display_startup_context *context )
{
  /* The Wii has an explicit call to display_init for now */
#ifndef GEKKO
  startup_manager_register_no_dependencies( STARTUP_MANAGER_MODULE_DISPLAY,
                                            display_init_wrapper, context,
                                            NULL );
#endif                          /* #ifndef GEKKO */
}

/* Mark as 'dirty' the pixels which have been changed by a write to
   'offset' within the RAM page containing the screen */
void
display_dirty_timex( libspectrum_word offset )
{
  switch ( scld_last_dec.mask.scrnmode ) {

    case STANDARD: /* standard Speccy screen */
    case HIRESATTR: /* strange mode */
      if( offset >= 0x1b00 ) break;
      if( offset <  0x1800 ) {		/* 0x1800 = first attributes byte */
        display_dirty8( offset );
      } else {
        display_dirty64( offset );
      }
      break;

    case ALTDFILE: /* second screen */
    case HIRESATTRALTD: /* strange mode using second screen */      
      if( offset < 0x2000 || offset >= 0x3b00 ) break;
      if( offset < 0x3800 ) {		/* 0x3800 = first attributes byte */
        display_dirty8( offset - ALTDFILE_OFFSET );
      } else {
        display_dirty64( offset - ALTDFILE_OFFSET );
      }
      break;

    case EXTCOLOUR: /* extended colours */
    case HIRES: /* hires mode */
      if( offset >= 0x3800 ) break;
      if( offset >= 0x1800 && offset < 0x2000 ) break;
      if( offset >= 0x2000 ) offset -= ALTDFILE_OFFSET;
      display_dirty8( offset );
      break;

    default:
    /* case EXTCOLALTD: extended colours, but attributes and data
       taken from second screen */
    /* case HIRESDOUBLECOL: hires mode, but data taken only from
       second screen */
      if( offset >= 0x2000 && offset < 0x3800 )
	display_dirty8( offset - ALTDFILE_OFFSET );
      break;
  }
}

void
display_dirty_pentagon_16_col( libspectrum_word offset )
{
  /* The only relevant sections of the page will be the two 6144 byte sections
     separated by ALTDFILE_OFFSET, which have the same display offset */
  if( offset >= 0x2000 ) offset -= ALTDFILE_OFFSET;
  /* No attributes are relevent in this mode */
  if( offset <  0x1800 ) {		/* 0x1800 = first attributes byte */
    display_dirty8( offset );
  }
}

void
display_dirty_sinclair( libspectrum_word offset )
{
  if( offset >= 0x1b00 ) return;
  if( offset <  0x1800 ) {		/* 0x1800 = first attributes byte */
    display_dirty8( offset );
  } else {
    display_dirty64( offset );
  }
}

/* Get the attribute byte or equivalent for the eight pixels starting at
   ( (8*x) , y ) */
static inline libspectrum_byte
display_get_attr_byte( int x, int y )
{
  libspectrum_byte attr;

  if ( scld_last_dec.name.hires ) {
    attr = hires_get_attr();
  } else {

    libspectrum_word offset;

    if( scld_last_dec.name.b1 ) {
      offset = display_line_start[y] + x + ALTDFILE_OFFSET;
    } else if( scld_last_dec.name.altdfile ) {
      offset = display_attr_start[y] + x + ALTDFILE_OFFSET;
    } else {
      offset = display_attr_start[y] + x;
    }

    attr = RAM[ memory_current_screen ][ offset ];
  }

  return attr;
}

static void
update_dirty_rects( void )
{
  int start, y;

  for( y=0; y<DISPLAY_SCREEN_HEIGHT; y++ ) {
    int x = 0;
    while( display_is_dirty[y] ) {

      /* Find the first dirty chunk on this row */
      while( !( display_is_dirty[y] & 0x01 ) ) {
        display_is_dirty[y] >>= 1;
        x++;
      }

      start = x;

      /* Walk to the end of the dirty region */
      do {
        display_is_dirty[y] >>= 1;
        x++;
      } while( display_is_dirty[y] & 0x01 );

      rectangle_add( y, start, x - start );
    }

    /* compress the active rectangles list */
    rectangle_end_line( y );
  }

  /* Force all rectangles into the inactive list */
  rectangle_end_line( DISPLAY_SCREEN_HEIGHT );
}

void
display_write_if_dirty_timex( int x, int y )
{
  int beam_x, beam_y;
  int index;
  libspectrum_word offset;
  libspectrum_byte *screen;
  libspectrum_byte data, data2;
  libspectrum_dword mode_data;
  libspectrum_dword last_chunk_detail;

  beam_x = x + DISPLAY_BORDER_WIDTH_COLS;
  beam_y = y + DISPLAY_BORDER_HEIGHT;
  offset = display_get_addr( x, y );

  /* Read byte, atrr/byte, and screen mode */
  screen = RAM[ memory_current_screen ];
  data = screen[ offset ];
  mode_data = scld_last_dec.byte;

  if( scld_last_dec.name.hires ) {
    switch( scld_last_dec.mask.scrnmode ) {

    case HIRESATTRALTD:
      offset = display_attr_start[ y ] + x + ALTDFILE_OFFSET;
      data2 = screen[ offset ];
      break;

    case HIRES:
      data2 = screen[ offset + ALTDFILE_OFFSET ];
      break;

    case HIRESDOUBLECOL:
      data2 = data;
      break;

    default: /* case HIRESATTR: */
      offset = display_attr_start[ y ] + x;
      data2 = screen[ offset ];
      break;

    }
  } else {
    data2 = display_get_attr_byte( x, y );
  }

  last_chunk_detail = (display_flash_reversed << 24) | (mode_data << 16) |
                      (data2 << 8) | data;
  /* And draw it if it is different to what was there last time */
  index = beam_x + beam_y * DISPLAY_SCREEN_WIDTH_COLS;
  if( display_last_screen[ index ] != last_chunk_detail ) {
    libspectrum_byte ink, paper;
    display_get_attr( x, y, &ink, &paper );
    if( scld_last_dec.name.hires ) {
      libspectrum_word hires_data = (data << 8) + data2;
      uidisplay_plot16( beam_x, beam_y, hires_data, ink, paper );
    } else {
      uidisplay_plot8( beam_x, beam_y, data, ink, paper );
    }

    /* Update last display record */
    display_last_screen[ index ] = last_chunk_detail;

    /* And now mark it dirty */
    display_is_dirty[ beam_y ] |= ( (libspectrum_qword)1 << beam_x );
  }
}

static inline void
pentagon_16c_get_colour( libspectrum_byte data, libspectrum_byte *colour1,
                         libspectrum_byte *colour2 )
{
  *colour1 = (data & 0x07) + ( (data & 0x40) >> 3 );
  *colour2 = ( (data & 0x38) >> 3 ) + ( (data & 0x80) >> 4 );
}

/* In this mode we need to gather the pixel information for the 8 pixels to
   be displayed, if current screen is 5 we need to read from pages 5 and 4,
   and if current screen is 7 we need to read from pages 7 and 6. */
void
display_write_if_dirty_pentagon_16_col( int x, int y )
{
  int beam_x, beam_y;
  int index;
  libspectrum_word offset;
  libspectrum_byte *screen;
  libspectrum_byte data1, data2, data3, data4;
  libspectrum_dword last_chunk_detail;
  libspectrum_byte colour1, colour2;

  /* We need to read the pixels from the appropriate two pages and write them
     out to the frame buffer */
  int memory_screen_page_1 = 5;
  int memory_screen_page_2 = 4;

  if( memory_current_screen == 7 ) {
    memory_screen_page_1 = 7;
    memory_screen_page_2 = 6;
  }

  beam_x = x + DISPLAY_BORDER_WIDTH_COLS;
  beam_y = y + DISPLAY_BORDER_HEIGHT;
  offset = display_get_addr( x, y );

  /* Read byte, atrr/byte, and screen mode */
  screen = RAM[ memory_screen_page_1 ];
  data2 = screen[ offset ];
  data4 = screen[ offset + ALTDFILE_OFFSET ];
  screen = RAM[ memory_screen_page_2 ];
  data1 = screen[ offset ];
  data3 = screen[ offset + ALTDFILE_OFFSET ];

  /* This is a bit of a cheat - we'd normally encode the screen mode in here
     as well to support screen mode mixing. I doubt there is much call for
     mixing 16 colour mode with other modes so will assume that as long as
     we are in 16 colour mode the screen we draw is in that mode as it seems
     a shame to chuck more memory at supporting just this obscure mode */
  last_chunk_detail = (data4 << 24) | (data3 << 16) | (data2 << 8) | data1;

  /* And draw it if it is different to what was there last time */
  index = beam_x + beam_y * DISPLAY_SCREEN_WIDTH_COLS;

  if( display_last_screen[ index ] != last_chunk_detail ) {
    /* Print pixel 1 & 2 from screen_page_2 base, pixel 3 & 4 from
       screen_page_1 base, pixel 5 & 6 from screen_page_2 ALTDFILE_OFFSET,
       pixel 7 & 8 from screen_page_1 ALTDFILE_OFFSET */

    int draw_x = beam_x << 3;
    pentagon_16c_get_colour( data1, &colour1, &colour2 );
    uidisplay_putpixel( draw_x++, beam_y, colour1 );
    uidisplay_putpixel( draw_x++, beam_y, colour2 );
    pentagon_16c_get_colour( data2, &colour1, &colour2 );
    uidisplay_putpixel( draw_x++, beam_y, colour1 );
    uidisplay_putpixel( draw_x++, beam_y, colour2 );
    pentagon_16c_get_colour( data3, &colour1, &colour2 );
    uidisplay_putpixel( draw_x++, beam_y, colour1 );
    uidisplay_putpixel( draw_x++, beam_y, colour2 );
    pentagon_16c_get_colour( data4, &colour1, &colour2 );
    uidisplay_putpixel( draw_x++, beam_y, colour1 );
    uidisplay_putpixel( draw_x  , beam_y, colour2 );

    /* Update last display record */
    display_last_screen[ index ] = last_chunk_detail;

    /* And now mark it dirty */
    display_is_dirty[ beam_y ] |= ( (libspectrum_qword)1 << beam_x );
  }
}

void
display_write_if_dirty_sinclair( int x, int y )
{
  int beam_x, beam_y;
  int index;
  libspectrum_word offset;
  libspectrum_byte *screen;
  libspectrum_byte data, data2;
  libspectrum_dword last_chunk_detail;

  beam_x = x + DISPLAY_BORDER_WIDTH_COLS;
  beam_y = y + DISPLAY_BORDER_HEIGHT;
  offset = display_get_addr( x, y );

  /* Read byte, atrr/byte, and screen mode */
  screen = RAM[ memory_current_screen ];
  data = screen[ offset ];
  data2 = display_get_attr_byte( x, y );

  last_chunk_detail = (display_flash_reversed << 24) | (data2 << 8) | data;
  /* And draw it if it is different to what was there last time */
  index = beam_x + beam_y * DISPLAY_SCREEN_WIDTH_COLS;
  if( display_last_screen[ index ] != last_chunk_detail ) {
    libspectrum_byte ink, paper;
    display_parse_attr( data2, &ink, &paper );
    uidisplay_plot8( beam_x, beam_y, data, ink, paper );

    /* Update last display record */
    display_last_screen[ index ] = last_chunk_detail;

    /* And now mark it dirty */
    display_is_dirty[ beam_y ] |= ( (libspectrum_qword)1 << beam_x );
  }
}

/* Plot any dirty data from ( x, y ) to ( end, y ) of the critical
   region to the drawing region */
static void
copy_critical_region_line( int y, int x, int end )
{
  libspectrum_dword bit_mask, dirty;

  if( x < DISPLAY_WIDTH_COLS ) {

    /* Build a mask for the bits we're interested in */
    bit_mask = display_all_dirty;

    bit_mask >>= x;
    bit_mask <<= x + ( 32 - end );
    bit_mask >>= ( 32 - end );

    /* Get the bits we're interested in */
    dirty = ( display_maybe_dirty[y] & bit_mask ) >> x;

    /* And remove those bits from the dirty mask */
    display_maybe_dirty[y] &= ~bit_mask;

  } else {

    dirty = 0;

  }

  while( dirty ) {

    /* Find the first dirty chunk on this row */
    while( !( dirty & 0x01 ) ) {

      dirty >>= 1;
      x++;

    }

    /* Walk to the end of the dirty region, writing the bytes to the
       drawing area along the way */
    do {

      display_write_if_dirty( x, y );

      dirty >>= 1;
      x++;

    } while( dirty & 0x01 );

  }
  
}

/* Copy any dirty data from the critical region to the drawing region */
static void
copy_critical_region( int beam_x, int beam_y )
{
  if( critical_region_y == beam_y ) {

    copy_critical_region_line( critical_region_y, critical_region_x, beam_x );

  } else {

    copy_critical_region_line( critical_region_y++, critical_region_x,
			       DISPLAY_WIDTH_COLS );
  
    for( ; critical_region_y < beam_y; critical_region_y++ )
      copy_critical_region_line( critical_region_y, 0,
				 DISPLAY_WIDTH_COLS );

    copy_critical_region_line( critical_region_y, 0, beam_x );
  }

  critical_region_x = beam_x;
}

static inline void
get_beam_position( int *x, int *y )
{
  if( tstates < machine_current->line_times[ 0 ] ) {
    *x = *y = -1;
    return;
  }

  *y = ( tstates - machine_current->line_times[ 0 ] ) /
    machine_current->timings.tstates_per_line;

  if( *y >= 0 && *y <= DISPLAY_SCREEN_HEIGHT )
    *x = ( tstates - machine_current->line_times[ *y ] ) / 4;
  else *x = 0;
}

void
display_update_critical( int x, int y )
{
  int beam_x, beam_y;

  get_beam_position( &beam_x, &beam_y );

  beam_x -= DISPLAY_BORDER_WIDTH_COLS;
  beam_y -= DISPLAY_BORDER_HEIGHT;

  if( beam_y < 0 ) {
    beam_x = beam_y = 0;
  } else if( beam_y >= DISPLAY_HEIGHT ) {
    beam_x = DISPLAY_WIDTH_COLS;
    beam_y = DISPLAY_HEIGHT - 1;
  }

  if( beam_x < 0 ) {
    beam_x = 0;
  } else if( beam_x > DISPLAY_WIDTH_COLS ) {
    beam_x = DISPLAY_WIDTH_COLS;
  }

  if(   y <  beam_y                 ||
      ( y == beam_y && x < beam_x )    )
    copy_critical_region( beam_x, beam_y );
}

/* Mark the 8-pixel chunk at (x,y) as maybe dirty and update the critical
   region as appropriate */
static inline void
display_dirty_chunk( int x, int y )
{
  /* If the write is between the start of the critical region and the
     current beam position, then we must copy the critical region now */
  if(   y >  critical_region_y                             ||
      ( y == critical_region_y && x >= critical_region_x )    ) {

    display_update_critical( x, y );
  }

  display_maybe_dirty[y] |= ( (libspectrum_dword)1 << x );
}

static void
display_dirty8( libspectrum_word offset )
{
  int x, y;

  x=display_dirty_xtable[ offset ];
  y=display_dirty_ytable[ offset ];

  display_dirty_chunk( x, y );
}

static void
display_dirty64( libspectrum_word offset )
{
  int i, x, y;

  x=display_dirty_xtable2[ offset - 0x1800 ];
  y=display_dirty_ytable2[ offset - 0x1800 ];

  for( i = 0; i < 8; i++ ) display_dirty_chunk( x, y + i );
}

/* Get the attributes for the eight pixels starting at
   ( (8*x) , y ) */
static void
display_get_attr( int x, int y,
                  libspectrum_byte *ink, libspectrum_byte *paper )
{
  display_parse_attr( display_get_attr_byte( x, y ), ink, paper );
}

void
display_parse_attr( libspectrum_byte attr,
		    libspectrum_byte *ink, libspectrum_byte *paper )
{
  if( (attr & 0x80) && display_flash_reversed ) {
    *ink  = (attr & ( 0x0f << 3 ) ) >> 3;
    *paper= (attr & 0x07) + ( (attr & 0x40) >> 3 );
  } else {
    *ink= (attr & 0x07) + ( (attr & 0x40) >> 3 );
    *paper= (attr & ( 0x0f << 3 ) ) >> 3;
  }
}

static void
push_border_change( int colour )
{
  int beam_x, beam_y;
  struct border_change_t *change;

  get_beam_position( &beam_x, &beam_y );

  if( beam_y >= DISPLAY_SCREEN_HEIGHT ) return;

  if( beam_x < 0 ) beam_x = 0;
  if( beam_x > DISPLAY_SCREEN_WIDTH_COLS ) beam_x = DISPLAY_SCREEN_WIDTH_COLS;
  if( beam_y < 0 ) beam_y = 0;

  change = alloc_change();

  change->x = beam_x;
  change->y = beam_y;
  change->colour = colour;
}

/* Change border colour if the colour in use changes */
static void
check_border_change( void )
{
  if( scld_last_dec.name.hires &&
      display_hires_border != display_last_border ) {
    push_border_change( display_hires_border );
    display_last_border = display_hires_border;
  } else if( !scld_last_dec.name.hires &&
             display_lores_border != display_last_border ) {
    push_border_change( display_lores_border );
    display_last_border = display_lores_border;
  }
}

void
display_set_lores_border( int colour )
{
  if( display_lores_border != colour ) {
    display_lores_border = colour;
  }
  check_border_change();
}

void
display_set_hires_border( int colour )
{
  if( display_hires_border != colour ) {
    display_hires_border = colour;
  }
  check_border_change();
}

static void
set_border( int y, int start, int end, int colour )
{
  libspectrum_dword chunk_detail = colour << 11;
  int index = start + y * DISPLAY_SCREEN_WIDTH_COLS;

  for( ; start < end; start++ ) {
    /* Draw it if it is different to what was there last time - we know that
    data and mode will have been the same */
    if( display_last_screen[ index ] != chunk_detail ) {
      uidisplay_plot8( start, y, 0x00, 0, colour );

      /* Update last display record */
      display_last_screen[ index ] = chunk_detail;

      /* And now mark it dirty */
      display_is_dirty[y] |= ( (libspectrum_qword)1 << start );
    }
    index++;
  }
}

static void
border_change_write( int y, int start, int end, int colour )
{
  if(   y <  DISPLAY_BORDER_HEIGHT                    ||
      ( y >= DISPLAY_BORDER_HEIGHT + DISPLAY_HEIGHT )    ) {

    /* Top and bottom borders */
    set_border( y, start, end, colour );

    return;
  }

  /* Left border */
  if( start < DISPLAY_BORDER_WIDTH_COLS ) {

    int left_end =
      end > DISPLAY_BORDER_WIDTH_COLS ? DISPLAY_BORDER_WIDTH_COLS : end;

    set_border( y, start, left_end, colour );
  }

  /* Right border */
  if( end > DISPLAY_BORDER_WIDTH_COLS + DISPLAY_WIDTH_COLS ) {

    if( start < DISPLAY_BORDER_WIDTH_COLS + DISPLAY_WIDTH_COLS )
      start = DISPLAY_BORDER_WIDTH_COLS + DISPLAY_WIDTH_COLS;

    set_border( y, start, end, colour );
  }
}

static void
border_change_line_part( int y, int start, int end, int colour )
{
  border_change_write( y, start, end, colour );
}

static void
border_change_line( int y, int colour )
{
  border_change_write( y, 0, DISPLAY_SCREEN_WIDTH_COLS, colour );
}

static void
do_border_change( struct border_change_t *first,
		  struct border_change_t *second )
{
  if( first->x ) {
    if( first->x != DISPLAY_SCREEN_WIDTH_COLS )
      border_change_line_part( first->y, first->x, DISPLAY_SCREEN_WIDTH_COLS,
			       first->colour );
    /* Don't extend region past the end of the screen */
    if( first->y < DISPLAY_SCREEN_HEIGHT - 1 ) first->y++;
  }

  for( ; first->y < second->y; first->y++ ) {
    border_change_line( first->y, first->colour );
  }

  if( second->x ) {
    if( second->x == DISPLAY_SCREEN_WIDTH_COLS ) {
      border_change_line( first->y, first->colour );
    } else {
      border_change_line_part( first->y, 0, second->x, first->colour );
    }
  }
}

/* Take account of all the border colour changes which happened in this
   frame */
static void
update_border( void )
{
  int pos;
  int error;

  /* Put the final sentinel onto the list */
  struct border_change_t *end_sentinel = alloc_change();

  memcpy( end_sentinel, &border_change_end_sentinel,
          sizeof( struct border_change_t ) );

  for( pos = 0; pos < border_changes_last-1; pos++ ) {
    do_border_change( border_changes+pos, border_changes+pos+1 );
  }

  border_changes_last = 0;

  error = add_border_sentinel(); if( error ) return;
}

/* Send the updated screen to the UI-specific code */
static void
update_ui_screen( void )
{
  static int frame_count = 0;
  int scale = machine_current->timex ? 2 : 1;
  size_t i;
  struct rectangle *ptr;

  if( settings_current.frame_rate <= ++frame_count ) {
    frame_count = 0;
    if( movie_recording ) {
      movie_start_frame();
    }

    if( display_redraw_all ) {
      if( movie_recording ) {
        movie_add_area( 0, 0, DISPLAY_ASPECT_WIDTH >> 3,
                        DISPLAY_SCREEN_HEIGHT );
      }
      uidisplay_area( 0, 0,
                      scale * DISPLAY_ASPECT_WIDTH,
                      scale * DISPLAY_SCREEN_HEIGHT );
      display_redraw_all = 0;
    } else {
      for( i = 0, ptr = rectangle_inactive;
           i < rectangle_inactive_count;
           i++, ptr++ ) {
            if( movie_recording ) {
              movie_add_area( ptr->x, ptr->y, ptr->w, ptr->h );
            }
              uidisplay_area( 8 * scale * ptr->x, scale * ptr->y,
                        8 * scale * ptr->w, scale * ptr->h );
      }
    }

    rectangle_inactive_count = 0;

    uidisplay_frame_end();
  }
}

int
display_frame( void )
{
  /* Copy all the critical region to the display */
  copy_critical_region( DISPLAY_WIDTH_COLS, DISPLAY_HEIGHT - 1 );
  critical_region_x = critical_region_y = 0;

  update_border();
  update_dirty_rects();
  update_ui_screen();

  display_frame_count++;
  if(display_frame_count==16) {
    display_flash_reversed=1;
    display_dirty_flashing();
  } else if(display_frame_count==32) {
    display_flash_reversed=0;
    display_dirty_flashing();
    display_frame_count=0;
  }
  
  return 0;
}

display_dirty_flashing_fn display_dirty_flashing;

void
display_dirty_flashing_timex(void)
{
  libspectrum_word offset;
  libspectrum_byte *screen, attr;

  screen = RAM[ memory_current_screen ];
  
  if( !scld_last_dec.name.hires ) {
    if( scld_last_dec.name.b1 ) {

      for( offset = ALTDFILE_OFFSET; offset < 0x3800; offset++ ) {
        attr = screen[ offset ];
        if( attr & 0x80 ) display_dirty8( offset - ALTDFILE_OFFSET );
      }

    } else if( scld_last_dec.name.altdfile ) {

      for( offset= 0x3800; offset < 0x3b00; offset++ ) {
        attr = screen[ offset ];
        if( attr & 0x80 ) display_dirty64( offset - ALTDFILE_OFFSET );
      }

    } else { /* Standard Speccy screen */

      display_dirty_flashing_sinclair();

    }
  }
}

void
display_dirty_flashing_pentagon_16_col(void)
{
  /* No flash attribute in 16 colour mode */
}

void
display_dirty_flashing_sinclair(void)
{
  libspectrum_word offset;
  libspectrum_byte *screen, attr;

  screen = RAM[ memory_current_screen ];
  
  /* Standard Speccy screen */
  for( offset = 0x1800; offset < 0x1b00; offset++ ) {
    attr = screen[ offset ];
    if( attr & 0x80 ) display_dirty64( offset );
  }
}

void display_refresh_main_screen(void)
{
  size_t i;

  for( i = 0; i < DISPLAY_HEIGHT; i++ )
    display_maybe_dirty[i] = display_all_dirty;
}

void display_refresh_all(void)
{
  size_t i;

  display_redraw_all = 1;

  display_refresh_main_screen();

  for( i = 0; i < DISPLAY_SCREEN_HEIGHT; i++ )
    display_is_dirty[i] = display_all_dirty;

  memset( display_last_screen, 0xff,
          DISPLAY_SCREEN_WIDTH_COLS * DISPLAY_SCREEN_HEIGHT 
          * sizeof(libspectrum_dword) );
}

/* Fetch pixel (x, y). On a Timex this will be a point on a 640x480 canvas,
   on a Sinclair/Amstrad/Russian clone this will be a point on a 320x240
   canvas */
int
display_getpixel( int x, int y )
{
  libspectrum_byte ink, paper;
  libspectrum_byte data, data2;
  int mask = 1 << (7 - (x % 8));
  int index;

  if( machine_current->timex ) {
    int column = x >> 4;
    scld mode_data;

    y >>= 1;
    index = column + y * DISPLAY_SCREEN_WIDTH_COLS;

    data = display_last_screen[ index ] & 0xff;
    data2 = (display_last_screen[ index ] & 0xff00)>>8;
    mode_data.byte = (display_last_screen[ index ] & 0xff0000)>>16;

    if( mode_data.name.hires ) {
      if( x % 16 > 7 ) data = data2;
      display_parse_attr( hires_convert_dec( mode_data.byte ), &ink, &paper );
    } else {
      /* divide x by two to get the same value for adjacent pixels */
      mask = 1 << (7 - ((x>>1) % 8));
      display_parse_attr( data2, &ink, &paper );
    }
  } else {
    int column = x >> 3;

    index = column + y * DISPLAY_SCREEN_WIDTH_COLS;

    data = display_last_screen[ index ] & 0xff;
    data2 = (display_last_screen[ index ] & 0xff00)>>8;

    display_parse_attr( data2, &ink, &paper );
  }

  if( data & mask ) return ink;

  return paper;
}
