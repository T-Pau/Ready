/* movie.c: Routines for creating 'movie' with border
   Copyright (c) 2006-2015 Gergely Szasz
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

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>

#include <libspectrum.h>
#ifdef HAVE_ZLIB_H
#define ZLIB_CONST
#include <zlib.h>
#endif

#include "display.h"
#include "fuse.h"
#include "machine.h"
#include "movie.h"
#include "movie_tables.h"
#include "options.h"
#include "peripherals/scld.h"
#include "screenshot.h"
#include "settings.h"
#include "sound.h"
#include "ui/ui.h"

#undef MOVIE_DEBUG_PRINT

/*
  FMF - Fuse Movie File

  Fuse Movie File:
    File Header:
      off  len  data          description
      0    4    "FMF_"	Magic header
      4    2    "V1"		Version
      6    1    <e|E>		Endianness (e - little / E- big)
      7    1    <U|Z>		Compression ( U - uncompressed / Z - zlib compressed )
      8    1    #		Frame rate ( 1:# )
      9    1    <$|R|C|X>	Screen type
      10   1    <A|B|C|D|E>	timing code
      11   1    <P|U|A>	Sound encoding
      12   2    Freq		Sound freq in Hz
      14   1    <S|M>		Sound stereo / mono
      15   1    "\n"		padding (<new line>)

    e.g. FMF_V1eZ\001$AU\000\175M	-> little endian compressed normal screen, 48k timing, u-Law mono 32000Hz sound
    Data
    Frame data header
      off  len  data          description
      0    1    <$|S|N|X>     Data chunk type

      $ -> screen area (slice)
      off  len  data          description
      0    1    x		X coord (0-39)
      1    2    y		Y coord (0-239)
      3    1    w		width (1-40)
      4    2    h		height (1-240)
      6    ?    runlength encoded data bytes
                bitmap1; attrib1		ZX$/TX$/HiCol ($/X/C)
                bitmap1; bitmap2; attrib	HiR	(R)
                runlength encoding:
                  abcdefghbb#gg# -> two identical byte plus len code a #+2 len

      S -> sound chunk
      off  len  data          description
      0    1    <P|U|A>	sound encoding type P-> 16bit signed PCM, U -> u-Law, A -> A-Law
      1    2    Freq		sound freq in Hz
      3    1    <S|M>		channels S-> stereo, M-> mono
      4    2    length-1	length in frames (0-65535) -> 1-65536 sound frame

      N -> New Frame
      off  len  data          description
      0    1    #		frame rate 1:#	(1:1 -> ~50/s, 1:2 -> ~25/a ...)
      1    1    <$|R|C|X>	screen type $ - standard screen,
                                          R - HiRes,
                                          C - HiColor,
                                          X - standard screen on Timex (double size)
      2    1    <A|B|C|D>	frame timing code A - 16, 48, TC2048, TC2068, Scorpion, SE
                                                B - 128, +2, +2A, +3, +3E
                                                C - TS2068
                                                D - Pentagon
                                                E - 48 NTSC
      In a frame there are no, one or several screen rectangle, changed from
      the previouse frame.

      X -> End of recording
      off  len  data          description
      There is no any data... It mark the end of the last frame. So we can
      concat several FMF file without any problem...
*/

int movie_recording = 0;
static int movie_paused = 0;

static int frame_no, slice_no;

static FILE *of = NULL;	/* out file */
static int fmf_screen;
static libspectrum_byte head[8];
static int freq = 0;
static char stereo = 'M';
static char format = '?';
static int framesiz = 4;

static libspectrum_byte sbuff[ 4096 ];
#ifdef HAVE_ZLIB_H
#define ZBUF_SIZE 8192
static int fmf_compr = -1;
static z_stream zstream;
static unsigned char zbuf_o[ ZBUF_SIZE ];
#endif	/* HAVE_ZLIB_H */

static unsigned char alaw_table[2048 + 1] = { ALAW_ENC_TAB };

void movie_start_frame( void );
void movie_init_sound( int f, int s );

