/* rectangle.c: routines for managing the set of screen area rectangles updated
                since the last display
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

#include <stdlib.h>

#include "fuse.h"
#include "rectangle.h"
#include "settings.h"
#include "ui/ui.h"

/* Those rectangles which were modified on the last line to be displayed */
static struct rectangle *rectangle_active = NULL;
static size_t rectangle_active_count = 0, rectangle_active_allocated = 0;

/* Those rectangles which weren't */
struct rectangle *rectangle_inactive = NULL;
size_t rectangle_inactive_count = 0, rectangle_inactive_allocated = 0;

/* Add the rectangle { x, line, w, 1 } to the list of rectangles to be
   redrawn, either by extending an existing rectangle or creating a
   new one */
void
rectangle_add( int y, int x, int w )
{
  size_t i;
  struct rectangle *ptr;

  /* Check through all 'active' rectangles (those which were modified
     on the previous line) and see if we can use this new rectangle
     to extend them */
  for( i = 0; i < rectangle_active_count; i++ ) {

    if( rectangle_active[i].x == x &&
	rectangle_active[i].w == w    ) {
      rectangle_active[i].h++;
      return;
    }
  }

  /* We couldn't find a rectangle to extend, so create a new one */
  if( ++rectangle_active_count > rectangle_active_allocated ) {

    size_t new_alloc;

    new_alloc = rectangle_active_allocated     ?
                2 * rectangle_active_allocated :
                8;

    ptr = libspectrum_renew( struct rectangle, rectangle_active, new_alloc );

    rectangle_active_allocated = new_alloc; rectangle_active = ptr;
  }

  ptr = &rectangle_active[ rectangle_active_count - 1 ];

  ptr->x = x; ptr->y = y;
  ptr->w = w; ptr->h = 1;
}

#ifndef MAX
#define MAX(a,b)    (((a) > (b)) ? (a) : (b))
#define MIN(a,b)    (((a) < (b)) ? (a) : (b))
#endif

static inline int
compare_and_merge_rectangles( struct rectangle *source )
{
  size_t z;

  /* Now look to see if there is an overlapping rectangle in the inactive
     list.  These occur when frame skip is on and the same lines are
     covered more than once... */
  for( z = 0; z < rectangle_inactive_count; z++ ) {
    if( rectangle_inactive[z].x == source->x &&
          rectangle_inactive[z].w == source->w ) {
      if( rectangle_inactive[z].y == source->y &&
            rectangle_inactive[z].h == source->h )
        return 1;

      if( ( rectangle_inactive[z].y < source->y && 
          ( source->y < ( rectangle_inactive[z].y +
            rectangle_inactive[z].h + 1 ) ) ) ||
          ( source->y < rectangle_inactive[z].y && 
          ( rectangle_inactive[z].y < ( source->y + source->h + 1 ) ) ) ) {
        /* rects overlap or touch in the y dimension, merge */
        rectangle_inactive[z].h = MAX( rectangle_inactive[z].y +
                                    rectangle_inactive[z].h,
                                    source->y + source->h ) -
                                  MIN( rectangle_inactive[z].y, source->y );
        rectangle_inactive[z].y = MIN( rectangle_inactive[z].y, source->y );

        return 1;
      }
    }
    if( rectangle_inactive[z].y == source->y &&
          rectangle_inactive[z].h == source->h ) {

      if( (rectangle_inactive[z].x < source->x && 
          ( source->x < ( rectangle_inactive[z].x +
            rectangle_inactive[z].w + 1 ) ) ) ||
          ( source->x < rectangle_inactive[z].x && 
          ( rectangle_inactive[z].x < ( source->x + source->w + 1 ) ) ) ) {
        /* rects overlap or touch in the x dimension, merge */
        rectangle_inactive[z].w = MAX( rectangle_inactive[z].x +
          rectangle_inactive[z].w, source->x +
          source->w ) - MIN( rectangle_inactive[z].x, source->x );
        rectangle_inactive[z].x = MIN( rectangle_inactive[z].x, source->x );
        return 1;
      }
    }
     /* Handle overlaps offset by both x and y? how much overlap and hence 
        overdraw can be tolerated? */
  }
  return 0;
}

/* Move all rectangles not updated on this line to the inactive list */
void
rectangle_end_line( int y )
{
  size_t i;
  struct rectangle *ptr;

  for( i = 0; i < rectangle_active_count; i++ ) {

    /* Skip if this rectangle was updated this line */
    if( rectangle_active[i].y + rectangle_active[i].h == y + 1 ) continue;

    if ( settings_current.frame_rate > 1 &&
	 compare_and_merge_rectangles( &rectangle_active[i] ) ) {

      /* Mark the active rectangle as done */
      rectangle_active[i].h = 0;
      continue;
    }

    /* We couldn't find a rectangle to extend, so create a new one */
    if( ++rectangle_inactive_count > rectangle_inactive_allocated ) {

      size_t new_alloc;

      new_alloc = rectangle_inactive_allocated     ?
	          2 * rectangle_inactive_allocated :
	          8;

      ptr =
        libspectrum_renew( struct rectangle, rectangle_inactive, new_alloc );

      rectangle_inactive_allocated = new_alloc; rectangle_inactive = ptr;
    }

    rectangle_inactive[ rectangle_inactive_count - 1 ] = rectangle_active[i];

    /* Mark the active rectangle as done */
    rectangle_active[i].h = 0;
  }

  /* Compress the list of active rectangles */
  for( i = 0, ptr = rectangle_active; i < rectangle_active_count; i++ ) {
    if( rectangle_active[i].h == 0 ) continue;
    *ptr = rectangle_active[i]; ptr++;
  }

  rectangle_active_count = ptr - rectangle_active;
}
