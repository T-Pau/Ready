/* timings.c: Timing routines
   Copyright (c) 2003 Philip Kendall
   Copyright (c) 2016 Stuart Brady

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

#include "internals.h"

typedef struct timings_frame_t {
  /* Line timings in tstates */
  libspectrum_word left_border, horizontal_screen, right_border,
	           horizontal_retrace;

  /* Frame timings in lines */
  libspectrum_word top_border, vertical_screen, bottom_border,
	           vertical_retrace;

  /* How long does an interrupt last in tstates */
  libspectrum_word interrupt_length;

  /* How long after interrupt is the top-level pixel of the main screen
     displayed */
  libspectrum_dword top_left_pixel;
} timings_frame_t;

static const timings_frame_t timings_frame_ferranti_5c_6c =
{
  24, 128, 24, 48, /* Horizontal, 224 clocks per line */
  48, 192, 48, 24, /* Vertical, 312 lines per frame */
  32, 14336
};

static const timings_frame_t timings_frame_ferranti_60hz =
{
  24, 128, 24, 48, /* Horizontal, 224 clocks per line */
  24, 192, 25, 23, /* Vertical, 264 lines per frame */
  32, 8960
};

static const timings_frame_t timings_frame_ferranti_7c =
{
  24, 128, 24, 52, /* Horizontal, 228 clocks per line */
  48, 192, 48, 23, /* Vertical, 311 lines per frame */
  36, 14362
};

static const timings_frame_t timings_frame_amstrad_asic =
{
  24, 128, 24, 52, /* Horizontal, 228 clocks per line */
  48, 192, 48, 23, /* Vertical, 311 lines per frame */
  32, 14365
};

static const timings_frame_t timings_frame_timex_scld_50hz =
{
  24, 128, 24, 48, /* Horizontal, 224 clocks per line */
  48, 192, 48, 24, /* Vertical, 312 lines per frame */
  32, 14321
};

static const timings_frame_t timings_frame_timex_scld_60hz =
{
  24, 128, 24, 48, /* Horizontal, 224 clocks per line */
  24, 192, 25, 21, /* Vertical, 262 lines per frame */
  32, 9169
};

static const timings_frame_t timings_frame_se =
{
  24, 128, 24, 48, /* Horizontal, 224 clocks per line */
  47, 192, 48, 25, /* Vertical, 312 lines per frame */
  32, 14336
};

static const timings_frame_t timings_frame_pentagon =
{
  36, 128, 28, 32, /* Horizontal, 224 clocks per line */
  64, 192, 48, 16, /* Vertical 320 lines per frame */
  36, 17988
};

static const timings_frame_t timings_frame_scorpion =
{
  24, 128, 32, 40, /* Horizontal, 224 clocks per line */
  48, 192, 48, 24, /* Vertical, 312 lines per frame */
  36, 14336
};

/* The frame timings of a machine */
typedef struct timings_t {

  /* Processor speed in Hz */
  libspectrum_dword processor_speed;

  /* AY clock speed in Hz */
  libspectrum_dword ay_speed;

  const timings_frame_t *frame_timings;

} timings_t;

/* The actual data from which the full timings are constructed */
static const timings_t base_timings[] = {

  /* 48K */
  { 3500000,       0, &timings_frame_ferranti_5c_6c },
  /* TC2048 */
  { 3500000,       0, &timings_frame_timex_scld_50hz },
  /* 128K */
  { 3546900, 1773400, &timings_frame_ferranti_7c },
  /* +2 */
  { 3546900, 1773400, &timings_frame_ferranti_7c },
  /* Pentagon */
  { 3584000, 1792000, &timings_frame_pentagon },
  /* +2A */
  { 3546900, 1773400, &timings_frame_amstrad_asic },
  /* +3 */
  { 3546900, 1773400, &timings_frame_amstrad_asic },
  /* Unknown machine */
  { 0, 0, NULL },
  /* 16K */
  { 3500000,       0, &timings_frame_ferranti_5c_6c },
  /* TC2068 */
  { 3500000, 1750000, &timings_frame_timex_scld_50hz },
  /* Scorpion */
  { 3500000, 1750000, &timings_frame_scorpion },
  /* +3e */
  { 3546900, 1773400, &timings_frame_amstrad_asic },
  /* SE */
  { 3500000, 1750000, &timings_frame_se },
  /* TS2068 */
  { 3528000, 1764000, &timings_frame_timex_scld_60hz },
  /* Pentagon 512K */
  { 3584000, 1792000, &timings_frame_pentagon },
  /* Pentagon 1024K */
  { 3584000, 1792000, &timings_frame_pentagon },
  /* 48K NTSC */
  { 3527500,       0, &timings_frame_ferranti_60hz },
  /* 128Ke */
  { 3546900, 1773400, &timings_frame_amstrad_asic },
};