static char
get_timing( void )
{
  switch( machine_current->machine ) {
  case LIBSPECTRUM_MACHINE_16:
  case LIBSPECTRUM_MACHINE_48:
  case LIBSPECTRUM_MACHINE_TC2048:
  case LIBSPECTRUM_MACHINE_TC2068:
  case LIBSPECTRUM_MACHINE_SCORP:
  case LIBSPECTRUM_MACHINE_SE:
    return 'A';
  case LIBSPECTRUM_MACHINE_128:
  case LIBSPECTRUM_MACHINE_PLUS2:
  case LIBSPECTRUM_MACHINE_PLUS2A:
  case LIBSPECTRUM_MACHINE_PLUS3:
  case LIBSPECTRUM_MACHINE_PLUS3E:
    return 'B';
  case LIBSPECTRUM_MACHINE_TS2068:
    return 'C';
  case LIBSPECTRUM_MACHINE_PENT:
  case LIBSPECTRUM_MACHINE_PENT512:
  case LIBSPECTRUM_MACHINE_PENT1024:
    return 'D';
  case LIBSPECTRUM_MACHINE_48_NTSC:
    return 'E';
  case LIBSPECTRUM_MACHINE_UNKNOWN:
  default:
    return '?';
  }
}

static char
get_screentype( void )
{
  if( machine_current->timex ) { /* ALTDFILE and default */
    if( scld_last_dec.name.hires )
      return 'R';	/* HIRES screen */
    else if( scld_last_dec.name.b1 )
      return 'C';	/* HICOLOR screen */
    else
      return 'X';	/* STANDARD screen on timex machine */
  }
  return '$';	/* STANDARD screen */
}

#ifdef HAVE_ZLIB_H
static void
fwrite_compr( const void *b, size_t n, size_t m, FILE *f )
{
  if( fmf_compr == 0 ) {
    fwrite( b, n, m, f );
  } else {
    zstream.avail_in = n * m;
    zstream.next_in = b;
    zstream.avail_out = ZBUF_SIZE;
    zstream.next_out = zbuf_o;
    do {
      deflate( &zstream, Z_NO_FLUSH );
      while( zstream.avail_out != ZBUF_SIZE ) {
        fwrite( zbuf_o, ZBUF_SIZE - zstream.avail_out, 1, of );
	zstream.avail_out = ZBUF_SIZE;
	zstream.next_out = zbuf_o;
        deflate( &zstream, Z_NO_FLUSH );
      }
    } while ( zstream.avail_in != 0 );
  }
}
#else	/* HAVE_ZLIB_H */
#define fwrite_compr fwrite
#endif	/* HAVE_ZLIB_H */

static void
movie_compress_area( int x, int y, int w, int h, int s )
{
  libspectrum_dword *dpoint, *dline;
  libspectrum_byte d, d1, *b;
  libspectrum_byte buff[ 960 ];
  int w0, h0, l;

  dline = &display_last_screen[x + 40 * y];
  b = buff; l = -1;
  d1 = ( ( *dline >> s ) & 0xff ) + 1;		/* *d1 != dpoint :-) */

  for( h0 = h; h0 > 0; h0--, dline += 40 ) {
    dpoint = dline;
    for( w0 = w; w0 > 0; w0--, dpoint++) {
      d = ( *dpoint >> s ) & 0xff;	/* bitmask1 */
      if( d != d1 ) {
        if( l > -1 ) {			/* save running length 0-255 */
	  *b++ = l;
	  l = -1;			/* reset l */
	}
        *b++ = d1 = d;
      } else if( l >= 0 ) {
	if( l == 255 ) {		/* close run, and may start a new? */
	  *b++ = l; *b++ = d; l = -1;
	} else {
	  l++;
	}
      } else {
        *b++ = d;
	l++;
      }
/*      d1 = d;				*/
    }
    if( b - buff > 960 - 128 ) {	/* worst case 40*1.5 per line */
      fwrite_compr( buff, b - buff, 1, of );
      b = buff;
    }
  }
  if( l > -1 ) {		/* save running length 0-255 */
    *b++ = l;
  }
  if( b != buff ) {	/* dump remain */
    fwrite_compr( buff, b - buff, 1, of );
  }
}

/* Fetch pixel (x, y). On a Timex this will be a point on a 640x480 canvas,
   on a Sinclair/Amstrad/Russian clone this will be a point on a 320x240
   canvas */

/* x: 0 - 39; y: 0 - 239 */

/* abcdefghijkl... cc# where # mean cc + # c char*/

void
movie_add_area( int x, int y, int w, int h )
{
  if( movie_paused ) {
    movie_start_frame();
    return;
  }
  head[0] = '$';			/* RLE compressed data... */
  head[1] = x;
  head[2] = y & 0xff;
  head[3] = y >> 8;
  head[4] = w;
  head[5] = h & 0xff;
  head[6] = h >> 8;
  fwrite_compr( head, 7, 1, of );
  movie_compress_area( x, y, w, h, 0 );	/* Bitmap1 */
  movie_compress_area( x, y, w, h, 8 );	/* Attrib/B2 */
  if( fmf_screen == 'R' ) {
    movie_compress_area( x, y, w, h, 16 );	/* HiRes attrib */
  }
  slice_no++;
}

static void
movie_start_fmf( const char *name )
{
  if( ( of = fopen(name, "wb") ) == NULL ) {  /* trunc old file ? or append ? */
    ui_error( UI_ERROR_ERROR, "error opening movie file '%s': %s", name,
              strerror( errno ) );
    return;
  }
#ifdef WORDS_BIGENDIAN
  fwrite( "FMF_V1E", 7, 1, of );	/* write magic header Fuse Movie File */
#else	/* WORDS_BIGENDIAN */
  fwrite( "FMF_V1e", 7, 1, of );	/* write magic header Fuse Movie File */
#endif	/* WORDS_BIGENDIAN */
#ifdef HAVE_ZLIB_H
  if( option_enumerate_movie_movie_compr() == 0 ) {
    fmf_compr = 0;
    fwrite( "U", 1, 1, of );		/* not compressed */
  } else {
    fmf_compr = Z_DEFAULT_COMPRESSION;
    fwrite( "Z", 1, 1, of );		/* compressed */
  }
  if( fmf_compr != 0 ) {
    zstream.zalloc = Z_NULL;
    zstream.zfree = Z_NULL;
    zstream.opaque = Z_NULL;
    zstream.avail_in = 0;
    zstream.next_in = Z_NULL;
    deflateInit( &zstream, fmf_compr );
  }
#else	/* HAVE_ZLIB_H */
  fwrite( "U", 1, 1, of );		/* cannot be compressed */
#endif	/* HAVE_ZLIB_H */
  movie_init_sound( settings_current.sound_freq,
                    sound_stereo_ay != SOUND_STEREO_AY_NONE );
  head[0] = settings_current.frame_rate;
  head[1] = get_screentype();
  head[2] = get_timing();
  head[3] = format;	/* sound format */
  head[4] = freq & 0xff;
  head[5] = freq >> 8;
  head[6] = stereo;
  head[7] = '\n';	/* padding */
  fwrite( head, 8, 1, of );		/* write initial params */
  movie_add_area( 0, 0, 40, 240 );
}

void
movie_start( const char *name )	/* some init, open file (name)*/
{
  frame_no = slice_no = 0;
  if( name == NULL || *name == '\0' )
    name = "fuse.fmf";			/* fuse movie file */

  movie_start_fmf( name );
  movie_recording = 1;
  ui_menu_activate( UI_MENU_ITEM_FILE_MOVIE_RECORDING, 1 );
  ui_menu_activate( UI_MENU_ITEM_FILE_MOVIE_PAUSE, 1 );
}