libspectrum_dword
libspectrum_timings_processor_speed( libspectrum_machine machine )
{
  return base_timings[ machine ].processor_speed;
}

libspectrum_dword
libspectrum_timings_ay_speed( libspectrum_machine machine )
{
  return base_timings[ machine ].ay_speed;
}

libspectrum_word
libspectrum_timings_left_border( libspectrum_machine machine )
{
  const timings_frame_t *f = base_timings[ machine ].frame_timings;
  if( !f ) return 0;
  return f->left_border;
}

libspectrum_word
libspectrum_timings_horizontal_screen( libspectrum_machine machine )
{
  const timings_frame_t *f = base_timings[ machine ].frame_timings;
  if( !f ) return 0;
  return f->horizontal_screen;
}

libspectrum_word
libspectrum_timings_right_border( libspectrum_machine machine )
{
  const timings_frame_t *f = base_timings[ machine ].frame_timings;
  if( !f ) return 0;
  return f->right_border;
}

libspectrum_word
libspectrum_timings_horizontal_retrace( libspectrum_machine machine )
{
  const timings_frame_t *f = base_timings[ machine ].frame_timings;
  if( !f ) return 0;
  return f->horizontal_retrace;
}

libspectrum_word
libspectrum_timings_top_border( libspectrum_machine machine )
{
  const timings_frame_t *f = base_timings[ machine ].frame_timings;
  if( !f ) return 0;
  return f->top_border;
}

libspectrum_word
libspectrum_timings_vertical_screen( libspectrum_machine machine )
{
  const timings_frame_t *f = base_timings[ machine ].frame_timings;
  if( !f ) return 0;
  return f->vertical_screen;
}

libspectrum_word
libspectrum_timings_bottom_border( libspectrum_machine machine )
{
  const timings_frame_t *f = base_timings[ machine ].frame_timings;
  if( !f ) return 0;
  return f->bottom_border;
}

libspectrum_word
libspectrum_timings_vertical_retrace( libspectrum_machine machine )
{
  const timings_frame_t *f = base_timings[ machine ].frame_timings;
  if( !f ) return 0;
  return f->vertical_retrace;
}

libspectrum_word
libspectrum_timings_interrupt_length( libspectrum_machine machine )
{
  const timings_frame_t *f = base_timings[ machine ].frame_timings;
  if( !f ) return 0;
  return f->interrupt_length;
}

libspectrum_word
libspectrum_timings_top_left_pixel( libspectrum_machine machine )
{
  const timings_frame_t *f = base_timings[ machine ].frame_timings;
  if( !f ) return 0;
  return f->top_left_pixel;
}

libspectrum_word
libspectrum_timings_tstates_per_line( libspectrum_machine machine )
{
  const timings_frame_t *f = base_timings[ machine ].frame_timings;
  if( !f ) return 0;
  return f->left_border + f->horizontal_screen + f->right_border +
	 f->horizontal_retrace;
}

libspectrum_word
libspectrum_timings_lines_per_frame( libspectrum_machine machine )
{
  const timings_frame_t *f = base_timings[ machine ].frame_timings;
  if( !f ) return 0;
  return f->top_border + f->vertical_screen + f->bottom_border +
	 f->vertical_retrace;
}

libspectrum_dword
libspectrum_timings_tstates_per_frame( libspectrum_machine machine )
{
  const timings_frame_t *f = base_timings[ machine ].frame_timings;
  if( !f ) return 0;
  return libspectrum_timings_tstates_per_line( machine ) *
    ( (libspectrum_dword)libspectrum_timings_lines_per_frame( machine ) );
}