void
movie_stop( void )
{
  if( !movie_paused && !movie_recording ) return;

  fwrite_compr( "X", 1, 1, of );	/* End of Recording! */
#ifdef HAVE_ZLIB_H
  {
    if( fmf_compr != 0 ) {		/* close zlib */
      zstream.avail_in = 0;
      do {
        zstream.avail_out = ZBUF_SIZE;
        zstream.next_out = zbuf_o;
        deflate( &zstream, Z_SYNC_FLUSH );
        if( zstream.avail_out != ZBUF_SIZE )
          fwrite( zbuf_o, ZBUF_SIZE - zstream.avail_out, 1, of );
      } while ( zstream.avail_out != ZBUF_SIZE );
      deflateEnd( &zstream );
      fmf_compr = -1;
    }
  }
#endif	/* HAVE_ZLIB_H */
  format = '?';
  if( of ) {
    fclose( of );
    of = NULL;
  }
#ifdef MOVIE_DEBUG_PRINT
  fprintf( stderr, "Debug movie: saved %d.%d frame(.slice)\n", frame_no, slice_no );
#endif 	/* MOVIE_DEBUG_PRINT */
  movie_recording = 0;
  movie_paused = 0;
  ui_menu_activate( UI_MENU_ITEM_FILE_MOVIE_RECORDING, 0 );
}

void
movie_pause( void )
{
  if( !movie_paused && !movie_recording ) return;

  if( movie_recording ) {
    movie_recording = 0;
    movie_paused = 1;
    ui_menu_activate( UI_MENU_ITEM_FILE_MOVIE_PAUSE, 0 );
  } else {
    movie_recording = 1;
    movie_paused = 1;
    ui_menu_activate( UI_MENU_ITEM_FILE_MOVIE_PAUSE, 1 );
  }
}

void
movie_init_sound( int f, int s )
{
  /* initialise sound format */
  format = option_enumerate_movie_movie_compr() == 2 ? 'A' : 'P';
  freq = f;
  stereo = ( s ? 'S' : 'M' );
  framesiz = ( stereo == 'S' ? 2 : 1 ) * ( format == 'P' ? 2 : 1 );
}

static inline void
write_alaw( libspectrum_signed_word *buff, int len )
{
  int i = 0;
  while( len-- ) {  
    if( *buff >= 0)
      sbuff[i++] = alaw_table[*buff >> 4];
    else
      sbuff[i++] = 0x7f & alaw_table [- *buff >> 4];
    buff++;
    if( i == 4096 ) {
      i = 0;
      fwrite_compr( sbuff, 4096, 1, of );	/* write frame */
    }
  }
  if( i )
    fwrite_compr( sbuff, i, 1, of );	/* write remaind */
}

static void
add_sound( libspectrum_signed_word *buff, int len )
{
  head[0] = 'S';	/* sound frame */
  head[1] = format;	/* sound format */
  head[2] = freq & 0xff;
  head[3] = freq >> 8;
  head[4] = stereo;
  len--;		/*len - 1*/
  head[5] = len & 0xff;
  head[6] = len >> 8;
  len++;		/* len :-) */
  fwrite_compr( head, 7, 1, of );	/* Sound frame */
  if( format == 'P' )
    fwrite_compr( buff, len * framesiz , 1, of );	/* write frame */
  else if( format == 'A' )
    write_alaw( buff, len * framesiz );
}

void
movie_add_sound( libspectrum_signed_word *buff, int len )
{
  while( len ) {
    if( stereo == 'S' ) {
      add_sound( buff, len > 131072 ? 65536 : len >> 1 );
      buff += len > 131072 ? 131072 : len;
      len -= len > 131072 ? 131072 : len;
    } else {
      add_sound( buff, len > 65536 ? 65536 : len );
      buff += len > 65536 ? 65536 : len;
      len -= len > 65536 ? 65536 : len;
    }
  }
}

void
movie_start_frame( void )
{
  /* $ - ZX$, T - TX$, C - HiCol, R - HiRes */
  head[0] = 'N';
  head[1] = settings_current.frame_rate;
  head[2] = get_screentype();
  head[3] = get_timing();
  fwrite_compr( head, 4, 1, of );	/* New frame! */
  frame_no++;
  if( movie_paused ) {
    movie_paused = 0;
    movie_add_area( 0, 0, 40, 240 );
  }
}

void
movie_init( void )
{
  /* start movie recording if user requested... */
  if( settings_current.movie_start )
    movie_start( settings_current.movie_start );
}
